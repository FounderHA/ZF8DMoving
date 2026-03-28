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
    if (InternalCheckIsServer(TEXT("ServerTryRemoveItemFromInventory_Implementation")))
    {
        switch (SortType)
        {
        case EZfInventorySortType::Alphabetical: InternalSortInventoryAlphabetically(); break;
        case EZfInventorySortType::ByItemType: InternalSortInventoryByItemType();     break;
        case EZfInventorySortType::ByRarity: InternalSortInventoryByRarity();       break;
        case EZfInventorySortType::Compact: InternalCompactInventory();       break;
        default: break;
        }
    }
    
    
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

int32 UZfInventoryComponent::AddExtraSlots(int32 ExtraSlots)
{
    if (!InternalCheckIsServer(TEXT("AddExtraSlots")))
    {
        return 0;
    }

    // Calcula quantos slots realmente podem ser adicionados
    const int32 SlotsAvailableToAdd = MaxAbsoluteSlotCount - CurrentSlotCount;

    const int32 SlotsToAdd = FMath::Min(ExtraSlots, SlotsAvailableToAdd);

    if (SlotsToAdd <= 0)
    {
        UE_LOG(LogZfInventory, Warning, TEXT("UZfInventoryComponent::AddExtraSlots — " "Limite máximo de slots atingido (%d)."), MaxAbsoluteSlotCount);
        return 0;
    }

    // Adiciona quantidade maxima de slot
    CurrentSlotCount += SlotsToAdd;

    OnInventorySizeChanged.Broadcast(CurrentSlotCount);
    InventoryList.MarkArrayDirty();

    UE_LOG(LogZfInventory, Log, TEXT("UZfInventoryComponent::AddExtraSlots — " "%d slots adicionados. Total: %d/%d"),
        SlotsToAdd, CurrentSlotCount, MaxAbsoluteSlotCount);

    return SlotsToAdd;
}

EZfItemMechanicResult UZfInventoryComponent::RemoveExtraSlots(int32 SlotsToRemove)
{
    if (!InternalCheckIsServer(TEXT("RemoveExtraSlots")))
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    // Não pode ter menos que o mínimo padrão
    const int32 MinSlots = DefaultSlotCount;
    const int32 ActualSlotsToRemove = FMath::Min(SlotsToRemove, CurrentSlotCount - MinSlots);

    if (ActualSlotsToRemove <= 0)
    {
        UE_LOG(LogZfInventory, Warning, TEXT("UZfInventoryComponent::RemoveExtraSlots — " "Não é possível remover slots abaixo do mínimo (%d)."), MinSlots);
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    // Verifica se há itens nos slots que serão removidos
    const int32 FirstSlotToRemove = CurrentSlotCount - ActualSlotsToRemove;
    for (int32 i = FirstSlotToRemove; i < CurrentSlotCount; i++)
    {
        
        if (!IsSlotEmpty(i))
        {
            // Tenta mover o item para um slot livre anterior
            UZfItemInstance* ItemToMove = GetItemAtSlot(i);
            const int32 FreeSlot = GetFirstEmptySlot();

            if (FreeSlot != INDEX_NONE && FreeSlot < FirstSlotToRemove)
            {
                InternalAddItem(ItemToMove, FreeSlot);
                InternalRemoveItem(i);
                OnItemMoved.Broadcast();
            }
            else
            {
                UE_LOG(LogZfInventory, Warning, TEXT("UZfInventoryComponent::RemoveExtraSlots — " "Não há slots livres para mover item do slot %d."), i);
                return EZfItemMechanicResult::Failed_InventoryFull;
            }
        }
    }

    // Remove os slots do final do array
    CurrentSlotCount = FirstSlotToRemove;

    OnInventorySizeChanged.Broadcast(CurrentSlotCount);
    InventoryList.MarkArrayDirty();

    UE_LOG(LogZfInventory, Log, TEXT("UZfInventoryComponent::RemoveExtraSlots — " "%d slots removidos. Total: %d"), ActualSlotsToRemove, CurrentSlotCount);

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

void UZfInventoryComponent::InternalRemoveItem(int32 SlotIndex)
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

void UZfInventoryComponent::InternalSortInventoryAlphabetically()
{
    if (!InternalCheckIsServer(TEXT("SortInventoryAlphabetically")))
    {
        return;
    }

    // Coleta todos os itens não nulos
    TArray<UZfItemInstance*> Items;
    for (FZfInventorySlot& Slot : InventoryList.Slots)
    {
        if (Slot.ItemInstance)
        {
            Items.Add(Slot.ItemInstance);
            Slot.ItemInstance = nullptr;
        }
    }

    // Ordena alfabeticamente pelo nome do item
    Items.Sort([](const UZfItemInstance& A, const UZfItemInstance& B)
    {
        return A.GetItemName().ToString() < B.GetItemName().ToString();
    });

    // Redistribui nos slots em ordem
    for (int32 i = 0; i < Items.Num(); i++)
    {
        InventoryList.Slots[i].ItemInstance = Items[i];
    }

    OnInventoryRefreshed.Broadcast();
    InventoryList.MarkArrayDirty();

    UE_LOG(LogZfInventory, Log,
        TEXT("UZfInventoryComponent::SortInventoryAlphabetically — " "Inventário ordenado alfabeticamente."));
}

void UZfInventoryComponent::InternalSortInventoryByItemType()
{
    if (!InternalCheckIsServer(TEXT("SortInventoryByItemType")))
    {
        return;
    }

    TArray<UZfItemInstance*> Items;
    for (FZfInventorySlot& Slot : InventoryList.Slots)
    {
        if (Slot.ItemInstance)
        {
            Items.Add(Slot.ItemInstance);
            Slot.ItemInstance = nullptr;
        }
    }

    // Ordena pela primeira tag do item (string comparison)
    Items.Sort([](const UZfItemInstance& A, const UZfItemInstance& B)
    {
        const FGameplayTagContainer TagsA = A.GetItemTags();
        const FGameplayTagContainer TagsB = B.GetItemTags();

        const FString TagStringA = TagsA.IsEmpty() ? TEXT("") : TagsA.GetByIndex(0).ToString();
        const FString TagStringB = TagsB.IsEmpty() ? TEXT("") : TagsB.GetByIndex(0).ToString();

        return TagStringA < TagStringB;
    });

    for (int32 i = 0; i < Items.Num(); i++)
    {
        InventoryList.Slots[i].ItemInstance = Items[i];
    }

    OnInventoryRefreshed.Broadcast();
    InventoryList.MarkArrayDirty();

    UE_LOG(LogZfInventory, Log, TEXT("UZfInventoryComponent::SortInventoryByItemType — " "Inventário ordenado por tipo."));
}

void UZfInventoryComponent::InternalSortInventoryByRarity()
{
    if (!InternalCheckIsServer(TEXT("SortInventoryByRarity")))
    {
        return;
    }

    TArray<UZfItemInstance*> Items;
    for (FZfInventorySlot& Slot : InventoryList.Slots)
    {
        if (Slot.ItemInstance)
        {
            Items.Add(Slot.ItemInstance);
            Slot.ItemInstance = nullptr;
        }
    }

    // Ordena por raridade — mais raro primeiro (valor mais alto do enum)
    Items.Sort([](const UZfItemInstance& A, const UZfItemInstance& B)
    {
        return static_cast<uint8>(A.ItemRarity) > static_cast<uint8>(B.ItemRarity);
    });

    for (int32 i = 0; i < Items.Num(); i++)
    {
        InventoryList.Slots[i].ItemInstance = Items[i];
    }

    OnInventoryRefreshed.Broadcast();
    InventoryList.MarkArrayDirty();

    UE_LOG(LogZfInventory, Log, TEXT("UZfInventoryComponent::SortInventoryByRarity — " "Inventário ordenado por raridade."));
}

void UZfInventoryComponent::InternalCompactInventory()
{
    if (!InternalCheckIsServer(TEXT("CompactInventory")))
    {
        return;
    }

    // Coleta itens em ordem
    TArray<UZfItemInstance*> Items;
    for (FZfInventorySlot& Slot : InventoryList.Slots)
    {
        if (Slot.ItemInstance)
        {
            Items.Add(Slot.ItemInstance);
            Slot.ItemInstance = nullptr;
        }
    }

    // Redistribui compactado nos primeiros slots
    for (int32 i = 0; i < Items.Num(); i++)
    {
        InventoryList.Slots[i].ItemInstance = Items[i];
    }

    OnInventoryRefreshed.Broadcast();
    InventoryList.MarkArrayDirty();

    UE_LOG(LogZfInventory, Log, TEXT("UZfInventoryComponent::CompactInventory — " "Inventário compactado. %d itens reorganizados."), Items.Num());
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