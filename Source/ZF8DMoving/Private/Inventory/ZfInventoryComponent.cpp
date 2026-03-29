// Copyright ZfGame Studio. All Rights Reserved.
// ZfInventoryComponent.cpp

#include "Inventory/ZfInventoryComponent.h"
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
#include "Inventory/ZfItemPickup.h"
#include "Player/ZfPlayerState.h"


EZfItemMechanicResult UZfInventoryComponent::TryEquipItemFromSlot(int32 SlotIndex)
{
    if (!InternalCheckIsServer(TEXT("TryEquipItemFromSlot")))
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    if (!IsValidSlotIndex(SlotIndex))
    {
        return EZfItemMechanicResult::Failed_InvalidSlot;
    }

    UZfItemInstance* ItemInstance = InventoryList.Slots[SlotIndex].ItemInstance;

    if (!ItemInstance)
    {
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    return TryEquipItem(ItemInstance);
}

EZfItemMechanicResult UZfInventoryComponent::TryEquipItem(UZfItemInstance* ItemInstance)
{
    if (!InternalCheckIsServer(TEXT("TryEquipItem")))
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    if (!ItemInstance)
    {
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    // Verifica se o item tem o fragment Equippable
    const UZfFragment_Equippable* EquippableFragment = ItemInstance->GetFragment<UZfFragment_Equippable>();

    if (!EquippableFragment)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfInventoryComponent::TryEquipItem — " "Item '%s' não tem UZfFragment_Equippable. " "Não pode ser equipado."),
            *ItemInstance->GetItemName().ToString());
        return EZfItemMechanicResult::Failed_CannotEquip;
    }

    // Verifica se o item está quebrado
    if (ItemInstance->bIsBroken)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfInventoryComponent::TryEquipItem — " "Item '%s' está quebrado e não pode ser equipado."),
            *ItemInstance->GetItemName().ToString());
        return EZfItemMechanicResult::Failed_ItemBroken;
    }

    // Remove o item do inventário antes de equipar
    const int32 SlotIndex = GetSlotIndexOfItem(ItemInstance);
    if (SlotIndex == INDEX_NONE)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfInventoryComponent::TryEquipItem — " "Item '%s' não encontrado no inventário."),
            *ItemInstance->GetItemName().ToString());
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    // Delega ao EquipmentComponent — ele valida slots, two-handed, etc.
    /*const EZfItemMechanicResult EquipResult = EquipmentComponent->TryEquipItem(ItemInstance);

    if (EquipResult == EZfItemMechanicResult::Success)
    {
        // Remove do inventário apenas se o equip foi bem sucedido
        InternalRemoveItem(SlotIndex);
        OnItemRemoved.Broadcast(ItemInstance, SlotIndex);
        InventoryList.MarkArrayDirty();

        UE_LOG(LogZfInventory, Log,
            TEXT("UZfInventoryComponent::TryEquipItem — " "Item '%s' equipado com sucesso."), *ItemInstance->GetItemName().ToString());
    }*/

    return EZfItemMechanicResult::Failed_ItemNotFound;
}

EZfItemMechanicResult UZfInventoryComponent::ReceiveUnequippedItem(UZfItemInstance* ItemInstance, int32 PreferredSlotIndex)
{
    if (!InternalCheckIsServer(TEXT("ReceiveUnequippedItem")))
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    if (!ItemInstance)
    {
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    // Tenta colocar no slot preferido primeiro
    if (IsValidSlotIndex(PreferredSlotIndex) && IsSlotEmpty(PreferredSlotIndex))
    {
        InventoryList.Slots[PreferredSlotIndex].ItemInstance = ItemInstance;
        ItemInstance->NotifyFragments_ItemAddedToInventory(this);
        OnItemAdded.Broadcast(ItemInstance, PreferredSlotIndex);
        InventoryList.MarkArrayDirty();

        UE_LOG(LogZfInventory, Log, TEXT("UZfInventoryComponent::ReceiveUnequippedItem — " "Item '%s' retornou ao slot preferido %d."),
        *ItemInstance->GetItemName().ToString(), PreferredSlotIndex);

        return EZfItemMechanicResult::Success;
    }

    // Slot preferido indisponível — busca o primeiro livre
    int32 OutSlot = INDEX_NONE;
    return TryAddItemToInventory(ItemInstance);
}

void UZfInventoryComponent::ServerRequestEquipItem_Implementation(int32 SlotIndex)
{
    TryEquipItemFromSlot(SlotIndex);
}

void UZfInventoryComponent::ClientNotifyInventoryUpdated_Implementation()
{
    // Notifica a UI que o inventário foi atualizado
    OnInventoryRefreshed.Broadcast();

    UE_LOG(LogZfInventory, Verbose, TEXT("UZfInventoryComponent::ClientNotifyInventoryUpdated — " "Inventário atualizado no cliente."));
}

void UZfInventoryComponent::ClientNotifyOperationFailed_Implementation(EZfItemMechanicResult FailureResult)
{
    UE_LOG(LogZfInventory, Warning, TEXT("UZfInventoryComponent::ClientNotifyOperationFailed — " "Operação falhou: %s"),
        *UEnum::GetValueAsString(FailureResult));
}

// ============================================================
// FUNÇÕES INTERNAS
// ============================================================

void UZfInventoryComponent::Internal_FindEquipmentComponent()
{
    if (GetOwner())
    {
        EquipmentComponent = GetOwner()->FindComponentByClass<UZfEquipmentComponent>();

        if (!EquipmentComponent)
        {
            UE_LOG(LogZfInventory, Warning, TEXT("UZfInventoryComponent::Internal_FindEquipmentComponent — " 
                "UZfEquipmentComponent não encontrado no ator '%s'. ""Funcionalidades de equipamento estarão indisponíveis."),
                *GetOwner()->GetName());
        }
        else
        {
            UE_LOG(LogZfInventory, Log,
                TEXT("UZfInventoryComponent::Internal_FindEquipmentComponent — " "UZfEquipmentComponent encontrado."));
        }
    }
}

// ============================================================
// DEBUG
// ============================================================

void UZfInventoryComponent::DebugLogInventory() const
{
    UE_LOG(LogZfInventory, Log,
        TEXT("=== INVENTORY DEBUG [%s] ==="), GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"));

    UE_LOG(LogZfInventory, Log,
        TEXT("Slots: %d/%d | Disponíveis: %d"), GetTotalSlots(), MaxAbsoluteSlotCount, GetAvailableSlots());

    for (const FZfInventorySlot& Slot : InventoryList.Slots)
    {
        if (Slot.ItemInstance)
        {
            UE_LOG(LogZfInventory, Log,
                TEXT("  [%d] %s"), Slot.SlotIndex, *Slot.ItemInstance->GetDebugString());
        }
        else
        {
            UE_LOG(LogZfInventory, Verbose,
                TEXT("  [%d] --- vazio ---"), Slot.SlotIndex);
        }
    }
}















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
        if (EquipmentComponent->GetItemAtEquipmentSlot(EZfEquipmentSlot::Backpack))
        {
            if (EquipmentComponent->GetItemAtEquipmentSlot(EZfEquipmentSlot::Backpack)->GetFragment<UZfFragment_InventoryExpansion>())
            {
                const UZfFragment_InventoryExpansion* Backpack = EquipmentComponent->GetItemAtEquipmentSlot(EZfEquipmentSlot::Backpack)->
                    GetFragment<UZfFragment_InventoryExpansion>();
                CurrentSlotCount = DefaultSlotCount + Backpack->ExtraSlotCount;
            }
            else
            {
                CurrentSlotCount = DefaultSlotCount;
            }
        }
        
        CurrentSlotCount = DefaultSlotCount;

        UE_LOG(LogZfInventory, Log, TEXT("UZfInventoryComponent::BeginPlay — " "Inventário inicializado com %d slots disponíveis."), CurrentSlotCount);
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
        InArraySerializer.OwnerComponent->OnItemRemoved.Broadcast(ItemInstance, SlotIndex);
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
        InArraySerializer.OwnerComponent->OnItemMoved.Broadcast();
        InArraySerializer.OwnerComponent->OnInventorySizeChanged.Broadcast();
    }
}

// ============================================================
// FUNÇÕES SERVER - GERENCIAMENTO
// ============================================================

void UZfInventoryComponent::ServerTryAddItemToInventory_Implementation(UZfItemInstance* InItemInstance)
{
    if (InternalCheckIsServer(TEXT("ServerTryAddItemToInventory_Implementation")))
    {
        TryAddItemToInventory(InItemInstance);
    }
}

void UZfInventoryComponent::ServerTryRemoveItemFromInventory_Implementation(int32 SlotIndex)
{
    if (InternalCheckIsServer(TEXT("ServerTryRemoveItemFromInventory_Implementation")))
    {
        TryRemoveItemFromInventory(SlotIndex);
    }
}

void UZfInventoryComponent::ServerTryMoveItem_Implementation(int32 FromSlotIndex, int32 ToSlotIndex)
{
    if (InternalCheckIsServer(TEXT("ServerTryRemoveItemFromInventory_Implementation")))
    {
        InternalMoveItemBetweenSlots(FromSlotIndex, ToSlotIndex);
    }
    
}

void UZfInventoryComponent::ServerTryRemoveAmountFromStack_Implementation(UZfItemInstance* ItemInstance, int32 Amount)
{
    if (InternalCheckIsServer(TEXT("ServerTryRemoveItemFromInventory_Implementation")))
    {
        TryRemoveAmountFromStack(ItemInstance, Amount);
    }
}

void UZfInventoryComponent::ServerTryDropItem_Implementation(int32 SlotIndex)
{
    if (!InternalCheckIsServer(TEXT("ServerTryRemoveItemFromInventory_Implementation")))
    {
        return;
    }
    
    UZfItemInstance* ItemInstance = GetItemAtSlot(SlotIndex);
    const EZfItemMechanicResult Result = TryRemoveItemFromInventory(SlotIndex);
    
    if (Result == EZfItemMechanicResult::Success && ItemInstance)
    {
        TrySpawnPickupItem(ItemInstance);
    }
}

void UZfInventoryComponent::ServerTrySortInventory_Implementation(EZfInventorySortType SortType)
{
    if (!InternalCheckIsServer(TEXT("ServerTrySortInventory")))
    {
        return;
    }

    InternalSortInventoryBySelected(SortType);
}

// ============================================================
// FUNÇÕES PRINCIPAIS - GERENCIAMENTO
// ============================================================

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
        if (InternalTryStackWithExistingItems(InItemInstance))
        {
            // Item foi completamente absorvido por stacks existentes
            UE_LOG(LogZfInventory, Log, TEXT("UZfInventoryComponent::TryAddItemToInventory — " 
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
    OnItemAdded.Broadcast(InItemInstance,EmptySlot);
    
    // Marca o inventário como modificado para replicação
    InventoryList.MarkArrayDirty();

    // Notifica os fragments do item
    InItemInstance->NotifyFragments_ItemAddedToInventory(this);
    
    UE_LOG(LogZfInventory, Log,
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
    OnItemRemoved.Broadcast(ItemInstance, SlotIndex);

    InventoryList.MarkArrayDirty();

    UE_LOG(LogZfInventory, Log, TEXT("UZfInventoryComponent::RemoveItemAtSlot — " "Item '%s' removido do slot %d."),
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

    UE_LOG(LogZfInventory, Log,
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
    const int32 AmountToRemove = FMath::Min(Amount, ItemInstance->CurrentStack);
    
    // Remove a quantidade — retorna true se o stack zerou
    if (ItemInstance->RemoveFromStack(AmountToRemove))
    {
        // Stack zerou — remove o item do inventário
        InternalRemoveItem(GetSlotIndexOfItem(ItemInstance));
        OnItemRemoved.Broadcast(ItemInstance, GetSlotIndexOfItem(ItemInstance));
        return EZfItemMechanicResult::Success;
    }

    InventoryList.MarkArrayDirty();
    return EZfItemMechanicResult::Success;
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

            UE_LOG(LogZfInventory, Log, 
                TEXT("ServerTryDropItem — Item '%s' dropado em %s."),
                *ItemInstance->GetItemName().ToString(), *DropLocation.ToString());
        }
    }
}

void UZfInventoryComponent::UpdateSlotCountFromEquippedBackpack()
{
    int32 ExtraSlots = 0;

    if (EquipmentComponent)
    {
        // Busca o item no slot de mochila usando a tag
        UZfItemInstance* BackpackItem = EquipmentComponent->GetItemAtSlotTag(FGameplayTag::RequestGameplayTag(TEXT("EquipmentSlot.Slot.Backpack")));

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
    InventoryList.MarkArrayDirty();
    //OnInventorySizeChanged.Broadcast();
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
    TSet<int32> OccupiedSlots;
    for (const FZfInventorySlot& Slot : InventoryList.Slots)
    {
        OccupiedSlots.Add(Slot.SlotIndex);
    }

    for (const FZfInventorySlot& Slot : InventoryList.Slots)
    {
        if (Slot.SlotIndex >= NewCapacity && Slot.ItemInstance)
        {
            // Acha o primeiro slot livre abaixo da nova capacidade
            for (int32 i = 0; i < NewCapacity; i++)
            {
                if (!OccupiedSlots.Contains(i))
                {
                    InternalRemoveItem(Slot.SlotIndex);
                    InternalAddItem(Slot.ItemInstance, i);
                    OccupiedSlots.Add(i);
                    break;
                }
            }
        }
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
}

void UZfInventoryComponent::  InternalRemoveItem(int32 SlotIndex)
{
    UZfItemInstance* ItemInstance = GetItemAtSlot(SlotIndex);
    check(ItemInstance != nullptr);

    InventoryList.Slots.RemoveAll([SlotIndex](const FZfInventorySlot& Slot)
    {
        return Slot.SlotIndex == SlotIndex;
    });

    RemoveReplicatedSubObject(ItemInstance);
}

bool UZfInventoryComponent::InternalTryStackWithExistingItems(UZfItemInstance* ItemInstance)
{
    if (!ItemInstance)
    {
        return false;
    }

    const UZfItemDefinition* IncomingDefinition = ItemInstance->GetItemDefinition();

    if (!IncomingDefinition)
    {
        return false;
    }

    // Percorre todos os slots buscando stacks do mesmo item
    for (FZfInventorySlot& Slot : InventoryList.Slots)
    {
        if (!Slot.ItemInstance)
        {
            continue;
        }

        // Verifica se é o mesmo tipo de item pelo ItemDefinition
        if (Slot.ItemInstance->GetItemDefinition() != IncomingDefinition)
        {
            continue;
        }

        // Tenta empilhar
        const int32 Overflow = Slot.ItemInstance->AddToStack(ItemInstance->CurrentStack);

        // Atualiza o stack do item incoming
        ItemInstance->SetCurrentStack(Overflow);

        //AddReplicatedSubObject(ItemInstance);
        InventoryList.MarkItemDirty(Slot);

        // Se não tem overflow, o item foi completamente absorvido
        if (Overflow <= 0)
        {
            OnItemAdded.Broadcast(Slot.ItemInstance,Slot.SlotIndex);
            return true;
        }
    }

    // Ainda tem quantidade restante — não foi completamente absorvido
    return false;
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
        InventoryList.MarkArrayDirty();
    }
    else
    { 
        Swap(SlotA->ItemInstance, SlotB->ItemInstance);
        
        InventoryList.MarkItemDirty(*SlotA);
        InventoryList.MarkItemDirty(*SlotB);
    }
    
    // Notifica a UI
    OnItemMoved.Broadcast();
    //OnInventoryRefreshed.Broadcast();
    
    UE_LOG(LogZfInventory, Log,
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
        Items.Sort([](const UZfItemInstance& A, const UZfItemInstance& B)
        {
            return static_cast<uint8>(A.ItemRarity) > static_cast<uint8>(B.ItemRarity);
        });
        break;

    case EZfInventorySortType::ByTier:
        Items.Sort([](const UZfItemInstance& A, const UZfItemInstance& B)
        {
            return A.ItemTier > B.ItemTier;
        });
        break;

    case EZfInventorySortType::ByQuantity:
        Items.Sort([](const UZfItemInstance& A, const UZfItemInstance& B)
        {
            return A.CurrentStack > B.CurrentStack;
        });
        break;

    case EZfInventorySortType::ByQuality:
        Items.Sort([](const UZfItemInstance& A, const UZfItemInstance& B)
        {
            return A.CurrentQuality > B.CurrentQuality;
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

    UE_LOG(LogZfInventory, Log, TEXT("UZfInventoryComponent::InternalSortInventoryBySelected — "
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