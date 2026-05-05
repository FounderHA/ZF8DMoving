// Copyright ZfGame Studio. All Rights Reserved.
// ZfInventoryComponent.cpp

#include "Inventory/ZfInventoryComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Inventory/ZfEquipmentComponent.h"
#include "Inventory/ZfItemInstance.h"
#include "Inventory/ZfItemDefinition.h"
#include "Inventory/Fragments/ZfFragment_Stackable.h"
#include "Inventory/Fragments/ZfFragment_Equippable.h"
#include "Inventory/Fragments/ZfFragment_InventoryExpansion.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Engine/ActorChannel.h"
#include "Inventory/Fragments/ZfFragment_Consumable.h"
#include "Items/ZfItemPickup.h"
#include "Player/ZfPlayerState.h"
#include "Systems/RefinerySystem/ZfRefineryTypes.h"

// ============================================================
// CONSTRUCTOR
// ============================================================

UZfInventoryComponent::UZfInventoryComponent()
{
    // Componente não precisa de tick por padrão
    PrimaryComponentTick.bCanEverTick = false;

    // Habilita replicação do componente
    SetIsReplicatedByDefault(true);
    bReplicateUsingRegisteredSubObjectList = true;
}

// ============================================================
// REPLICAÇÃO
// ============================================================

void UZfInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Replica o inventário via FastArraySerializer
    // DOREPLIFETIME_CONDITION com COND_OwnerOnly garante que
    // apenas o dono recebe os dados completos do inventário
    DOREPLIFETIME_CONDITION(UZfInventoryComponent, InventoryList,COND_OwnerOnly);

    // Tamanho atual replicado para todos (UI de outros jogadores pode precisar)
    DOREPLIFETIME(UZfInventoryComponent, CurrentSlotCount);
}

// ============================================================
// CICLO DE VIDA - BEGINPLAY, TICK...
// ============================================================

void UZfInventoryComponent::BeginPlay()
{
    Super::BeginPlay();

    // Busca o EquipmentComponent no ator dono
    EquipmentComponent = Cast<AZfPlayerState>(GetOwner())->FindComponentByClass<UZfEquipmentComponent>();
 
    // Inicializa os slots apenas no servidor
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        // Seta o owner e o tamanho inicial diretamente
        InventoryList.OwnerComponent = this;
        if (EquipmentComponent->GetItemAtEquipmentSlot(ZfEquipmentTags::EquipmentSlots::Slot_Backpack_1))
        {
            if (EquipmentComponent->GetItemAtEquipmentSlot(ZfEquipmentTags::EquipmentSlots::Slot_Backpack_1)->GetFragment<UZfFragment_InventoryExpansion>())
            {
                const UZfFragment_InventoryExpansion* Backpack = EquipmentComponent->GetItemAtEquipmentSlot(ZfEquipmentTags::EquipmentSlots::Slot_Backpack_1)->
                    GetFragment<UZfFragment_InventoryExpansion>();
                CurrentSlotCount = DefaultSlotCount + Backpack->ExtraSlotCount;
            }
            else
            {
                CurrentSlotCount = DefaultSlotCount;
            }
        }
        
        CurrentSlotCount = DefaultSlotCount;

        UE_LOG(LogZfInventory, Verbose, TEXT("UZfInventoryComponent::BeginPlay — " "Inventário inicializado com %d slots disponíveis."), CurrentSlotCount);
    }
}

// ============================================================
// FAST ARRAY
// ========'====================================================

void FZfInventorySlot::PreReplicatedRemove(const FZfInventoryList& InArraySerializer)
{
    UE_LOG(LogZfInventory, Warning,
    TEXT("FZfInventorySlot::PreReplicatedRemove — SlotIndex: %d | OwnerComponent: %s"),
    SlotIndex,
    InArraySerializer.OwnerComponent ? TEXT("VÁLIDO") : TEXT("NULO"));
    
    if (InArraySerializer.OwnerComponent && ItemInstance)
    {
        InArraySerializer.OwnerComponent->OnItemRemoved.Broadcast(SlotIndex);
    }
}

void FZfInventorySlot::PostReplicatedAdd(const FZfInventoryList& InArraySerializer)
   {
    UE_LOG(LogZfInventory, Warning,
        TEXT("FZfInventorySlot::PostReplicatedAdd — SlotIndex: %d | OwnerComponent: %s"),
        SlotIndex,
        InArraySerializer.OwnerComponent ? TEXT("VÁLIDO") : TEXT("NULO"));
    
    if (InArraySerializer.OwnerComponent && ItemInstance)
    {
        UE_LOG(LogZfInventory, Warning,
         TEXT("FZfInventorySlot::PostReplicatedAdd — Broadcast disparado para item: %s"),
         *ItemInstance->GetItemName().ToString());
        
        InArraySerializer.OwnerComponent->OnItemAdded.Broadcast(ItemInstance, SlotIndex);
    }
}

void FZfInventorySlot::PostReplicatedChange(const FZfInventoryList& InArraySerializer)
{
    if (InArraySerializer.OwnerComponent)
    {
        InArraySerializer.OwnerComponent->OnInventoryRefreshed.Broadcast();
        InArraySerializer.OwnerComponent->OnInventorySizeChanged.Broadcast();
        InArraySerializer.OwnerComponent->OnItemStackChanged.Broadcast(ItemInstance, SlotIndex);
    }
}


void UZfInventoryComponent::AddItemToTargetInterface_Implementation(UObject* ItemComesFrom, UZfItemInstance* InItemInstance, int32 AmountToAdd,
        int32 SlotIndexComesFrom, int32 TargetSlotIndex,
        EZfRefinerySlotType SlotTypeComesFrom, EZfRefinerySlotType TargetSlotType,
        FGameplayTag SlotTagComesFrom, FGameplayTag TargetSlotTag)
{
    ServerTryAddItem(ItemComesFrom, InItemInstance, AmountToAdd, SlotIndexComesFrom, TargetSlotIndex, SlotTypeComesFrom, TargetSlotType, SlotTagComesFrom, TargetSlotTag);
}

bool UZfInventoryComponent::RemoveItemFromTargetInterface_Implementation(int32 ItemAmountToRemove, int32 TargetSlotIndex, EZfRefinerySlotType TargetSlotType, FGameplayTag TargetSlotTag)
{
    if (InternalCheckIsServer("RemoveItemFromTargetInterface_Implementation"))
    {
        if (TryRemoveItem(ItemAmountToRemove, TargetSlotIndex, TargetSlotType, TargetSlotTag) == EZfItemMechanicResult::Success) return true;
        return false;
    }
    return false;
}

void UZfInventoryComponent::AddItemBackToTargetInterface_Implementation(UZfItemInstance* InItemInstance, int32 TargetSlotIndex, EZfRefinerySlotType TargetSlotType, FGameplayTag TargetSlotTag)
{
    InternalAddItem(InItemInstance, TargetSlotIndex);
}

// ============================================================
// FUNÇÕES SERVER - GERENCIAMENTO
// ============================================================

void UZfInventoryComponent::ServerTryAddItem_Implementation(UObject* ItemComesFrom, UZfItemInstance* InItemInstance, int32 AmountToAdd,
        int32 SlotIndexComesFrom, int32 TargetSlotIndex,
        EZfRefinerySlotType SlotTypeComesFrom, EZfRefinerySlotType TargetSlotType,
        FGameplayTag SlotTagComesFrom, FGameplayTag TargetSlotTag)
{
    if (InternalCheckIsServer(TEXT("ServerTryAddItemToInventory_Implementation")))
    {
        TryAddItem(ItemComesFrom, InItemInstance, AmountToAdd, SlotIndexComesFrom, TargetSlotIndex, SlotTypeComesFrom, TargetSlotType, SlotTagComesFrom, TargetSlotTag);
    }
}

void UZfInventoryComponent::ServerTryRemoveItem_Implementation(int32 ItemAmountToRemove, int32 TargetSlotIndex, EZfRefinerySlotType TargetSlotType, FGameplayTag TargetSlotTag)
{
    if (InternalCheckIsServer(TEXT("ServerTryRemoveItemFromInventory_Implementation")))
    {
        TryRemoveItem(ItemAmountToRemove, TargetSlotIndex, TargetSlotType, TargetSlotTag);
    }
}

void UZfInventoryComponent::ServerTryDropItem_Implementation(UZfItemInstance* ItemInstance, int32 SlotIndex)
{
    if (!InternalCheckIsServer(TEXT("ServerTryRemoveItemFromInventory_Implementation")))
    {
        return;
    }
    
    //UZfItemInstance* ItemInstance = GetItemAtSlot(SlotIndex);
    /*const EZfItemMechanicResult Result = TryRemoveItemFromInventory(SlotIndex);
    
    if (Result == EZfItemMechanicResult::Success && ItemInstance)
    {
        TrySpawnPickupItem(ItemInstance);
    }
    */
}

void UZfInventoryComponent::ServerTrySortInventory_Implementation(EZfInventorySortType SortType)
{
    if (InternalCheckIsServer(TEXT("ServerTrySortInventory")))
    {
        InternalSortInventoryBySelected(SortType);
    }
}

void UZfInventoryComponent::ServerTryUseItemAtSlot_Implementation(int32 SlotIndex)
{
    if (InternalCheckIsServer(TEXT("ServerTryUseItemAtSlot")))
    {
        TryUseItemAtSlot(SlotIndex);
    }
}

void UZfInventoryComponent::ServerTrySplitStack_Implementation(int32 FromSlotIndex, int32 ToSlotIndex, int32 Amount)
{
    if (InternalCheckIsServer(TEXT("ServerTrySplitStack")))
    {
        TrySplitStack(FromSlotIndex, ToSlotIndex, Amount);
    }
}

EZfItemMechanicResult UZfInventoryComponent::TryPickupItem(UZfItemInstance* ItemInstance, AZfItemPickup* ItemPickup)
{
    if (InternalCheckIsServer(TEXT("TryPickupItem")))
    {
        return TryAddItemFromPickup(ItemInstance, ItemPickup);
    }
    
    return EZfItemMechanicResult::Failed_InvalidOperation;
}

// ============================================================
// FUNÇÕES PRINCIPAIS - GERENCIAMENTO
// ============================================================

EZfItemMechanicResult UZfInventoryComponent::TryAddItem(UObject* ItemComesFrom, UZfItemInstance* InItemInstance, int32 AmountToAdd,
        int32 SlotIndexComesFrom, int32 TargetSlotIndex,
        EZfRefinerySlotType SlotTypeComesFrom, EZfRefinerySlotType TargetSlotType,
        FGameplayTag SlotTagComesFrom, FGameplayTag TargetSlotTag)
{
    if (TargetSlotIndex != INDEX_NONE)
    {
        if (TargetSlotIndex == SlotIndexComesFrom) return EZfItemMechanicResult::Failed_InvalidOperation;
    }
    
    if (!InItemInstance) return EZfItemMechanicResult::Failed_ItemNotFound;
    
    if (InItemInstance->GetFragment<UZfFragment_Stackable>())
    {
        if (AmountToAdd < 0 || AmountToAdd > InItemInstance->GetFragment<UZfFragment_Stackable>()->MaxStackSize) return EZfItemMechanicResult::Failed_IncompatibleItem;
    }

    // ==================== QUICK TRANSFER ====================
    
    if (TargetSlotIndex ==  INDEX_NONE)
    {
        // ── Stackável — tenta empilhar em slot existente antes de ocupar novo ─────
        if (InItemInstance->GetFragment<UZfFragment_Stackable>())
        {
            int32 Overflow = InternalTryStackWithExistingItems(InItemInstance, AmountToAdd);
            if (Overflow == 0)
            {
                // Completamente absorvido por stacks existentes
                Execute_RemoveItemFromTargetInterface(ItemComesFrom, AmountToAdd, SlotIndexComesFrom, SlotTypeComesFrom, SlotTagComesFrom);
                return EZfItemMechanicResult::Success;
            }

            if (Overflow < 0) return EZfItemMechanicResult::Failed_IncompatibleItem;

            if (Overflow > 0)
            {
                // Pega Slot Vazio se Tiver
                const int32 EmptySlot = GetFirstEmptySlot();
                
                if (EmptySlot == INDEX_NONE)
                {
                    // Sem slot disponível — Devolver o Resto ao Dono Original
                    Execute_RemoveItemFromTargetInterface(ItemComesFrom, AmountToAdd - Overflow, SlotIndexComesFrom, SlotTypeComesFrom, SlotTagComesFrom);
                    return EZfItemMechanicResult::Failed_InventoryFull;
                }

                // Slot disponível - Muda Stack
                UZfItemInstance* NewItem = InItemInstance->CreateServerCopy(Overflow, GetOwner());
                if (!Execute_RemoveItemFromTargetInterface(ItemComesFrom, AmountToAdd, SlotIndexComesFrom, SlotTypeComesFrom, SlotTagComesFrom)) return EZfItemMechanicResult::Failed_InvalidOperation;
                InternalAddItem(NewItem, EmptySlot);
                return EZfItemMechanicResult::Success;
            }
        }
        
        // Pega Slot Vazio se Tiver
        const int32 EmptySlot = GetFirstEmptySlot();
        
        if (EmptySlot == INDEX_NONE) return EZfItemMechanicResult::Failed_InventoryFull;

        // Slot disponível
        if (!Execute_RemoveItemFromTargetInterface(ItemComesFrom, AmountToAdd, SlotIndexComesFrom, SlotTypeComesFrom, SlotTagComesFrom)) return EZfItemMechanicResult::Failed_InvalidOperation;
        InternalAddItem(InItemInstance, EmptySlot);
        return EZfItemMechanicResult::Success;
    }
    
    
    
    // ==================== DRAG AND DROP ====================
    
    UZfItemInstance* ItemAtTarget = GetItemAtSlot(TargetSlotIndex);

    // Slot vazio
    if (!ItemAtTarget)
    {
        if (InItemInstance->GetCurrentStack() != AmountToAdd)
        {
            UZfItemInstance* NewItem = InItemInstance->CreateServerCopy(AmountToAdd, GetOwner());
            if (!Execute_RemoveItemFromTargetInterface(ItemComesFrom, AmountToAdd, SlotIndexComesFrom, SlotTypeComesFrom, SlotTagComesFrom)) return EZfItemMechanicResult::Failed_InvalidOperation;
            InternalAddItem(NewItem, TargetSlotIndex);
            return EZfItemMechanicResult::Success;
        }
        if (!Execute_RemoveItemFromTargetInterface(ItemComesFrom, InItemInstance->GetCurrentStack(), SlotIndexComesFrom, SlotTypeComesFrom, SlotTagComesFrom)) return EZfItemMechanicResult::Failed_InvalidOperation;
        InternalAddItem(InItemInstance, TargetSlotIndex);

        if (ItemComesFrom == EquipmentComponent)
        {
            if (InItemInstance->GetFragment<UZfFragment_InventoryExpansion>())
            {
                RelocateItemsAboveCapacity(DefaultSlotCount);
            }
        }
        return EZfItemMechanicResult::Success;
    }

    // Item Stackavel
    if (ItemAtTarget->GetItemDefinition() == InItemInstance->GetItemDefinition())
    {
        if (InItemInstance->GetFragment<UZfFragment_Stackable>())
        {
            int32 Overflow = TryAddToStack(TargetSlotIndex, AmountToAdd);
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
    }
    
    // Swap nos Slot
    if (ItemComesFrom != this)
    {
        if (InItemInstance->GetFragment<UZfFragment_InventoryExpansion>()) return EZfItemMechanicResult::Failed_InvalidOperation;
    }
    
    if (InItemInstance->GetCurrentStack() != AmountToAdd) return EZfItemMechanicResult::Failed_SlotBlocked;

    if (!Execute_RemoveItemFromTargetInterface(ItemComesFrom, AmountToAdd, SlotIndexComesFrom, SlotTypeComesFrom, SlotTagComesFrom)) return EZfItemMechanicResult::Failed_InvalidOperation;
    if (!Execute_RemoveItemFromTargetInterface(this, ItemAtTarget->GetCurrentStack(), TargetSlotIndex, TargetSlotType, TargetSlotTag)) return EZfItemMechanicResult::Failed_InvalidOperation;
    
    Execute_AddItemBackToTargetInterface(ItemComesFrom, InItemInstance , TargetSlotIndex, TargetSlotType, TargetSlotTag);
    Execute_AddItemBackToTargetInterface(ItemComesFrom, ItemAtTarget , SlotIndexComesFrom, SlotTypeComesFrom, SlotTagComesFrom);

    return EZfItemMechanicResult::Success;
}

EZfItemMechanicResult UZfInventoryComponent::TryRemoveItem(int32 ItemAmountToRemove, int32 TargetSlotIndex, EZfRefinerySlotType TargetSlotType, FGameplayTag TargetSlotTag)
{
    UZfItemInstance* ItemAtTarget = GetItemAtSlot(TargetSlotIndex);

    if (!ItemAtTarget) return EZfItemMechanicResult::Failed_InvalidOperation;

    if (ItemAtTarget->GetFragment<UZfFragment_Stackable>())
    {
        if (ItemAmountToRemove < 0 || ItemAmountToRemove > ItemAtTarget->GetFragment<UZfFragment_Stackable>()->MaxStackSize) return EZfItemMechanicResult::Failed_IncompatibleItem;
    }

    int32 ItemAmountAtSlot = ItemAtTarget->GetCurrentStack();
    
    if (ItemAmountToRemove > ItemAmountAtSlot) return EZfItemMechanicResult::Failed_InvalidOperation;
    
    if (ItemAmountToRemove == ItemAtTarget->GetCurrentStack())
    {
        InternalRemoveItem(TargetSlotIndex);
        return EZfItemMechanicResult::Success;
    }

    ItemAtTarget->SetCurrentStack(ItemAmountAtSlot - ItemAmountToRemove);
    FZfInventorySlot* Slot = FindSlotByIndex(TargetSlotIndex);
    if (Slot)
    {
        InventoryList.MarkItemDirty(*Slot);
    }
    OnItemStackChanged.Broadcast(ItemAtTarget, TargetSlotIndex);
    return EZfItemMechanicResult::Success;
}

EZfItemMechanicResult UZfInventoryComponent::TryAddItemToInventory(UZfItemInstance* InItemInstance)
{
    if (!InItemInstance)
    {
        UE_LOG(LogZfInventory, Warning, TEXT("UZfInventoryComponent::TryAddItemToInventory — " "ItemInstance é nulo."));
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    // Tenta empilhar com itens existentes se for stackável
    const UZfFragment_Stackable* StackFragment = InItemInstance->GetFragment<UZfFragment_Stackable>();
    if (StackFragment)
    {
        if (InternalTryStackWithExistingItems(InItemInstance, InItemInstance->GetCurrentStack()))
        {
            // Item foi completamente absorvido por stacks existentes
            UE_LOG(LogZfInventory, Verbose, TEXT("UZfInventoryComponent::TryAddItemToInventory — " 
            "Item '%s' completamente empilhado em stacks existentes."), *InItemInstance->GetItemName().ToString());
            return EZfItemMechanicResult::Success;
        }
    }

    // Se ainda tem quantidade restante ou não é stackável,
    // busca o primeiro slot vazio
    const int32 EmptySlot = GetFirstEmptySlot();
    if (EmptySlot == INDEX_NONE)
    {
        UE_LOG(LogZfInventory, Warning, TEXT("UZfInventoryComponent::TryAddItemToInventory — " 
        "Inventário cheio. Item '%s' não pode ser adicionado."), *InItemInstance->GetItemName().ToString());
        return EZfItemMechanicResult::Failed_InventoryFull;
    }
    
    // Cria uma nova entrada no array apenas com o item
    InternalAddItem(InItemInstance, EmptySlot);
    
    // Marca o inventário como modificado para replicação
    InventoryList.MarkArrayDirty();

    OnItemAdded.Broadcast(InItemInstance,EmptySlot);
    
    // Notifica os fragments do item
    InItemInstance->NotifyFragments_ItemAddedToInventory(this);
    
    UE_LOG(LogZfInventory, Verbose,
        TEXT("UZfInventoryComponent::TryAddItemToInventory — " "Item '%s' adicionado ao slot %d."),
        *InItemInstance->GetItemName().ToString(), EmptySlot);

    return EZfItemMechanicResult::Success;
}

EZfItemMechanicResult UZfInventoryComponent::TryRemoveItemFromInventory(int32 SlotIndex)
{
    if (!IsValidSlotIndex(SlotIndex))
    {
        return EZfItemMechanicResult::Failed_InvalidSlot;
    }

    UZfItemInstance* ItemInstance = GetItemAtSlot(SlotIndex);

    if (!ItemInstance)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfInventoryComponent::RemoveItemAtSlot — " "Slot %d está vazio."), SlotIndex);
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    // Notifica os fragments antes de remover
    ItemInstance->NotifyFragments_ItemRemovedFromInventory(this);

    // Remove Item
    InternalRemoveItem(SlotIndex);

    // Notifica a UI
    OnItemRemoved.Broadcast(SlotIndex);

    InventoryList.MarkArrayDirty();

    UE_LOG(LogZfInventory, Verbose, TEXT("UZfInventoryComponent::RemoveItemAtSlot — " "Item '%s' removido do slot %d."),
        *ItemInstance->GetItemName().ToString(), SlotIndex);

    return EZfItemMechanicResult::Success;
}

EZfItemMechanicResult UZfInventoryComponent::TryAddItemToSpecificSlot(UZfItemInstance* InItemInstance, int32 TargetSlotIndex)
{
    if (!InItemInstance)
    {
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    if (!IsValidSlotIndex(TargetSlotIndex))
    {
        return EZfItemMechanicResult::Failed_InvalidSlot;
    }

    if (!IsSlotEmpty(TargetSlotIndex))
    {
        TryAddItemToInventory(InItemInstance);
        return EZfItemMechanicResult::Failed_SlotBlocked;
    }

    InternalAddItem(InItemInstance, TargetSlotIndex);
    OnItemAdded.Broadcast(InItemInstance, TargetSlotIndex);
    InventoryList.MarkArrayDirty();

    InItemInstance->NotifyFragments_ItemAddedToInventory(this);

    UE_LOG(LogZfInventory, Verbose,
        TEXT("TryAddItemToSpecificSlot — Item '%s' adicionado ao slot %d."),
        *InItemInstance->GetItemName().ToString(), TargetSlotIndex);

    return EZfItemMechanicResult::Success;
}

EZfItemMechanicResult UZfInventoryComponent::TryRemoveAmountFromStack( UZfItemInstance* ItemInstance, int32 Amount)
{
    if (!InternalCheckIsServer(TEXT("RemoveAmountFromStack")))
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    if (!ItemInstance)
    {
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    // Verifica se o item é stackável
    if (!ItemInstance->HasFragment<UZfFragment_Stackable>())
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfInventoryComponent::RemoveAmountFromStack — " "Item '%s' não é stackável."),
            *ItemInstance->GetItemName().ToString());
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    // Garante que não remove mais do que o stack atual
    const int32 AmountToRemove = FMath::Min(Amount, ItemInstance->GetCurrentStack());
    
    // Remove a quantidade — retorna true se o stack zerou
    if (ItemInstance->RemoveFromStack(AmountToRemove))
    {
        // Stack zerou — remove o item do inventário
        InternalRemoveItem(GetSlotIndexOfItem(ItemInstance));
        OnItemRemoved.Broadcast(GetSlotIndexOfItem(ItemInstance));
        return EZfItemMechanicResult::Success;
    }

    FZfInventorySlot* Slot = FindSlotByIndex(GetSlotIndexOfItem(ItemInstance));
    if (Slot)
    {
        InventoryList.MarkItemDirty(*Slot);
    }
    OnItemStackChanged.Broadcast(ItemInstance, GetSlotIndexOfItem(ItemInstance));
    return EZfItemMechanicResult::Success;
}

int32 UZfInventoryComponent::TryAddToStack(int32 SlotIndex, int32 Amount)
{
    if (!InternalCheckIsServer(TEXT("TryAddToStack"))) return Amount;

    UZfItemInstance* Item = GetItemAtSlot(SlotIndex);
    if (!Item) return Amount;

    const int32 Overflow = Item->AddToStack(Amount);

    for (FZfInventorySlot& Slot : InventoryList.Slots)
    {
        if (Slot.SlotIndex == SlotIndex)
        {
            InventoryList.MarkItemDirty(Slot);
            break;
        }
    }

    OnItemStackChanged.Broadcast(Item, SlotIndex);
    return Overflow;
}

void UZfInventoryComponent::TrySplitStack(int32 FromSlotIndex, int32 ToSlotIndex, int32 Amount)
{
    // Verifica se o item é valido no slot
    UZfItemInstance* ItemInstance = GetItemAtSlot(FromSlotIndex);
    if (!ItemInstance) return;

    // Verifica se o item é stackável 
    const UZfFragment_Stackable* StackFrag = ItemInstance->GetFragment<UZfFragment_Stackable>();
    if (!StackFrag) return;

    // Validação de quantidade
    if (Amount <= 0 || Amount >= ItemInstance->GetCurrentStack()) return;

    // Resolve slot de destino
    const int32 ResolvedSlot = (ToSlotIndex == INDEX_NONE) ? GetFirstEmptySlot() : ToSlotIndex;

    if (ResolvedSlot == INDEX_NONE || !IsSlotEmpty(ResolvedSlot)) return;

    // ---- Lógica do split ----
    // 1. Cria nova instância do mesmo ItemDefinition
    UZfItemInstance* NewItem = NewObject<UZfItemInstance>(GetOwner(), ItemInstance->GetClass());
    //NewItem->SetNewGuid();
    NewItem->SetCurrentStack(Amount);
    
    if (!NewItem) return;

    // 2. Subtrai do stack original
    ItemInstance->SetCurrentStack(ItemInstance->GetCurrentStack() - Amount);
    
    // 3. Marca o slot original como dirty
    if (FindSlotByIndex(FromSlotIndex)) InventoryList.MarkItemDirty(*FindSlotByIndex(FromSlotIndex));

    // 4. Adiciona a nova instância no slot destino
    InternalAddItem(NewItem, ResolvedSlot);

    UE_LOG(LogZfInventory, Verbose, TEXT("ServerTrySplitStack — Slot %d dividido: %d → slot %d"),FromSlotIndex, Amount, ResolvedSlot);
}

void UZfInventoryComponent::TrySpawnPickupItem(UZfItemInstance* ItemInstance) const
{
    // Spawna o ItemPickup no mundo
    if (GetOwner() && ItemInstance->GetItemDefinition())
    {
        APawn* InstigatorPawn = nullptr;
        if (APlayerState* PS = Cast<AZfPlayerState>(GetOwner()))
        {
            InstigatorPawn = PS->GetPawn();
        }
            
        const FVector DropLocation = InstigatorPawn->GetActorLocation() + GetOwner()->GetActorForwardVector() * 200.0f;
        const FRotator DropRotation = FRotator::ZeroRotator;
        const FTransform DropTransform(DropRotation, DropLocation);
            
        const TSubclassOf<AZfItemPickup> PickupClass = ItemInstance->GetItemDefinition()->ItemPickupActorClass.LoadSynchronous();

        if (!PickupClass)
        {
            UE_LOG(LogZfInventory, Error, 
                TEXT("ServerTryDropItem — Item '%s' sem PickupClass!"), *ItemInstance->GetItemName().ToString());
            return;
        }
            
        // Spawna o pickup no mundo
        AZfItemPickup* Pickup = GetWorld()->SpawnActorDeferred<AZfItemPickup>(
            PickupClass, DropTransform,GetOwner(),InstigatorPawn,
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

        if (Pickup)
        {
            // Seta o item ANTES do BeginPlay
            Pickup->InitializePickup(ItemInstance);

            // Agora sim roda o BeginPlay
            Pickup->FinishSpawning(DropTransform);

            UE_LOG(LogZfInventory, Verbose, 
                TEXT("ServerTryDropItem — Item '%s' dropado em %s."),
                *ItemInstance->GetItemName().ToString(), *DropLocation.ToString());
        }
    }
}

void UZfInventoryComponent::TryUseItemAtSlot(int32 SlotIndex)
{
    UZfItemInstance* Item = GetItemAtSlot(SlotIndex);
    if (!Item) return;

    // Verifica se tem Fragment_Consumable
    const UZfFragment_Consumable* Fragment = Item->GetFragment<UZfFragment_Consumable>();
    if (!Fragment) return;

    // Consome ANTES de disparar o evento
    if (Fragment->bConsumeOnUse)
    {
        const bool bStackable = Item->HasFragment<UZfFragment_Stackable>();
        if (bStackable)
        {
            if (Item->GetCurrentStack() > 1)
            {
                Item->SetCurrentStack(Item->GetCurrentStack() - 1);
            }
            else
            {
                InternalRemoveItem(SlotIndex);
            }
        }
        else
        {
            InternalRemoveItem(SlotIndex);
        }
    }

    // Dispara o evento — GA processa os efeitos
    IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(GetOwner());
    if (!ASCInterface) return;

    UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent();
    if (!ASC) return;

    FGameplayEventData EventData;
    EventData.OptionalObject = Item;
    EventData.EventTag = ZfUniqueItemTags::ItemEvents::Item_Event_Use;
    ASC->HandleGameplayEvent(EventData.EventTag, &EventData);
}

void UZfInventoryComponent::UpdateSlotCountFromEquippedBackpack()
{
    int32 ExtraSlots = 0;

    if (EquipmentComponent)
    {
        // Busca o item no slot de mochila usando a tag
        UZfItemInstance* BackpackItem = EquipmentComponent->GetItemAtSlotTag(FGameplayTag::RequestGameplayTag(TEXT("EquipmentSlot.Slot.Backpack.1")));

        if (BackpackItem)
        {
            const UZfFragment_InventoryExpansion* ExpansionFragment = BackpackItem->GetFragment<UZfFragment_InventoryExpansion>();

            if (ExpansionFragment)
            {
                ExtraSlots = ExpansionFragment->ExtraSlotCount;
            }
        }
    }
    
    CurrentSlotCount = FMath::Min(DefaultSlotCount + ExtraSlots, MaxAbsoluteSlotCount);
}

EZfItemMechanicResult UZfInventoryComponent::TryAddItemFromPickup(UZfItemInstance* ItemInstance, AZfItemPickup* ItemPickup)
{
    if (!ItemInstance) return EZfItemMechanicResult::Failed_IncompatibleItem;

    // Item Stackavel
    if (ItemInstance->GetFragment<UZfFragment_Stackable>() != nullptr)
    {
        int32 Oveflow = InternalTryStackWithExistingItems(ItemInstance, ItemInstance->GetCurrentStack());

        if (Oveflow == 0) return EZfItemMechanicResult::Success;
        
        if (Oveflow == -1) return EZfItemMechanicResult::Failed_IncompatibleItem;

        if (Oveflow > 0)
        {
            int32 EmptySlot = GetFirstEmptySlot();
            if (EmptySlot == INDEX_NONE)
            {
                ItemPickup->SetNewStack(Oveflow);
                return EZfItemMechanicResult::Failed_InventoryFull;
            }

            InternalAddItem(ItemInstance, EmptySlot);
            return EZfItemMechanicResult::Success;
        }
    }

    // Item Não Stackavel
    int32 EmptySlot = GetFirstEmptySlot();
    if (EmptySlot == INDEX_NONE)
    {
        return EZfItemMechanicResult::Failed_InventoryFull;
    }
    
    InternalAddItem(ItemInstance, EmptySlot);
    return EZfItemMechanicResult::Success;
}

// ============================================================
// FUNÇÕES DE CONSULTA
// ============================================================

UZfItemInstance* UZfInventoryComponent::GetItemAtSlot(int32 SlotIndex) const
{
    for (const FZfInventorySlot& Slot : InventoryList.Slots)
    {
        if (Slot.SlotIndex == SlotIndex)
        {
            return Slot.ItemInstance;
        }
    }
    return nullptr;
}

int32 UZfInventoryComponent::GetSlotIndexOfItem(UZfItemInstance* ItemInstance) const
{
    if (!ItemInstance)
    {
        return INDEX_NONE;
    }

    for (const FZfInventorySlot& Slot : InventoryList.Slots)
    {
        if (Slot.ItemInstance == ItemInstance)
        {
            return Slot.SlotIndex;
        }
    }
    return INDEX_NONE;
}

int32 UZfInventoryComponent::GetFirstEmptySlot() const
{
    // Coleta todos os SlotIndex ocupados
    TSet<int32> OccupiedSlots;
    for (const FZfInventorySlot& Slot : InventoryList.Slots)
    {
        OccupiedSlots.Add(Slot.SlotIndex);
    }

    // Retorna o primeiro índice não ocupado
    for (int32 i = 0; i < CurrentSlotCount; i++)
    {
        if (!OccupiedSlots.Contains(i))
        {
            return i;
        }
    }

    return INDEX_NONE;
}

TArray<UZfItemInstance*> UZfInventoryComponent::GetItemsByTag(const FGameplayTag& Tag) const
{
    TArray<UZfItemInstance*> Result;
    for (const FZfInventorySlot& Slot : InventoryList.Slots)
    {
        if (Slot.ItemInstance && Slot.ItemInstance->HasItemTag(Tag))
        {
            Result.Add(Slot.ItemInstance);
        }
    }
    return Result;
}

int32 UZfInventoryComponent::GetAvailableSlots() const
{
    return CurrentSlotCount - InventoryList.Slots.Num();
}

int32 UZfInventoryComponent::GetAvailableDefaultSlots() const
{
    int32 OccupiedDefaultSlots = 0;
    for (const FZfInventorySlot& Slot : InventoryList.Slots)
    {
        if (Slot.SlotIndex < DefaultSlotCount)
        {
            OccupiedDefaultSlots++;
        }
    }
    return DefaultSlotCount - OccupiedDefaultSlots;
}

int32 UZfInventoryComponent::GetAvailableSlotsWithExpansion(UZfItemInstance* BackpackInstance) const
{
    int32 ExtraSlots = 0;

    if (BackpackInstance)
    {
        if (const UZfFragment_InventoryExpansion* ExpansionFragment = BackpackInstance->GetFragment<UZfFragment_InventoryExpansion>())
        {
            ExtraSlots = ExpansionFragment->ExtraSlotCount;
        }
    }

    const int32 ExpandedCapacity = FMath::Min(DefaultSlotCount + ExtraSlots, MaxAbsoluteSlotCount);

    int32 OccupiedSlots = 0;
    for (const FZfInventorySlot& Slot : InventoryList.Slots)
    {
        if (Slot.SlotIndex < ExpandedCapacity)
        {
            OccupiedSlots++;
        }
    }

    return ExpandedCapacity - OccupiedSlots;
}

int32 UZfInventoryComponent::GetAvailableSlotsUpToIndex(int32 MaxIndex) const
{
    int32 OccupiedSlots = 0;
    for (const FZfInventorySlot& Slot : InventoryList.Slots)
    {
        if (Slot.SlotIndex < MaxIndex && Slot.ItemInstance)
        {
            OccupiedSlots++;
        }
    }
    return MaxIndex + 1 - OccupiedSlots;
}

int32 UZfInventoryComponent::GetItemCountFromInitialSlot(int32 InitialSlotIndex) const
{
    int32 Count = 0;
    for (const FZfInventorySlot& Slot : InventoryList.Slots)
    {
        if (Slot.SlotIndex >= InitialSlotIndex && Slot.ItemInstance)
        {
            Count++;
        }
    }
    return Count;
}

void UZfInventoryComponent::RelocateItemsAboveCapacity(int32 NewCapacity)
{
    TArray<UZfItemInstance*> ItemsToMove;
    
    for (const FZfInventorySlot& Slot : InventoryList.Slots)
    {
        if (Slot.SlotIndex >= NewCapacity && Slot.ItemInstance)
        {
            ItemsToMove.Add(Slot.ItemInstance);
        }
    }

    for (UZfItemInstance* Item : ItemsToMove)
    {
        InternalRemoveItem(GetSlotIndexOfItem(Item));
        InternalAddItem(Item, GetFirstEmptySlot());
    }

    InventoryList.MarkArrayDirty();
}

int32 UZfInventoryComponent::GetTotalSlots() const
{
    return CurrentSlotCount;
}

bool UZfInventoryComponent::IsSlotEmpty(int32 SlotIndex) const
{
    // Slot vazio = não existe entrada no array com esse índice
    for (const FZfInventorySlot& Slot : InventoryList.Slots)
    {
        if (Slot.SlotIndex == SlotIndex)
        {
            return false;
        }
    }
    return true;
}

TArray<UZfItemInstance*> UZfInventoryComponent::GetAllItems() const
{
    TArray<UZfItemInstance*> Result;
    for (const FZfInventorySlot& Slot : InventoryList.Slots)
    {
        if (Slot.ItemInstance)
        {
            Result.Add(Slot.ItemInstance);
        }
    }
    return Result;
}

bool UZfInventoryComponent::IsValidSlotIndex(int32 SlotIndex) const
{
    // Válido se estiver dentro do range do inventário
    return SlotIndex >= 0 && SlotIndex < CurrentSlotCount;
}

FZfInventorySlot* UZfInventoryComponent::FindSlotByIndex(int32 SlotIndex)
{
    return InventoryList.Slots.FindByPredicate(
        [SlotIndex](const FZfInventorySlot& Slot)
        {
            return Slot.SlotIndex == SlotIndex;
        });
}

void UZfInventoryComponent::ServerTryRemoveAmountFromStack_Implementation(UZfItemInstance* ItemInstance, int32 Amount)
{
    if (InternalCheckIsServer(TEXT("ServerTryRemoveItemFromInventory_Implementation")))
    {
        TryRemoveAmountFromStack(ItemInstance, Amount);
    }
}

void UZfInventoryComponent::ServerTryRemoveItemFromInventory_Implementation(int32 SlotIndex)
{
    if (InternalCheckIsServer(TEXT("ServerTryRemoveItemFromInventory_Implementation")))
    {
        TryRemoveItemFromInventory(SlotIndex);
    }
}

void UZfInventoryComponent::ServerTryAddItemToInventory_Implementation(UZfItemInstance* InItemInstance)
{
    if (InternalCheckIsServer(TEXT("ServerTryAddItemToInventory_Implementation")))
    {
        TryAddItemToInventory(InItemInstance);
    }
}

// ============================================================
// FUNÇÕES INTERNAS - GERENCIAMENTO
// ============================================================

void UZfInventoryComponent::InternalAddItem(UZfItemInstance* InItemInstance, int32 TargetSlot)
{
    check(InItemInstance != nullptr);
    
    FZfInventorySlot NewSlot;
    NewSlot.SlotIndex = TargetSlot;
    NewSlot.ItemInstance = InItemInstance;
    InventoryList.Slots.Add(NewSlot);
    
    AddReplicatedSubObject(InItemInstance);
    InventoryList.MarkArrayDirty();
    OnItemAdded.Broadcast(InItemInstance, TargetSlot);
}

void UZfInventoryComponent::  InternalRemoveItem(int32 SlotIndex)
{
    UZfItemInstance* ItemInstance = GetItemAtSlot(SlotIndex);
    check(ItemInstance != nullptr);

    InventoryList.Slots.RemoveAll([SlotIndex](const FZfInventorySlot& Slot)
    {
        return Slot.SlotIndex == SlotIndex;
    });
    InventoryList.MarkArrayDirty();

    RemoveReplicatedSubObject(ItemInstance);
    OnItemRemoved.Broadcast(SlotIndex);
}

int32 UZfInventoryComponent::InternalTryStackWithExistingItems(UZfItemInstance* ItemInstance, int32 AmountToRemove)
{
    if (!ItemInstance) return -1;

    const UZfItemDefinition* IncomingDefinition = ItemInstance->GetItemDefinition();
    if (!IncomingDefinition) return -1;

    int32 Overflow = AmountToRemove;

    // Percorre por SlotIndex em ordem crescente sem reordenar o array
    for (int32 i = 0; i < CurrentSlotCount; i++)
    {
        for (FZfInventorySlot& Slot : InventoryList.Slots)
        {
            if (Slot.SlotIndex != i) continue;
            if (!Slot.ItemInstance) continue;
            if (Slot.ItemInstance->GetItemDefinition() != IncomingDefinition) continue;

            Overflow = Slot.ItemInstance->AddToStack(Overflow);
            InventoryList.MarkItemDirty(Slot);
            OnItemStackChanged.Broadcast(Slot.ItemInstance, Slot.SlotIndex);

            if (Overflow <= 0) return 0;
            break;
        }
    }

    return Overflow;
}

EZfItemMechanicResult UZfInventoryComponent::InternalMoveItemBetweenSlots(int32 FromSlotIndex, int32 ToSlotIndex)
 {
    // Valida os dois slots
    if (!IsValidSlotIndex(FromSlotIndex) || !IsValidSlotIndex(ToSlotIndex))
    {
        UE_LOG(LogZfInventory, Warning, TEXT("UZfInventoryComponent::MoveItemBetweenSlots — " "Slot inválido. From: %d | To: %d"),
        FromSlotIndex, ToSlotIndex);
        return EZfItemMechanicResult::Failed_InvalidSlot;
    }

    // Slots iguais — nada a fazer
    if (FromSlotIndex == ToSlotIndex)
    {
        UE_LOG(LogZfInventory, Warning, TEXT("UZfInventoryComponent::MoveItemBetweenSlots — " "Slot inválido. From: %d é igual a To: %d"),
        FromSlotIndex, ToSlotIndex);
        return EZfItemMechanicResult::Failed_InvalidSlot;
    }

    UZfItemInstance* ItemToMove = GetItemAtSlot(FromSlotIndex);

    if (!ItemToMove)
    {
        UE_LOG(LogZfInventory, Warning, TEXT("UZfInventoryComponent::MoveItemBetweenSlots — " "Slot de origem %d está vazio."), FromSlotIndex);
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    FZfInventorySlot* SlotA = FindSlotByIndex(FromSlotIndex);
    FZfInventorySlot* SlotB = FindSlotByIndex(ToSlotIndex);
    
    if (IsSlotEmpty(ToSlotIndex))
    {
        InternalRemoveItem(FromSlotIndex);
        InternalAddItem(ItemToMove, ToSlotIndex);
    }
    else
    { 
        Swap(SlotA->ItemInstance, SlotB->ItemInstance);
        
        InventoryList.MarkItemDirty(*SlotA);
        InventoryList.MarkItemDirty(*SlotB);
    }

    UE_LOG(LogZfInventory, Verbose,
        TEXT("UZfInventoryComponent::MoveItemBetweenSlots — " "Item '%s' movido do slot %d para %d."),
        *ItemToMove->GetItemName().ToString(), FromSlotIndex, ToSlotIndex);

    return EZfItemMechanicResult::Success;
}


// ============================================================
// FUNÇÕES INTERNAS - ORGANIZAÇÃO
// ============================================================

void UZfInventoryComponent::InternalSortInventoryBySelected(EZfInventorySortType SortType)
{
    if (!InternalCheckIsServer(TEXT("InternalSortInventoryBySelected")))
    {
        return;
    }

    // Coleta todos os ItemInstances presentes no inventário
    TArray<UZfItemInstance*> Items;
    for (const FZfInventorySlot& Slot : InventoryList.Slots)
    {
        if (Slot.ItemInstance)
        {
            Items.Add(Slot.ItemInstance);
        }
    }

    // Ordena de acordo com o tipo selecionado
    switch (SortType)
    {
    case EZfInventorySortType::Alphabetical:
        Items.Sort([](const UZfItemInstance& A, const UZfItemInstance& B)
        {
            return A.GetItemName().ToString() < B.GetItemName().ToString();
        });
        break;

    case EZfInventorySortType::ByItemType:
        Items.Sort([](const UZfItemInstance& A, const UZfItemInstance& B)
        {
            const FString TagA = A.GetItemTags().IsEmpty() ? TEXT("") : A.GetItemTags().GetByIndex(0).ToString();
            const FString TagB = B.GetItemTags().IsEmpty() ? TEXT("") : B.GetItemTags().GetByIndex(0).ToString();
            return TagA < TagB;
        });
        break;

    case EZfInventorySortType::ByRarity:
        Items.Sort([](UZfItemInstance& A, UZfItemInstance& B)
        {
            return static_cast<uint8>(A.GetItemRarity()) > static_cast<uint8>(B.GetItemRarity());
        });
        break;

    case EZfInventorySortType::ByTier:
        Items.Sort([](const UZfItemInstance& A, const UZfItemInstance& B)
        {
            return A.GetItemTier() > B.GetItemTier();
        });
        break;

    case EZfInventorySortType::ByQuantity:
        Items.Sort([](const UZfItemInstance& A, const UZfItemInstance& B)
        {
            return A.GetCurrentStack() > B.GetCurrentStack();
        });
        break;

    case EZfInventorySortType::ByQuality:
        Items.Sort([](const UZfItemInstance& A, const UZfItemInstance& B)
        {
            return A.GetItemQuality() > B.GetItemQuality();
        });
        break;

    case EZfInventorySortType::Compact:
        // Compact não precisa de ordenação — só redistribui na ordem atual
        break;

    default:
        break;
    }

    // Reconstrói o array de slots com SlotIndex sequencial e ordenado
    InventoryList.Slots.Empty();
    for (int32 i = 0; i < Items.Num(); i++)
    {
        FZfInventorySlot NewSlot;
        NewSlot.SlotIndex = i;
        NewSlot.ItemInstance = Items[i];
        InventoryList.Slots.Add(NewSlot);
    }

    OnInventoryRefreshed.Broadcast();
    InventoryList.MarkArrayDirty();

    UE_LOG(LogZfInventory, Verbose, TEXT("UZfInventoryComponent::InternalSortInventoryBySelected — "
        "Inventário ordenado por '%s'. %d itens reorganizados."),
        *UEnum::GetValueAsString(SortType), Items.Num());
}

// ============================================================
// FUNÇÕES INTERNAS - VALIDAÇÃO
// ============================================================

bool UZfInventoryComponent::InternalCheckIsServer(const FString& FunctionName) const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogZfInventory, Error, TEXT("UZfInventoryComponent::%s — GetWorld() retornou nulo."), *FunctionName);
        return false;
    }

    if (World->GetNetMode() == NM_Client)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfInventoryComponent::%s — " "Operação de servidor chamada no cliente!"), *FunctionName);
        return false;
    }

    return true;
}