// Copyright ZfGame Studio. All Rights Reserved.
// ZfEquipmentComponent.cpp

#include "Inventory/ZfEquipmentComponent.h"
#include "Inventory/ZfInventoryComponent.h"
#include "Inventory/ZfItemInstance.h"
#include "Inventory/ZfItemDefinition.h"
#include "Inventory/Fragments/ZfFragment_Equippable.h"
#include "Inventory/Fragments/ZfFragment_SetPiece.h"
#include "Inventory/Fragments/ZfFragment_InventoryExpansion.h"
#include "Inventory/ZfModifierDataTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "GameplayTagsManager.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Inventory/Fragments/ZfFragment_Consumable.h"
#include "Inventory/Fragments/ZfFragment_Stackable.h"
#include "Player/ZfPlayerState.h"
#include "Systems/RefinerySystem/ZfRefineryTypes.h"

// ============================================================
// FAST ARRAY CALLBACKS
// ============================================================

void FZfEquipmentSlotEntry::PreReplicatedRemove(const FZfEquipmentList& InArraySerializer)
{
    if (InArraySerializer.OwnerComponent && ItemInstance)
    {
        InArraySerializer.OwnerComponent->OnItemUnequipped.Broadcast(SlotTag);
    }
}

void FZfEquipmentSlotEntry::PostReplicatedAdd(const FZfEquipmentList& InArraySerializer)
{
    if (InArraySerializer.OwnerComponent && ItemInstance)
    {
        InArraySerializer.OwnerComponent->OnItemEquipped.Broadcast(ItemInstance, SlotTag);
    }
}

void FZfEquipmentSlotEntry::PostReplicatedChange(const FZfEquipmentList& InArraySerializer)
{
    if (InArraySerializer.OwnerComponent && ItemInstance)
    {
        InArraySerializer.OwnerComponent->OnStackChanged.Broadcast(ItemInstance, SlotTag);
    }
}

DEFINE_LOG_CATEGORY(LogZfInventory);

// ============================================================
// CONSTRUCTOR
// ============================================================

UZfEquipmentComponent::UZfEquipmentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
    bReplicateUsingRegisteredSubObjectList = true;
}

// ============================================================
// INTERFACE
// ============================================================

void UZfEquipmentComponent::AddItemToTargetInterface_Implementation(UObject* ItemComesFrom, UZfItemInstance* InItemInstance, int32 AmountToAdd,
        int32 SlotIndexComesFrom, int32 TargetSlotIndex,
        EZfRefinerySlotType SlotTypeComesFrom, EZfRefinerySlotType TargetSlotType,
        FGameplayTag SlotTagComesFrom, FGameplayTag TargetSlotTag)
{
    if (InternalCheckIsServer("AddItemToTargetInterface_Implementation"))
    {
        TryEquipItemInterface(ItemComesFrom, InItemInstance, AmountToAdd, SlotIndexComesFrom, TargetSlotIndex, SlotTypeComesFrom, TargetSlotType, SlotTagComesFrom, TargetSlotTag);
    }
}

void UZfEquipmentComponent::RemoveItemFromTargetInterface_Implementation(int32 ItemAmountToRemove, int32 TargetSlotIndex, EZfRefinerySlotType TargetSlotType, FGameplayTag TargetSlotTag)
{
    if (InternalCheckIsServer("RemoveItemFromTargetInterface_Implementation"))
    {
        TryUnequipItemInterface(ItemAmountToRemove, TargetSlotIndex, TargetSlotType, TargetSlotTag);
    }
}

// ============================================================
// CICLO DE VIDA
// ============================================================

void UZfEquipmentComponent::BeginPlay()
{
    Super::BeginPlay();

    if (GetOwner())
    {
        InventoryComponent = Cast<AZfPlayerState>(GetOwner())->FindComponentByClass<UZfInventoryComponent>();
    }
}

// ============================================================
// REPLICAÇÃO
// ============================================================

void UZfEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UZfEquipmentComponent, EquipmentList);
    DOREPLIFETIME(UZfEquipmentComponent, bOffHandSlotBlocked);
}

// ============================================================
// DESEQUIPAR TODOS
// ============================================================

void UZfEquipmentComponent::UnequipAllItems()
{
    if (!InternalCheckIsServer(TEXT("UnequipAllItems"))) return;

    TArray<FGameplayTag> SlotsToUnequip;
    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.ItemInstance) SlotsToUnequip.Add(Entry.SlotTag);
    }

    for (const FGameplayTag& SlotTag : SlotsToUnequip)
    {
        //UnequipItemAtSlot(SlotTag);
    }

    UE_LOG(LogZfInventory, Log,
        TEXT("UZfEquipmentComponent::UnequipAllItems — %d itens desequipados."),
        SlotsToUnequip.Num());
}

// ============================================================
// TROCA RÁPIDA
// ============================================================

EZfItemMechanicResult UZfEquipmentComponent::QuickSwapItem(int32 InventorySlotIndex, FGameplayTag EquipmentSlotTag, int32 EquipSlotIndex)
{
    if (!InternalCheckIsServer(TEXT("QuickSwapItem"))) return EZfItemMechanicResult::Failed_InvalidOperation;
    if (!InventoryComponent)                           return EZfItemMechanicResult::Failed_InvalidOperation;

    UZfItemInstance* ItemFromInventory = InventoryComponent->GetItemAtSlot(InventorySlotIndex);
    if (!ItemFromInventory) return EZfItemMechanicResult::Failed_ItemNotFound;

    return EZfItemMechanicResult::Success;
}

bool UZfEquipmentComponent::ServerRequestQuickSwap_Validate(int32 InventorySlotIndex, FGameplayTag EquipSlotTag, int32 EquipSlotIndex)
{
    return InventorySlotIndex >= 0 &&
           EquipSlotTag != ZfEquipmentTags::EquipmentSlots::Slot_None &&
           EquipSlotIndex >= 0;
}

void UZfEquipmentComponent::ServerRequestQuickSwap_Implementation(int32 InventorySlotIndex, FGameplayTag EquipSlotTag, int32 EquipSlotIndex)
{
    QuickSwapItem(InventorySlotIndex, EquipSlotTag, EquipSlotIndex);
}

// ============================================================
// DURABILIDADE
// ============================================================

void UZfEquipmentComponent::NotifyEquippedItemBroken(UZfItemInstance* ItemInstance)
{
    if (!ItemInstance || !IsItemEquipped(ItemInstance)) return;

    Internal_RemoveItemGameplayEffects(ItemInstance);
    Internal_UpdateSetBonuses(ItemInstance, false);
    OnEquippedItemBroken.Broadcast(ItemInstance);

    UE_LOG(LogZfInventory, Warning,
        TEXT("UZfEquipmentComponent::NotifyEquippedItemBroken — Item '%s' quebrou."),
        *ItemInstance->GetItemName().ToString());
}

void UZfEquipmentComponent::NotifyEquippedItemRepaired(UZfItemInstance* ItemInstance)
{
    if (!ItemInstance || !IsItemEquipped(ItemInstance)) return;

    Internal_ApplyItemGameplayEffects(ItemInstance);
    Internal_UpdateSetBonuses(ItemInstance, true);
    OnEquippedItemRepaired.Broadcast(ItemInstance);

    UE_LOG(LogZfInventory, Log,
        TEXT("UZfEquipmentComponent::NotifyEquippedItemRepaired — Item '%s' reparado."),
        *ItemInstance->GetItemName().ToString());
}

// ============================================================
// SERVER RPCs
// ============================================================

void UZfEquipmentComponent::ServerTryEquipItem_Implementation(UZfItemInstance* ItemInstance, int32 FromInventorySlot, FGameplayTag SlotTag)
{
    if (InternalCheckIsServer(TEXT("ServerTryEquipItem")))
        TryEquipItem(ItemInstance, FromInventorySlot, SlotTag);
}

void UZfEquipmentComponent::ServerTryUnequipItem_Implementation(FGameplayTag SlotTag, int32 TargetInventorySlot)
{
    if (InternalCheckIsServer(TEXT("ServerTryUnequipItem")))
        TryUnequipItem(SlotTag, TargetInventorySlot);
}

void UZfEquipmentComponent::ServerTryUseQuickSlot_Implementation(FGameplayTag SlotTag)
{
    if (InternalCheckIsServer(TEXT("ServerTryUseQuickSlot")))
        TryUseQuickSlot(SlotTag);
}

void UZfEquipmentComponent::ServerTryRemoveItemStackFromEquipmentSlot_Implementation(FGameplayTag SlotTag)
{
    if (InternalCheckIsServer(TEXT("ServerTryRemoveItemStackFromEquipmentSlot")))
        TryRemoveItemStackFromEquipmentSlot(SlotTag);
}

// ============================================================
// FUNÇÕES PRINCIPAIS — EQUIPAR / DESEQUIPAR
// ============================================================

EZfItemMechanicResult UZfEquipmentComponent::TryEquipItem(UZfItemInstance* ItemFromInventory, int32 FromInventorySlot, FGameplayTag SlotTag)
{
    if (!ItemFromInventory) return EZfItemMechanicResult::Failed_InvalidOperation;

    const UZfFragment_Equippable* EquippableFragment = ItemFromInventory->GetFragment<UZfFragment_Equippable>();
    if (!EquippableFragment) return EZfItemMechanicResult::Failed_CannotEquip;

    // ── Resolve o slot real ───────────────────────────────────────────
    // Se nenhum slot foi especificado, usa o do fragment como pai
    // e encontra o primeiro filho vazio.
    // Se um slot foi especificado mas é tag pai (tem filhos), também resolve.
    FGameplayTag ResolvedSlot = SlotTag;

    const FGameplayTag ParentToResolve = SlotTag.IsValid() ? SlotTag : EquippableFragment->EquipmentSlotTag;
    const FGameplayTagContainer Children = UGameplayTagsManager::Get().RequestGameplayTagChildren(ParentToResolve);

    // Se não tem filhos, a tag passada já é filha — sobe para o pai
    const FGameplayTag ActualParent = Children.IsEmpty() ? ParentToResolve.RequestDirectParent() : ParentToResolve;

    // QUICK TRANSFER
    if (SlotTag == FGameplayTag::EmptyTag)
    {
        // ── Stackável — tenta empilhar em slot existente antes de ocupar novo ─────
        const UZfFragment_Stackable* StackFragment = ItemFromInventory->GetFragment<UZfFragment_Stackable>();
        if (StackFragment)
        {
            // Verifica se o item é compatível com o slot antes de tentar empilhar
            if (!ActualParent.MatchesTag(EquippableFragment->EquipmentSlotTag) && !EquippableFragment->EquipmentSlotTag.MatchesTag(ActualParent))
            {
                return EZfItemMechanicResult::Failed_CannotEquip;
            }
            
            const int32 OriginalStack = ItemFromInventory->GetCurrentStack();
            
            // Procura slot filho ocupado com o mesmo ItemDefinition
            for (FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
            {
                if (!Entry.ItemInstance) continue;
                if (!Entry.SlotTag.MatchesTag(ParentToResolve)) continue;
                if (Entry.ItemInstance->GetItemDefinition() != ItemFromInventory->GetItemDefinition()) continue;
                if (Entry.ItemInstance->GetCurrentStack() == StackFragment->MaxStackSize) continue;
                
                const int32 Overflow = Entry.ItemInstance->AddToStack(ItemFromInventory->GetCurrentStack());
                EquipmentList.MarkItemDirty(Entry);
                OnStackChanged.Broadcast(Entry.ItemInstance, Entry.SlotTag);

                
                if (Overflow == 0)
                {
                    // Tudo coube — remove do inventário a quantidade toda
                    return EZfItemMechanicResult::Success;
                }

                if (Overflow < 0) return EZfItemMechanicResult::Failed_InvalidOperation;

                // Overflow — remove do inventário apenas o que foi absorvido
                const int32 Absorbed = OriginalStack - Overflow;
                if (Absorbed > 0)
                {
                    //Execute_AddItemToTargetInterface(ItemComesFrom, TargetSlotIndex, SlotIndexComesFrom, Item, Overflow, this, SlotType, FGameplayTag(), false);
                }
            }
            
            ResolvedSlot = GetFirstEmptySlotByParentTag(ActualParent);
            if (ResolvedSlot.IsValid())
            {
                if (OriginalStack == ItemFromInventory->GetCurrentStack())
                {
                    InventoryComponent->TryRemoveItemFromInventory(InventoryComponent->GetSlotIndexOfItem(ItemFromInventory));
                    InternalEquipItem(ItemFromInventory, ResolvedSlot);
                }
                return EZfItemMechanicResult::Success;
            }
            return EZfItemMechanicResult::Failed_InventoryFull;
        }
    }
    
    
    
    if (!Children.IsEmpty())
    {
        // Tag pai — encontra o primeiro filho vazio
        ResolvedSlot = GetFirstEmptySlotByParentTag(ParentToResolve);
        if (!ResolvedSlot.IsValid())
            return EZfItemMechanicResult::Failed_InventoryFull; // todos os slots cheios
    }

    // Valida que o slot resolvido pertence ao tipo correto do item
    if (ResolvedSlot.IsValid() && !ResolvedSlot.MatchesTag(EquippableFragment->EquipmentSlotTag))
        return EZfItemMechanicResult::Failed_CannotEquip;

    if (CanEquipItem(ItemFromInventory, SlotTag, SlotTag))
        return EZfItemMechanicResult::Failed_InvalidOperation;

    // Mochila tem fluxo especial
    if (ItemFromInventory->GetFragment<UZfFragment_InventoryExpansion>())
    {
        if (TryEquipBackpack(ResolvedSlot, ItemFromInventory, 1) == EZfItemMechanicResult::Success)
            return EZfItemMechanicResult::Success;
        else
            return EZfItemMechanicResult::Failed_InventoryFull;
    }

    // Slot já ocupado — troca
    if (UZfItemInstance* EquipedItem = GetItemAtSlotTag(ResolvedSlot))
    {
        InternalUnequipItem(ResolvedSlot);
        InternalEquipItem(ItemFromInventory, ResolvedSlot);

        InventoryComponent->TryRemoveItemFromInventory(FromInventorySlot);
        InventoryComponent->TryAddItemToSpecificSlot(EquipedItem, FromInventorySlot);
        
        return EZfItemMechanicResult::Success;
    }

    // Slot vazio
    InternalEquipItem(ItemFromInventory, ResolvedSlot);
    InventoryComponent->TryRemoveItemFromInventory(FromInventorySlot);
    return EZfItemMechanicResult::Success;
}

EZfItemMechanicResult UZfEquipmentComponent::TryEquipItemInterface(UObject* ItemComesFrom, UZfItemInstance* InItemInstance, int32 AmountToAdd,
        int32 SlotIndexComesFrom, int32 TargetSlotIndex,
        EZfRefinerySlotType SlotTypeComesFrom, EZfRefinerySlotType TargetSlotType,
        FGameplayTag SlotTagComesFrom, FGameplayTag TargetSlotTag)
{
    if (TargetSlotTag == SlotTagComesFrom) return EZfItemMechanicResult::Failed_InvalidOperation;
    
    if (InItemInstance->GetFragment<UZfFragment_Equippable>()->EquipmentSlotTag != TargetSlotTag.RequestDirectParent()) return EZfItemMechanicResult::Failed_IncompatibleItem;
    
    if (!InItemInstance) return EZfItemMechanicResult::Failed_ItemNotFound;

    // Tenho Requisistos para equipar este Item --------------------------------------------------------------------------------------

    //if (CanEquipItem(InItemInstance, SlotTagComesFrom, TargetSlotTag)) return EZfItemMechanicResult::Failed_IncompatibleItem;
    
    if (InItemInstance->GetFragment<UZfFragment_Stackable>())
    {
        if (AmountToAdd < 0 || AmountToAdd > InItemInstance->GetFragment<UZfFragment_Stackable>()->MaxStackSize) return EZfItemMechanicResult::Failed_IncompatibleItem;
    }
    
    // Pega Slot Vazio se Tiver
    FGameplayTag EmpitySlotTag = GetFirstEmptySlotByParentTag(TargetSlotTag);










    // ==================== DRAG AND DROP ====================
    
    UZfItemInstance* ItemAtTarget = GetItemAtEquipmentSlot(TargetSlotTag);

    // Slot vazio
    if (!ItemAtTarget)
    {
        if (InItemInstance->GetCurrentStack() != AmountToAdd)
        {
            UZfItemInstance* NewItem = InItemInstance->CreateShallowCopy(AmountToAdd);
            Execute_RemoveItemFromTargetInterface(ItemComesFrom, AmountToAdd, SlotIndexComesFrom, SlotTypeComesFrom, SlotTagComesFrom);
            InternalEquipItem(NewItem, TargetSlotTag);
            if (InItemInstance->GetFragment<UZfFragment_InventoryExpansion>())
            {
                InventoryComponent->UpdateSlotCountFromEquippedBackpack();
            }
            return EZfItemMechanicResult::Success;
        }
        InternalEquipItem(InItemInstance, TargetSlotTag);
        if (InItemInstance->GetFragment<UZfFragment_InventoryExpansion>())
        {
            InventoryComponent->UpdateSlotCountFromEquippedBackpack();
        }
        Execute_RemoveItemFromTargetInterface(ItemComesFrom, AmountToAdd, SlotIndexComesFrom, SlotTypeComesFrom, SlotTagComesFrom);
        return EZfItemMechanicResult::Success;
    }

    // Mochila Já Equipada
    if (InItemInstance->GetFragment<UZfFragment_InventoryExpansion>())
    {
        UZfItemInstance* BackpackAtInventory = InItemInstance;
        UZfItemInstance* EquipedBackpack = GetItemAtSlotTag(TargetSlotTag);
        
        int32 NewExtraSlots = BackpackAtInventory->GetFragment<UZfFragment_InventoryExpansion>()->ExtraSlotCount;
        int32 OldExtraSlots = EquipedBackpack->GetFragment<UZfFragment_InventoryExpansion>()->ExtraSlotCount;

        if (NewExtraSlots - OldExtraSlots >= 0)
        {
            InternalUnequipItem(TargetSlotTag);
            InternalEquipItem(BackpackAtInventory, TargetSlotTag);

            Execute_RemoveItemFromTargetInterface(ItemComesFrom, BackpackAtInventory->GetCurrentStack(), SlotIndexComesFrom, SlotTypeComesFrom, SlotTagComesFrom);
            Execute_AddItemToTargetInterface(ItemComesFrom, this, EquipedBackpack, EquipedBackpack->GetCurrentStack(),TargetSlotIndex, SlotIndexComesFrom, TargetSlotType, SlotTypeComesFrom, TargetSlotTag, SlotTagComesFrom);
            
            InventoryComponent->UpdateSlotCountFromEquippedBackpack();
            
            return EZfItemMechanicResult::Success;
        }
        else
        {
            int32 NewCapacity    = InventoryComponent->GetDefaultSlotCount() + NewExtraSlots;
            int32 ItensToMove    = InventoryComponent->GetItemCountFromInitialSlot(NewCapacity);
            int32 AvaliableSlots = InventoryComponent->GetAvailableSlotsWithExpansion(BackpackAtInventory);

            if (AvaliableSlots >= ItensToMove)
            {
                InternalUnequipItem(TargetSlotTag);
                InternalEquipItem(BackpackAtInventory, TargetSlotTag);
            
                Execute_AddItemToTargetInterface(ItemComesFrom, this, EquipedBackpack, EquipedBackpack->GetCurrentStack(),TargetSlotIndex, SlotIndexComesFrom, TargetSlotType, SlotTypeComesFrom, TargetSlotTag, SlotTagComesFrom);
                InventoryComponent->RelocateItemsAboveCapacity(NewCapacity);
                InventoryComponent->UpdateSlotCountFromEquippedBackpack();
            
                return EZfItemMechanicResult::Success;
            }
            else
            {
                return EZfItemMechanicResult::Failed_InventoryFull;
            }
        }
    }
    
    // Item Stackavel
    if (InItemInstance->GetFragment<UZfFragment_Stackable>())
    {
        // Item No Slot é Diferente
        if (ItemAtTarget->GetItemDefinition() != InItemInstance->GetItemDefinition()) return EZfItemMechanicResult::Failed_SlotBlocked;
        
        int32 Overflow = TryAddToStack(TargetSlotTag, AmountToAdd);
        if (Overflow == 0)
        {
            // Completamente absorvido pelo Stack - Remover Tudo do Slot Antigo
            Execute_RemoveItemFromTargetInterface(ItemComesFrom, AmountToAdd, SlotIndexComesFrom, SlotTypeComesFrom, SlotTagComesFrom);
            return EZfItemMechanicResult::Success;
        }

        if (Overflow < 0) return EZfItemMechanicResult::Failed_InvalidOperation;

        if (Overflow > 0)
        {
            // Remover Quantidade que foi Absorvida
            Execute_RemoveItemFromTargetInterface(ItemComesFrom, AmountToAdd - Overflow, SlotIndexComesFrom, SlotTypeComesFrom, SlotTagComesFrom);
            return EZfItemMechanicResult::Success;
        }
    }

    return EZfItemMechanicResult::Failed_SlotBlocked;
}

EZfItemMechanicResult UZfEquipmentComponent::TryUnequipItem(FGameplayTag SlotTag, int32 TargetInventorySlot)
{

    
    FZfEquipmentSlotEntry* SlotEntry = InternalFindSlotEntry(SlotTag);
    if (!SlotEntry || !SlotEntry->ItemInstance) return EZfItemMechanicResult::Failed_ItemNotFound;
    if (!InventoryComponent)                    return EZfItemMechanicResult::Failed_InvalidOperation;

    UZfItemInstance* ItemInEquipment = SlotEntry->ItemInstance;

    // Mochila tem fluxo especial
    if (ItemInEquipment->GetFragment<UZfFragment_InventoryExpansion>())
    {
        if (TryUnequipBackpack(GetEquipmentSlotOfItem(ItemInEquipment), TargetInventorySlot) == EZfItemMechanicResult::Success)
            return EZfItemMechanicResult::Success;
        else
            return EZfItemMechanicResult::Failed_InventoryFull;
    }

    // Slot do inventário ocupado
    if (UZfItemInstance* ItemAtInventory = InventoryComponent->GetItemAtSlot(TargetInventorySlot))
    {
        const UZfFragment_Equippable* AtInvEquip = ItemAtInventory->GetFragment<UZfFragment_Equippable>();
        if (AtInvEquip && AtInvEquip->EquipmentSlotTag == SlotTag)
        {
            if (CanEquipItem(ItemAtInventory, SlotTag, SlotTag))
            {
                InternalUnequipItem(SlotTag);
                InternalEquipItem(ItemAtInventory, SlotTag);

                InventoryComponent->TryRemoveItemFromInventory(TargetInventorySlot);
                InventoryComponent->TryAddItemToSpecificSlot(ItemInEquipment, TargetInventorySlot);
                
                return EZfItemMechanicResult::Success;
            }
        }

        if (InventoryComponent->GetAvailableSlots() >= 1)
        {
            InternalUnequipItem(SlotTag);
            InventoryComponent->TryAddItemToInventory(ItemInEquipment);
            return EZfItemMechanicResult::Success;
        }

        return EZfItemMechanicResult::Failed_InventoryFull;
    }

    // Slot do inventário vazio
    InternalUnequipItem(SlotTag);
    InventoryComponent->TryAddItemToSpecificSlot(ItemInEquipment, TargetInventorySlot);
    return EZfItemMechanicResult::Success;
}

EZfItemMechanicResult UZfEquipmentComponent::TryUnequipItemInterface(int32 ItemAmountToRemove, int32 TargetSlotIndex, EZfRefinerySlotType TargetSlotType, FGameplayTag TargetSlotTag)
{

    UZfItemInstance* InItemInstance = GetItemAtEquipmentSlot(TargetSlotTag);
    
    if (!InItemInstance) return EZfItemMechanicResult::Failed_InvalidOperation;

    if (!CanUnequipItem(TargetSlotTag)) return EZfItemMechanicResult::Failed_InvalidOperation;

    if (InItemInstance->GetFragment<UZfFragment_Stackable>())
    {
        if (ItemAmountToRemove < 0 || ItemAmountToRemove > InItemInstance->GetFragment<UZfFragment_Stackable>()->MaxStackSize) return EZfItemMechanicResult::Failed_IncompatibleItem;
    }
    
    int32 ItemAmountAtSlot = InItemInstance->GetCurrentStack();
    
    if (ItemAmountToRemove > ItemAmountAtSlot) return EZfItemMechanicResult::Failed_InvalidOperation;
    
    if (ItemAmountToRemove == ItemAmountAtSlot)
    {
        InternalUnequipItem(TargetSlotTag);
        if (InItemInstance->GetFragment<UZfFragment_InventoryExpansion>())
        {
            InventoryComponent->RelocateItemsAboveCapacity(InventoryComponent->DefaultSlotCount);
            InventoryComponent->UpdateSlotCountFromEquippedBackpack();
        }
        return EZfItemMechanicResult::Success;
    }

    InItemInstance->SetCurrentStack(ItemAmountAtSlot - ItemAmountToRemove);
    FZfEquipmentSlotEntry* Entry = FindSlotEntryByItem(InItemInstance);
    if (Entry)
    {
        EquipmentList.MarkItemDirty(*Entry);
    }
    OnStackChanged.Broadcast(InItemInstance, TargetSlotTag);
    return EZfItemMechanicResult::Success;
}

EZfItemMechanicResult UZfEquipmentComponent::TryEquipBackpack(FGameplayTag SlotTag, UZfItemInstance* InItemInstance, int32 FromInventorySlot)
{
    if (!InventoryComponent) return EZfItemMechanicResult::Failed_InvalidOperation;

    UZfItemInstance* BackpackAtInventory = InItemInstance;

    if (UZfItemInstance* EquipedBackpack = GetItemAtSlotTag(SlotTag))
    {
        int32 NewExtraSlots = BackpackAtInventory->GetFragment<UZfFragment_InventoryExpansion>()->ExtraSlotCount;
        int32 OldExtraSlots = EquipedBackpack->GetFragment<UZfFragment_InventoryExpansion>()->ExtraSlotCount;

        if (NewExtraSlots - OldExtraSlots >= 0)
        {
            InternalUnequipItem(SlotTag);
            InternalEquipItem(BackpackAtInventory, SlotTag);
            
            InventoryComponent->TryAddItemToSpecificSlot(EquipedBackpack, FromInventorySlot);
            InventoryComponent->UpdateSlotCountFromEquippedBackpack();
            
            return EZfItemMechanicResult::Success;
        }
        else
        {
            int32 NewCapacity    = InventoryComponent->GetDefaultSlotCount() + NewExtraSlots;
            int32 ItensToMove    = InventoryComponent->GetItemCountFromInitialSlot(NewCapacity);
            int32 AvaliableSlots = InventoryComponent->GetAvailableSlotsWithExpansion(BackpackAtInventory);

            if (AvaliableSlots >= ItensToMove)
            {
                InternalUnequipItem(SlotTag);
                InternalEquipItem(BackpackAtInventory, SlotTag);
                
                InventoryComponent->TryAddItemToSpecificSlot(EquipedBackpack, FromInventorySlot);
                InventoryComponent->RelocateItemsAboveCapacity(NewCapacity);
                InventoryComponent->UpdateSlotCountFromEquippedBackpack();
                
                return EZfItemMechanicResult::Success;
            }
            else
            {
                return EZfItemMechanicResult::Failed_InventoryFull;
            }
        }
    }
    else
    {
        InternalEquipItem(BackpackAtInventory, SlotTag);
        InventoryComponent->UpdateSlotCountFromEquippedBackpack();
        return EZfItemMechanicResult::Success;
    }
}

EZfItemMechanicResult UZfEquipmentComponent::TryUnequipBackpack(FGameplayTag SlotTag, int32 TargetInventorySlot)
{
    if (!InventoryComponent) return EZfItemMechanicResult::Failed_InvalidOperation;

    UZfItemInstance* BackpackAtInventory = InventoryComponent->GetItemAtSlot(TargetInventorySlot);
    UZfItemInstance* EquipedBackpack     = GetItemAtSlotTag(SlotTag);
    int32 OldExtraSlots = EquipedBackpack->GetFragment<UZfFragment_InventoryExpansion>()->ExtraSlotCount;

    if (BackpackAtInventory && BackpackAtInventory->GetFragment<UZfFragment_InventoryExpansion>())
    {
        int32 NewExtraSlots = BackpackAtInventory->GetFragment<UZfFragment_InventoryExpansion>()->ExtraSlotCount;

        if (NewExtraSlots - OldExtraSlots >= 0)
        {
            InternalUnequipItem(SlotTag);
            InternalEquipItem(BackpackAtInventory, SlotTag);

            InventoryComponent->TryRemoveItemFromInventory(TargetInventorySlot);
            InventoryComponent->TryAddItemToSpecificSlot(EquipedBackpack, TargetInventorySlot);
            InventoryComponent->UpdateSlotCountFromEquippedBackpack();
            
            return EZfItemMechanicResult::Success;
        }
        else
        {
            int32 AvaliableSlots = InventoryComponent->GetAvailableSlotsWithExpansion(BackpackAtInventory);
            int32 ItensToMove    = InventoryComponent->GetItemCountFromInitialSlot(AvaliableSlots);
            int32 NewCapacity    = InventoryComponent->GetDefaultSlotCount() + NewExtraSlots;

            if (AvaliableSlots - ItensToMove >= 0)
            {
                InternalUnequipItem(SlotTag);
                InternalEquipItem(BackpackAtInventory, SlotTag);

                InventoryComponent->TryRemoveItemFromInventory(TargetInventorySlot);
                InventoryComponent->TryAddItemToSpecificSlot(EquipedBackpack, TargetInventorySlot);
                InventoryComponent->RelocateItemsAboveCapacity(NewCapacity);
                InventoryComponent->UpdateSlotCountFromEquippedBackpack();

                OnItemUnequipped.Broadcast(BackpackAtInventory->GetFragment<UZfFragment_Equippable>()->EquipmentSlotTag);
                EquipmentList.MarkArrayDirty();
                return EZfItemMechanicResult::Success;
            }
            else
            {
                return EZfItemMechanicResult::Failed_InventoryFull;
            }
        }
    }
    else
    {
        int32 AvaliableSlots = InventoryComponent->GetAvailableSlotsWithExpansion(BackpackAtInventory);
        int32 ItensToMove    = InventoryComponent->GetItemCountFromInitialSlot(InventoryComponent->GetDefaultSlotCount());

        if (AvaliableSlots - 1 - ItensToMove >= 0)
        {
            InternalUnequipItem(EquipedBackpack->GetFragment<UZfFragment_Equippable>()->EquipmentSlotTag);

            InventoryComponent->TryAddItemToSpecificSlot(EquipedBackpack, TargetInventorySlot);
            InventoryComponent->RelocateItemsAboveCapacity(InventoryComponent->GetAvailableDefaultSlots());
            InventoryComponent->UpdateSlotCountFromEquippedBackpack();
            
            return EZfItemMechanicResult::Success;
        }

        return EZfItemMechanicResult::Failed_InventoryFull;
    }
}

void UZfEquipmentComponent::TryUseQuickSlot(FGameplayTag SlotTag)
{
    UZfItemInstance* Item = GetItemAtSlotTag(SlotTag);
    if (!Item) return;

    const UZfFragment_Consumable* Fragment = Item->GetFragment<UZfFragment_Consumable>();
    if (!Fragment) return;

    const UZfFragment_Stackable* Stackable = Item->GetFragment<UZfFragment_Stackable>();

    if (Stackable)
    {
        if (Fragment->bConsumeOnUse) TryRemoveItemStackFromEquipmentSlot(SlotTag);
    }
    else
    {
        if (Fragment->bConsumeOnUse)
        {
            InternalUnequipItem(SlotTag);
        }
    }

    UAbilitySystemComponent* ASC = Internal_GetAbilitySystemComponent();
    if (!ASC) return;

    FGameplayEventData EventData;
    EventData.OptionalObject = Item;
    EventData.EventTag       = ZfUniqueItemTags::ItemEvents::Item_Event_Use;
    ASC->HandleGameplayEvent(EventData.EventTag, &EventData);
}

void UZfEquipmentComponent::TryRemoveItemStackFromEquipmentSlot(FGameplayTag SlotTag)
{
    if (!InternalCheckIsServer(TEXT("TryRemoveItemStackFromEquipmentSlot"))) return;

    UZfItemInstance* Item = GetItemAtSlotTag(SlotTag);
    if (!Item) return;
    if (!Item->HasFragment<UZfFragment_Stackable>()) return;

    if (Item->RemoveFromStack(1))
    {
        InternalUnequipItem(SlotTag);
        return;
    }
    
    EquipmentList.MarkItemDirty(*FindSlotEntryByItem(Item));
    OnStackChanged.Broadcast(Item, SlotTag);
}

int32 UZfEquipmentComponent::TryAddToStack(FGameplayTag SlotTag, int32 Amount)
{
    if (!InternalCheckIsServer(TEXT("TryAddToStack"))) return Amount;

    UZfItemInstance* Item = GetItemAtSlotTag(SlotTag);
    if (!Item) return Amount;

    const int32 Overflow = Item->AddToStack(Amount);

    FZfEquipmentSlotEntry* Entry = FindSlotEntryByItem(Item);
    if (Entry)
    {
        EquipmentList.MarkItemDirty(*Entry);
    }

    OnStackChanged.Broadcast(Item, SlotTag);
    return Overflow;
}

// ============================================================
// FUNÇÕES DE CONSULTA
// ============================================================

bool UZfEquipmentComponent::CanEquipItem(UZfItemInstance* InItemInstance, FGameplayTag SlotTagComesFrom, FGameplayTag TargetSlotTag)
{
    if (!InItemInstance) return false;
    
    if (!InItemInstance->GetFragment<UZfFragment_Equippable>()) return false;

    if (InItemInstance->GetIsBroken()) return false;

    UZfItemInstance* ItemAtTarget = GetItemAtSlotTag(TargetSlotTag);
    // Mesmo Tipo de Slot
    if (!ItemAtTarget)
    {
        if (SlotTagComesFrom.RequestDirectParent() != TargetSlotTag.RequestDirectParent()) return false;
    }
    
    // Adidioncar Requisitos de Item aqui
    
    if (ItemAtTarget)
    {
        // Stackable
        if (InItemInstance->GetFragment<UZfFragment_Stackable>()) return true;

        //Mochila
        if (InItemInstance->GetFragment<UZfFragment_InventoryExpansion>())
        {
            UZfItemInstance* BackpackAtInventory = InItemInstance;
            UZfItemInstance* EquipedBackpack = ItemAtTarget;

            int32 NewExtraSlots  = BackpackAtInventory->GetFragment<UZfFragment_InventoryExpansion>()->ExtraSlotCount;
            int32 OldExtraSlots  = EquipedBackpack->GetFragment<UZfFragment_InventoryExpansion>()->ExtraSlotCount;

            if (NewExtraSlots - OldExtraSlots >= 0) 
            {
                return true;
            }
            
            int32 NewCapacity    = InventoryComponent->GetDefaultSlotCount() + NewExtraSlots;
            int32 ItensToMove    = InventoryComponent->GetItemCountFromInitialSlot(NewCapacity);
            int32 AvaliableSlots = InventoryComponent->GetAvailableSlotsWithExpansion(BackpackAtInventory);

            if (AvaliableSlots >= ItensToMove) return true;
            
            return false;
        }
    }
    return true;
}

bool UZfEquipmentComponent::CanUnequipItem(FGameplayTag TargetSlotTag, int32 PreviewAmount)
{
    UZfItemInstance* EquipmentAtSlot = GetItemAtSlotTag(TargetSlotTag);
    if (!EquipmentAtSlot) return false;
    
    if (EquipmentAtSlot->GetFragment<UZfFragment_InventoryExpansion>())
    {
        int32 AvaliableSlots = InventoryComponent->GetAvailableDefaultSlots();
        int32 ItensToMove    = InventoryComponent->GetItemCountFromInitialSlot(InventoryComponent->DefaultSlotCount);
            
        if (AvaliableSlots - PreviewAmount - ItensToMove >= 0) return true;
        
        return false;
    }
    return true;
}

UZfItemInstance* UZfEquipmentComponent::GetItemAtSlotTag(FGameplayTag SlotTag) const
{
    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.SlotTag == SlotTag) return Entry.ItemInstance;
    }
    return nullptr;
}

UZfItemInstance* UZfEquipmentComponent::GetItemAtEquipmentSlot(FGameplayTag EquipmentSlotTag, int32 SlotIndex) const
{
    const FZfEquipmentSlotEntry* SlotEntry = Internal_FindSlotEntryConst(EquipmentSlotTag);
    return SlotEntry ? SlotEntry->ItemInstance.Get() : nullptr;
}

bool UZfEquipmentComponent::IsItemEquipped(UZfItemInstance* ItemInstance) const
{
    if (!ItemInstance) return false;
    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.ItemInstance == ItemInstance) return true;
    }
    return false;
}

bool UZfEquipmentComponent::IsEquipmentSlotOccupied(FGameplayTag EquipSlotTag, int32 SlotIndex) const
{
    const FZfEquipmentSlotEntry* SlotEntry = Internal_FindSlotEntryConst(EquipSlotTag);
    return SlotEntry && SlotEntry->ItemInstance != nullptr;
}

bool UZfEquipmentComponent::IsEquipmentSlotBlocked(FGameplayTagContainer EquipmentTags, int32 SlotIndex) const
{
    if (EquipmentTags.HasTagExact(ZfEquipmentTags::EquipmentSlots::Slot_OffHand))
        return bOffHandSlotBlocked;
    return false;
}

TArray<UZfItemInstance*> UZfEquipmentComponent::GetAllEquippedItems() const
{
    TArray<UZfItemInstance*> Result;
    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.ItemInstance) Result.Add(Entry.ItemInstance.Get());
    }
    return Result;
}

TArray<UZfItemInstance*> UZfEquipmentComponent::GetEquippedItemsByTag(const FGameplayTag& Tag) const
{
    TArray<UZfItemInstance*> Result;
    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.ItemInstance && Entry.ItemInstance->HasItemTag(Tag))
            Result.Add(Entry.ItemInstance.Get());
    }
    return Result;
}

FGameplayTag UZfEquipmentComponent::GetEquipmentSlotOfItem(UZfItemInstance* ItemInstance) const
{
    if (!ItemInstance) return ZfEquipmentTags::EquipmentSlots::Slot_None;
    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.ItemInstance == ItemInstance) return Entry.SlotTag;
    }
    return ZfEquipmentTags::EquipmentSlots::Slot_None;
}

FGameplayTag UZfEquipmentComponent::GetEquipmentSlotTagOfItem(UZfItemInstance* ItemInstance) const
{
    if (!ItemInstance) return FGameplayTag();
    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.ItemInstance == ItemInstance) return Entry.SlotTag;
    }
    return FGameplayTag();
}

FGameplayTag UZfEquipmentComponent::GetFirstEmptySlotByParentTag(FGameplayTag ParentTag) const
{
    FGameplayTagContainer ChildTags = UGameplayTagsManager::Get().RequestGameplayTagChildren(ParentTag);

    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (!Entry.ItemInstance) continue;
        ChildTags.RemoveTag(Entry.SlotTag);
    }

    if (ChildTags.IsEmpty()) return FGameplayTag::EmptyTag;
    return ChildTags.First();
}

const TArray<FZfEquipmentSlotEntry>& UZfEquipmentComponent::GetAllEquipmentSlots() const
{
    return EquipmentList.EquippedItems;
}

FZfEquipmentSlotEntry* UZfEquipmentComponent::FindSlotEntryByItem(UZfItemInstance* Item)
{
    if (!Item) return nullptr;
    for (FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.ItemInstance == Item) return &Entry;
    }
    return nullptr;
}

// ============================================================
// COMBO SETS
// ============================================================

int32 UZfEquipmentComponent::GetEquippedPieceCountForSet(const FGameplayTag& SetIdentifierTag) const
{
    const FZfActiveSetBonus* ActiveBonus = ActiveSetBonuses.Find(SetIdentifierTag);
    return ActiveBonus ? ActiveBonus->ActivePieceCount : 0;
}

TArray<FZfActiveSetBonus> UZfEquipmentComponent::GetAllActiveSetBonuses() const
{
    TArray<FZfActiveSetBonus> Result;
    for (const TPair<FGameplayTag, FZfActiveSetBonus>& Pair : ActiveSetBonuses)
        Result.Add(Pair.Value);
    return Result;
}

// ============================================================
// EXPANSÃO DE SLOTS
// ============================================================

void UZfEquipmentComponent::OnBackpackEquipped(int32 ExtraSlots)
{
    if (!InventoryComponent) return;
    UE_LOG(LogZfInventory, Log, TEXT("UZfEquipmentComponent::OnBackpackEquipped — %d slots adicionados."), ExtraSlots);
}

void UZfEquipmentComponent::OnBackpackUnequipped(int32 ExtraSlots)
{
    if (!InventoryComponent) return;
    UE_LOG(LogZfInventory, Log, TEXT("UZfEquipmentComponent::OnBackpackUnequipped — %d slots removidos."), ExtraSlots);
}

// ============================================================
// FUNÇÕES INTERNAS — GERENCIAMENTO
// ============================================================

void UZfEquipmentComponent::Internal_InitializeEquipmentSlots()
{
    EquipmentList.EquippedItems.Empty();
    EquipmentList.OwnerComponent = this;

    for (FZfEquipmentSlotEntry& DefaultSlot : DefaultEquipmentSlots)
    {
        FZfEquipmentSlotEntry NewEntry;
        NewEntry.SlotTag        = DefaultSlot.SlotTag;
        NewEntry.ItemInstance   = nullptr;
        NewEntry.ReplicationKey = Internal_GenerateReplicationKey();
        EquipmentList.EquippedItems.Add(NewEntry);
    }

    UE_LOG(LogZfInventory, Log,
        TEXT("UZfEquipmentComponent::Internal_InitializeEquipmentSlots — %d slots inicializados."),
        EquipmentList.EquippedItems.Num());
}

void UZfEquipmentComponent::InternalEquipItem(UZfItemInstance* InItemInstance, FGameplayTag ResolvedSlotTag)
{
    check(InItemInstance != nullptr);

    const UZfFragment_Equippable* EquippableFragment = InItemInstance->GetFragment<UZfFragment_Equippable>();

    FZfEquipmentSlotEntry NewSlot;
    // Usa o slot resolvido (filho) se fornecido, senão usa o do fragment (pai)
    NewSlot.SlotTag      = ResolvedSlotTag.IsValid() ? ResolvedSlotTag : EquippableFragment->EquipmentSlotTag;
    NewSlot.ItemInstance = InItemInstance;
    EquipmentList.EquippedItems.Add(NewSlot);

    AddReplicatedSubObject(InItemInstance);
    EquipmentList.MarkArrayDirty();
    OnItemEquipped.Broadcast(InItemInstance, ResolvedSlotTag);
    Internal_ApplyItemGameplayEffects(InItemInstance);
    InItemInstance->NotifyFragments_ItemEquipped(this, GetOwner());
}

void UZfEquipmentComponent::InternalUnequipItem(FGameplayTag TargetSlotTag)
{
    UZfItemInstance* ItemInstance = GetItemAtEquipmentSlot(TargetSlotTag);
    check(ItemInstance != nullptr);

    Internal_RemoveItemGameplayEffects(ItemInstance);

    EquipmentList.EquippedItems.RemoveAll([TargetSlotTag](const FZfEquipmentSlotEntry& Entry)
    {
        return Entry.SlotTag == TargetSlotTag;
    });
    EquipmentList.MarkArrayDirty();
    
    RemoveReplicatedSubObject(ItemInstance);
    OnItemUnequipped.Broadcast(TargetSlotTag);
    ItemInstance->NotifyFragments_ItemUnequipped(this, GetOwner());
}

FZfEquipmentSlotEntry* UZfEquipmentComponent::InternalFindSlotEntry(FGameplayTag SlotTag)
{
    for (FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.SlotTag == SlotTag) return &Entry;
    }
    return nullptr;
}

const FZfEquipmentSlotEntry* UZfEquipmentComponent::Internal_FindSlotEntryConst(FGameplayTag EquipSlotTag) const
{
    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.SlotTag == EquipSlotTag) return &Entry;
    }
    return nullptr;
}

// ============================================================
// FUNÇÕES INTERNAS — GAMEPLAY EFFECTS / MODIFIER RULES
// ============================================================

UAbilitySystemComponent* UZfEquipmentComponent::Internal_GetAbilitySystemComponent() const
{
    if (!GetOwner()) return nullptr;

    if (IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(GetOwner()))
        return ASCInterface->GetAbilitySystemComponent();

    return GetOwner()->FindComponentByClass<UAbilitySystemComponent>();
}

void UZfEquipmentComponent::Internal_ApplyItemGameplayEffects(UZfItemInstance* ItemInstance)
{
    if (!ItemInstance) return;

    UAbilitySystemComponent* ASC = Internal_GetAbilitySystemComponent();
    if (!ASC)
    {
        UE_LOG(LogZfInventory, Warning, TEXT("UZfEquipmentComponent::Internal_ApplyItemGameplayEffects — ASC não encontrado."));
        return;
    }

    UDataTable* ModifierDataTable = nullptr;
    if (const UZfFragment_Modifiers* ModifierFragment = ItemInstance->GetFragment<UZfFragment_Modifiers>())
        ModifierDataTable = ModifierFragment->GetLoadedModifierDataTable();

    for (FZfAppliedModifier& Modifier : ItemInstance->GetAppliedModifiers())
    {
        if (Modifier.TargetType == EZfModifierTargetType::ItemProperty) continue;

        TSubclassOf<UZfModifierRule> RuleClass = nullptr;
        if (ModifierDataTable)
        {
            const FZfModifierDataTypes* ModData = ModifierDataTable->FindRow<FZfModifierDataTypes>(
                Modifier.ModifierRowName, TEXT("Internal_ApplyItemGameplayEffects"));
            if (ModData) RuleClass = ModData->RuleClass;
        }

        Modifier.FinalValue = Modifier.CurrentValue;

        if (RuleClass)
        {
            UZfModifierRule* RuleInstance = NewObject<UZfModifierRule>(GetTransientPackage(), RuleClass);
            if (RuleInstance)
            {
                FZfModifierRuleContext Context;
                Context.ASC          = ASC;
                Context.ItemInstance = ItemInstance;
                RuleInstance->BindToSource(Context);

                Modifier.FinalValue = RuleInstance->Calculate(Modifier.CurrentValue);
                ActiveModifierRules.FindOrAdd(ItemInstance).Add(Modifier.ModifierRowName, RuleInstance);

                const FName CapturedRowName = Modifier.ModifierRowName;
                RuleInstance->OnRuleValueChanged.AddLambda([this, ItemInstance, CapturedRowName]()
                {
                    if (UAbilitySystemComponent* InnerASC = Internal_GetAbilitySystemComponent())
                        Internal_OnModifierRuleValueChanged(ItemInstance, CapturedRowName, InnerASC);
                });
            }
        }

        Modifier.AppliedValue = Modifier.FinalValue;

        if (Modifier.GameplayEffect.IsNull()) continue;

        TSubclassOf<UGameplayEffect> GEClass = Modifier.GameplayEffect.LoadSynchronous();
        if (!GEClass) continue;

        FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
        EffectContext.AddSourceObject(GetOwner());

        FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(GEClass, 1.f, EffectContext);
        if (Spec.IsValid())
            Modifier.ActiveEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    }
}

void UZfEquipmentComponent::Internal_RemoveItemGameplayEffects(UZfItemInstance* ItemInstance)
{
    if (!ItemInstance) return;

    UAbilitySystemComponent* ASC = Internal_GetAbilitySystemComponent();
    if (!ASC) return;

    Internal_DeactivateModifierRules(ItemInstance);

    for (FZfAppliedModifier& Modifier : ItemInstance->GetAppliedModifiers())
    {
        if (Modifier.TargetType == EZfModifierTargetType::ItemProperty) continue;

        if (Modifier.ActiveEffectHandle.IsValid())
        {
            ASC->RemoveActiveGameplayEffect(Modifier.ActiveEffectHandle);
            Modifier.ActiveEffectHandle = FActiveGameplayEffectHandle();
        }
    }
}

void UZfEquipmentComponent::Internal_DeactivateModifierRules(UZfItemInstance* ItemInstance)
{
    TMap<FName, TObjectPtr<UZfModifierRule>>* RulesForItem = ActiveModifierRules.Find(ItemInstance);
    if (!RulesForItem) return;

    for (TPair<FName, TObjectPtr<UZfModifierRule>>& Pair : *RulesForItem)
    {
        if (Pair.Value)
        {
            Pair.Value->UnbindFromSource();
            Pair.Value->OnRuleValueChanged.Clear();
        }
    }

    ActiveModifierRules.Remove(ItemInstance);
}

void UZfEquipmentComponent::Internal_OnModifierRuleValueChanged(
    UZfItemInstance* ItemInstance, FName ModifierRowName, UAbilitySystemComponent* ASC)
{
    if (!ItemInstance || !ASC) return;

    FZfAppliedModifier* Modifier = nullptr;
    for (FZfAppliedModifier& Mod : ItemInstance->GetAppliedModifiers())
    {
        if (Mod.ModifierRowName == ModifierRowName) { Modifier = &Mod; break; }
    }
    if (!Modifier) return;

    TMap<FName, TObjectPtr<UZfModifierRule>>* RulesForItem = ActiveModifierRules.Find(ItemInstance);
    if (!RulesForItem) return;

    TObjectPtr<UZfModifierRule>* RulePtr = RulesForItem->Find(ModifierRowName);
    if (!RulePtr || !(*RulePtr)) return;

    const float NewFinalValue = (*RulePtr)->Calculate(Modifier->CurrentValue);
    Internal_ReapplyModifier(ItemInstance, *Modifier, NewFinalValue, ASC);
}

void UZfEquipmentComponent::Internal_ReapplyModifier(
    UZfItemInstance* ItemInstance, FZfAppliedModifier& Modifier, float NewFinalValue, UAbilitySystemComponent* ASC)
{
    if (Modifier.TargetType == EZfModifierTargetType::ItemProperty) return;

    if (Modifier.ActiveEffectHandle.IsValid())
    {
        ASC->RemoveActiveGameplayEffect(Modifier.ActiveEffectHandle);
        Modifier.ActiveEffectHandle = FActiveGameplayEffectHandle();
    }

    Modifier.FinalValue   = NewFinalValue;
    Modifier.AppliedValue = NewFinalValue;

    if (!Modifier.GameplayEffect.IsNull())
    {
        TSubclassOf<UGameplayEffect> GEClass = Modifier.GameplayEffect.LoadSynchronous();
        if (GEClass)
        {
            FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
            EffectContext.AddSourceObject(GetOwner());

            FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(GEClass, 1.f, EffectContext);
            if (Spec.IsValid())
                Modifier.ActiveEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
        }
    }
}

void UZfEquipmentComponent::Internal_UpdateSetBonuses(UZfItemInstance* ItemInstance, bool bWasEquipped)
{
    if (!ItemInstance) return;

    const UZfFragment_SetPiece* SetFragment = ItemInstance->GetFragment<UZfFragment_SetPiece>();
    if (!SetFragment || !SetFragment->SetIdentifier.IsValid()) return;

    const FGameplayTag SetTag = SetFragment->SetIdentifier;

    FZfActiveSetBonus& ActiveBonus = ActiveSetBonuses.FindOrAdd(SetTag);
    ActiveBonus.SetIdentifierTag   = SetTag;

    if (bWasEquipped)
        ActiveBonus.ActivePieceCount++;
    else
        ActiveBonus.ActivePieceCount = FMath::Max(0, ActiveBonus.ActivePieceCount - 1);

    Internal_RemoveAllSetBonuses(SetTag);
    Internal_ApplySetBonuses(SetTag, ActiveBonus.ActivePieceCount);
    OnSetBonusChanged.Broadcast(SetTag, ActiveBonus.ActivePieceCount);
}

void UZfEquipmentComponent::Internal_ApplySetBonuses(const FGameplayTag& SetTag, int32 PieceCount)
{
    if (PieceCount <= 0) return;

    UAbilitySystemComponent* ASC = Internal_GetAbilitySystemComponent();
    if (!ASC) return;

    FZfActiveSetBonus* ActiveBonus = ActiveSetBonuses.Find(SetTag);
    if (!ActiveBonus) return;

    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (!Entry.ItemInstance) continue;

        const UZfFragment_SetPiece* SetFragment = Entry.ItemInstance->GetFragment<UZfFragment_SetPiece>();
        if (!SetFragment || SetFragment->SetIdentifier != SetTag) continue;

        const TArray<FZfSetBonusEntry> ActiveBonuses = SetFragment->GetActiveBonusesForPieceCount(PieceCount);
        for (const FZfSetBonusEntry& BonusEntry : ActiveBonuses)
        {
            if (!BonusEntry.BonusGameplayEffect.IsNull())
            {
                TSubclassOf<UGameplayEffect> GEClass = BonusEntry.BonusGameplayEffect.LoadSynchronous();
                if (GEClass)
                {
                    FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
                    FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectToSelf(
                        GEClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ContextHandle);
                    ActiveBonus->ActiveBonusHandles.Add(BonusEntry.RequiredPieceCount, Handle);
                }
            }
        }
        break;
    }
}

void UZfEquipmentComponent::Internal_RemoveAllSetBonuses(const FGameplayTag& SetTag)
{
    FZfActiveSetBonus* ActiveBonus = ActiveSetBonuses.Find(SetTag);
    if (!ActiveBonus) return;

    UAbilitySystemComponent* ASC = Internal_GetAbilitySystemComponent();
    if (!ASC) return;

    for (TPair<int32, FActiveGameplayEffectHandle>& HandlePair : ActiveBonus->ActiveBonusHandles)
    {
        if (HandlePair.Value.IsValid()) ASC->RemoveActiveGameplayEffect(HandlePair.Value);
    }

    ActiveBonus->ActiveBonusHandles.Empty();
}

void UZfEquipmentComponent::Internal_BlockOffHandSlot()   { bOffHandSlotBlocked = true; }
void UZfEquipmentComponent::Internal_UnblockOffHandSlot() { bOffHandSlotBlocked = false; }

int32 UZfEquipmentComponent::InternalTryStackWithExistingItems(UZfItemInstance* InItemInstance, FGameplayTag SlotTag)
{
    if (!InItemInstance) return -1;
    
    const UZfFragment_Stackable* StackFragment = InItemInstance->GetFragment<UZfFragment_Stackable>();
    if (!StackFragment) return -1;
    
    const UZfItemDefinition* IncomingDefinition = InItemInstance->GetItemDefinition();
    if (!IncomingDefinition) return -1;
    
    int32 Overflow = 0;
    
    for (FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (!Entry.ItemInstance) continue;
        if (!Entry.SlotTag.MatchesTag(SlotTag)) continue;
        if (Entry.ItemInstance->GetItemDefinition() != IncomingDefinition) continue;
        if (Entry.ItemInstance->GetCurrentStack() == StackFragment->MaxStackSize) continue;
                
        Overflow = Entry.ItemInstance->AddToStack(InItemInstance->GetCurrentStack());
        EquipmentList.MarkItemDirty(Entry);
        OnStackChanged.Broadcast(Entry.ItemInstance, Entry.SlotTag);
                
        if (Overflow == 0) return 0;

        if (Overflow < 0) return -1;
    }
    
    if (Overflow > 0) return Overflow;

    return -1;
}

// ============================================================
// FUNÇÕES INTERNAS — VALIDAÇÃO / UTILIDADE
// ============================================================

bool UZfEquipmentComponent::InternalCheckIsServer(const FString& FunctionName) const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogZfInventory, Error,
            TEXT("UZfEquipmentComponent::%s — GetWorld() retornou nulo."), *FunctionName);
        return false;
    }

    if (World->GetNetMode() == NM_Client)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfEquipmentComponent::%s — Operação de servidor chamada no cliente!"), *FunctionName);
        return false;
    }

    return true;
}

int32 UZfEquipmentComponent::Internal_GenerateReplicationKey() const
{
    static int32 KeyCounter = 0;
    return ++KeyCounter;
}