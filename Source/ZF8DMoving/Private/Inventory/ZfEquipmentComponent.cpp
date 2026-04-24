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
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Inventory/Fragments/ZfFragment_Consumable.h"
#include "Inventory/Fragments/ZfFragment_Stackable.h"
#include "Player/ZfPlayerState.h"

void UZfEquipmentComponent::UnequipAllItems()
{
    if (!InternalCheckIsServer(TEXT("UnequipAllItems")))
    {
        return;
    }

    // Coleta todos os itens equipados antes de iterar
    // para evitar modificar o array durante o loop
    TArray<TPair<FGameplayTag, int32>> SlotsToUnequip;
    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.ItemInstance)
        {
            SlotsToUnequip.Add({ Entry.SlotTag, Entry.SlotPosition });
        }
    }

    for (const TPair<FGameplayTag, int32>& SlotPair : SlotsToUnequip)
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

EZfItemMechanicResult UZfEquipmentComponent::QuickSwapItem(int32 InventorySlotIndex, FGameplayTag EquipSlotTag, int32 EquipSlotIndex)
{
    if (!InternalCheckIsServer(TEXT("QuickSwapItem")))
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    if (!InventoryComponent)
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    // Obtém o item do inventário
    UZfItemInstance* ItemFromInventory = InventoryComponent->GetItemAtSlot(InventorySlotIndex);

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

UZfItemInstance* UZfEquipmentComponent::GetItemAtEquipmentSlot(FGameplayTag EquipSlotTag, int32 SlotIndex) const
{
    const FZfEquipmentSlotEntry* SlotEntry =
        Internal_FindSlotEntryConst(EquipSlotTag, SlotIndex);

    return SlotEntry ? SlotEntry->ItemInstance.Get() : nullptr;
}

bool UZfEquipmentComponent::IsItemEquipped(UZfItemInstance* ItemInstance) const
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

bool UZfEquipmentComponent::IsEquipmentSlotOccupied(FGameplayTag EquipSlotTag, int32 SlotIndex) const
{
    const FZfEquipmentSlotEntry* SlotEntry =
        Internal_FindSlotEntryConst(EquipSlotTag, SlotIndex);

    return SlotEntry && SlotEntry->ItemInstance != nullptr;
}

bool UZfEquipmentComponent::IsEquipmentSlotBlocked(FGameplayTagContainer EquipmentTags,int32 SlotIndex) const
{
    // Apenas o OffHand pode ser bloqueado por arma de duas mãos
    if (EquipmentTags.HasTagExact(ZfEquipmentTags::EquipmentSlots::Slot_OffHand))
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

FGameplayTag UZfEquipmentComponent::GetEquipmentSlotOfItem(UZfItemInstance* ItemInstance) const
{
    if (!ItemInstance)
    {
        return ZfEquipmentTags::EquipmentSlots::Slot_None;
    }

    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.ItemInstance == ItemInstance)
        {
            return Entry.SlotTag;
        }
    }
    return ZfEquipmentTags::EquipmentSlots::Slot_None;
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
    if (!InventoryComponent)
    {
        return;
    }

    //const int32 SlotsAdded = InventoryComponent->TryAddExtraSlots(ExtraSlots);
    int32 SlotsAdded = 2;
    UE_LOG(LogZfInventory, Log, TEXT("UZfEquipmentComponent::OnBackpackEquipped — " "%d slots adicionados ao inventário."), SlotsAdded);
}

void UZfEquipmentComponent::OnBackpackUnequipped(int32 ExtraSlots)
{
    if (!InventoryComponent)
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
        NewEntry.SlotTag = DefaultSlot.SlotTag;
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

UAbilitySystemComponent* UZfEquipmentComponent::Internal_GetAbilitySystemComponent() const
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
    if (!ItemInstance) return;

    UAbilitySystemComponent* ASC = Internal_GetAbilitySystemComponent();
    if (!ASC)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfEquipmentComponent::Internal_ApplyItemGameplayEffects — "
                 "ASC não encontrado. Modifiers não serão aplicados."));
        return;
    }

    UDataTable* ModifierDataTable = nullptr;
    if (const UZfFragment_Modifiers* ModifierFragment = ItemInstance->GetFragment<UZfFragment_Modifiers>())
    {
        ModifierDataTable = ModifierFragment->GetLoadedModifierDataTable();
    }

    for (FZfAppliedModifier& Modifier : ItemInstance->GetAppliedModifiers())
    {
        // ItemProperty — ignorado ao equipar
        if (Modifier.TargetType == EZfModifierTargetType::ItemProperty)
        {
            continue;
        }

        // ── 1. Busca RuleClass no DataTable ──────────────────────────────
        TSubclassOf<UZfModifierRule> RuleClass = nullptr;
        if (ModifierDataTable)
        {
            const FZfModifierDataTypes* ModData = ModifierDataTable->FindRow<FZfModifierDataTypes>(
                Modifier.ModifierRowName, TEXT("Internal_ApplyItemGameplayEffects"));

            if (ModData)
            {
                RuleClass = ModData->RuleClass;
            }
        }

        // ── 2. Calcula FinalValue ─────────────────────────────────────────
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
                        {
                            Internal_OnModifierRuleValueChanged(ItemInstance, CapturedRowName, InnerASC);
                        }
                    });
            }
        }

        // ── 3. Snapshot do valor aplicado ─────────────────────────────────
        Modifier.AppliedValue = Modifier.FinalValue;

        // ── 4. Aplica GE no ASC ───────────────────────────────────────────
        if (Modifier.GameplayEffect.IsNull())
        {
            UE_LOG(LogZfInventory, Warning,
                TEXT("Internal_ApplyItemGameplayEffects — Modifier '%s': "
                     "GameplayEffect está nulo."),
                *Modifier.ModifierRowName.ToString());
            continue;
        }

        TSubclassOf<UGameplayEffect> GEClass = Modifier.GameplayEffect.LoadSynchronous();
        if (!GEClass) continue;

        FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
        EffectContext.AddSourceObject(GetOwner());

        FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(GEClass, 1.f, EffectContext);
        if (Spec.IsValid())
        {
            Modifier.ActiveEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

            UE_LOG(LogZfInventory, Log,
                TEXT("Internal_ApplyItemGameplayEffects — Modifier '%s': "
                     "GE aplicado. FinalValue=%.2f"),
                *Modifier.ModifierRowName.ToString(), Modifier.FinalValue);
        }
    }
}

void UZfEquipmentComponent::Internal_RemoveItemGameplayEffects(UZfItemInstance* ItemInstance)
{
    if (!ItemInstance) return;

    UAbilitySystemComponent* ASC = Internal_GetAbilitySystemComponent();
    if (!ASC) return;

    // Desativa Rules antes de remover os efeitos
    Internal_DeactivateModifierRules(ItemInstance);

    for (FZfAppliedModifier& Modifier : ItemInstance->GetAppliedModifiers())
    {
        // ItemProperty — ignorado ao desequipar
        if (Modifier.TargetType == EZfModifierTargetType::ItemProperty)
        {
            continue;
        }

        if (Modifier.ActiveEffectHandle.IsValid())
        {
            ASC->RemoveActiveGameplayEffect(Modifier.ActiveEffectHandle);
            Modifier.ActiveEffectHandle = FActiveGameplayEffectHandle();

            UE_LOG(LogZfInventory, Log,
                TEXT("Internal_RemoveItemGameplayEffects — Modifier '%s': GE removido."),
                *Modifier.ModifierRowName.ToString());
        }
    }
}

// ============================================================
// MODIFIER RULES — RUNTIME
// ============================================================

void UZfEquipmentComponent::Internal_DeactivateModifierRules(UZfItemInstance* ItemInstance)
{
    TMap<FName, TObjectPtr<UZfModifierRule>>* RulesForItem = ActiveModifierRules.Find(ItemInstance);

    if (!RulesForItem) return;

    for (TPair<FName, TObjectPtr<UZfModifierRule>>& Pair : *RulesForItem)
    {
        if (Pair.Value)
        {
            // Desconecta os delegates da fonte e limpa o callback
            Pair.Value->UnbindFromSource();
            Pair.Value->OnRuleValueChanged.Clear();
        }
    }

    // Remove o item do mapa — GC destrói as instâncias
    ActiveModifierRules.Remove(ItemInstance);
}

void UZfEquipmentComponent::Internal_OnModifierRuleValueChanged(
    UZfItemInstance* ItemInstance,
    FName ModifierRowName,
    UAbilitySystemComponent* ASC)
{
    if (!ItemInstance || !ASC) return;

    // Encontra o modifier no ItemInstance
    FZfAppliedModifier* Modifier = nullptr;
    for (FZfAppliedModifier& Mod : ItemInstance->GetAppliedModifiers())
    {
        if (Mod.ModifierRowName == ModifierRowName)
        {
            Modifier = &Mod;
            break;
        }
    }

    if (!Modifier)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("Internal_OnModifierRuleValueChanged — "
                 "Modifier '%s' não encontrado no ItemInstance."),
            *ModifierRowName.ToString());
        return;
    }

    // Encontra a Rule ativa correspondente
    TMap<FName, TObjectPtr<UZfModifierRule>>* RulesForItem = ActiveModifierRules.Find(ItemInstance);

    if (!RulesForItem) return;

    TObjectPtr<UZfModifierRule>* RulePtr = RulesForItem->Find(ModifierRowName);
    if (!RulePtr || !(*RulePtr)) return;

    // Recalcula o FinalValue com o novo estado da fonte
    const float NewFinalValue = (*RulePtr)->Calculate(Modifier->CurrentValue);

    Internal_ReapplyModifier(ItemInstance, *Modifier, NewFinalValue, ASC);
}

void UZfEquipmentComponent::Internal_ReapplyModifier(
    UZfItemInstance* ItemInstance,
    FZfAppliedModifier& Modifier,
    float NewFinalValue,
    UAbilitySystemComponent* ASC)
{
    // ItemProperty — ignorado no ciclo de equip/unequip
    if (Modifier.TargetType == EZfModifierTargetType::ItemProperty)
    {
        return;
    }

    // Remove o GE com o valor antigo
    if (Modifier.ActiveEffectHandle.IsValid())
    {
        ASC->RemoveActiveGameplayEffect(Modifier.ActiveEffectHandle);
        Modifier.ActiveEffectHandle = FActiveGameplayEffectHandle();
    }

    // Atualiza o snapshot
    Modifier.FinalValue   = NewFinalValue;
    Modifier.AppliedValue = NewFinalValue;

    // Reaplica o GE com o novo FinalValue
    if (!Modifier.GameplayEffect.IsNull())
    {
        TSubclassOf<UGameplayEffect> GEClass = Modifier.GameplayEffect.LoadSynchronous();
        if (GEClass)
        {
            FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
            EffectContext.AddSourceObject(GetOwner());

            FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(GEClass, 1.f, EffectContext);
            if (Spec.IsValid())
            {
                Modifier.ActiveEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
            }
        }
    }

    UE_LOG(LogZfInventory, Log,
        TEXT("Internal_ReapplyModifier — Modifier '%s': "
             "GE reaplicado. Novo FinalValue=%.2f"),
        *Modifier.ModifierRowName.ToString(), NewFinalValue);
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

const FZfEquipmentSlotEntry* UZfEquipmentComponent::Internal_FindSlotEntryConst(FGameplayTag EquipSlotTag, int32 SlotIndex) const
{
    for (const FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.SlotTag == EquipSlotTag && Entry.SlotPosition == SlotIndex)
        {
            return &Entry;
        }
    }
    return nullptr;
}








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
// CICLO DE VIDA - BEGINPLAY, TICK...
// ============================================================

void UZfEquipmentComponent::BeginPlay()
{
    Super::BeginPlay();

    // Busca o InventoryComponent no ator dono
    InventoryComponent = Cast<AZfPlayerState>(GetOwner())->FindComponentByClass<UZfInventoryComponent>();

    if (GetOwner())
    {
        InventoryComponent = Cast<AZfPlayerState>(GetOwner())->FindComponentByClass<UZfInventoryComponent>();
    }
}


// ============================================================
// FAST Array
// ============================================================

void FZfEquipmentSlotEntry::PreReplicatedRemove(const FZfEquipmentList& InArraySerializer)
{
    if (InArraySerializer.OwnerComponent && ItemInstance)
    {
        InArraySerializer.OwnerComponent->OnItemUnequipped.Broadcast(ItemInstance, ItemInstance->GetFragment<UZfFragment_Equippable>()->EquipmentSlotTag, SlotPosition);
    }
}

void FZfEquipmentSlotEntry::PostReplicatedAdd(const FZfEquipmentList& InArraySerializer)
{
    if (InArraySerializer.OwnerComponent && ItemInstance)
    {
        InArraySerializer.OwnerComponent->OnItemEquipped.Broadcast(ItemInstance, ItemInstance->GetFragment<UZfFragment_Equippable>()->EquipmentSlotTag, SlotPosition);
    }
}

void FZfEquipmentSlotEntry::PostReplicatedChange(const FZfEquipmentList& InArraySerializer)
{
    if (InArraySerializer.OwnerComponent && ItemInstance)
    {
        InArraySerializer.OwnerComponent->OnStackChanged.Broadcast(ItemInstance, ItemInstance->GetFragment<UZfFragment_Equippable>()->EquipmentSlotTag, SlotPosition);
    }
}

// Definição central da categoria de log do sistema de inventário.
// Declarada em ZfInventoryTypes.h com DECLARE_LOG_CATEGORY_EXTERN.
// Uso em qualquer arquivo: UE_LOG(LogZfInventory, Log, TEXT("..."));
DEFINE_LOG_CATEGORY(LogZfInventory);

// ============================================================
// FUNÇÕES SERVER - GERENCIAMENTO
// ============================================================

void UZfEquipmentComponent::ServerTryEquipItem_Implementation(UZfItemInstance* ItemInstance, int32 FromInventorySlot, int32 SlotPosition, FGameplayTag SlotTag)
{
    if (InternalCheckIsServer("ServerTryEquipItem_Implementation"))
    {
        TryEquipItem(ItemInstance, FromInventorySlot, SlotPosition, SlotTag);
    }
}

void UZfEquipmentComponent::ServerTryUnequipItem_Implementation(FGameplayTag SlotTag, int32 TagetInventorySlot, int32 SlotPosition)
{
    if (InternalCheckIsServer(TEXT("ServerTryUnequipItem")))
    {
        TryUnequipItem(SlotTag, TagetInventorySlot, SlotPosition);
    }
}

void UZfEquipmentComponent::ServerTryUseQuickSlot_Implementation(int32 SlotPosition)
{
    if (InternalCheckIsServer(TEXT("ServerTryUseQuickSlot")))
    {
        TryUseQuickSlot(SlotPosition);
    }
}

void UZfEquipmentComponent::ServerTryRemoveItemStackFromEquipmentSlot_Implementation(FGameplayTag SlotTag, int32 SlotPosition)
{
    if (InternalCheckIsServer(TEXT("TryRemoveItemStackFromEquipmentSlot")))
    {
        TryRemoveItemStackFromEquipmentSlot(SlotTag,SlotPosition);
    }
}

// ============================================================
// FUNÇÕES PRINCIPAIS - GERENCIAMENTO
// ============================================================



EZfItemMechanicResult UZfEquipmentComponent::TryEquipItem(UZfItemInstance* ItemFromInventory, int32 FromInventorySlot, int32 SlotPosition, FGameplayTag SlotTag)
{
    if (!ItemFromInventory)
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    const UZfFragment_Equippable* EquippableFragment = ItemFromInventory->GetFragment<UZfFragment_Equippable>();

    if (!EquippableFragment)
    {
        return EZfItemMechanicResult::Failed_CannotEquip;
    }
    
    if (SlotTag != SlotTag.EmptyTag && EquippableFragment->EquipmentSlotTag != SlotTag)
    {
        return EZfItemMechanicResult::Failed_CannotEquip;
    }
    
    // Valida se o item pode ser equipado (não nulo, não quebrado, tem fragment, slot livre)
    if (CanEquipItem(ItemFromInventory, FromInventorySlot, SlotPosition) != EZfItemMechanicResult::Success)
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }

    // Tenta Equipar Mochila
    if (ItemFromInventory->GetFragment<UZfFragment_InventoryExpansion>())
    {
        if (TryEquipBackpack(EquippableFragment->EquipmentSlotTag, FromInventorySlot, SlotPosition) == EZfItemMechanicResult::Success)
        {
            return EZfItemMechanicResult::Success;
        }
        else
        {
            return EZfItemMechanicResult::Failed_InventoryFull;
        }
    }

    // Verifica se tenho item ja equipado
    if (UZfItemInstance* EquipedItem = GetItemAtSlotTag(EquippableFragment->EquipmentSlotTag, SlotPosition))
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
        OnItemEquipped.Broadcast(ItemFromInventory,ItemFromInventory->GetFragment<UZfFragment_Equippable>()->EquipmentSlotTag, SlotPosition);

        return EZfItemMechanicResult::Success;
    }
    else
    {
        // Adiciona o item ao EquipmentList e registra para replicação
        InternalEquipItem(ItemFromInventory, SlotPosition);
        EquipmentList.MarkArrayDirty();
        OnItemEquipped.Broadcast(ItemFromInventory,EquippableFragment->EquipmentSlotTag, SlotPosition);

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
        if (TryUnequipBackpack(ItemInstanceInEquipment->GetFragment<UZfFragment_Equippable>()->EquipmentSlotTag, TagetInventorySlot, SlotPosition) == EZfItemMechanicResult::Success)
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
        // Slot do inventário ocupado
        if (ItemInstanceAtInventory->GetFragment<UZfFragment_Equippable>()->EquipmentSlotTag == SlotTag)
        {
            // Posso Equipar o Item do Inventário
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
                OnItemUnequipped.Broadcast(ItemInstanceInEquipment,ItemInstanceInEquipment->GetFragment<UZfFragment_Equippable>()->EquipmentSlotTag, SlotPosition);

                return EZfItemMechanicResult::Success;
            }
        }
        // Não posso equipar o item do inventário
        if (InventoryComponent->GetAvailableSlots() >= 1)
        {
            InternalUnequipItem(ItemInstanceInEquipment, SlotPosition);
            EquipmentList.MarkArrayDirty();
            OnItemUnequipped.Broadcast(ItemInstanceInEquipment,ItemInstanceInEquipment->GetFragment<UZfFragment_Equippable>()->EquipmentSlotTag, SlotPosition);

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
    // Slot do inventário vazio → simplesmente desequipa e coloca lá
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
            OnItemEquipped.Broadcast(BackpackAtInventory,BackpackAtInventory->GetFragment<UZfFragment_Equippable>()->EquipmentSlotTag, SlotPosition);


            return EZfItemMechanicResult::Success;
        }
        // CASO 2: Inventário irá diminuir, Tenho que mover os itens de posição
        else
        {
            int32 NewCapacity = InventoryComponent->GetDefaultSlotCount() + NewExtraSlots;
            int32 ItensToMove = InventoryComponent->GetItemCountFromInitialSlot(NewCapacity);
            int32 AvaliableSlots = InventoryComponent->GetAvailableSlotsWithExpansion(BackpackAtInventory);

            // CASO 2.1: Tenho espaço suficiente para mover os itens
            if (AvaliableSlots >= ItensToMove)
            {
                // Remove o item atual do equipamento
                // Equipa o item que veio do inventário
                InternalUnequipItem(EquipedBackpack, SlotPosition);
                InternalEquipItem(BackpackAtInventory, SlotPosition);

                // Remove o item novo do inventário (ele agora está equipado)
                // Coloca o item antigo no slot que ficou livre no inventário
                InventoryComponent->TryRemoveItemFromInventory(FromInventorySlot);
                InventoryComponent->TryAddItemToSpecificSlot(EquipedBackpack, FromInventorySlot);

                InventoryComponent->RelocateItemsAboveCapacity(NewCapacity);
                InventoryComponent->UpdateSlotCountFromEquippedBackpack();
                
                EquipmentList.MarkArrayDirty();
                OnItemEquipped.Broadcast(BackpackAtInventory,BackpackAtInventory->GetFragment<UZfFragment_Equippable>()->EquipmentSlotTag, SlotPosition);
                
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

        OnItemEquipped.Broadcast(BackpackAtInventory,BackpackAtInventory->GetFragment<UZfFragment_Equippable>()->EquipmentSlotTag, SlotPosition);
        
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
            
            OnItemUnequipped.Broadcast(BackpackAtInventory,BackpackAtInventory->GetFragment<UZfFragment_Equippable>()->EquipmentSlotTag, SlotPosition);
            EquipmentList.MarkArrayDirty();

            return EZfItemMechanicResult::Success;
        }
        // CASO 2: Inventário irá diminuir, Tenho que mover os itens de posição
        else
        {
            int32 AvaliableSlots = InventoryComponent->GetAvailableSlotsWithExpansion(BackpackAtInventory);
            int32 ItensToMove = InventoryComponent->GetItemCountFromInitialSlot(AvaliableSlots);
            int32 NewCapacity = InventoryComponent->GetDefaultSlotCount() + NewExtraSlots;

            // CASO 2.1: Tenho espaço suficiente para mover os itens
            if (AvaliableSlots - ItensToMove >= 0)
            {
                // Remove o item atual do equipamento
                // Equipa o item que veio do inventário
                InternalUnequipItem(EquipedBackpack, SlotPosition);
                InternalEquipItem(BackpackAtInventory, SlotPosition);

                // Remove o item novo do inventário (ele agora está equipado)
                // Coloca o item antigo no slot que ficou livre no inventário
                InventoryComponent->TryRemoveItemFromInventory(TargetInventorySlot);
                InventoryComponent->TryAddItemToSpecificSlot(EquipedBackpack, TargetInventorySlot);

                InventoryComponent->RelocateItemsAboveCapacity(NewCapacity);
                InventoryComponent->UpdateSlotCountFromEquippedBackpack();

                OnItemUnequipped.Broadcast(BackpackAtInventory,BackpackAtInventory->GetFragment<UZfFragment_Equippable>()->EquipmentSlotTag, SlotPosition);
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
    // CASO 3: Slot do Equipamento vazio → simplesmente Desequipa
    else
    {
        int32 AvaliableSlots = InventoryComponent->GetAvailableSlotsWithExpansion(BackpackAtInventory);
        int32 ItensToMove = InventoryComponent->GetItemCountFromInitialSlot(InventoryComponent->GetDefaultSlotCount());
        
        if (AvaliableSlots - 1 - ItensToMove >= 0)
        {

            InternalUnequipItem(EquipedBackpack, SlotPosition);
            EquipmentList.MarkArrayDirty();
            
            InventoryComponent->TryAddItemToSpecificSlot(EquipedBackpack, TargetInventorySlot);   
            
            InventoryComponent->RelocateItemsAboveCapacity(InventoryComponent->GetAvailableDefaultSlots());
            InventoryComponent->UpdateSlotCountFromEquippedBackpack();

            OnItemUnequipped.Broadcast(EquipedBackpack,EquipedBackpack->GetFragment<UZfFragment_Equippable>()->EquipmentSlotTag, SlotPosition);

            
            return EZfItemMechanicResult::Success;
        }
        return EZfItemMechanicResult::Failed_InventoryFull;
    }
}

void UZfEquipmentComponent::TryUseQuickSlot(int32 SlotPosition)
{
    if (SlotPosition < 0 || SlotPosition > 2) return;

    const FGameplayTag SlotTag = ZfEquipmentTags::EquipmentSlots::Slot_Consumable;

    UZfItemInstance* Item = GetItemAtEquipmentSlot(SlotTag, SlotPosition);
    if (!Item) return;

    // Verifica se tem Fragment_Consumable
    const UZfFragment_Consumable* Fragment = Item->GetFragment<UZfFragment_Consumable>();
    if (!Fragment) return;

    // Verifica se tem Fragment_Consumable
    const UZfFragment_Stackable* Stackable = Item->GetFragment<UZfFragment_Stackable>();

    // Consome ANTES de disparar o evento
    if (Stackable)
    {
        
        if (Fragment->bConsumeOnUse)
        {
            TryRemoveItemStackFromEquipmentSlot(SlotTag, SlotPosition);
        }
    }
    else
    {
        if (Fragment->bConsumeOnUse)
        {
            InternalUnequipItem(Item, SlotPosition);
            EquipmentList.MarkArrayDirty();
            OnItemUnequipped.Broadcast(Item, SlotTag, SlotPosition);
        }
    }
    

    // Dispara o evento — GA processa os efeitos
    UAbilitySystemComponent* ASC = Internal_GetAbilitySystemComponent();
    if (!ASC) return;

    FGameplayEventData EventData;
    EventData.OptionalObject = Item;
    EventData.EventTag = ZfUniqueItemTags::ItemEvents::Item_Event_Use;
    ASC->HandleGameplayEvent(EventData.EventTag, &EventData);
}

void UZfEquipmentComponent::TryRemoveItemStackFromEquipmentSlot(FGameplayTag SlotTag, int32 SlotPosition)
{
    if (!InternalCheckIsServer(TEXT("RemoveItemStackFromEquipmentSlot"))) return;

    UZfItemInstance* Item = GetItemAtEquipmentSlot(SlotTag, SlotPosition);
    if (!Item) return;

    if (!Item->HasFragment<UZfFragment_Stackable>()) return;

    
    
    // Reduz 1 do stack. So remove o item se o stack zerar.
    if (Item->RemoveFromStack(1))
    {
        // Nao stackavel OU stack zerou — remove o item do slot.
        InternalUnequipItem(Item, SlotPosition);
        EquipmentList.MarkArrayDirty();
        OnItemUnequipped.Broadcast(Item, SlotTag, SlotPosition);
        return;
    }
    
    // Stack ainda tem unidades — notifica dirty para replicar.
    FZfEquipmentSlotEntry* Entry = FindSlotEntryByItem(Item);
    if (Entry)
    {
        EquipmentList.MarkItemDirty(*Entry);
    }
    OnStackChanged.Broadcast(Item, SlotTag, SlotPosition);
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
    
    if (InItemInstance == GetItemAtSlotTag(InItemInstance->GetFragment<UZfFragment_Equippable>()->EquipmentSlotTag))
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }
    
    // Item nulo — nada a equipar
    if (!InItemInstance)
    {
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }

    // Item com durabilidade zero — não pode ser equipado
    if (InItemInstance->GetIsBroken())
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

const TArray<FZfEquipmentSlotEntry>& UZfEquipmentComponent::GetAllEquipmentSlots() const
{
    return EquipmentList.EquippedItems;
}

FZfEquipmentSlotEntry* UZfEquipmentComponent::FindSlotEntryByItem(UZfItemInstance* Item)
{
    if (!Item) return nullptr;

    for (FZfEquipmentSlotEntry& Entry : EquipmentList.EquippedItems)
    {
        if (Entry.ItemInstance == Item)
        {
            return &Entry;
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
    NewSlot.SlotTag      = EquippableFragment->EquipmentSlotTag;
    NewSlot.ItemInstance = InItemInstance;
    NewSlot.SlotPosition = SlotPosition;
    EquipmentList.EquippedItems.Add(NewSlot);
    
    AddReplicatedSubObject(InItemInstance);

    // Aplica GEs e ativa Rules dos modifiers
    Internal_ApplyItemGameplayEffects(InItemInstance);
    
    // Notifica os fragments
    InItemInstance->NotifyFragments_ItemEquipped(this, GetOwner());
}

void UZfEquipmentComponent::InternalUnequipItem(UZfItemInstance* InItemInstance, int32 SlotPosition)
{
    check(InItemInstance != nullptr);

    // Remove GEs e desativa Rules antes de remover do array
    Internal_RemoveItemGameplayEffects(InItemInstance);

    EquipmentList.EquippedItems.RemoveAll(
        [InItemInstance, SlotPosition](const FZfEquipmentSlotEntry& Entry)
        {
            return Entry.ItemInstance == InItemInstance
                && Entry.SlotPosition == SlotPosition;
        });

    RemoveReplicatedSubObject(InItemInstance);

    InItemInstance->NotifyFragments_ItemUnequipped(this, GetOwner());
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

int32 UZfEquipmentComponent::Internal_GenerateReplicationKey() const
{
    // Gera uma chave única baseada no tamanho atual do array
    // + um offset para evitar colisões
    static int32 KeyCounter = 0;
    return ++KeyCounter;
}