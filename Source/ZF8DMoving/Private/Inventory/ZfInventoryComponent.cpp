// Copyright ZfGame Studio. All Rights Reserved.
// ZfInventoryComponent.cpp

#include "Inventory/ZfInventoryComponent.h"
#include "Inventory/ZfEquipmentComponent.h"
#include "Inventory/ZfItemInstance.h"
#include "Inventory/ZfItemDefinition.h"
#include "Inventory/Fragments/ZfFragment_Stackable.h"
#include "Inventory/Fragments/ZfFragment_Equippable.h"
#include "Inventory/Fragments/ZfFragment_InventoryExpansion.h"
#include "Inventory/ZfItemPickup.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

// ============================================================
// Constructor
// ============================================================

UZfInventoryComponent::UZfInventoryComponent()
{
    // Componente não precisa de tick por padrão
    PrimaryComponentTick.bCanEverTick = false;

    // Habilita replicação do componente
    SetIsReplicatedByDefault(true);
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
// CICLO DE VIDA
// ============================================================

void UZfInventoryComponent::BeginPlay()
{
    Super::BeginPlay();

    // Busca o EquipmentComponent no ator dono
    Internal_FindEquipmentComponent();

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
// ADICIONAR ITEM
// ============================================================

EZfItemMechanicResult UZfInventoryComponent::TryAddItemToInventory(UZfItemInstance* ItemInstance,int32& OutSlotIndex)
{
    OutSlotIndex = INDEX_NONE;

    if (!Internal_CheckIsServer(TEXT("TryAddItemToInventory")))
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    if (!ItemInstance)
    {
        UE_LOG(LogZfInventory, Warning, TEXT("UZfInventoryComponent::TryAddItemToInventory — " "ItemInstance é nulo."));
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    // Tenta empilhar com itens existentes se for stackável
    const UZfFragment_Stackable* StackFragment = ItemInstance->GetFragment<UZfFragment_Stackable>();

    if (StackFragment && StackFragment->bAutoStackOnPickup)
    {
        if (Internal_TryStackWithExistingItems(ItemInstance))
        {
            // Item foi completamente absorvido por stacks existentes
            UE_LOG(LogZfInventory, Log, TEXT("UZfInventoryComponent::TryAddItemToInventory — " 
            "Item '%s' completamente empilhado em stacks existentes."),
            *ItemInstance->GetItemName().ToString());
            return EZfItemMechanicResult::Success;
        }
    }

    // Se ainda tem quantidade restante ou não é stackável,
    // busca o primeiro slot vazio
    const int32 EmptySlot = GetFirstEmptySlot();
    if (EmptySlot == INDEX_NONE)
    {
        UE_LOG(LogZfInventory, Warning, TEXT("UZfInventoryComponent::TryAddItemToInventory — " 
        "Inventário cheio. Item '%s' não pode ser adicionado."), *ItemInstance->GetItemName().ToString());
        return EZfItemMechanicResult::Failed_InventoryFull;
    }

    // Cria uma nova entrada no array apenas com o item
    FZfInventorySlot NewSlot;
    NewSlot.SlotIndex     = EmptySlot;
    NewSlot.ItemInstance  = ItemInstance;
    InventoryList.Slots.Add(NewSlot);
    
    OutSlotIndex = EmptySlot;

    // Notifica os fragments do item
    ItemInstance->NotifyFragments_ItemAddedToInventory(this);
    
    // Notifica a UI e outros sistemas via delegate
    OnItemAdded.Broadcast(ItemInstance, EmptySlot);

    // Marca o inventário como modificado para replicação
    InventoryList.MarkArrayDirty();

    UE_LOG(LogZfInventory, Log,
        TEXT("UZfInventoryComponent::TryAddItemToInventory — "
             "Item '%s' adicionado ao slot %d."),
        *ItemInstance->GetItemName().ToString(), EmptySlot);

    return EZfItemMechanicResult::Success;
}

EZfItemMechanicResult UZfInventoryComponent::TryAddItemToSpecificSlot(UZfItemInstance* ItemInstance, int32 SlotIndex)
{
    if (!Internal_CheckIsServer(TEXT("TryAddItemToSpecificSlot")))
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    if (!ItemInstance)
    {
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    if (!IsValidSlotIndex(SlotIndex))
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfInventoryComponent::TryAddItemToSpecificSlot — "
                 "SlotIndex %d inválido."), SlotIndex);
        return EZfItemMechanicResult::Failed_InvalidSlot;
    }

    // Se o slot já tem um item, troca os dois de lugar
    UZfItemInstance* ExistingItem = GetItemAtSlot(SlotIndex);

    if (ExistingItem)
    {
        // Busca o slot do item que está sendo inserido
        const int32 IncomingItemSlot = GetSlotIndexOfItem(ItemInstance);

        // Faz a troca
        InventoryList.Slots[SlotIndex].ItemInstance = ItemInstance;

        if (IncomingItemSlot != INDEX_NONE)
        {
            InventoryList.Slots[IncomingItemSlot].ItemInstance = ExistingItem;
            OnItemMoved.Broadcast(ExistingItem, SlotIndex, IncomingItemSlot);
        }

        OnItemMoved.Broadcast(ItemInstance, IncomingItemSlot, SlotIndex);
    }
    else
    {
        // Slot vazio — apenas coloca o item
        InventoryList.Slots[SlotIndex].ItemInstance = ItemInstance;
        ItemInstance->NotifyFragments_ItemAddedToInventory(this);
        OnItemAdded.Broadcast(ItemInstance, SlotIndex);
    }

    InventoryList.MarkArrayDirty();

    UE_LOG(LogZfInventory, Log, TEXT("UZfInventoryComponent::TryAddItemToSpecificSlot — " "Item '%s' adicionado ao slot %d."),
    *ItemInstance->GetItemName().ToString(), SlotIndex);

    return EZfItemMechanicResult::Success;
}

EZfItemMechanicResult UZfInventoryComponent::CreateAndAddItemToInventory(UZfItemDefinition* ItemDefinition, int32 Tier,
    EZfItemRarity Rarity, UZfItemInstance*& OutItemInstance)
{
    OutItemInstance = nullptr;

    if (!Internal_CheckIsServer(TEXT("CreateAndAddItemToInventory")))
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    if (!ItemDefinition)
    {
        UE_LOG(LogZfInventory, Error,
            TEXT("UZfInventoryComponent::CreateAndAddItemToInventory — " "ItemDefinition é nulo."));
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    // Verifica se há espaço antes de criar o item
    if (IsInventoryFull())
    {
        return EZfItemMechanicResult::Failed_InventoryFull;
    }

    // Cria o ItemInstance com o ator dono como Outer
    // O Outer deve ser o ator para replicação correta
    UZfItemInstance* NewInstance = NewObject<UZfItemInstance>(GetOwner());
    if (!NewInstance)
    {
        UE_LOG(LogZfInventory, Error,
            TEXT("UZfInventoryComponent::CreateAndAddItemToInventory — " "Falha ao criar UZfItemInstance."));
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    // Inicializa o item
    NewInstance->InitializeItemInstance(ItemDefinition, Tier, Rarity);

    // Tenta adicionar ao inventário
    int32 SlotIndex = INDEX_NONE;
    const EZfItemMechanicResult Result = TryAddItemToInventory(NewInstance, SlotIndex);

    if (Result == EZfItemMechanicResult::Success)
    {
        OutItemInstance = NewInstance;

        UE_LOG(LogZfInventory, Log,
            TEXT("UZfInventoryComponent::CreateAndAddItemToInventory — " "Item '%s' criado e adicionado ao slot %d. GUID: %s"),
            *ItemDefinition->ItemName.ToString(), SlotIndex, *NewInstance->GetItemGuid().ToString());
    }

    return Result;
}

// ============================================================
// REMOVER ITEM
// ============================================================

EZfItemMechanicResult UZfInventoryComponent::RemoveItemAtSlot(int32 SlotIndex, UZfItemInstance*& OutItemInstance)
{
    OutItemInstance = nullptr;

    if (!Internal_CheckIsServer(TEXT("RemoveItemAtSlot")))
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
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfInventoryComponent::RemoveItemAtSlot — " "Slot %d está vazio."), SlotIndex);
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    // Notifica os fragments antes de remover
    ItemInstance->NotifyFragments_ItemRemovedFromInventory(this);

    // Limpa o slot
    OutItemInstance = ItemInstance;
    Internal_ClearSlot(SlotIndex);

    // Notifica a UI
    OnItemRemoved.Broadcast(ItemInstance, SlotIndex);

    InventoryList.MarkArrayDirty();

    UE_LOG(LogZfInventory, Log, TEXT("UZfInventoryComponent::RemoveItemAtSlot — " "Item '%s' removido do slot %d."),
        *ItemInstance->GetItemName().ToString(), SlotIndex);

    return EZfItemMechanicResult::Success;
}

EZfItemMechanicResult UZfInventoryComponent::RemoveItemInstance(UZfItemInstance* ItemInstance)
{
    if (!Internal_CheckIsServer(TEXT("RemoveItemInstance")))
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    if (!ItemInstance)
    {
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    const int32 SlotIndex = GetSlotIndexOfItem(ItemInstance);
    if (SlotIndex == INDEX_NONE)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfInventoryComponent::RemoveItemInstance — " "Item '%s' não encontrado no inventário."),
            *ItemInstance->GetItemName().ToString());
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    UZfItemInstance* RemovedItem = nullptr;
    return RemoveItemAtSlot(SlotIndex, RemovedItem);
}

EZfItemMechanicResult UZfInventoryComponent::RemoveAmountFromStack( UZfItemInstance* ItemInstance, int32 Amount)
{
    if (!Internal_CheckIsServer(TEXT("RemoveAmountFromStack")))
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

    // Remove a quantidade — retorna true se o stack zerou
    bool bStackDepleted = ItemInstance->RemoveFromStack(Amount);

    if (bStackDepleted)
    {
        // Stack zerou — remove o item do inventário
        return RemoveItemInstance(ItemInstance);
    }

    InventoryList.MarkArrayDirty();
    return EZfItemMechanicResult::Success;
}

// ============================================================
// MOVER ITEM
// ============================================================

EZfItemMechanicResult UZfInventoryComponent::MoveItemBetweenSlots(int32 FromSlotIndex, int32 ToSlotIndex)
{
    if (!Internal_CheckIsServer(TEXT("MoveItemBetweenSlots")))
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

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
        return EZfItemMechanicResult::Success;
    }

    UZfItemInstance* ItemToMove = InventoryList.Slots[FromSlotIndex].ItemInstance;

    if (!ItemToMove)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfInventoryComponent::MoveItemBetweenSlots — " "Slot de origem %d está vazio."), FromSlotIndex);
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    
    

    if (IsSlotEmpty(ToSlotIndex))
    {
        TryAddItemToSpecificSlot(ItemToMove, ToSlotIndex);
        Internal_ClearSlot(FromSlotIndex);
    }
    else
    {
        // Pega o item no slot de destino
        UZfItemInstance* ItemAtDestination = InventoryList.Slots[ToSlotIndex].ItemInstance;

        // Faz a troca (ou move para slot vazio)
        InventoryList.Slots[ToSlotIndex].ItemInstance = ItemToMove;
        InventoryList.Slots[FromSlotIndex].ItemInstance = ItemAtDestination;
    }
    
    

    // Notifica a UI
    OnItemMoved.Broadcast(ItemToMove, FromSlotIndex, ToSlotIndex);

    /*
    if (ItemAtDestination)
    {
        OnItemMoved.Broadcast(ItemAtDestination, ToSlotIndex, FromSlotIndex);
    }
    */
    
    InventoryList.MarkArrayDirty();

    UE_LOG(LogZfInventory, Log,
        TEXT("UZfInventoryComponent::MoveItemBetweenSlots — " "Item '%s' movido do slot %d para %d."),
        *ItemToMove->GetItemName().ToString(), FromSlotIndex, ToSlotIndex);

    return EZfItemMechanicResult::Success;
}

// ============================================================
// EQUIPAR
// ============================================================

EZfItemMechanicResult UZfInventoryComponent::TryEquipItemFromSlot(int32 SlotIndex)
{
    if (!Internal_CheckIsServer(TEXT("TryEquipItemFromSlot")))
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
    if (!Internal_CheckIsServer(TEXT("TryEquipItem")))
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    if (!ItemInstance)
    {
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    if (!Internal_CheckEquipmentComponent(TEXT("TryEquipItem")))
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
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
    const EZfItemMechanicResult EquipResult = EquipmentComponent->TryEquipItem(ItemInstance, SlotIndex);

    if (EquipResult == EZfItemMechanicResult::Success)
    {
        // Remove do inventário apenas se o equip foi bem sucedido
        Internal_ClearSlot(SlotIndex);
        OnItemRemoved.Broadcast(ItemInstance, SlotIndex);
        InventoryList.MarkArrayDirty();

        UE_LOG(LogZfInventory, Log,
            TEXT("UZfInventoryComponent::TryEquipItem — " "Item '%s' equipado com sucesso."), *ItemInstance->GetItemName().ToString());
    }

    return EquipResult;
}

EZfItemMechanicResult UZfInventoryComponent::ReceiveUnequippedItem(UZfItemInstance* ItemInstance, int32 PreferredSlotIndex)
{
    if (!Internal_CheckIsServer(TEXT("ReceiveUnequippedItem")))
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
    return TryAddItemToInventory(ItemInstance, OutSlot);
}

// ============================================================
// ORGANIZAÇÃO
// ============================================================

void UZfInventoryComponent::SortInventoryAlphabetically()
{
    if (!Internal_CheckIsServer(TEXT("SortInventoryAlphabetically")))
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

void UZfInventoryComponent::SortInventoryByItemType()
{
    if (!Internal_CheckIsServer(TEXT("SortInventoryByItemType")))
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

void UZfInventoryComponent::SortInventoryByRarity()
{
    if (!Internal_CheckIsServer(TEXT("SortInventoryByRarity")))
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

void UZfInventoryComponent::CompactInventory()
{
    if (!Internal_CheckIsServer(TEXT("CompactInventory")))
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
// CONSULTA
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

bool UZfInventoryComponent::ContainsItem(UZfItemInstance* ItemInstance) const
{
    return GetSlotIndexOfItem(ItemInstance) != INDEX_NONE;
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

int32 UZfInventoryComponent::GetAvailableSlotCount() const
{
    int32 Count = 0;
    for (const FZfInventorySlot& Slot : InventoryList.Slots)
    {
        if (!Slot.ItemInstance)
        {
            Count++;
        }
    }
    return Count;
}

int32 UZfInventoryComponent::GetTotalSlotCount() const
{
    return CurrentSlotCount;
}

bool UZfInventoryComponent::IsInventoryFull() const
{
    return GetFirstEmptySlot() == INDEX_NONE;
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

bool UZfInventoryComponent::IsValidSlotIndex(int32 SlotIndex) const
{
    // Válido se estiver dentro do range do inventário
    return SlotIndex >= 0 && SlotIndex < CurrentSlotCount;
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

TArray<FZfInventorySlot> UZfInventoryComponent::GetAllSlots() const
{
    return InventoryList.Slots;
}

// ============================================================
// EXPANSÃO DE SLOTS
// ============================================================

int32 UZfInventoryComponent::AddExtraSlots(int32 ExtraSlots)
{
    if (!Internal_CheckIsServer(TEXT("AddExtraSlots")))
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
    if (!Internal_CheckIsServer(TEXT("RemoveExtraSlots")))
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
                InventoryList.Slots[FreeSlot].ItemInstance = ItemToMove;
                Internal_ClearSlot(i);
                OnItemMoved.Broadcast(ItemToMove, i, FreeSlot);
            }
            else
            {
                UE_LOG(LogZfInventory, Warning, TEXT("UZfInventoryComponent::RemoveExtraSlots — " "Não há slots livres para mover item do slot %d."), i);
                return EZfItemMechanicResult::Failed_InventoryFull;
            }
        }
    }

    // Remove os slots do final do array
    InventoryList.Slots.SetNum(FirstSlotToRemove);
    CurrentSlotCount = FirstSlotToRemove;

    OnInventorySizeChanged.Broadcast(CurrentSlotCount);
    InventoryList.MarkArrayDirty();

    UE_LOG(LogZfInventory, Log, TEXT("UZfInventoryComponent::RemoveExtraSlots — " "%d slots removidos. Total: %d"), ActualSlotsToRemove, CurrentSlotCount);

    return EZfItemMechanicResult::Success;
}

// ============================================================
// RPCs
// ============================================================

bool UZfInventoryComponent::ServerRequestMoveItem_Validate(int32 FromSlotIndex, int32 ToSlotIndex)
{
    // Valida que os índices são razoáveis antes de processar
    return IsValidSlotIndex(FromSlotIndex) && IsValidSlotIndex(ToSlotIndex);
}

void UZfInventoryComponent::ServerRequestMoveItem_Implementation(int32 FromSlotIndex, int32 ToSlotIndex)
{
    MoveItemBetweenSlots(FromSlotIndex, ToSlotIndex);
}

bool UZfInventoryComponent::ServerRequestEquipItem_Validate(int32 SlotIndex)
{
    return IsValidSlotIndex(SlotIndex);
}

void UZfInventoryComponent::ServerRequestEquipItem_Implementation(int32 SlotIndex)
{
    TryEquipItemFromSlot(SlotIndex);
}

bool UZfInventoryComponent::ServerRequestDropItem_Validate(int32 SlotIndex)
{
    return IsValidSlotIndex(SlotIndex);
}

void UZfInventoryComponent::ServerRequestDropItem_Implementation(int32 SlotIndex)
{
    UZfItemInstance* ItemInstance = nullptr;
    const EZfItemMechanicResult Result = RemoveItemAtSlot(SlotIndex, ItemInstance);

    if (Result == EZfItemMechanicResult::Success && ItemInstance)
    {
        // Spawna o ItemPickup no mundo
        if (GetOwner() && ItemInstance->GetItemDefinition())
        {
            const FVector DropLocation = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * 100.0f;

            // O spawn do pickup será implementado no ZfItemPickup (Parte 8)
            UE_LOG(LogZfInventory, Log, TEXT("UZfInventoryComponent::ServerRequestDropItem — "  "Item '%s' dropado em %s."),
                *ItemInstance->GetItemName().ToString(), *DropLocation.ToString());
        }
    }
}

bool UZfInventoryComponent::ServerRequestSortInventory_Validate(int32 SortTypeIndex)
{
    // Valida que o índice de sort é válido (0=Alpha, 1=Type, 2=Rarity)
    return SortTypeIndex >= 0 && SortTypeIndex <= 2;
}

void UZfInventoryComponent::ServerRequestSortInventory_Implementation(int32 SortTypeIndex)
{
    switch (SortTypeIndex)
    {
        case 0: SortInventoryAlphabetically(); break;
        case 1: SortInventoryByItemType();     break;
        case 2: SortInventoryByRarity();       break;
        default: break;
    }
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

/*/void UZfInventoryComponent::Internal_InitializeSlots()
{
    InventoryList.Slots.Empty();
    InventoryList.OwnerComponent = this;
    
    

    for (int32 i = 0; i < DefaultSlotCount; i++)
    {
        Internal_AddEmptySlot(i);
    }

    CurrentSlotCount = DefaultSlotCount;
    
    InventoryList.MarkArrayDirty();

    UE_LOG(LogZfInventory, Log,
        TEXT("UZfInventoryComponent::Internal_InitializeSlots — "
             "%d slots inicializados."), DefaultSlotCount);
}
*/
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

bool UZfInventoryComponent::Internal_TryStackWithExistingItems(UZfItemInstance* ItemInstance)
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

        InventoryList.MarkArrayDirty();

        // Se não tem overflow, o item foi completamente absorvido
        if (Overflow <= 0)
        {
            return true;
        }
    }

    // Ainda tem quantidade restante — não foi completamente absorvido
    return false;
}

void UZfInventoryComponent::Internal_ClearSlot(int32 SlotIndex)
{
    // Remove a entrada do array completamente
    // em vez de setar ItemInstance = nullptr
    InventoryList.Slots.RemoveAll([SlotIndex](const FZfInventorySlot& Slot){return Slot.SlotIndex == SlotIndex;});
}

bool UZfInventoryComponent::Internal_CheckIsServer(const FString& FunctionName) const
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

bool UZfInventoryComponent::Internal_CheckEquipmentComponent(const FString& FunctionName) const
{
    if (!EquipmentComponent)
    {
        UE_LOG(LogZfInventory, Error,
            TEXT("UZfInventoryComponent::%s — " "EquipmentComponent não está disponível."), *FunctionName);
        return false;
    }
    return true;
}

// ============================================================
// DEBUG
// ============================================================

void UZfInventoryComponent::DebugLogInventory() const
{
    UE_LOG(LogZfInventory, Log,
        TEXT("=== INVENTORY DEBUG [%s] ==="), GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"));

    UE_LOG(LogZfInventory, Log,
        TEXT("Slots: %d/%d | Disponíveis: %d"), GetTotalSlotCount(), MaxAbsoluteSlotCount, GetAvailableSlotCount());

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

void UZfInventoryComponent::DrawDebugInventory() const
{
    if (!GetOwner())
    {
        return;
    }

    const FVector BaseLocation = GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, 100.0f);

    const FString InventorySummary = FString::Printf(
        TEXT("[Inventory] %s | Slots: %d/%d"), *GetOwner()->GetName(), GetTotalSlotCount() - GetAvailableSlotCount(), GetTotalSlotCount());

    DrawDebugString(GetWorld(), BaseLocation, InventorySummary, nullptr, FColor::Cyan, 0.0f, false, 1.2f);
}