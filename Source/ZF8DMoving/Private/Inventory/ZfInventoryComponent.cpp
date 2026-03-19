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
    bReplicateUsingRegisteredSubObjectList = true;
}

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
    if (!InArraySerializer.OwnerComponent || !Item) return;

    // CLIENTE: reconstrói os Fragments localmente a partir da ItemDefinition
    // já replicada. Os dados mutáveis chegam via subobject replication
    // logo após e sobrescrevem os valores default automaticamente.
    Item->InitializeFragmentsOnClient();

    InArraySerializer.OwnerComponent->OnItemAdded.Broadcast(Item);

    for (UZfItemFragment* Fragment : Item->Fragments)
    {
        if (Fragment) Fragment->OnItemAdded(Item);
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
//============================ Add Item ============================
//
UZfItemInstance* FZfInventoryList::AddItem(UZfItemDefinition* ItemDefinition, int32 SlotIndex)
{
    check(OwnerComponent != nullptr);
    AActor* OwnerActor = OwnerComponent->GetOwner();
    check(OwnerActor->HasAuthority());

    FZfInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();

    // Outer do Item = Actor (PlayerState)
    NewEntry.Item = NewObject<UZfItemInstance>(OwnerActor);
    NewEntry.SlotIndex = SlotIndex;

    // Outer dos Fragments = Actor (PlayerState), passado explicitamente
    NewEntry.Item->InitializeFromDefinition(ItemDefinition, OwnerActor);

    for (UZfItemFragment* Fragment : NewEntry.Item->Fragments)
    {
        if (Fragment) Fragment->OnItemAdded(NewEntry.Item);
    }

    MarkItemDirty(NewEntry);
    return NewEntry.Item;
}

void FZfInventoryList::AddItem(UZfItemInstance* Item, int32 SlotIndex)
{
    if (!Item) return;
    FZfInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
    NewEntry.Item = Item;
    NewEntry.SlotIndex = SlotIndex;
    MarkItemDirty(NewEntry);
}

//
//============================ Remove Item ============================
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
            if (!Entry.Item || Entry.Item->ItemDefinition != InItemDefinition) continue;

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

    int32 EmptySlot = GetFirstEmptySlot();
    if (EmptySlot == -1)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Inventário cheio!"));
        return;
    }

    UZfItemInstance* NewInstance = InventoryList.AddItem(InItemDefinition, EmptySlot);

    // Registra o Item no canal de replicação do componente
    AddReplicatedSubObject(NewInstance);

    // Registra cada Fragment individualmente — eles têm o Actor como Outer,
    // então o engine consegue estabelecer o canal corretamente
    for (UZfItemFragment* Fragment : NewInstance->Fragments)
    {
        if (Fragment) AddReplicatedSubObject(Fragment);
    }

    OnItemAdded.Broadcast(NewInstance);
}

//
//============================ Server Remove Item ============================
//
void UZfInventoryComponent::Server_RemoveItem_Implementation(UZfItemInstance* InItem)
{
    if (!InItem) return;

    for (UZfItemFragment* Fragment : InItem->Fragments)
    {
        if (Fragment) RemoveReplicatedSubObject(Fragment);
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

    const bool bWasInInventory = InventoryList.Entries.ContainsByPredicate([InItem](const FZfInventoryEntry& E) { return E.Item == InItem; });
    if (!bWasInInventory || !InItem->ItemDefinition) return;

    APawn* OwnerPawn = nullptr;
    if (APlayerState* PS = Cast<APlayerState>(GetOwner()))
    {
        OwnerPawn = PS->GetPawn();
    }
    if (!OwnerPawn) return;

    FVector Location = OwnerPawn->GetActorLocation() + OwnerPawn->GetActorForwardVector() * DistanceDrop;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AZfItemPickup* DroppedActor = GetWorld()->SpawnActor<AZfItemPickup>(AZfItemPickup::StaticClass(), Location, FRotator::ZeroRotator, Params);
    if (!DroppedActor) return;

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
//============================ Server AddItem At Slot ============================
//
void UZfInventoryComponent::Server_AddItemAtSlot_Implementation(UZfItemInstance* InItem, int32 SlotIndex)
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

    FZfInventoryEntry* FromEntry = InventoryList.Entries.FindByPredicate([FromSlot](const FZfInventoryEntry& E) { return E.SlotIndex == FromSlot; });
    if (!FromEntry || !FromEntry->Item) return;

    FZfInventoryEntry* ToEntry = InventoryList.Entries.FindByPredicate([ToSlot](const FZfInventoryEntry& E) { return E.SlotIndex == ToSlot; });

    if (ToEntry && ToEntry->Item)
    {
        ToEntry->SlotIndex = FromSlot;
        InventoryList.MarkItemDirty(*ToEntry);
    }

    FromEntry->SlotIndex = ToSlot;
    InventoryList.MarkItemDirty(*FromEntry);
}

//
//============================ Extra Slots ============================
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

    for (const FZfInventoryEntry& Entry : InventoryList.Entries)
    {
        if (Entry.Item && Entry.SlotIndex >= NewMaxSlots) return false;
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
    return Entry ? Entry->Item : nullptr;
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
//============================ Debug Inventory ============================
//
void UZfInventoryComponent::DebugInventory()
{
    for (const FZfInventoryEntry& Entry : InventoryList.Entries)
    {
        if (!Entry.Item) continue;

        FString ItemName = Entry.Item->ItemDefinition ? Entry.Item->ItemDefinition->ItemName.ToString() : TEXT("sem definição");

        if (Entry.Item->ItemDefinition)
        {
            GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Green,
                FString::Printf(TEXT("  → DataAsset tem %d fragments"),
                    Entry.Item->ItemDefinition->Fragments.Num()));
        }

        for (auto& Fragment : Entry.Item->Fragments)
        {
            FString FragmentName = Fragment ? Fragment->GetClass()->GetName() : TEXT("nullptr");
            GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Cyan,
                FString::Printf(TEXT("  → Fragment: %s | Outer: %s"),
                    *FragmentName,
                    Fragment ? *Fragment->GetOuter()->GetName() : TEXT("N/A")));
        }

        FString StackInfo = TEXT("Não stackável");
        if (UZfStackableFragment* Stack = Entry.Item->FindFragmentByClass<UZfStackableFragment>())
        {
            StackInfo = FString::Printf(TEXT("Stack: %d / %d"),
                Stack->CurrentStackSize, Stack->MaxStackSize);
        }

        GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow,
            FString::Printf(TEXT("[Slot %d] %s | Fragments: %d | %s | Item Outer: %s"),
                Entry.SlotIndex, *ItemName, Entry.Item->Fragments.Num(), *StackInfo,
                *Entry.Item->GetOuter()->GetName()));
    }
}

//
//============================ Debug Fragment ============================
//
 
#include "Inventory/Fragments/ZfTestFragment.h"   // <-- inclua no topo do .cpp
 
void UZfInventoryComponent::Server_DebugSetTestValue_Implementation(int32 NewValue)
{
    for (FZfInventoryEntry& Entry : InventoryList.Entries)
    {
        if (!Entry.Item) continue;
 
        UZfTestFragment* TestFrag = Entry.Item->FindFragmentByClass<UZfTestFragment>();
        if (!TestFrag) continue;
 
        // Modifica o valor — a replicação via Subobject cuida do resto
        TestFrag->SetTestValue(NewValue);
 
        GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Yellow,
            FString::Printf(TEXT("[SERVER] Item '%s' → TestValue = %d"),
                *Entry.Item->ItemDefinition->ItemName.ToString(), NewValue));
 
        return; // modifica só o primeiro item com o fragment
    }
 
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
        TEXT("[SERVER] Nenhum item com ZfTestFragment encontrado no inventário!"));
}