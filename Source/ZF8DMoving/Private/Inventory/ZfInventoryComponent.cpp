

#include "Inventory/ZfInventoryComponent.h"
#include "Inventory/ZfItemDefinition.h"
#include "Inventory/ZfItemActor.h"
#include "Inventory/Fragments/ZfItemFragment.h"
#include "Inventory/Fragments/ZfStackableFragment.h"
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
void FZfInventoryList::AddItem(UZfItemInstance* Item)
{
    if (!Item) return;

    FZfInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
    NewEntry.Item = Item;
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

void UZfInventoryComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UZfInventoryComponent, InventoryList);
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

    InventoryList.AddItem(InItem);
    OnItemAdded.Broadcast(InItem);
}

void UZfInventoryComponent::Server_RemoveItem_Implementation(UZfItemInstance* InItem)
{
    if (!InItem) return;

    InventoryList.RemoveItem(InItem);
    OnItemRemoved.Broadcast(InItem);
}

void UZfInventoryComponent::Server_DropItem_Implementation(
    UZfItemInstance* InItem, FVector Location)
{
    if (!InItem || !GetWorld()) return;

    InventoryList.RemoveItem(InItem);
    OnItemRemoved.Broadcast(InItem);

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AZfItemActor* DroppedActor = GetWorld()->SpawnActor<AZfItemActor>(
        AZfItemActor::StaticClass(), Location, FRotator::ZeroRotator, Params
    );

    if (DroppedActor)
    {
        InItem->Rename(nullptr, DroppedActor);
        DroppedActor->InitializeWithItem(InItem);
    }
}

TArray<UZfItemInstance*> UZfInventoryComponent::GetAllItems() const
{
    TArray<UZfItemInstance*> Items;
    for (const FZfInventoryEntry& Entry : InventoryList.Entries)
    {
        if (Entry.Item)
        {
            Items.Add(Entry.Item);
        }
    }
    return Items;
}
