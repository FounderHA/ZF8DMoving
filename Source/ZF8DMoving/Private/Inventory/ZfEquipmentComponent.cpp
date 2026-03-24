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
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

// ============================================================
// Constructor
// ============================================================

UZfEquipmentComponent::UZfEquipmentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    // Replicado para todos — outros jogadores precisam ver
    // os itens equipados para visuais e animações
    SetIsReplicatedByDefault(true);
}

// ============================================================
// REPLICAÇÃO
// ============================================================

void UZfEquipmentComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // EquipmentList replicado para todos — visuais de outros jogadores
    DOREPLIFETIME(UZfEquipmentComponent, EquipmentList);

    // Estado do OffHand bloqueado replicado para todos
    DOREPLIFETIME(UZfEquipmentComponent, bOffHandSlotBlocked);
}

// ============================================================
// CICLO DE VIDA
// ============================================================

void UZfEquipmentComponent::BeginPlay()
{
    Super::BeginPlay();

    // Busca componentes no ator dono
    Internal_FindInventoryComponent();

    // Inicializa slots apenas no servidor
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        Internal_InitializeEquipmentSlots();
    }
}

// ============================================================
// EQUIPAR
// ============================================================

EZfItemMechanicResult UZfEquipmentComponent::TryEquipItem(
    UZfItemInstance* ItemInstance,
    int32 InventorySlotIndex)
{
    if (!Internal_CheckIsServer(TEXT("TryEquipItem")))
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    if (!ItemInstance)
    {
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    // Valida se pode ser equipado e obtém o motivo se não puder
    EZfItemMechanicResult ValidationResult;
    if (!CanEquipItem(ItemInstance, ValidationResult))
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfEquipmentComponent::TryEquipItem — "
                 "Item '%s' não pode ser equipado: %s"),
            *ItemInstance->GetItemName().ToString(),
            *UEnum::GetValueAsString(ValidationResult));
        return ValidationResult;
    }

    // Obtém o fragment Equippable para saber o slot alvo
    const UZfFragment_Equippable* EquippableFragment =
        ItemInstance->GetFragment<UZfFragment_Equippable>();

    // Determina o slot alvo
    EZfEquipmentSlot TargetSlotType = EquippableFragment->EquipmentSlot;
    int32 TargetSlotIndex = 0;

    // Busca o primeiro slot livre do tipo correto se bAutoFindFreeSlot
    if (EquippableFragment->bAutoFindFreeSlot)
    {
        bool bFoundFreeSlot = false;
        for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
        {
            if (Entry.SlotType == TargetSlotType && !Entry.ItemInstance)
            {
                TargetSlotIndex = Entry.SlotIndex;
                bFoundFreeSlot = true;
                break;
            }
        }

        // Se não há slot livre, usa o primeiro slot do tipo
        // (vai trocar o item existente)
        if (!bFoundFreeSlot)
        {
            for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
            {
                if (Entry.SlotType == TargetSlotType)
                {
                    TargetSlotIndex = Entry.SlotIndex;
                    break;
                }
            }
        }
    }
    else
    {
        TargetSlotIndex = EquippableFragment->PreferredSlotIndex;
    }

    return TryEquipItemToSpecificSlot(
        ItemInstance,
        TargetSlotType,
        TargetSlotIndex,
        InventorySlotIndex);
}

EZfItemMechanicResult UZfEquipmentComponent::TryEquipItemToSpecificSlot(
    UZfItemInstance* ItemInstance,
    EZfEquipmentSlot SlotType,
    int32 SlotIndex,
    int32 InventorySlotIndex)
{
    if (!Internal_CheckIsServer(TEXT("TryEquipItemToSpecificSlot")))
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    if (!ItemInstance)
    {
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    // Verifica se o slot está bloqueado por arma de duas mãos
    if (IsEquipmentSlotBlocked(SlotType, SlotIndex))
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfEquipmentComponent::TryEquipItemToSpecificSlot — "
                 "Slot bloqueado por arma de duas mãos."));
        return EZfItemMechanicResult::Failed_SlotBlocked;
    }

    // Busca a entrada do slot no EquipmentList
    FZfEquipmentSlotEntry* SlotEntry =
        Internal_FindSlotEntry(SlotType, SlotIndex);

    if (!SlotEntry)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfEquipmentComponent::TryEquipItemToSpecificSlot — "
                 "Slot %s[%d] não existe neste EquipmentComponent."),
            *UEnum::GetValueAsString(SlotType), SlotIndex);
        return EZfItemMechanicResult::Failed_InvalidSlot;
    }

    // Se o slot já tem item, devolve ao inventário primeiro
    if (SlotEntry->ItemInstance)
    {
        UZfItemInstance* ExistingItem = SlotEntry->ItemInstance;

        UE_LOG(LogZfInventory, Log,
            TEXT("UZfEquipmentComponent::TryEquipItemToSpecificSlot — "
                 "Slot ocupado por '%s'. Devolvendo ao inventário."),
            *ExistingItem->GetItemName().ToString());

        // Remove efeitos do item existente
        Internal_RemoveItemGameplayEffects(ExistingItem);

        // Atualiza set bonuses antes de remover
        Internal_UpdateSetBonuses(ExistingItem, false);

        // Libera o slot
        SlotEntry->ItemInstance = nullptr;

        // Se era two-handed, libera o OffHand
        const UZfFragment_Equippable* ExistingEquippable =
            ExistingItem->GetFragment<UZfFragment_Equippable>();
        if (ExistingEquippable && ExistingEquippable->IsTwoHanded())
        {
            Internal_UnblockOffHandSlot();
        }

        // Notifica os fragments do item removido
        ExistingItem->NotifyFragments_ItemUnequipped(
            this, GetOwner());

        // Dispara delegate
        OnItemUnequipped.Broadcast(ExistingItem, SlotType);

        // Devolve ao inventário
        if (Internal_CheckInventoryComponent(
                TEXT("TryEquipItemToSpecificSlot")))
        {
            InventoryComponent->ReceiveUnequippedItem(
                ExistingItem, InventorySlotIndex);
        }
    }

    // Equipa o novo item no slot
    SlotEntry->ItemInstance = ItemInstance;

    // Verifica se é two-handed e bloqueia OffHand se necessário
    const UZfFragment_Equippable* NewEquippable =
        ItemInstance->GetFragment<UZfFragment_Equippable>();

    if (NewEquippable && NewEquippable->IsTwoHanded())
    {
        // Se há item no OffHand, desequipa primeiro
        if (IsEquipmentSlotOccupied(EZfEquipmentSlot::OffHand))
        {
            UnequipItemAtSlot(EZfEquipmentSlot::OffHand);
        }
        Internal_BlockOffHandSlot();
    }

    // Aplica GameplayEffects dos modifiers via GAS
    Internal_ApplyItemGameplayEffects(ItemInstance);

    // Atualiza bônus de Combo Sets
    Internal_UpdateSetBonuses(ItemInstance, true);

    // Verifica se é mochila e expande o inventário
    const UZfFragment_InventoryExpansion* ExpansionFragment =
        ItemInstance->GetFragment<UZfFragment_InventoryExpansion>();
    if (ExpansionFragment)
    {
        OnBackpackEquipped(ExpansionFragment->ExtraSlotCount);
    }

    // Notifica os fragments do item equipado
    ItemInstance->NotifyFragments_ItemEquipped(this, GetOwner());

    // Dispara delegate
    OnItemEquipped.Broadcast(ItemInstance, SlotType);

    // Marca o array como dirty para replicação
    EquipmentList.MarkArrayDirty();

    UE_LOG(LogZfInventory, Log,
        TEXT("UZfEquipmentComponent::TryEquipItemToSpecificSlot — "
             "Item '%s' equipado no slot %s[%d]."),
        *ItemInstance->GetItemName().ToString(),
        *UEnum::GetValueAsString(SlotType),
        SlotIndex);

    return EZfItemMechanicResult::Success;
}

// ============================================================
// DESEQUIPAR
// ============================================================

EZfItemMechanicResult UZfEquipmentComponent::UnequipItemAtSlot(EZfEquipmentSlot SlotType, int32 SlotIndex)
{
    if (!Internal_CheckIsServer(TEXT("UnequipItemAtSlot")))
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    FZfEquipmentSlotEntry* SlotEntry = Internal_FindSlotEntry(SlotType, SlotIndex);

    if (!SlotEntry || !SlotEntry->ItemInstance)
    {
        UE_LOG(LogZfInventory, Warning, TEXT("UZfEquipmentComponent::UnequipItemAtSlot — " "Slot %s[%d] vazio ou inválido."),
            *UEnum::GetValueAsString(SlotType), SlotIndex);
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    UZfItemInstance* ItemInstance = SlotEntry->ItemInstance;

    // Remove GameplayEffects
    Internal_RemoveItemGameplayEffects(ItemInstance);

    // Atualiza set bonuses
    Internal_UpdateSetBonuses(ItemInstance, false);

    // Libera o slot
    SlotEntry->ItemInstance = nullptr;

    // Se era two-handed, libera OffHand
    const UZfFragment_Equippable* EquippableFragment = ItemInstance->GetFragment<UZfFragment_Equippable>();
    if (EquippableFragment && EquippableFragment->IsTwoHanded())
    {
        Internal_UnblockOffHandSlot();
    }

    // Se era mochila, remove os slots extras
    const UZfFragment_InventoryExpansion* ExpansionFragment = ItemInstance->GetFragment<UZfFragment_InventoryExpansion>();
    if (ExpansionFragment)
    {
        OnBackpackUnequipped(ExpansionFragment->ExtraSlotCount);
    }

    // Notifica os fragments
    ItemInstance->NotifyFragments_ItemUnequipped(this, GetOwner());

    // Dispara delegate
    OnItemUnequipped.Broadcast(ItemInstance, SlotType);

    // Devolve ao inventário
    if (Internal_CheckInventoryComponent(TEXT("UnequipItemAtSlot")))
    {
        InventoryComponent->ReceiveUnequippedItem(ItemInstance, SlotIndex);
    }

    // Marca dirty para replicação
    EquipmentList.MarkArrayDirty();

    UE_LOG(LogZfInventory, Log, TEXT("UZfEquipmentComponent::UnequipItemAtSlot — " "Item '%s' desequipado do slot %s[%d]."),
        *ItemInstance->GetItemName().ToString(), *UEnum::GetValueAsString(SlotType),SlotIndex);

    return EZfItemMechanicResult::Success;
}

EZfItemMechanicResult UZfEquipmentComponent::UnequipItem(
    UZfItemInstance* ItemInstance)
{
    if (!Internal_CheckIsServer(TEXT("UnequipItem")))
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    if (!ItemInstance)
    {
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    // Busca o slot do item
    for (FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.ItemInstance == ItemInstance)
        {
            return UnequipItemAtSlot(Entry.SlotType, Entry.SlotIndex);
        }
    }

    UE_LOG(LogZfInventory, Warning,
        TEXT("UZfEquipmentComponent::UnequipItem — "
             "Item '%s' não está equipado."),
        *ItemInstance->GetItemName().ToString());

    return EZfItemMechanicResult::Failed_ItemNotFound;
}

void UZfEquipmentComponent::UnequipAllItems()
{
    if (!Internal_CheckIsServer(TEXT("UnequipAllItems")))
    {
        return;
    }

    // Coleta todos os itens equipados antes de iterar
    // para evitar modificar o array durante o loop
    TArray<TPair<EZfEquipmentSlot, int32>> SlotsToUnequip;
    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.ItemInstance)
        {
            SlotsToUnequip.Add({ Entry.SlotType, Entry.SlotIndex });
        }
    }

    for (const TPair<EZfEquipmentSlot, int32>& SlotPair : SlotsToUnequip)
    {
        UnequipItemAtSlot(SlotPair.Key, SlotPair.Value);
    }

    UE_LOG(LogZfInventory, Log,
        TEXT("UZfEquipmentComponent::UnequipAllItems — "
             "%d itens desequipados."), SlotsToUnequip.Num());
}

// ============================================================
// TROCA RÁPIDA
// ============================================================

EZfItemMechanicResult UZfEquipmentComponent::QuickSwapItem(
    int32 InventorySlotIndex,
    EZfEquipmentSlot EquipSlotType,
    int32 EquipSlotIndex)
{
    if (!Internal_CheckIsServer(TEXT("QuickSwapItem")))
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    if (!Internal_CheckInventoryComponent(TEXT("QuickSwapItem")))
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    // Obtém o item do inventário
    UZfItemInstance* ItemFromInventory =
        InventoryComponent->GetItemAtSlot(InventorySlotIndex);

    if (!ItemFromInventory)
    {
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    // Tenta equipar — o TryEquipItemToSpecificSlot já lida
    // com a devolução do item existente ao inventário
    return TryEquipItemToSpecificSlot(
        ItemFromInventory,
        EquipSlotType,
        EquipSlotIndex,
        InventorySlotIndex);
}

// ============================================================
// DURABILIDADE
// ============================================================

void UZfEquipmentComponent::NotifyEquippedItemBroken(
    UZfItemInstance* ItemInstance)
{
    if (!ItemInstance)
    {
        return;
    }

    // Verifica se o item realmente está equipado
    if (!IsItemEquipped(ItemInstance))
    {
        return;
    }

    // Remove os GameplayEffects sem desequipar
    Internal_RemoveItemGameplayEffects(ItemInstance);

    // Remove bônus de set pois o item está inutilizável
    Internal_UpdateSetBonuses(ItemInstance, false);

    // Dispara delegate para a UI mostrar aviso ao player
    OnEquippedItemBroken.Broadcast(ItemInstance);

    UE_LOG(LogZfInventory, Warning,
        TEXT("UZfEquipmentComponent::NotifyEquippedItemBroken — "
             "Item equipado '%s' quebrou. Bônus desativados."),
        *ItemInstance->GetItemName().ToString());
}

void UZfEquipmentComponent::NotifyEquippedItemRepaired(
    UZfItemInstance* ItemInstance)
{
    if (!ItemInstance)
    {
        return;
    }

    if (!IsItemEquipped(ItemInstance))
    {
        return;
    }

    // Reaplica os GameplayEffects
    Internal_ApplyItemGameplayEffects(ItemInstance);

    // Reativa bônus de set
    Internal_UpdateSetBonuses(ItemInstance, true);

    // Dispara delegate
    OnEquippedItemRepaired.Broadcast(ItemInstance);

    UE_LOG(LogZfInventory, Log,
        TEXT("UZfEquipmentComponent::NotifyEquippedItemRepaired — "
             "Item equipado '%s' reparado. Bônus reativados."),
        *ItemInstance->GetItemName().ToString());
}

// ============================================================
// CONSULTA
// ============================================================

UZfItemInstance* UZfEquipmentComponent::GetItemAtEquipmentSlot(
    EZfEquipmentSlot SlotType,
    int32 SlotIndex) const
{
    const FZfEquipmentSlotEntry* SlotEntry =
        Internal_FindSlotEntryConst(SlotType, SlotIndex);

    return SlotEntry ? SlotEntry->ItemInstance.Get() : nullptr;
}

bool UZfEquipmentComponent::IsItemEquipped(
    UZfItemInstance* ItemInstance) const
{
    if (!ItemInstance)
    {
        return false;
    }

    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.ItemInstance == ItemInstance)
        {
            return true;
        }
    }
    return false;
}

bool UZfEquipmentComponent::IsEquipmentSlotOccupied(
    EZfEquipmentSlot SlotType,
    int32 SlotIndex) const
{
    const FZfEquipmentSlotEntry* SlotEntry =
        Internal_FindSlotEntryConst(SlotType, SlotIndex);

    return SlotEntry && SlotEntry->ItemInstance != nullptr;
}

bool UZfEquipmentComponent::IsEquipmentSlotBlocked(
    EZfEquipmentSlot SlotType,
    int32 SlotIndex) const
{
    // Apenas o OffHand pode ser bloqueado por arma de duas mãos
    if (SlotType == EZfEquipmentSlot::OffHand)
    {
        return bOffHandSlotBlocked;
    }
    return false;
}

TArray<UZfItemInstance*> UZfEquipmentComponent::GetAllEquippedItems() const
{
    TArray<UZfItemInstance*> Result;
    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.ItemInstance)
        {
            Result.Add(Entry.ItemInstance.Get());
        }
    }
    return Result;
}

TArray<UZfItemInstance*> UZfEquipmentComponent::GetEquippedItemsByTag(
    const FGameplayTag& Tag) const
{
    TArray<UZfItemInstance*> Result;
    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.ItemInstance && Entry.ItemInstance->HasItemTag(Tag))
        {
            Result.Add(Entry.ItemInstance.Get());
        }
    }
    return Result;
}

EZfEquipmentSlot UZfEquipmentComponent::GetEquipmentSlotOfItem(
    UZfItemInstance* ItemInstance) const
{
    if (!ItemInstance)
    {
        return EZfEquipmentSlot::None;
    }

    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.ItemInstance == ItemInstance)
        {
            return Entry.SlotType;
        }
    }
    return EZfEquipmentSlot::None;
}

bool UZfEquipmentComponent::CanEquipItem(
    UZfItemInstance* ItemInstance,
    EZfItemMechanicResult& OutReason) const
{
    if (!ItemInstance)
    {
        OutReason = EZfItemMechanicResult::Failed_ItemNotFound;
        return false;
    }

    // Verifica se tem o fragment Equippable
    const UZfFragment_Equippable* EquippableFragment =
        ItemInstance->GetFragment<UZfFragment_Equippable>();

    if (!EquippableFragment)
    {
        OutReason = EZfItemMechanicResult::Failed_CannotEquip;
        return false;
    }

    // Verifica se o item está quebrado
    if (ItemInstance->bIsBroken)
    {
        OutReason = EZfItemMechanicResult::Failed_ItemBroken;
        return false;
    }

    // Verifica se o slot alvo está bloqueado
    if (IsEquipmentSlotBlocked(EquippableFragment->EquipmentSlot))
    {
        OutReason = EZfItemMechanicResult::Failed_SlotBlocked;
        return false;
    }

    // Verifica tags de requisito do ator
    if (!EquippableFragment->RequiredActorTags.IsEmpty())
    {
        // Obtém as tags do ator via AbilitySystem
        UAbilitySystemComponent* ASC =
            Internal_GetAbilitySystemComponent();

        if (ASC)
        {
            FGameplayTagContainer ActorTags;
            ASC->GetOwnedGameplayTags(ActorTags);

            if (!ActorTags.HasAll(EquippableFragment->RequiredActorTags))
            {
                OutReason = EZfItemMechanicResult::Failed_IncompatibleItem;
                return false;
            }
        }
    }

    OutReason = EZfItemMechanicResult::Success;
    return true;
}

// ============================================================
// COMBO SETS
// ============================================================

int32 UZfEquipmentComponent::GetEquippedPieceCountForSet(
    const FGameplayTag& SetIdentifierTag) const
{
    const FZfActiveSetBonus* ActiveBonus =
        ActiveSetBonuses.Find(SetIdentifierTag);

    return ActiveBonus ? ActiveBonus->ActivePieceCount : 0;
}

TArray<FZfActiveSetBonus> UZfEquipmentComponent::GetAllActiveSetBonuses() const
{
    TArray<FZfActiveSetBonus> Result;
    for (const TPair<FGameplayTag, FZfActiveSetBonus>& Pair : ActiveSetBonuses)
    {
        Result.Add(Pair.Value);
    }
    return Result;
}

// ============================================================
// EXPANSÃO DE SLOTS
// ============================================================

void UZfEquipmentComponent::OnBackpackEquipped(int32 ExtraSlots)
{
    if (!Internal_CheckInventoryComponent(TEXT("OnBackpackEquipped")))
    {
        return;
    }

    const int32 SlotsAdded = InventoryComponent->AddExtraSlots(ExtraSlots);

    UE_LOG(LogZfInventory, Log, TEXT("UZfEquipmentComponent::OnBackpackEquipped — " "%d slots adicionados ao inventário."), SlotsAdded);
}

void UZfEquipmentComponent::OnBackpackUnequipped(int32 ExtraSlots)
{
    if (!Internal_CheckInventoryComponent(TEXT("OnBackpackUnequipped")))
    {
        return;
    }

    InventoryComponent->RemoveExtraSlots(ExtraSlots);

    UE_LOG(LogZfInventory, Log, TEXT("UZfEquipmentComponent::OnBackpackUnequipped — " "%d slots removidos do inventário."), ExtraSlots);
}

// ============================================================
// RPCs
// ============================================================

bool UZfEquipmentComponent::ServerRequestUnequipItem_Validate(
    EZfEquipmentSlot SlotType, int32 SlotIndex)
{
    return SlotType != EZfEquipmentSlot::None && SlotIndex >= 0;
}

void UZfEquipmentComponent::ServerRequestUnequipItem_Implementation(
    EZfEquipmentSlot SlotType, int32 SlotIndex)
{
    UnequipItemAtSlot(SlotType, SlotIndex);
}

bool UZfEquipmentComponent::ServerRequestQuickSwap_Validate(
    int32 InventorySlotIndex,
    EZfEquipmentSlot EquipSlotType,
    int32 EquipSlotIndex)
{
    return InventorySlotIndex >= 0 &&
           EquipSlotType != EZfEquipmentSlot::None &&
           EquipSlotIndex >= 0;
}

void UZfEquipmentComponent::ServerRequestQuickSwap_Implementation(
    int32 InventorySlotIndex,
    EZfEquipmentSlot EquipSlotType,
    int32 EquipSlotIndex)
{
    QuickSwapItem(InventorySlotIndex, EquipSlotType, EquipSlotIndex);
}

// ============================================================
// FUNÇÕES INTERNAS
// ============================================================

void UZfEquipmentComponent::Internal_InitializeEquipmentSlots()
{
    EquipmentList.EquippedItems.Empty();
    EquipmentList.OwnerComponent = this;

    // Copia os slots padrão configurados no editor
    for (FZfEquipmentSlotEntry& DefaultSlot : DefaultEquipmentSlots)
    {
        FZfEquipmentSlotEntry NewEntry;
        NewEntry.SlotType = DefaultSlot.SlotType;
        NewEntry.SlotIndex = DefaultSlot.SlotIndex;
        NewEntry.ItemInstance = nullptr;
        NewEntry.ReplicationKey = Internal_GenerateReplicationKey();
        EquipmentList.EquippedItems.Add(NewEntry);
    }

    UE_LOG(LogZfInventory, Log,
        TEXT("UZfEquipmentComponent::Internal_InitializeEquipmentSlots — "
             "%d slots inicializados."),
        EquipmentList.EquippedItems.Num());
}

void UZfEquipmentComponent::Internal_FindInventoryComponent()
{
    if (GetOwner())
    {
        InventoryComponent =
            GetOwner()->FindComponentByClass<UZfInventoryComponent>();

        if (!InventoryComponent)
        {
            UE_LOG(LogZfInventory, Warning,
                TEXT("UZfEquipmentComponent::Internal_FindInventoryComponent — "
                     "UZfInventoryComponent não encontrado no ator '%s'."),
                *GetOwner()->GetName());
        }
    }
}

UAbilitySystemComponent*
UZfEquipmentComponent::Internal_GetAbilitySystemComponent() const
{
    if (!GetOwner())
    {
        return nullptr;
    }

    // Tenta obter o ASC via interface IAbilitySystemInterface
    if (IAbilitySystemInterface* ASCInterface =
        Cast<IAbilitySystemInterface>(GetOwner()))
    {
        return ASCInterface->GetAbilitySystemComponent();
    }

    // Fallback: busca diretamente no ator
    return GetOwner()->FindComponentByClass<UAbilitySystemComponent>();
}

void UZfEquipmentComponent::Internal_ApplyItemGameplayEffects(UZfItemInstance* ItemInstance)
{
    if (!ItemInstance)
    {
        return;
    }

    UAbilitySystemComponent* ASC = Internal_GetAbilitySystemComponent();
    if (!ASC)
    {
        UE_LOG(LogZfInventory, Warning, TEXT("UZfEquipmentComponent::Internal_ApplyItemGameplayEffects — " "AbilitySystemComponent não encontrado. " "GameplayEffects não serão aplicados."));
        return;
    }

    // Aplica os GameplayEffects de cada modifier do item
    for (FZfAppliedModifier& Modifier : ItemInstance->AppliedModifiers)
    {
        // O ModifierSubsystem (parte futura) fornecerá o GE base
        // e populará os modificadores em runtime via DataTable.
        // Por ora, o handle é armazenado para remoção posterior.
        UE_LOG(LogZfInventory, Verbose, TEXT("UZfEquipmentComponent::Internal_ApplyItemGameplayEffects — " "Aplicando modifier '%s' ao ASC."),
        *Modifier.ModifierRowName.ToString());
    }
}

void UZfEquipmentComponent::Internal_RemoveItemGameplayEffects(
    UZfItemInstance* ItemInstance)
{
    if (!ItemInstance)
    {
        return;
    }

    UAbilitySystemComponent* ASC = Internal_GetAbilitySystemComponent();
    if (!ASC)
    {
        return;
    }

    // Remove todos os GameplayEffects ativos dos modifiers
    for (FZfAppliedModifier& Modifier : ItemInstance->AppliedModifiers)
    {
        if (Modifier.ActiveEffectHandle.IsValid())
        {
            ASC->RemoveActiveGameplayEffect(Modifier.ActiveEffectHandle);
            Modifier.ActiveEffectHandle = FActiveGameplayEffectHandle();

            UE_LOG(LogZfInventory, Verbose,
                TEXT("UZfEquipmentComponent::Internal_RemoveItemGameplayEffects — "
                     "Modifier '%s' removido do ASC."),
                *Modifier.ModifierRowName.ToString());
        }
    }
}

void UZfEquipmentComponent::Internal_UpdateSetBonuses(
    UZfItemInstance* ItemInstance,
    bool bWasEquipped)
{
    if (!ItemInstance)
    {
        return;
    }

    // Verifica se o item tem o fragment de set
    const UZfFragment_SetPiece* SetFragment =
        ItemInstance->GetFragment<UZfFragment_SetPiece>();

    if (!SetFragment || !SetFragment->SetIdentifier.IsValid())
    {
        return;
    }

    const FGameplayTag SetTag = SetFragment->SetIdentifier;

    // Atualiza a contagem de peças ativas
    FZfActiveSetBonus& ActiveBonus = ActiveSetBonuses.FindOrAdd(SetTag);
    ActiveBonus.SetIdentifierTag = SetTag;

    if (bWasEquipped)
    {
        ActiveBonus.ActivePieceCount++;
    }
    else
    {
        ActiveBonus.ActivePieceCount =
            FMath::Max(0, ActiveBonus.ActivePieceCount - 1);
    }

    // Remove os bônus antigos e reaaplica os corretos
    Internal_RemoveAllSetBonuses(SetTag);
    Internal_ApplySetBonuses(SetTag, ActiveBonus.ActivePieceCount);

    // Dispara delegate para UI
    OnSetBonusChanged.Broadcast(SetTag, ActiveBonus.ActivePieceCount);

    UE_LOG(LogZfInventory, Log,
        TEXT("UZfEquipmentComponent::Internal_UpdateSetBonuses — "
             "Set '%s' | Peças ativas: %d"),
        *SetTag.ToString(), ActiveBonus.ActivePieceCount);
}

void UZfEquipmentComponent::Internal_ApplySetBonuses(
    const FGameplayTag& SetTag,
    int32 PieceCount)
{
    if (PieceCount <= 0)
    {
        return;
    }

    UAbilitySystemComponent* ASC = Internal_GetAbilitySystemComponent();
    if (!ASC)
    {
        return;
    }

    FZfActiveSetBonus* ActiveBonus = ActiveSetBonuses.Find(SetTag);
    if (!ActiveBonus)
    {
        return;
    }

    // Busca o primeiro item equipado deste set para obter os SetBonuses
    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (!Entry.ItemInstance)
        {
            continue;
        }

        const UZfFragment_SetPiece* SetFragment =
            Entry.ItemInstance->GetFragment<UZfFragment_SetPiece>();

        if (!SetFragment || SetFragment->SetIdentifier != SetTag)
        {
            continue;
        }

        // Aplica os bônus ativos para a contagem atual de peças
        const TArray<FZfSetBonusEntry> ActiveBonuses =
            SetFragment->GetActiveBonusesForPieceCount(PieceCount);

        for (const FZfSetBonusEntry& BonusEntry : ActiveBonuses)
        {
            // Carrega e aplica o GameplayEffect de bônus
            if (!BonusEntry.BonusGameplayEffect.IsNull())
            {
                TSubclassOf<UGameplayEffect> GEClass =
                    BonusEntry.BonusGameplayEffect.LoadSynchronous();

                if (GEClass)
                {
                    FGameplayEffectContextHandle ContextHandle =
                        ASC->MakeEffectContext();

                    FActiveGameplayEffectHandle Handle =
                        ASC->ApplyGameplayEffectToSelf(
                            GEClass->GetDefaultObject<UGameplayEffect>(),
                            1.0f,
                            ContextHandle);

                    // Armazena o handle para remoção posterior
                    ActiveBonus->ActiveBonusHandles.Add(
                        BonusEntry.RequiredPieceCount, Handle);

                    UE_LOG(LogZfInventory, Log,
                        TEXT("UZfEquipmentComponent::Internal_ApplySetBonuses — "
                             "Bônus de %d peças do set '%s' aplicado."),
                        BonusEntry.RequiredPieceCount,
                        *SetTag.ToString());
                }
            }
        }
        break;
    }
}

void UZfEquipmentComponent::Internal_RemoveAllSetBonuses(
    const FGameplayTag& SetTag)
{
    FZfActiveSetBonus* ActiveBonus = ActiveSetBonuses.Find(SetTag);
    if (!ActiveBonus)
    {
        return;
    }

    UAbilitySystemComponent* ASC = Internal_GetAbilitySystemComponent();
    if (!ASC)
    {
        return;
    }

    // Remove todos os GEs de bônus deste set
    for (TPair<int32, FActiveGameplayEffectHandle>& HandlePair :
         ActiveBonus->ActiveBonusHandles)
    {
        if (HandlePair.Value.IsValid())
        {
            ASC->RemoveActiveGameplayEffect(HandlePair.Value);
        }
    }

    ActiveBonus->ActiveBonusHandles.Empty();
}

void UZfEquipmentComponent::Internal_BlockOffHandSlot()
{
    bOffHandSlotBlocked = true;

    UE_LOG(LogZfInventory, Log,
        TEXT("UZfEquipmentComponent::Internal_BlockOffHandSlot — "
             "Slot OffHand bloqueado por arma de duas mãos."));
}

void UZfEquipmentComponent::Internal_UnblockOffHandSlot()
{
    bOffHandSlotBlocked = false;

    UE_LOG(LogZfInventory, Log,
        TEXT("UZfEquipmentComponent::Internal_UnblockOffHandSlot — "
             "Slot OffHand liberado."));
}

FZfEquipmentSlotEntry* UZfEquipmentComponent::Internal_FindSlotEntry(
    EZfEquipmentSlot SlotType,
    int32 SlotIndex)
{
    for (FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.SlotType == SlotType && Entry.SlotIndex == SlotIndex)
        {
            return &Entry;
        }
    }
    return nullptr;
}

const FZfEquipmentSlotEntry*
UZfEquipmentComponent::Internal_FindSlotEntryConst(
    EZfEquipmentSlot SlotType,
    int32 SlotIndex) const
{
    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.SlotType == SlotType && Entry.SlotIndex == SlotIndex)
        {
            return &Entry;
        }
    }
    return nullptr;
}

int32 UZfEquipmentComponent::Internal_GenerateReplicationKey() const
{
    // Gera uma chave única baseada no tamanho atual do array
    // + um offset para evitar colisões
    static int32 KeyCounter = 0;
    return ++KeyCounter;
}

bool UZfEquipmentComponent::Internal_CheckIsServer(
    const FString& FunctionName) const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogZfInventory, Error,
            TEXT("UZfEquipmentComponent::%s — GetWorld() retornou nulo."),
            *FunctionName);
        return false;
    }

    if (World->GetNetMode() == NM_Client)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfEquipmentComponent::%s — "
                 "Operação de servidor chamada no cliente!"),
            *FunctionName);
        return false;
    }

    return true;
}

bool UZfEquipmentComponent::Internal_CheckInventoryComponent(
    const FString& FunctionName) const
{
    if (!InventoryComponent)
    {
        UE_LOG(LogZfInventory, Error,
            TEXT("UZfEquipmentComponent::%s — "
                 "InventoryComponent não disponível."),
            *FunctionName);
        return false;
    }
    return true;
}

// ============================================================
// DEBUG
// ============================================================

void UZfEquipmentComponent::DebugLogEquipment() const
{
    UE_LOG(LogZfInventory, Log,
        TEXT("=== EQUIPMENT DEBUG [%s] ==="),
        GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"));

    UE_LOG(LogZfInventory, Log,
        TEXT("OffHandBlocked: %s"),
        bOffHandSlotBlocked ? TEXT("Yes") : TEXT("No"));

    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.ItemInstance)
        {
            UE_LOG(LogZfInventory, Log,
                TEXT("  [%s][%d] %s"),
                *UEnum::GetValueAsString(Entry.SlotType),
                Entry.SlotIndex,
                *Entry.ItemInstance->GetDebugString());
        }
        else
        {
            UE_LOG(LogZfInventory, Verbose,
                TEXT("  [%s][%d] --- vazio ---"),
                *UEnum::GetValueAsString(Entry.SlotType),
                Entry.SlotIndex);
        }
    }

    // Loga bônus de set ativos
    for (const TPair<FGameplayTag, FZfActiveSetBonus>& SetPair :
         ActiveSetBonuses)
    {
        if (SetPair.Value.ActivePieceCount > 0)
        {
            UE_LOG(LogZfInventory, Log,
                TEXT("  [SET] %s — %d peças ativas"),
                *SetPair.Key.ToString(),
                SetPair.Value.ActivePieceCount);
        }
    }
}

void UZfEquipmentComponent::DrawDebugEquipment() const
{
    if (!GetOwner())
    {
        return;
    }

    const FVector BaseLocation =
        GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, 200.0f);

    int32 EquippedCount = 0;
    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.ItemInstance)
        {
            EquippedCount++;
        }
    }

    const FString EquipSummary = FString::Printf(
        TEXT("[Equipment] %s | Equipped: %d | OffHandBlocked: %s"),
        *GetOwner()->GetName(),
        EquippedCount,
        bOffHandSlotBlocked ? TEXT("Yes") : TEXT("No"));

    DrawDebugString(
        GetWorld(),
        BaseLocation,
        EquipSummary,
        nullptr,
        FColor::Green,
        0.0f,
        false,
        1.2f);
}