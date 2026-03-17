#include "Inventory/ZfInventoryComponent.h"
#include "Inventory/ZfItemDefinition.h"
#include "Inventory/ZfItemPickup.h"
#include "Inventory/Fragments/ZfItemFragment.h"
#include "Inventory/Fragments/ZfStackableFragment.h"
#include "Player/ZfPlayerState.h"
#include "Net/UnrealNetwork.h"



// FastArray callbacks
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

// FZfInventoryList
void FZfInventoryList::AddItem(UZfItemInstance* Item, int32 SlotIndex)
{
    if (!Item) return;

    FZfInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
    NewEntry.Item = Item;
    NewEntry.SlotIndex = SlotIndex;
    MarkItemDirty(NewEntry);
}

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

// UZfInventoryComponent
UZfInventoryComponent::UZfInventoryComponent()
{
    SetIsReplicatedByDefault(true);
    InventoryList.OwnerComponent = this;
}

void UZfInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UZfInventoryComponent::OnRegister()
{
    Super::OnRegister();
    // Garante que OwnerComponent está setado tanto no servidor quanto no cliente
    InventoryList.OwnerComponent = this;
}

void UZfInventoryComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UZfInventoryComponent, InventoryList);
    DOREPLIFETIME(UZfInventoryComponent, MaxSlots);
}

void UZfInventoryComponent::OnRep_MaxSlots()
{
    OnSlotsChanged.Broadcast(MaxSlots);
}

void UZfInventoryComponent::Server_AddItemFromDefinition_Implementation(
    UZfItemDefinition* ItemDefinition)
{
    if (!ItemDefinition) return;

    UZfItemInstance* NewItem = NewObject<UZfItemInstance>(this);
    NewItem->InitializeFromDefinition(ItemDefinition);

    Server_AddItem_Implementation(NewItem);
}

void UZfInventoryComponent::Server_AddItem_Implementation(UZfItemInstance* InItem)
{
    if (!InItem) return;

    // Verifica stack
    UZfStackableFragment* Stackable =
        InItem->FindFragmentByClass<UZfStackableFragment>();

    if (Stackable)
    {
        for (FZfInventoryEntry& Entry : InventoryList.Entries)
        {
            if (!Entry.Item) continue;
            if (Entry.Item->ItemDefinition != InItem->ItemDefinition) continue;

            UZfStackableFragment* ExistingStack =
                Entry.Item->FindFragmentByClass<UZfStackableFragment>();

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

    InventoryList.AddItem(InItem, EmptySlot);
    OnItemAdded.Broadcast(InItem);
}

void UZfInventoryComponent::Server_AddItemAtSlot_Implementation(
    UZfItemInstance* InItem, int32 SlotIndex)
{
    if (!InItem) return;
    if (SlotIndex < 0 || SlotIndex >= MaxSlots) return;
    if (!IsSlotEmpty(SlotIndex)) return;

    InventoryList.AddItem(InItem, SlotIndex);
    OnItemAdded.Broadcast(InItem);
}

void UZfInventoryComponent::Server_MoveItem_Implementation(
    int32 FromSlot, int32 ToSlot)
{
    if (FromSlot < 0 || FromSlot >= MaxSlots) return;
    if (ToSlot < 0 || ToSlot >= MaxSlots) return;

    // Busca entry no slot de origem
    FZfInventoryEntry* FromEntry = InventoryList.Entries.FindByPredicate(
        [FromSlot](const FZfInventoryEntry& E) { return E.SlotIndex == FromSlot; });

    if (!FromEntry || !FromEntry->Item) return;

    // Busca entry no slot de destino
    FZfInventoryEntry* ToEntry = InventoryList.Entries.FindByPredicate(
        [ToSlot](const FZfInventoryEntry& E) { return E.SlotIndex == ToSlot; });

    if (ToEntry && ToEntry->Item)
    {
        // Troca os dois itens de slot
        ToEntry->SlotIndex = FromSlot;
        InventoryList.MarkItemDirty(*ToEntry);
    }

    FromEntry->SlotIndex = ToSlot;
    InventoryList.MarkItemDirty(*FromEntry);
}

void UZfInventoryComponent::Server_RemoveItem_Implementation(UZfItemInstance* InItem)
{
    if (!InItem) return;

    InventoryList.RemoveItem(InItem);
    OnItemRemoved.Broadcast(InItem);
}

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

    // 5 — Pickup criado com sucesso — remove do inventário direto na _Implementation
    // ✅ sem RPC dentro de RPC
    InventoryList.RemoveItem(InItem);
    OnItemRemoved.Broadcast(InItem);

    // 6 — Rename só depois de tudo validado
    InItem->Rename(nullptr, DroppedActor);
    DroppedActor->InitializeWithItem(InItem);
}

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
    const FZfInventoryEntry* Entry = InventoryList.Entries.FindByPredicate(
        [SlotIndex](const FZfInventoryEntry& E) { return E.SlotIndex == SlotIndex; });
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


// Debug inventory
void UZfInventoryComponent::DebugInventory()
{
    for (const FZfInventoryEntry& Entry : InventoryList.Entries)
    {
        FString ItemName = TEXT("null");

        if (Entry.Item && Entry.Item->ItemDefinition)
        {
            ItemName = Entry.Item->ItemDefinition->ItemName.ToString();
        }
        else if (Entry.Item)
        {
            ItemName = TEXT("Item sem definição");
        }

        GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow,
            FString::Printf(TEXT("Slot %d: %s"), Entry.SlotIndex, *ItemName));
    }
}