#include "Inventory/ZfInventoryComponent.h"
#include "Inventory/ZfItemDefinition.h"
#include "Inventory/ZfItemPickup.h"
#include "Inventory/Fragments/ZfItemFragment.h"
#include "Inventory/Fragments/ZfStackableFragment.h"
#include "Player/ZfPlayerState.h"
#include "Net/UnrealNetwork.h"

//
//============================ UZfInventoryComponent ============================
// 
UZfInventoryComponent::UZfInventoryComponent()
{
    SetIsReplicatedByDefault(true);
    InventoryList.OwnerComponent = this;

    // Habilita o sistema moderno de subobject replication
    bReplicateUsingRegisteredSubObjectList = true;
}

// Replicação de Variaveis
void UZfInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UZfInventoryComponent, InventoryList);
    DOREPLIFETIME(UZfInventoryComponent, MaxSlots);
}

//
//============================ FastArray callbacks ============================
// 
void FZfInventoryEntry::PreReplicatedRemove(const FZfInventoryList& InArraySerializer)
{
    if (InArraySerializer.OwnerComponent && Item)
    {
        InArraySerializer.OwnerComponent->OnItemRemoved.Broadcast(Item);

        for (UZfItemFragment* Fragment : Item->Fragments)
        {
            if (Fragment) Fragment->OnItemRemoved(Item);
        }
    }
}

void FZfInventoryEntry::PostReplicatedAdd(const FZfInventoryList& InArraySerializer)
{
    if (InArraySerializer.OwnerComponent && Item)
    {
        InArraySerializer.OwnerComponent->OnItemAdded.Broadcast(Item);

        for (UZfItemFragment* Fragment : Item->Fragments)
        {
            if (Fragment) Fragment->OnItemAdded(Item);
        }
    }
}

void FZfInventoryEntry::PostReplicatedChange(const FZfInventoryList& InArraySerializer)
{
    if (InArraySerializer.OwnerComponent && Item)
    {
        InArraySerializer.OwnerComponent->OnItemAdded.Broadcast(Item);
    }
}

//
//============================ Add Item no FastArray ============================
// 
UZfItemInstance* FZfInventoryList::AddItem(UZfItemDefinition* ItemDefinition, int32 SlotIndex)
{
    
    check(OwnerComponent != nullptr);
    check(OwnerComponent->GetOwner()->HasAuthority());
    
    FZfInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
    NewEntry.Item = NewObject<UZfItemInstance>(OwnerComponent->GetOwner());
    NewEntry.SlotIndex = SlotIndex;
    NewEntry.Item->InitializeFromDefinition(ItemDefinition);
       
    
    for (UZfItemFragment* ItemFragment : NewEntry.Item->Fragments)
    {
        if (ItemFragment)
        {
            ItemFragment->OnItemAdded(NewEntry.Item);
        }
    }
    
    MarkItemDirty(NewEntry);

    return NewEntry.Item;
}

//
//============================ Remove Item no FastArray ============================
//
void FZfInventoryList::RemoveItem(UZfItemInstance* Item)
{
    for (int32 i = Entries.Num() - 1; i >= 0; i--)
    {
        if (Entries[i].Item == Item)
        {
            Entries.RemoveAt(i);
            MarkArrayDirty();
            return;
        }
    }
}

void FZfInventoryList::RemoveItem(UZfItemDefinition* ItemDefinition)
{
    for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
    {
        if (EntryIt->Item && EntryIt->Item->ItemDefinition == ItemDefinition)
        {
            EntryIt.RemoveCurrent();
            MarkArrayDirty();
        }
    }
}

//
//============================ AddItem (por UZfItemInstance direto) ============================
//

// Mantida para compatibilidade com Server_AddItemAtSlot
void FZfInventoryList::AddItem(UZfItemInstance* Item, int32 SlotIndex)
{
    if (!Item) return;

    FZfInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
    NewEntry.Item = Item;
    NewEntry.SlotIndex = SlotIndex;
    MarkItemDirty(NewEntry);
}

//
//============================ Server Add Item ============================
//
void UZfInventoryComponent::Server_AddItem_Implementation(UZfItemDefinition* InItemDefinition)
{
    if (!InItemDefinition) return;

    // Verifica stack

    if (const UZfStackableFragment* Stackable = InItemDefinition->FindFragmentByClass<UZfStackableFragment>())
    {
        for (FZfInventoryEntry& Entry : InventoryList.Entries)
        {
            if (!Entry.Item) continue;
            if (Entry.Item->ItemDefinition != InItemDefinition) continue;

            UZfStackableFragment* ExistingStack = Entry.Item->FindFragmentByClass<UZfStackableFragment>();
            if (!ExistingStack) continue;

            if (ExistingStack->CurrentStackSize < ExistingStack->MaxStackSize)
            {
                ExistingStack->CurrentStackSize++;
                InventoryList.MarkItemDirty(Entry);
                OnItemAdded.Broadcast(Entry.Item);
                return;
            }
        }
    }

    // Busca primeiro slot vazio
    int32 EmptySlot = GetFirstEmptySlot();
    if (EmptySlot == -1)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Inventário cheio!"));
        return;
    }

    UZfItemInstance* NewInstance = InventoryList.AddItem(InItemDefinition, EmptySlot);
    
    // Registra a instância do item no canal de replicação
    AddReplicatedSubObject(NewInstance);
 
    // registra também cada Fragment individualmente
    for (UZfItemFragment* Fragment : NewInstance->Fragments)
    {
        if (Fragment)
        {
            AddReplicatedSubObject(Fragment);
        }
    }
 
    OnItemAdded.Broadcast(NewInstance);
}

//
//============================ Server Remove Item ============================
//
void UZfInventoryComponent::Server_RemoveItem_Implementation(UZfItemInstance* InItem)
{
    if (!InItem) return;
 
    // remove os Fragments do canal de replicação antes de remover o item
    for (UZfItemFragment* Fragment : InItem->Fragments)
    {
        if (Fragment)
        {
            RemoveReplicatedSubObject(Fragment);
        }
    }
 
    RemoveReplicatedSubObject(InItem);
    InventoryList.RemoveItem(InItem);
    OnItemRemoved.Broadcast(InItem);
}


//
//============================ Server Drop Item ============================
//
void UZfInventoryComponent::Server_DropItem_Implementation(UZfItemInstance* InItem, float DistanceDrop)
{
    if (!InItem || !GetWorld()) return;

    // 1 — Verifica se está no inventário
    const bool bWasInInventory = InventoryList.Entries.ContainsByPredicate([InItem](const FZfInventoryEntry& E) { return E.Item == InItem; });
    if (!bWasInInventory) return;

    // 2 — Valida definição
    if (!InItem->ItemDefinition) return;

    // 3 — Sanitiza localização — evita drop em coordenada arbitrária vinda do cliente
    APawn* OwnerPawn = nullptr;
    if (APlayerState* PS = Cast<APlayerState>(GetOwner()))
    {
        OwnerPawn = PS->GetPawn();
    }
    
    FVector Location = OwnerPawn->GetActorLocation() + OwnerPawn->GetActorForwardVector() * DistanceDrop;

    // 4 — Spawna o pickupe
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AZfItemPickup* DroppedActor = GetWorld()->SpawnActor<AZfItemPickup>(AZfItemPickup::StaticClass(), Location, FRotator::ZeroRotator, Params);

    if (!DroppedActor) return;
 
    // remove do canal de replicação antes de remover do FastArray
    for (UZfItemFragment* Fragment : InItem->Fragments)
    {
        if (Fragment) RemoveReplicatedSubObject(Fragment);
    }
    RemoveReplicatedSubObject(InItem);
 
    InventoryList.RemoveItem(InItem);
    OnItemRemoved.Broadcast(InItem);
 
    InItem->Rename(nullptr, DroppedActor);
    DroppedActor->UpdateVisual();
}

//
//============================ Server Add Item to Slot ============================
//
void UZfInventoryComponent::Server_AddItemAtSlot_Implementation(
    UZfItemInstance* InItem, int32 SlotIndex)
{
    if (!InItem) return;
    if (SlotIndex < 0 || SlotIndex >= MaxSlots) return;
    if (!IsSlotEmpty(SlotIndex)) return;

    InventoryList.AddItem(InItem, SlotIndex);
 
    AddReplicatedSubObject(InItem);
    for (UZfItemFragment* Fragment : InItem->Fragments)
    {
        if (Fragment) AddReplicatedSubObject(Fragment);
    }
 
    OnItemAdded.Broadcast(InItem);
}

//
//============================ Server Move Item ============================
//
void UZfInventoryComponent::Server_MoveItem_Implementation(int32 FromSlot, int32 ToSlot)
{
    if (FromSlot < 0 || FromSlot >= MaxSlots) return;
    if (ToSlot < 0 || ToSlot >= MaxSlots) return;

    // Busca entry no slot de origem
    FZfInventoryEntry* FromEntry = InventoryList.Entries.FindByPredicate([FromSlot](const FZfInventoryEntry& E) { return E.SlotIndex == FromSlot; });

    if (!FromEntry || !FromEntry->Item) return;

    // Busca entry no slot de destino
    FZfInventoryEntry* ToEntry = InventoryList.Entries.FindByPredicate([ToSlot](const FZfInventoryEntry& E) { return E.SlotIndex == ToSlot; });

    if (ToEntry && ToEntry->Item)
    {
        // Troca os dois itens de slot
        ToEntry->SlotIndex = FromSlot;
        InventoryList.MarkItemDirty(*ToEntry);
    }

    FromEntry->SlotIndex = ToSlot;
    InventoryList.MarkItemDirty(*FromEntry);
}

//
//============================ Fragmento de ExtraSlot ============================
//
void UZfInventoryComponent::AddExtraSlots(int32 Amount)
{
    MaxSlots += Amount;
    OnSlotsChanged.Broadcast(MaxSlots);
}

bool UZfInventoryComponent::TryRemoveExtraSlots(int32 Amount)
{
    int32 NewMaxSlots = MaxSlots - Amount;
    if (NewMaxSlots < BaseSlots) NewMaxSlots = BaseSlots;

    // Verifica se tem itens nos slots que vão sumir
    for (const FZfInventoryEntry& Entry : InventoryList.Entries)
    {
        if (Entry.Item && Entry.SlotIndex >= NewMaxSlots)
        {
            // Tem item em slot que vai sumir — não deixa desequipar
            return false;
        }
    }

    MaxSlots = NewMaxSlots;
    OnSlotsChanged.Broadcast(MaxSlots);
    return true;
}

void UZfInventoryComponent::OnRep_MaxSlots()
{
    OnSlotsChanged.Broadcast(MaxSlots);
}

//
//============================ Helpers ============================
//
TArray<UZfItemInstance*> UZfInventoryComponent::GetAllItems() const
{
    TArray<UZfItemInstance*> Items;
    for (const FZfInventoryEntry& Entry : InventoryList.Entries)
    {
        if (Entry.Item) Items.Add(Entry.Item);
    }
    return Items;
}

UZfItemInstance* UZfInventoryComponent::GetItemAtSlot(int32 SlotIndex) const
{
    const FZfInventoryEntry* Entry = InventoryList.Entries.FindByPredicate([SlotIndex](const FZfInventoryEntry& E) { return E.SlotIndex == SlotIndex; });
    if (!Entry) return nullptr;
    return Entry->Item;
}

bool UZfInventoryComponent::IsSlotEmpty(int32 SlotIndex) const
{
    return GetItemAtSlot(SlotIndex) == nullptr;
}

int32 UZfInventoryComponent::GetFirstEmptySlot() const
{
    for (int32 i = 0; i < MaxSlots; i++)
    {
        if (IsSlotEmpty(i)) return i;
    }
    return -1;
}

void UZfInventoryComponent::MarkItemDirty(UZfItemInstance* InItem)
{
    for (FZfInventoryEntry& Entry : InventoryList.Entries)
    {
        if (Entry.Item == InItem)
        {
            InventoryList.MarkItemDirty(Entry);
            return;
        }
    }
}

//
//============================ Debug ============================
//
void UZfInventoryComponent::DebugInventory()
{
    for (const FZfInventoryEntry& Entry : InventoryList.Entries)
    {
        if (!Entry.Item) continue;

        FString ItemName = Entry.Item->ItemDefinition ? Entry.Item->ItemDefinition->ItemName.ToString() : TEXT("sem definição");

        // Mostra quantos fragments tem no DataAsset para comparar
        if (Entry.Item->ItemDefinition)
        {
            GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Green,
                FString::Printf(TEXT("  → DataAsset tem %d fragments"),
                    Entry.Item->ItemDefinition->Fragments.Num()
                ));
        }

        // Mostra cada fragment individualmente
        for (auto& Fragment : Entry.Item->Fragments)
        {
            FString FragmentName = Fragment 
                ? Fragment->GetClass()->GetName() 
                : TEXT("nullptr");

            GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Cyan,
                FString::Printf(TEXT("  → Fragment: %s"), *FragmentName));
        }

        // Stack info
        FString StackInfo = TEXT("Não stackável");
        if (UZfStackableFragment* Stack = Entry.Item->FindFragmentByClass<UZfStackableFragment>())
        {
            StackInfo = FString::Printf(TEXT("Stack: %d / %d"), 
                Stack->CurrentStackSize, Stack->MaxStackSize);
        }
        
        // Mostra o item e quantos fragments ele tem
        GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow,
            FString::Printf(TEXT("[Slot %d] %s | Fragments: %d | %s"),
                Entry.SlotIndex, *ItemName, Entry.Item->Fragments.Num(), *StackInfo));
    }
}