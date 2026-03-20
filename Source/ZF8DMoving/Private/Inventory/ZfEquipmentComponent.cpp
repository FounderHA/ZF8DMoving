#include "Inventory/ZfEquipmentComponent.h"
#include "Inventory/ZfInventoryComponent.h"
#include "Inventory/ZfItemInstance.h"
#include "Inventory/ZfItemEquipped.h"
#include "Inventory/ZfItemDefinition.h"
#include "Inventory/Fragments/ZfEquippableFragment.h"
#include "Inventory/Fragments/ZfBackpackFragment.h"
#include "Player/ZfPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Tests/ToolMenusTestUtilities.h"

UZfEquipmentComponent::UZfEquipmentComponent()
{
    SetIsReplicatedByDefault(true);
}

void UZfEquipmentComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UZfEquipmentComponent, EquippedItems);
}

FZfEquipmentEntry* UZfEquipmentComponent::FindEntry(EZfEquipSlot Slot)
{
    return EquippedItems.FindByPredicate([Slot](const FZfEquipmentEntry& E)
    {
        return E.Slot == Slot;
    });
}

//
//============================ EquipItem ============================
//
void UZfEquipmentComponent::Server_EquipItem_Implementation(UZfItemInstance* InItem, UZfInventoryComponent* FromInventory, EZfEquipSlot TargetSlot, int32 UnequipToSlot)
{
    if (!InItem || !FromInventory) return;

    UZfEquippableFragment* Equippable = InItem->FindFragmentByClass<UZfEquippableFragment>();
    if (!Equippable || Equippable->EquipSlot == EZfEquipSlot::None) return;
    if (Equippable->EquipSlot != TargetSlot) return;

    // Se slot ocupado desequipa para o slot especificado
    if (IsSlotOccupied(Equippable->EquipSlot))
    {
        Server_UnequipItem_Implementation(Equippable->EquipSlot, FromInventory, UnequipToSlot);
    }

    FromInventory->Server_RemoveItem_Implementation(InItem);

    AZfPlayerState* PS = Cast<AZfPlayerState>(GetOwner());
    if (!PS) return;

    ACharacter* Character = Cast<ACharacter>(PS->GetPawn());
    if (!Character) return;

    FActorSpawnParameters Params;
    Params.Owner = Character;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AZfItemEquipped* SpawnedActor = GetWorld()->SpawnActor<AZfItemEquipped>(
        AZfItemEquipped::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);

    if (SpawnedActor)
    {
        SpawnedActor->InitializeWithItem(InItem, Character->GetMesh(), Equippable->SocketName);
        SpawnedActor->SetActorRelativeTransform(Equippable->AttachOffset);

        FZfEquipmentEntry NewEntry;
        NewEntry.Slot = Equippable->EquipSlot;
        NewEntry.Item = InItem;
        NewEntry.SpawnedActor = SpawnedActor;
        EquippedItems.Add(NewEntry);

        UZfBackpackFragment* Backpack = InItem->FindFragmentByClass<UZfBackpackFragment>();
        if (Backpack)
        {
            FromInventory->AddExtraSlots(Backpack->ExtraSlots);
        }

        OnItemEquipped.Broadcast(Equippable->EquipSlot, InItem);
    }
}

//
//============================ CanUnequipItem ============================
//
bool UZfEquipmentComponent::CanUnequipItem(EZfEquipSlot Slot, UZfInventoryComponent* ToInventory, int32 TargetInventorySlot) const
{
    if (Slot == EZfEquipSlot::None || !ToInventory) return false;

    const FZfEquipmentEntry* Entry = EquippedItems.FindByPredicate([Slot](const FZfEquipmentEntry& E) { return E.Slot == Slot; });
    if (!Entry || !Entry->Item) return false;

    const UZfBackpackFragment* Backpack = Entry->Item->FindFragmentByClass<UZfBackpackFragment>();
    if (Backpack && Backpack->ExtraSlots > 0)
    {
        // Tem item no slot alvo? → é swap, mochila vai pro slot do item (ReservedSlots = 0)
        // Slot vazio ou sem slot alvo? → mochila precisa de 1 slot (ReservedSlots = 1)
        bool bIsSwap = TargetInventorySlot >= 0 && !ToInventory->IsSlotEmpty(TargetInventorySlot);
        int32 ReservedSlots = bIsSwap ? 0 : 1;

        if (!ToInventory->CanRemoveExtraSlots(Backpack->ExtraSlots, ReservedSlots)) return false;
    }

    return true;
}

//
//============================ Equip Item ============================
//
void UZfEquipmentComponent::AddEquip(UZfItemInstance* InItem)
{
    if (!InItem) return;

    UZfEquippableFragment* Equippable = InItem->FindFragmentByClass<UZfEquippableFragment>();
    if (!Equippable) return;

    AZfPlayerState* PS = Cast<AZfPlayerState>(GetOwner());
    if (!PS) return;

    ACharacter* Character = Cast<ACharacter>(PS->GetPawn());
    if (!Character) return;

    FActorSpawnParameters Params;
    Params.Owner = Character;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AZfItemEquipped* SpawnedActor = GetWorld()->SpawnActor<AZfItemEquipped>(
        AZfItemEquipped::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);

    if (!SpawnedActor) return;

    SpawnedActor->InitializeWithItem(InItem, Character->GetMesh(), Equippable->SocketName);
    SpawnedActor->SetActorRelativeTransform(Equippable->AttachOffset);

    FZfEquipmentEntry NewEntry;
    NewEntry.Slot = Equippable->EquipSlot;
    NewEntry.Item = InItem;
    NewEntry.SpawnedActor = SpawnedActor;
    EquippedItems.Add(NewEntry);

    OnItemEquipped.Broadcast(Equippable->EquipSlot, InItem);
}

//
//============================ Remove Unequip ============================
//
void UZfEquipmentComponent::RemoveEquip(EZfEquipSlot Slot)
{
    FZfEquipmentEntry* Entry = FindEntry(Slot);
    if (!Entry) return;

    UZfItemInstance* ItemBroadcast = Entry->Item;

    if (Entry->SpawnedActor)
    {
        Entry->SpawnedActor->Destroy();
        Entry->SpawnedActor = nullptr;
    }

    EquippedItems.RemoveAll([Slot](const FZfEquipmentEntry& E) { return E.Slot == Slot; });
    OnItemUnequipped.Broadcast(Slot, ItemBroadcast);
}

//
//============================ UnequipItem ============================
//
void UZfEquipmentComponent::Server_UnequipItem_Implementation(EZfEquipSlot Slot, UZfInventoryComponent* ToInventory, int32 TargetInventorySlot)
{

    FZfEquipmentEntry* Entry = FindEntry(Slot);
    if (!Entry || !Entry->Item) return;

    //Tenho Requisitos Mínimos? 

    UZfItemInstance* ItemToReturn = Entry->Item;
    UZfItemInstance* ItemAtTarget = ToInventory->GetItemAtSlot(TargetInventorySlot);
    
    
    

    if (ItemToReturn->FindFragmentByClass<UZfBackpackFragment>())
    {
        UZfBackpackFragment* BackpackFromEquipment = ItemToReturn->FindFragmentByClass<UZfBackpackFragment>();
        UZfBackpackFragment* BackpackFromInventory = ItemAtTarget ? ItemAtTarget->FindFragmentByClass<UZfBackpackFragment>() : nullptr;

        // Swap de mochila por mochila
        if (BackpackFromInventory)
        {
            int32 DiferenceSlots = BackpackFromInventory->ExtraSlots - BackpackFromEquipment->ExtraSlots;
            if (DiferenceSlots < 0 && !ToInventory->CanRemoveExtraSlots(FMath::Abs(DiferenceSlots))) return;

            // Swap de mochila por mochila
            ToInventory->Server_RemoveItem_Implementation(ItemAtTarget);
            RemoveEquip(Slot);
            // 1. Adiciona mochila antiga ANTES de remover slots
            ToInventory->Server_AddItemAtSlot_Implementation(ItemToReturn, TargetInventorySlot);
            // 2. Remove slots antigos
            ToInventory->TryRemoveExtraSlots(BackpackFromEquipment->ExtraSlots);
            // 3. Equipa nova mochila e adiciona slots novos
            AddEquip(ItemAtTarget);
            return;
        }

        // Só desequipa a mochila
        if (!ToInventory->CanRemoveExtraSlots(BackpackFromEquipment->ExtraSlots, 1)) return;

        int32 TargetSlot = TargetInventorySlot >= 0 && ToInventory->IsSlotEmpty(TargetInventorySlot) ? TargetInventorySlot : ToInventory->GetFirstEmptySlot();

        if (TargetSlot == -1) return;

        RemoveEquip(Slot);
        ToInventory->Server_AddItemAtSlot_Implementation(ItemToReturn, TargetSlot);
        ToInventory->TryRemoveExtraSlots(BackpackFromEquipment->ExtraSlots);
        return;
    }

    // Item normal - sem mochila
    if (TargetInventorySlot >= 0)
    {
        // Slot alvo tem item equipável do mesmo slot → swap
        if (ItemAtTarget)
        {
            const UZfEquippableFragment* TargetEquippable = ItemAtTarget->FindFragmentByClass<UZfEquippableFragment>();
            if (TargetEquippable && TargetEquippable->EquipSlot == Slot)
            {
                ToInventory->Server_RemoveItem_Implementation(ItemAtTarget);
                RemoveEquip(Slot);
                ToInventory->Server_AddItemAtSlot_Implementation(ItemToReturn, TargetInventorySlot);
                AddEquip(ItemAtTarget);
                return;
            }
        }

        // Slot alvo vazio → coloca direto
        if (ToInventory->IsSlotEmpty(TargetInventorySlot))
        {
            RemoveEquip(Slot);
            ToInventory->Server_AddItemAtSlot_Implementation(ItemToReturn, TargetInventorySlot);
            return;
        }
    }

    // Sem slot alvo ou slot ocupado por item diferente → primeiro slot vazio
    int32 FirstEmpty = ToInventory->GetFirstEmptySlot();
    if (FirstEmpty != -1)
    {
        RemoveEquip(Slot);
        ToInventory->Server_AddItemAtSlot_Implementation(ItemToReturn, FirstEmpty);
    }
        
        
    /*    
    if (!CanUnequipItem(Slot, ToInventory, TargetInventorySlot)) return;

    UZfItemInstance* ItemToReturn = Entry->Item;
    UZfBackpackFragment* Backpack = ItemToReturn->FindFragmentByClass<UZfBackpackFragment>();

    // Caso 1: slot alvo tem item → SWAP
    if (TargetInventorySlot >= 0)
    {
        UZfItemInstance* ItemAtTarget = ToInventory->GetItemAtSlot(TargetInventorySlot);
        if (ItemAtTarget)
        {
            const UZfEquippableFragment* TargetEquippable = ItemAtTarget->FindFragmentByClass<UZfEquippableFragment>();
            if (TargetEquippable && TargetEquippable->EquipSlot == Slot)
            {
                ToInventory->Server_RemoveItem_Implementation(ItemAtTarget);
                InternalUnequip(Slot, ToInventory, TargetInventorySlot);

                // Swap: mochila vai pro slot do item → adiciona primeiro → move extras
                ToInventory->Server_AddItemAtSlot_Implementation(ItemToReturn, TargetInventorySlot);
                if (Backpack) ToInventory->TryRemoveExtraSlots(Backpack->ExtraSlots, 0);

                Server_EquipItem_Implementation(ItemAtTarget, ToInventory, Slot, TargetInventorySlot);
                return;
            }
        }

        // Caso 2: slot alvo vazio
        if (ToInventory->IsSlotEmpty(TargetInventorySlot))
        {
            InternalUnequip(Slot, ToInventory, TargetInventorySlot);

            // Adiciona mochila primeiro → slot fica ocupado → move extras sem conflito
            ToInventory->Server_AddItemAtSlot_Implementation(ItemToReturn, TargetInventorySlot);
            if (Backpack) ToInventory->TryRemoveExtraSlots(Backpack->ExtraSlots, 0);
            return;
        }
    }

    // Caso 3: primeiro slot vazio disponível
    int32 FirstEmpty = ToInventory->GetFirstEmptySlot();
    if (FirstEmpty != -1)
    {
        InternalUnequip(Slot, ToInventory, -1);

        // Adiciona mochila primeiro → depois move extras
        ToInventory->Server_AddItemAtSlot_Implementation(ItemToReturn, FirstEmpty);
        if (Backpack) ToInventory->TryRemoveExtraSlots(Backpack->ExtraSlots, 0);
    }
    */
}

//
//============================ Helpers ============================
//
UZfItemInstance* UZfEquipmentComponent::GetEquippedItem(EZfEquipSlot Slot) const
{
    const FZfEquipmentEntry* Entry = EquippedItems.FindByPredicate(
        [Slot](const FZfEquipmentEntry& E) { return E.Slot == Slot; });
    if (!Entry) return nullptr;
    return Entry->Item;
}

bool UZfEquipmentComponent::IsSlotOccupied(EZfEquipSlot Slot) const
{
    const FZfEquipmentEntry* Entry = EquippedItems.FindByPredicate(
        [Slot](const FZfEquipmentEntry& E) { return E.Slot == Slot; });
    return Entry && Entry->Item != nullptr;
}

//
//============================ Debug ============================
//
void UZfEquipmentComponent::DebugEquipament()
{
    for (const FZfEquipmentEntry& Entry : EquippedItems)
    {
        if (!Entry.Item) continue;

        FString ItemName = TEXT("sem definicao");
        if (UZfItemDefinition* Def = Entry.Item->ItemDefinition.Get())
        {
            ItemName = Def->ItemName.ToString();
        }

        FString SlotName = UEnum::GetValueAsString(Entry.Slot);
        FString ActorInfo = Entry.SpawnedActor ? Entry.SpawnedActor->GetName() : TEXT("sem actor");

        GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow,
            FString::Printf(TEXT("[Slot %s] Item: %s | Fragments: %d | Actor: %s"),
                *SlotName, *ItemName, Entry.Item->Fragments.Num(), *ActorInfo));

        for (int32 i = 0; i < Entry.Item->Fragments.Num(); i++)
        {
            UZfItemFragment* Fragment = Entry.Item->Fragments[i];
            FString FragName = Fragment ? Fragment->GetClass()->GetName() : TEXT("nullptr");
            GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Cyan,
                FString::Printf(TEXT("  Fragment[%d]: %s"), i, *FragName));
        }
    }
}      