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
#include "Player/ZfPlayerState.h"

// ============================================================
// Constructor
// ============================================================

UZfEquipmentComponent::UZfEquipmentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    // Replicado para todos — outros jogadores precisam ver
    // os itens equipados para visuais e animações
    SetIsReplicatedByDefault(true);
    bReplicateUsingRegisteredSubObjectList = true;
}

// ============================================================
// REPLICAÇÃO
// ============================================================

void UZfEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
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

    if (GetOwner())
    {
        InventoryComponent = Cast<AZfPlayerState>(GetOwner())->FindComponentByClass<UZfInventoryComponent>();
    }
}

// ============================================================
// DESEQUIPAR
// ============================================================



void UZfEquipmentComponent::UnequipAllItems()
{
    if (!InternalCheckIsServer(TEXT("UnequipAllItems")))
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
            SlotsToUnequip.Add({ Entry.SlotType, Entry.SlotPosition });
        }
    }

    for (const TPair<EZfEquipmentSlot, int32>& SlotPair : SlotsToUnequip)
    {
        //UnequipItemAtSlot(SlotPair.Key, SlotPair.Value);
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
    if (!InternalCheckIsServer(TEXT("QuickSwapItem")))
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
    return EZfItemMechanicResult::Success;
}

// ============================================================
// DURABILIDADE
// ============================================================

void UZfEquipmentComponent::NotifyEquippedItemBroken(UZfItemInstance* ItemInstance)
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

    UE_LOG(LogZfInventory, Warning,TEXT("UZfEquipmentComponent::NotifyEquippedItemBroken — "
    "Item equipado '%s' quebrou. Bônus desativados."),
    *ItemInstance->GetItemName().ToString());
}

void UZfEquipmentComponent::NotifyEquippedItemRepaired(UZfItemInstance* ItemInstance)
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

bool UZfEquipmentComponent::CanEquipItemOld(UZfItemInstance* ItemInstance, EZfItemMechanicResult& OutReason) const
{
    if (!ItemInstance)
    {
        OutReason = EZfItemMechanicResult::Failed_ItemNotFound;
        return false;
    }

    // Verifica se tem o fragment Equippable
    const UZfFragment_Equippable* EquippableFragment = ItemInstance->GetFragment<UZfFragment_Equippable>();

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
        UAbilitySystemComponent* ASC = Internal_GetAbilitySystemComponent();

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

    //const int32 SlotsAdded = InventoryComponent->TryAddExtraSlots(ExtraSlots);
    int32 SlotsAdded = 2;
    UE_LOG(LogZfInventory, Log, TEXT("UZfEquipmentComponent::OnBackpackEquipped — " "%d slots adicionados ao inventário."), SlotsAdded);
}

void UZfEquipmentComponent::OnBackpackUnequipped(int32 ExtraSlots)
{
    if (!Internal_CheckInventoryComponent(TEXT("OnBackpackUnequipped")))
    {
        return;
    }

    //InventoryComponent->TryRemoveExtraSlots(ExtraSlots);
    int32 SlotsAdded = 2;
    UE_LOG(LogZfInventory, Log, TEXT("UZfEquipmentComponent::OnBackpackUnequipped — " "%d slots removidos do inventário."), ExtraSlots);
}

// ============================================================
// RPCs
// ============================================================

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
        NewEntry.SlotPosition = DefaultSlot.SlotPosition;
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

void UZfEquipmentComponent::Internal_RemoveItemGameplayEffects(UZfItemInstance* ItemInstance)
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

void UZfEquipmentComponent::Internal_UpdateSetBonuses(UZfItemInstance* ItemInstance, bool bWasEquipped)
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

const FZfEquipmentSlotEntry* UZfEquipmentComponent::Internal_FindSlotEntryConst(EZfEquipmentSlot SlotType, int32 SlotIndex) const
{
    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.SlotType == SlotType && Entry.SlotPosition == SlotIndex)
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

bool UZfEquipmentComponent::Internal_CheckInventoryComponent(const FString& FunctionName) const
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











// ============================================================
// FAST Array
// ============================================================

void FZfEquipmentSlotEntry::PreReplicatedRemove(const FZfEquipmentList& InArraySerializer)
{
    if (InArraySerializer.OwnerComponent && ItemInstance)
    {
        InArraySerializer.OwnerComponent->OnItemUnequipped.Broadcast(ItemInstance, ItemInstance->GetFragment<UZfFragment_Equippable>()->EquipmentTags.First(), SlotPosition);
    }
}

void FZfEquipmentSlotEntry::PostReplicatedAdd(const FZfEquipmentList& InArraySerializer)
{
    if (InArraySerializer.OwnerComponent && ItemInstance)
    {
        InArraySerializer.OwnerComponent->OnItemEquipped.Broadcast(ItemInstance, ItemInstance->GetFragment<UZfFragment_Equippable>()->EquipmentTags.First(), SlotPosition);
    }
}

void FZfEquipmentSlotEntry::PostReplicatedChange(const FZfEquipmentList& InArraySerializer)
{
   
}

// Definição central da categoria de log do sistema de inventário.
// Declarada em ZfInventoryTypes.h com DECLARE_LOG_CATEGORY_EXTERN.
// Uso em qualquer arquivo: UE_LOG(LogZfInventory, Log, TEXT("..."));
DEFINE_LOG_CATEGORY(LogZfInventory);

// ============================================================
// FUNÇÕES SERVER - GERENCIAMENTO
// ============================================================

void UZfEquipmentComponent::ServerTryEquipItem_Implementation(UZfItemInstance* ItemInstance, int32 FromInventorySlot, int32 SlotPosition)
{
    if (InternalCheckIsServer("ServerTryEquipItem_Implementation"))
    {
        TryEquipItem(ItemInstance, FromInventorySlot, SlotPosition);
    }
}

void UZfEquipmentComponent::ServerTryUnequipItem_Implementation(FGameplayTag SlotTag, int32 TagetInventorySlot, int32 SlotPosition)
{
    if (InternalCheckIsServer(TEXT("ServerTryUnequipItem")))
    {
        TryUnequipItem(SlotTag, TagetInventorySlot, SlotPosition);
    }
}

// ============================================================
// FUNÇÕES PRINCIPAIS - GERENCIAMENTO
// ============================================================

EZfItemMechanicResult UZfEquipmentComponent::TryEquipItem(UZfItemInstance* ItemFromInventory, int32 FromInventorySlot, int32 SlotPosition)
{
    // Valida se o item pode ser equipado (não nulo, não quebrado, tem fragment, slot livre)
    if (CanEquipItem(ItemFromInventory, FromInventorySlot, SlotPosition) != EZfItemMechanicResult::Success)
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }
    
    if (!InventoryComponent)
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    // Obtém o fragment para saber a tag do slot alvo
    const UZfFragment_Equippable* EquippableFragment = ItemFromInventory->GetFragment<UZfFragment_Equippable>();

    // Tenta Equipar Mochila
    if (ItemFromInventory->GetFragment<UZfFragment_InventoryExpansion>())
    {
        if (TryEquipBackpack(EquippableFragment->EquipmentTags.First(), FromInventorySlot, SlotPosition) == EZfItemMechanicResult::Success)
        {
            return EZfItemMechanicResult::Success;
        }
        else
        {
            return EZfItemMechanicResult::Failed_InventoryFull;
        }
    }

    // Verifica se tenho item ja equipado
    if (UZfItemInstance* EquipedItem = GetItemAtSlotTag(EquippableFragment->EquipmentTags.First(), SlotPosition))
    {
        // Remove o item atual do equipamento
        // Equipa o item que veio do inventário
        InternalUnequipItem(EquipedItem, SlotPosition);
        InternalEquipItem(ItemFromInventory, SlotPosition);

        // Remove o item novo do inventário (ele agora está equipado)
        // Coloca o item antigo no slot que ficou livre no inventário
        InventoryComponent->TryRemoveItemFromInventory(FromInventorySlot);
        InventoryComponent->TryAddItemToSpecificSlot(EquipedItem, FromInventorySlot);

        EquipmentList.MarkArrayDirty();
        OnItemEquipped.Broadcast(ItemFromInventory,ItemFromInventory->GetFragment<UZfFragment_Equippable>()->EquipmentTags.First(), SlotPosition);

        return EZfItemMechanicResult::Success;
    }
    else
    {
        // Adiciona o item ao EquipmentList e registra para replicação
        InternalEquipItem(ItemFromInventory, SlotPosition);
        EquipmentList.MarkArrayDirty();
        OnItemEquipped.Broadcast(ItemFromInventory,EquippableFragment->EquipmentTags.First(), SlotPosition);

        // Remove o item do inventário — ele agora pertence ao equipamento
        InventoryComponent->TryRemoveItemFromInventory(FromInventorySlot);
    

        return EZfItemMechanicResult::Success;
    }
}

EZfItemMechanicResult UZfEquipmentComponent::TryUnequipItem(FGameplayTag SlotTag, int32 TagetInventorySlot, int32 SlotPosition)
{
    // Busca o slot de equipamento pela tag e posição
    FZfEquipmentSlotEntry* SlotEntry = InternalFindSlotEntry(SlotTag, SlotPosition);

    if (!SlotEntry || !SlotEntry->ItemInstance)
    {
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    // Guarda referência ao item equipado antes de remover
    UZfItemInstance* ItemInstanceInEquipment = SlotEntry->ItemInstance;

    if (!InventoryComponent)
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }
    
    // Tenta Equipar Mochila
    if (ItemInstanceInEquipment->GetFragment<UZfFragment_InventoryExpansion>())
    {
        if (TryUnequipBackpack(ItemInstanceInEquipment->GetFragment<UZfFragment_Equippable>()->EquipmentTags.First(), TagetInventorySlot, SlotPosition) == EZfItemMechanicResult::Success)
        {
            return EZfItemMechanicResult::Success;
        }
        else
        {
            return EZfItemMechanicResult::Failed_InventoryFull;
        }
    }

    
    // Verifica se o slot alvo no inventário já tem um item
    if (UZfItemInstance* ItemInstanceAtInventory = InventoryComponent->GetItemAtSlot(TagetInventorySlot))
    {
        // CASO 1: Slot do inventário ocupado E o item pode ser equipado → TROCA
        // Desequipa o atual, equipa o do inventário, e devolve o antigo no mesmo slot
        if (CanEquipItem(ItemInstanceAtInventory,TagetInventorySlot,SlotPosition) == EZfItemMechanicResult::Success)
        {
            // Remove o item atual do equipamento
            // Equipa o item que veio do inventário
            InternalUnequipItem(ItemInstanceInEquipment, SlotPosition);
            InternalEquipItem(ItemInstanceAtInventory, SlotPosition);

            // Remove o item novo do inventário (ele agora está equipado)
            // Coloca o item antigo no slot que ficou livre no inventário
            InventoryComponent->TryRemoveItemFromInventory(TagetInventorySlot);
            InventoryComponent->TryAddItemToSpecificSlot(ItemInstanceInEquipment, TagetInventorySlot);

            EquipmentList.MarkArrayDirty();
            OnItemUnequipped.Broadcast(ItemInstanceInEquipment,ItemInstanceInEquipment->GetFragment<UZfFragment_Equippable>()->EquipmentTags.First(), SlotPosition);

            return EZfItemMechanicResult::Success;
        }
        // CASO 2: Slot do inventário ocupado MAS o item NÃO pode ser equipado
        // Apenas desequipa e coloca no primeiro slot livre do inventário
        else
        {
            if (InventoryComponent->GetAvailableSlots() >= 1)
            {
                InternalUnequipItem(ItemInstanceInEquipment, SlotPosition);
                EquipmentList.MarkArrayDirty();
                OnItemUnequipped.Broadcast(ItemInstanceInEquipment,ItemInstanceInEquipment->GetFragment<UZfFragment_Equippable>()->EquipmentTags.First(), SlotPosition);

                // Adiciona no primeiro slot vazio disponível
                InventoryComponent->TryAddItemToInventory(ItemInstanceInEquipment);

                return EZfItemMechanicResult::Success;
            }
            else
            {
                // Inventário cheio — não pode desequipar
                return EZfItemMechanicResult::Failed_InventoryFull;
            }
        }
    }
    // CASO 3: Slot do inventário vazio → simplesmente desequipa e coloca lá
    else
    {
        InternalUnequipItem(ItemInstanceInEquipment, SlotPosition);
        EquipmentList.MarkArrayDirty();
        OnItemUnequipped.Broadcast(ItemInstanceInEquipment, SlotTag, SlotPosition);

        // Coloca direto no slot vazio que o jogador escolheu
        InventoryComponent->TryAddItemToSpecificSlot(ItemInstanceInEquipment, TagetInventorySlot);
        return EZfItemMechanicResult::Success;
    }
}

EZfItemMechanicResult UZfEquipmentComponent::TryEquipBackpack(FGameplayTag SlotTag, int32 FromInventorySlot, int32 SlotPosition)
{
    if (!InventoryComponent)
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    UZfItemInstance* BackpackAtInventory = InventoryComponent->GetItemAtSlot(FromInventorySlot);
    
    // Verifica se o Equipmentslot alvo no Equipamento já tem um item
    if (UZfItemInstance* EquipedBackpack = GetItemAtSlotTag(SlotTag, SlotPosition))
    {
        int32 NewExtraSlots = BackpackAtInventory->GetFragment<UZfFragment_InventoryExpansion>()->ExtraSlotCount;
        int32 OldExtraSlots = EquipedBackpack->GetFragment<UZfFragment_InventoryExpansion>()->ExtraSlotCount;

        // CASO 1: Irá aumentar o tamanho do inventario, não há necessiade de mudar os itens de lugar
        // Desequipa o atual, equipa o do inventário, e devolve o antigo no mesmo slot
        if (NewExtraSlots - OldExtraSlots >= 0)
        {
            // Remove o item atual do equipamento
            // Equipa o item que veio do inventário
            InternalUnequipItem(EquipedBackpack, SlotPosition);
            InternalEquipItem(BackpackAtInventory, SlotPosition);

            // Remove o item novo do inventário (ele agora está equipado)
            // Coloca o item antigo no slot que ficou livre no inventário
            InventoryComponent->TryRemoveItemFromInventory(FromInventorySlot);
            InventoryComponent->TryAddItemToSpecificSlot(EquipedBackpack, FromInventorySlot);
            InventoryComponent->UpdateSlotCountFromEquippedBackpack();
            
            EquipmentList.MarkArrayDirty();
            OnItemEquipped.Broadcast(BackpackAtInventory,BackpackAtInventory->GetFragment<UZfFragment_Equippable>()->EquipmentTags.First(), SlotPosition);


            return EZfItemMechanicResult::Success;
        }
        // CASO 2: Inventário irá diminuir, Tenho que mover os itens de posição
        else
        {
            int32 AvaliableSlots = InventoryComponent->GetAvailableSlotsWithExpansion(BackpackAtInventory);
            int32 ItensToMove = InventoryComponent->GetItemCountFromInitialSlot(AvaliableSlots);
            int32 NewCapacity = OldExtraSlots - NewExtraSlots;

            // CASO 2.1: Tenho espaço suficiente para mover os itens
            if (AvaliableSlots - ItensToMove >= 0)
            {
                InventoryComponent->RelocateItemsAboveCapacity(NewCapacity);

                // Remove o item atual do equipamento
                // Equipa o item que veio do inventário
                InternalUnequipItem(EquipedBackpack, SlotPosition);
                InternalEquipItem(BackpackAtInventory, SlotPosition);

                // Remove o item novo do inventário (ele agora está equipado)
                // Coloca o item antigo no slot que ficou livre no inventário
                InventoryComponent->TryRemoveItemFromInventory(FromInventorySlot);
                InventoryComponent->TryAddItemToSpecificSlot(EquipedBackpack, FromInventorySlot);
                InventoryComponent->UpdateSlotCountFromEquippedBackpack();
                
                EquipmentList.MarkArrayDirty();
                OnItemEquipped.Broadcast(BackpackAtInventory,BackpackAtInventory->GetFragment<UZfFragment_Equippable>()->EquipmentTags.First(), SlotPosition);

                
                return EZfItemMechanicResult::Success;
            }
            // CASO 2.1: Não tenho expaço suficiente para mover os itens
            else
            {
                return EZfItemMechanicResult::Failed_InventoryFull;
            }
        }
    }
    // CASO 3: Slot do Equipamento vazio → simplesmente Equipa
    else
    {
        InternalEquipItem(BackpackAtInventory, SlotPosition);
        EquipmentList.MarkArrayDirty();
        
        // Remove Item do Slot do inventario
        InventoryComponent->TryRemoveItemFromInventory(FromInventorySlot);
        InventoryComponent->UpdateSlotCountFromEquippedBackpack();

        OnItemEquipped.Broadcast(BackpackAtInventory,BackpackAtInventory->GetFragment<UZfFragment_Equippable>()->EquipmentTags.First(), SlotPosition);
        
        return EZfItemMechanicResult::Success;
    }
}

EZfItemMechanicResult UZfEquipmentComponent::TryUnequipBackpack(FGameplayTag SlotTag, int32 TargetInventorySlot, int32 SlotPosition)
{
    if (!InventoryComponent)
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    UZfItemInstance* BackpackAtInventory = InventoryComponent->GetItemAtSlot(TargetInventorySlot);

    
    UZfItemInstance* EquipedBackpack = GetItemAtSlotTag(SlotTag);
    
    int32 OldExtraSlots = EquipedBackpack->GetFragment<UZfFragment_InventoryExpansion>()->ExtraSlotCount;
    
    // Verifica se o Equipmentslot alvo no Equipamento já tem um item
    if (BackpackAtInventory && BackpackAtInventory->GetFragment<UZfFragment_InventoryExpansion>())
    {
        int32 NewExtraSlots = BackpackAtInventory->GetFragment<UZfFragment_InventoryExpansion>()->ExtraSlotCount;
        
        // CASO 1: Irá aumentar o tamanho do inventario, não há necessiade de mudar os itens de lugar
        // Desequipa o atual, equipa o do inventário, e devolve o antigo no mesmo slot
        if (NewExtraSlots - OldExtraSlots >= 0)
        {
            // Remove o item atual do equipamento
            // Equipa o item que veio do inventário
            InternalUnequipItem(EquipedBackpack, SlotPosition);
            InternalEquipItem(BackpackAtInventory, SlotPosition);

            // Remove o item novo do inventário (ele agora está equipado)
            // Coloca o item antigo no slot que ficou livre no inventário
            InventoryComponent->TryRemoveItemFromInventory(TargetInventorySlot);
            InventoryComponent->TryAddItemToSpecificSlot(EquipedBackpack, TargetInventorySlot);
            InventoryComponent->UpdateSlotCountFromEquippedBackpack();
            
            OnItemUnequipped.Broadcast(BackpackAtInventory,BackpackAtInventory->GetFragment<UZfFragment_Equippable>()->EquipmentTags.First(), SlotPosition);
            EquipmentList.MarkArrayDirty();

            return EZfItemMechanicResult::Success;
        }
        // CASO 2: Inventário irá diminuir, Tenho que mover os itens de posição
        else
        {
            int32 AvaliableSlots = InventoryComponent->GetAvailableSlotsWithExpansion(BackpackAtInventory);
            int32 ItensToMove = InventoryComponent->GetItemCountFromInitialSlot(AvaliableSlots);
            int32 NewCapacity = OldExtraSlots - NewExtraSlots;

            // CASO 2.1: Tenho espaço suficiente para mover os itens
            if (AvaliableSlots - ItensToMove >= 0)
            {
                InventoryComponent->RelocateItemsAboveCapacity(NewCapacity);

                // Remove o item atual do equipamento
                // Equipa o item que veio do inventário
                InternalUnequipItem(EquipedBackpack, SlotPosition);
                InternalEquipItem(BackpackAtInventory, SlotPosition);

                // Remove o item novo do inventário (ele agora está equipado)
                // Coloca o item antigo no slot que ficou livre no inventário
                InventoryComponent->TryRemoveItemFromInventory(TargetInventorySlot);
                InventoryComponent->TryAddItemToSpecificSlot(EquipedBackpack, TargetInventorySlot);
                InventoryComponent->UpdateSlotCountFromEquippedBackpack();

                OnItemUnequipped.Broadcast(BackpackAtInventory,BackpackAtInventory->GetFragment<UZfFragment_Equippable>()->EquipmentTags.First(), SlotPosition);
                EquipmentList.MarkArrayDirty();
                
                
                return EZfItemMechanicResult::Success;
            }
            // CASO 2.1: Não tenho expaço suficiente para mover os itens
            else
            {
                return EZfItemMechanicResult::Failed_InventoryFull;
            }
        }
    }
    // CASO 3: Slot do Equipamento vazio → simplesmente Equipa
    else
    {
        int32 AvaliableSlots = InventoryComponent->GetAvailableSlotsWithExpansion(BackpackAtInventory);
        int32 ItensToMove = InventoryComponent->GetItemCountFromInitialSlot(AvaliableSlots);
        
        if (AvaliableSlots - 1 - ItensToMove >= 0)
        {

            InternalUnequipItem(EquipedBackpack, SlotPosition);
            EquipmentList.MarkArrayDirty();
            
            InventoryComponent->TryAddItemToSpecificSlot(EquipedBackpack, TargetInventorySlot);   

            int32 NewCapacity = OldExtraSlots - InventoryComponent->GetAvailableDefaultSlots();
            InventoryComponent->RelocateItemsAboveCapacity(NewCapacity);
            InventoryComponent->UpdateSlotCountFromEquippedBackpack();

            OnItemUnequipped.Broadcast(EquipedBackpack,EquipedBackpack->GetFragment<UZfFragment_Equippable>()->EquipmentTags.First(), SlotPosition);

            
            return EZfItemMechanicResult::Success;
        }
        return EZfItemMechanicResult::Failed_InventoryFull;
    }
}


// ============================================================
// FUNÇÕES DE CONSULTA
// ============================================================

EZfItemMechanicResult UZfEquipmentComponent::CanEquipItem(UZfItemInstance* InItemInstance, int32 FromInventorySlot, int32 SlotPosition)
{

    if (FromInventorySlot < 0)
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }
    
    if (InItemInstance == GetItemAtSlotTag(InItemInstance->GetFragment<UZfFragment_Equippable>()->EquipmentTags.First()))
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }
    
    // Item nulo — nada a equipar
    if (!InItemInstance)
    {
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    // Item com durabilidade zero — não pode ser equipado
    if (InItemInstance->bIsBroken)
    {
        return EZfItemMechanicResult::Failed_ItemBroken;
    }

    // Verifica se o item tem o fragment que define em qual slot ele pode ir
    const UZfFragment_Equippable* EquippableFragment = InItemInstance->GetFragment<UZfFragment_Equippable>();

    if (!EquippableFragment)
    {
        return EZfItemMechanicResult::Failed_CannotEquip;
    }

    return EZfItemMechanicResult::Success;
}

FGameplayTag UZfEquipmentComponent::GetEquipmentSlotTagOfItem(UZfItemInstance* ItemInstance) const
{
    if (!ItemInstance)
    {
        return FGameplayTag();
    }

    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.ItemInstance == ItemInstance)
        {
            return Entry.SlotTag;
        }
    }

    return FGameplayTag();
}

UZfItemInstance* UZfEquipmentComponent::GetItemAtSlotTag(FGameplayTag SlotTag, int32 SlotPosition) const
{
    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.SlotTag == SlotTag && Entry.SlotPosition == SlotPosition)
        {
            return Entry.ItemInstance;
        }
    }

    return nullptr;
}

// ============================================================
// FUNÇÕES INTERNAS - ORGANIZAÇÃO
// ============================================================

void UZfEquipmentComponent::InternalEquipItem(UZfItemInstance* InItemInstance, int32 SlotPosition)
{
    check(InItemInstance != nullptr);
    
    const UZfFragment_Equippable* EquippableFragment = InItemInstance->GetFragment<UZfFragment_Equippable>();
    
    FZfEquipmentSlotEntry NewSlot;
    NewSlot.SlotTag = EquippableFragment->EquipmentTags.First();
    NewSlot.ItemInstance = InItemInstance;
    NewSlot.SlotPosition = SlotPosition;
    EquipmentList.EquippedItems.Add(NewSlot);
    
    AddReplicatedSubObject(InItemInstance);
}

void UZfEquipmentComponent::InternalUnequipItem(UZfItemInstance* InItemInstance, int32 SlotPosition)
{
    check(InItemInstance != nullptr);

    EquipmentList.EquippedItems.RemoveAll(
        [InItemInstance, SlotPosition](const FZfEquipmentSlotEntry& Entry)
        {
            return Entry.ItemInstance == InItemInstance && Entry.SlotPosition == SlotPosition;
        });

    RemoveReplicatedSubObject(InItemInstance);
}

FZfEquipmentSlotEntry* UZfEquipmentComponent::InternalFindSlotEntry(FGameplayTag SlotTag, int32 SlotPosition )
{
    for (FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.SlotTag == SlotTag && Entry.SlotPosition  == SlotPosition )
        {
            return &Entry;
        }
    }
    return nullptr;
}

// ============================================================
// FUNÇÕES INTERNAS - VALIDAÇÃO
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
            TEXT("UZfEquipmentComponent::%s — " "Operação de servidor chamada no cliente!"), *FunctionName);
        return false;
    }

    return true;
}

