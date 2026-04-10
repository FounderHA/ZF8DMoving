// Copyright ZfGame Studio. All Rights Reserved.
// ZfItemInstance.cpp

#include "Inventory/ZfItemInstance.h"
#include "Inventory/ZfInventoryTypes.h"
#include "Inventory/ZfItemDefinition.h"
#include "Inventory/Fragments/ZfItemFragment.h"
#include "Inventory/Fragments/ZfFragment_Durability.h"
#include "Inventory/Fragments/ZfFragment_Stackable.h"
#include "Inventory/ZfInventoryComponent.h"
#include "Inventory/ZfEquipmentComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Tags/ZfGameplayTags.h"
#include "Inventory/Fragments/ZfFragment_ItemUnique.h"
#include "Inventory/Fragments/ZfFragment_Modifiers.h"
#include "Inventory/Fragments/ZfFragment_QualityAttributes.h"

// ============================================================
// Constructor
// ============================================================

UZfItemInstance::UZfItemInstance()
{
    // ItemInstance não precisa de tick
    // Inicialização de valores feita via InitializeItemInstance
}

// ============================================================
// REPLICAÇÃO
// ============================================================

void UZfItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Identidade — replicado para todos, nunca muda após init
    DOREPLIFETIME(UZfItemInstance, ItemGuid);
    DOREPLIFETIME(UZfItemInstance, ItemDefinition);

    // Progressão — replicado para todos
    DOREPLIFETIME(UZfItemInstance, ItemTier);
    DOREPLIFETIME(UZfItemInstance, ItemRarity);
    DOREPLIFETIME(UZfItemInstance, CurrentQuality);

    // Stack — replicado para todos
    DOREPLIFETIME(UZfItemInstance, CurrentStack);

    // Durabilidade — replicado para todos
    DOREPLIFETIME(UZfItemInstance, CurrentDurability);
    DOREPLIFETIME(UZfItemInstance, BonusMaxDurability);
    DOREPLIFETIME(UZfItemInstance, TotalMaxDurability);
    DOREPLIFETIME(UZfItemInstance, bIsRepairable);
    DOREPLIFETIME(UZfItemInstance, bIsBroken);

    
    // Stats base — replicado para todos
    DOREPLIFETIME(UZfItemInstance, ItemAttributes);

    // Modifiers — replicado para todos
    DOREPLIFETIME(UZfItemInstance, AppliedModifiers);

    // Corrupção — replicado para todos
    DOREPLIFETIME(UZfItemInstance, CorruptionState);

    // Economia — replicado para todos
    DOREPLIFETIME(UZfItemInstance, CalculatedMarketValue);
}

// ============================================================
// INICIALIZAÇÃO
// ============================================================

void UZfItemInstance::SetCurrentDurability_Implementation(float NewDurability)
{
    if (CurrentDurability != NewDurability)
    {
        CurrentDurability = NewDurability;

        // Notifica Rules dinâmicas baseadas em durabilidade.
        // Binding feito pelo EquipmentComponent ao equipar o item.
        OnDurabilityChanged.Broadcast(CurrentDurability);
    }
}

void UZfItemInstance::InitializeItemInstance(UZfItemDefinition* InItemDefinition, int32 InItemTier, EZfItemRarity InItemRarity)
{
    // Garante que está rodando no servidor
    if (!Internal_CheckIsServer(TEXT("InitializeItemInstance")))
    {
        return;
    }

    // Valida o ItemDefinition
    if (!InItemDefinition)
    {
        UE_LOG(LogZfInventory, Error, TEXT("UZfItemInstance::InitializeItemInstance — " "InItemDefinition é nulo. Item não será inicializado."));
        return;
    }

    // Gera GUID único para este item
    ItemGuid = FGuid::NewGuid();

    // Armazena a definição e dados de progressão
    ItemDefinition = InItemDefinition;
    ItemTier = FMath::Clamp(InItemTier, 0, 5);
    ItemRarity = InItemRarity;

    // Inicializa stack padrão
    CurrentStack = 1;

    // Inicializa qualidade em 0
    CurrentQuality = 0;
    

    // Se for item Único, aplica modifiers fixos do PDA
    if (InItemDefinition->FindFragment<UZfFragment_ItemUnique>())
    {
        Internal_InitializeUniqueModifiers();
    }

    // Após inicializar os outros campos — garante que TotalMaxDurability
    // está correto desde o momento da criação do item.
    Internal_RecalculateTotalMaxDurability();
    
    if (TotalMaxDurability != 0.f)
    {
        SetCurrentDurability(TotalMaxDurability);
    }
    
    
    // Calcula valor de mercado inicial
    RecalculateMarketValue();

    UE_LOG(LogZfInventory, Log, TEXT("UZfItemInstance::InitializeItemInstance — " "Item '%s' inicializado. GUID: %s | Tier: %d | Rarity: %s"),
        *InItemDefinition->ItemName.ToString(),
        *ItemGuid.ToString(),
        ItemTier,
        *UEnum::GetValueAsString(ItemRarity));
}

void UZfItemInstance::InitializeDurability()
{
    Internal_RecalculateTotalMaxDurability();
    SetCurrentDurability(TotalMaxDurability);
}

// ============================================================
// ATUALIZAÇÃO DOS ATRIBUTOS
// ============================================================

void UZfItemInstance::RecalculateItemAttributes()
{
    ItemAttributes.Empty();

    // Busca o fragment que define os atributos por qualidade
    const UZfFragment_QualityAttributes* QualityFragment =
        GetFragment<UZfFragment_QualityAttributes>();

    if (!QualityFragment)
        return;

    // Busca o DataTable de modifiers internamente
    UDataTable* ModifierDataTable = nullptr;
    const UZfFragment_Modifiers* ModifierFragment =
        GetFragment<UZfFragment_Modifiers>();

    if (ModifierFragment)
        ModifierDataTable = ModifierFragment->ModifierConfig.ModifierDataTable.LoadSynchronous();

    // Calcula os valores para cada atributo configurado no fragment
    for (const FZfQualityAttributesEntry& Entry : QualityFragment->Entries)
    {
        FZfItemAttributeValue AttributeValue;
        AttributeValue.AttributeTag  = Entry.AttributeTag;
        AttributeValue.DisplayName   = Entry.DisplayName;
        AttributeValue.BaseValue     = Entry.GetValueForQuality(CurrentQuality);
        AttributeValue.ModifierBonus = 0.f;
        AttributeValue.FinalValue    = AttributeValue.BaseValue;

        // Calcula o bônus dos modifiers que afetam esta tag
        if (ModifierDataTable)
        {
            for (const FZfAppliedModifier& Mod : AppliedModifiers)
            {
                // Busca os dados do modifier no DataTable
                const FZfModifierDataTypes* ModData =
                    ModifierDataTable->FindRow<FZfModifierDataTypes>(
                        Mod.ModifierRowName, TEXT("RecalculateItemAttributes"));

                if (!ModData) continue;

                // Ignora modifiers que não afetam este atributo
                if (ModData->AffectedAttributeTag != Entry.AttributeTag) continue;

                switch (ModData->OperationType)
                {
                // Soma flat ao bonus
                case EZfModifierOperationType::Additive:
                    AttributeValue.ModifierBonus += Mod.CurrentValue;
                    break;

                // Soma percentual do BaseValue ao bonus
                case EZfModifierOperationType::MultiplyBase:
                    AttributeValue.ModifierBonus +=
                        AttributeValue.BaseValue * (Mod.CurrentValue / 100.f);
                    break;

                // Substitui o FinalValue completamente e para o cálculo
                case EZfModifierOperationType::Override:
                    AttributeValue.ModifierBonus = 0.f;
                    AttributeValue.FinalValue    = Mod.CurrentValue;
                    ItemAttributes.Add(AttributeValue);
                    goto NextEntry;
                }
            }

            // Calcula o FinalValue com o bonus acumulado
            AttributeValue.FinalValue =
                AttributeValue.BaseValue + AttributeValue.ModifierBonus;
        }

        ItemAttributes.Add(AttributeValue);

        UE_LOG(LogZfInventory, Log,
            TEXT("RecalculateItemAttributes: %s | Base=%.1f | ModBonus=%.1f | Final=%.1f"),
            *Entry.AttributeTag.ToString(),
            AttributeValue.BaseValue,
            AttributeValue.ModifierBonus,
            AttributeValue.FinalValue);

        NextEntry:;
    }
}


// ============================================================
// FUNÇÕES DE ACESSO AOS DADOS
// ============================================================

UZfItemFragment* UZfItemInstance::GetFragmentByClass(TSubclassOf<UZfItemFragment> FragmentClass) const
{
    if (!ItemDefinition || !FragmentClass)
        return nullptr;

    return ItemDefinition->FindFragmentByClass(FragmentClass);
}

FText UZfItemInstance::GetItemName() const
{
    if (!ItemDefinition)
    {
        UE_LOG(LogZfInventory, Warning, TEXT("UZfItemInstance::GetItemName — ItemDefinition é nulo. GUID: %s"), *ItemGuid.ToString());
        return FText::GetEmpty();
    }
    return ItemDefinition->ItemName;
}

float UZfItemInstance::GetCurrentDurability() const
{
    return CurrentDurability;
}

float UZfItemInstance::GetBonusMaxDurability() const
{
    return BonusMaxDurability;
}

float UZfItemInstance::GetTotalMaxDurability() const
{
    return TotalMaxDurability;
}

FGameplayTagContainer UZfItemInstance::GetItemTags() const
{
    if (!ItemDefinition)
    {
        return FGameplayTagContainer::EmptyContainer;
    }
    return ItemDefinition->ItemTags;
}

bool UZfItemInstance::HasItemTag(const FGameplayTag& Tag) const
{
    if (!ItemDefinition)
    {
        return false;
    }
    return ItemDefinition->HasItemTag(Tag);
}

// ============================================================
// STACK
// ============================================================

void UZfItemInstance::SetCurrentStack(int32 NewStack)
{
    if (!Internal_CheckIsServer(TEXT("SetCurrentStack")))
    {
        return;
    }

    // Busca o fragment de stack para obter o limite máximo
    const UZfFragment_Stackable* StackFragment = GetFragment<UZfFragment_Stackable>();
    const int32 MaxStack = StackFragment ? StackFragment->MaxStackSize : 1;

    // Clamp entre 1 e o máximo permitido
    CurrentStack = FMath::Clamp(NewStack, 1, MaxStack);

    UE_LOG(LogZfInventory, Verbose, TEXT("UZfItemInstance::SetCurrentStack — " "GUID: %s | Stack: %d/%d"),
        *ItemGuid.ToString(), CurrentStack, MaxStack);
}



int32 UZfItemInstance::AddToStack(int32 AmountToAdd)
{
    if (!Internal_CheckIsServer(TEXT("AddToStack")))
    {
        return AmountToAdd;
    }

    if (AmountToAdd <= 0)
    {
        return 0;
    }

    const UZfFragment_Stackable* StackFragment = GetFragment<UZfFragment_Stackable>();
    const int32 MaxStack = StackFragment ? StackFragment->MaxStackSize : 1;

    // Calcula quanto realmente cabe
    const int32 SpaceAvailable = MaxStack - CurrentStack;
    const int32 AmountAdded = FMath::Min(AmountToAdd, SpaceAvailable);
    const int32 Overflow = AmountToAdd - AmountAdded;

    CurrentStack += AmountAdded;

    UE_LOG(LogZfInventory, Verbose, TEXT("UZfItemInstance::AddToStack — " "GUID: %s | Adicionado: %d | Stack: %d/%d | Overflow: %d"),
        *ItemGuid.ToString(), AmountAdded, CurrentStack, MaxStack, Overflow);

    // Retorna o que não coube
    return Overflow;
}

bool UZfItemInstance::RemoveFromStack(int32 AmountToRemove)
{
    if (!Internal_CheckIsServer(TEXT("RemoveFromStack")))
    {
        return false;
    }

    if (AmountToRemove <= 0)
    {
        return false;
    }

    CurrentStack = FMath::Max(0, CurrentStack - AmountToRemove);

    UE_LOG(LogZfInventory, Verbose, TEXT("UZfItemInstance::RemoveFromStack — " "GUID: %s | Removido: %d | Stack restante: %d"),
        *ItemGuid.ToString(), AmountToRemove, CurrentStack);

    // Retorna true se o stack zerou (item deve ser removido do inventário)
    return CurrentStack <= 0;
}

// ============================================================
// DURABILIDADE
// ============================================================

void UZfItemInstance::ApplyDurabilityDamage(float DamageAmount)
{
    if (!Internal_CheckIsServer(TEXT("ApplyDurabilityDamage")))
    {
        return;
    }

    // Item sem fragment de durabilidade não sofre dano
    if (!HasFragment<UZfFragment_Durability>())
    {
        return;
    }

    // Item já quebrado não sofre mais dano
    if (bIsBroken)
    {
        return;
    }

    const float PreviousDurability = CurrentDurability;
    SetCurrentDurability(FMath::Max(0.f, CurrentDurability - DamageAmount));

    UE_LOG(LogZfInventory, Verbose, TEXT("UZfItemInstance::ApplyDurabilityDamage — " 
        "GUID: %s | Dano: %.1f | Durabilidade: %.1f → %.1f"),
        *ItemGuid.ToString(), DamageAmount, PreviousDurability, CurrentDurability);

    // Se zerou, quebra o item
    if (CurrentDurability <= 0.0f && !bIsBroken)
    {
        bIsBroken = true;
        NotifyFragments_ItemBroken();

        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfItemInstance::ApplyDurabilityDamage — "
                 "Item '%s' (GUID: %s) quebrou!"),
            *GetItemName().ToString(), *ItemGuid.ToString());
    }
}

void UZfItemInstance::RepairItem()
{
    if (!Internal_CheckIsServer(TEXT("RepairItem")))
    {
        return;
    }

    if (!bIsRepairable)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfItemInstance::RepairItem — "
                 "Item GUID: %s não pode ser reparado (bIsRepairable = false)."),
            *ItemGuid.ToString());
        return;
    }

    const UZfFragment_Durability* DurabilityFragment =
        GetFragment<UZfFragment_Durability>();

    if (!DurabilityFragment)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfItemInstance::RepairItem — "
                 "Item GUID: %s não tem UZfFragment_Durability."),
            *ItemGuid.ToString());
        return;
    }

    const bool bWasBroken = bIsBroken;

    // Restaura para o máximo definido no fragment
    CurrentDurability = DurabilityFragment->MaxDurability;
    bIsBroken = false;

    // Notifica fragments apenas se estava quebrado
    if (bWasBroken)
    {
        NotifyFragments_ItemRepaired();
    }

    UE_LOG(LogZfInventory, Log,
        TEXT("UZfItemInstance::RepairItem — "
             "Item '%s' (GUID: %s) reparado para %.1f de durabilidade."),
        *GetItemName().ToString(), *ItemGuid.ToString(), CurrentDurability);
}

void UZfItemInstance::RepairItemByAmount(float RepairAmount)
{
    if (!Internal_CheckIsServer(TEXT("RepairItemByAmount")))
    {
        return;
    }

    if (!bIsRepairable)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfItemInstance::RepairItemByAmount — "
                 "Item GUID: %s não pode ser reparado."),
            *ItemGuid.ToString());
        return;
    }

    const UZfFragment_Durability* DurabilityFragment =
        GetFragment<UZfFragment_Durability>();

    if (!DurabilityFragment)
    {
        return;
    }

    const bool bWasBroken = bIsBroken;
    const float PreviousDurability = CurrentDurability;

    // Restaura pela quantidade solicitada, sem exceder o máximo
    CurrentDurability = FMath::Min(
        CurrentDurability + RepairAmount,
        DurabilityFragment->MaxDurability);

    // Se saiu do estado de quebrado, notifica os fragments
    if (bWasBroken && CurrentDurability > 0.0f)
    {
        bIsBroken = false;
        NotifyFragments_ItemRepaired();
    }

    UE_LOG(LogZfInventory, Verbose,
        TEXT("UZfItemInstance::RepairItemByAmount — "
             "GUID: %s | Reparado: %.1f | Durabilidade: %.1f → %.1f"),
        *ItemGuid.ToString(), RepairAmount,
        PreviousDurability, CurrentDurability);
}

// ============================================================
// QUALIDADE
// ============================================================






// ============================================================
// MARKET VALUE
// ============================================================

void UZfItemInstance::RecalculateMarketValue()
{
    if (!ItemDefinition)
    {
        return;
    }

    // Valor base do item definido no PDA
    float FinalValue = ItemDefinition->BaseMarketValue;

    // Multiplicador por raridade
    const int32 RarityIndex = static_cast<int32>(ItemRarity);
    const FZfMarketValueConfig& MarketConfig =
        FZfMarketValueConfig();

    if (MarketConfig.RarityMultipliers.IsValidIndex(RarityIndex))
    {
        FinalValue *= MarketConfig.RarityMultipliers[RarityIndex];
    }

    // Multiplicador por tier
    if (MarketConfig.TierMultipliers.IsValidIndex(ItemTier))
    {
        FinalValue *= MarketConfig.TierMultipliers[ItemTier];
    }

    // Bônus por rank médio dos modifiers
    if (AppliedModifiers.Num() > 0)
    {
        float TotalRankSum = 0.0f;
        for (const FZfAppliedModifier& Modifier : AppliedModifiers)
        {
            TotalRankSum += static_cast<float>(Modifier.CurrentRank);
        }
        const float AverageRank = TotalRankSum / AppliedModifiers.Num();
        FinalValue += AverageRank * MarketConfig.ModifierRankValueMultiplier;
    }

    CalculatedMarketValue = FinalValue;

    UE_LOG(LogZfInventory, Verbose,
        TEXT("UZfItemInstance::RecalculateMarketValue — "
             "GUID: %s | Valor: %.2f"),
        *ItemGuid.ToString(), CalculatedMarketValue);
}

// ============================================================
// MODIFIERS
// ============================================================

bool UZfItemInstance::AddAppliedModifier(const FZfAppliedModifier& NewModifier)
{
    if (!Internal_CheckIsServer(TEXT("AddAppliedModifier")))
    {
        return false;
    }

    //Verifica de tem Fragmento de Modifiers
    if (HasFragment<UZfFragment_Modifiers>())
    {
        UE_LOG(LogZfInventory, Warning, TEXT("UZfItemInstance::AddAppliedModifier — " "Não Possui o Fragmento de Modifiers."));
        return false;
    }
    
    // Verifica se já existe modifier com o mesmo nome (sem duplicatas)
    if (HasAppliedModifier(NewModifier.ModifierRowName))
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfItemInstance::AddAppliedModifier — " "Modifier '%s' já existe no item GUID: %s."),
            *NewModifier.ModifierRowName.ToString(), *ItemGuid.ToString());
        return false;
    }

    // Verifica limite total de modifiers
    if (!ItemDefinition)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfItemInstance::AddAppliedModifier — " "Modifier '%s' já existe no item GUID: %s."),
            *NewModifier.ModifierRowName.ToString(), *ItemGuid.ToString());
        return false;
    }

    const UZfFragment_Modifiers* Modifiers = GetFragment<UZfFragment_Modifiers>();
    const int32 MaxTotal = ZfModifierRangeByRarity.Find(ItemRarity)->Max;
    
    if (AppliedModifiers.Num() >= MaxTotal)
    {
        UE_LOG(LogZfInventory, Warning, TEXT("UZfItemInstance::AddAppliedModifier — " "Item GUID: %s atingiu o limite máximo de modifiers (%d)."),
                *ItemGuid.ToString(), MaxTotal);
        return false;
    }
        
    
    // Adiciona o modifier
    AppliedModifiers.Add(NewModifier);

    // Recalcula o valor de mercado
    RecalculateMarketValue();

    UE_LOG(LogZfInventory, Log,
        TEXT("UZfItemInstance::AddAppliedModifier — " "Modifier '%s' adicionado ao item GUID: %s. " "Total: %d/%d"),
        *NewModifier.ModifierRowName.ToString(), *ItemGuid.ToString(), AppliedModifiers.Num(), MaxTotal);

    return true;
}

bool UZfItemInstance::RemoveAppliedModifier(const FName& ModifierRowName)
{
    if (!Internal_CheckIsServer(TEXT("RemoveAppliedModifier")))
    {
        return false;
    }

    const int32 RemovedCount = AppliedModifiers.RemoveAll(
        [&ModifierRowName](const FZfAppliedModifier& Modifier)
        {
            return Modifier.ModifierRowName == ModifierRowName;
        });

    if (RemovedCount > 0)
    {
        RecalculateMarketValue();

        UE_LOG(LogZfInventory, Log,
            TEXT("UZfItemInstance::RemoveAppliedModifier — "
                 "Modifier '%s' removido do item GUID: %s."),
            *ModifierRowName.ToString(), *ItemGuid.ToString());

        return true;
    }

    UE_LOG(LogZfInventory, Warning,
        TEXT("UZfItemInstance::RemoveAppliedModifier — "
             "Modifier '%s' não encontrado no item GUID: %s."),
        *ModifierRowName.ToString(), *ItemGuid.ToString());

    return false;
}

void UZfItemInstance::RemoveAllNonDebuffModifiers()
{
    if (!Internal_CheckIsServer(TEXT("RemoveAllNonDebuffModifiers")))
    {
        return;
    }

    const int32 RemovedCount = AppliedModifiers.RemoveAll(
        [](const FZfAppliedModifier& Modifier)
        {
            // Remove apenas os que NÃO são debuff
            return !Modifier.bIsDebuffModifier;
        });

    if (RemovedCount > 0)
    {
        RecalculateMarketValue();

        UE_LOG(LogZfInventory, Log,
            TEXT("UZfItemInstance::RemoveAllNonDebuffModifiers — "
                 "Removidos %d modifiers do item GUID: %s."),
            RemovedCount, *ItemGuid.ToString());
    }
}


const FZfAppliedModifier& UZfItemInstance::FindAppliedModifier(const FName& ModifierRowName)
{
    return *AppliedModifiers.FindByPredicate([&ModifierRowName](const FZfAppliedModifier& Modifier)
        {
            return Modifier.ModifierRowName == ModifierRowName;
        });
}


bool UZfItemInstance::HasAppliedModifier(const FName& ModifierRowName) const
{
    return AppliedModifiers.ContainsByPredicate(
        [&ModifierRowName](const FZfAppliedModifier& Modifier)
        {
            return Modifier.ModifierRowName == ModifierRowName;
        });
}

bool UZfItemInstance::CanAddModifierOfClass(EZfModifierClass ModifierClass) const
{
    if (!ItemDefinition)
    {
        return false;
    }

    //Verifica de tem Fragmento de Modifiers
    if (HasFragment<UZfFragment_Modifiers>())
    {
        UE_LOG(LogZfInventory, Warning, TEXT("UZfItemInstance::AddAppliedModifier — " "Não Possui o Fragmento de Modifiers."));
        return false;
    }
    
    // Verifica limite total

    const UZfFragment_Modifiers* Modifiers = GetFragment<UZfFragment_Modifiers>();
    const int32 MaxTotal = ZfModifierRangeByRarity.Find(ItemRarity)->Max;
    if (AppliedModifiers.Num() >= MaxTotal)
    {
        return false;
    }

    // Verifica limite por classe
    const int32 ClassLimit = Modifiers->ModifierConfig.GetClassLimit(ModifierClass);
    const int32 CurrentClassCount = CountModifiersOfClass(ModifierClass);

    return CurrentClassCount < ClassLimit;
}

int32 UZfItemInstance::CountModifiersOfClass(EZfModifierClass ModifierClass) const
{
    // Para contar por classe precisamos do DataTable
    // Por ora retorna a contagem total — será expandido
    // quando o ModifierSubsystem for implementado
    int32 Count = 0;
    for (const FZfAppliedModifier& Modifier : AppliedModifiers)
    {
        // A classe do modifier é obtida via DataTable em runtime
        // Implementação completa no ModifierSubsystem (Parte posterior)
        if (Modifier.ModifierClass == ModifierClass)
        {
            Count++;
        }
    }
    return Count;
}

// ============================================================
// CORRUPÇÃO
// ============================================================

bool UZfItemInstance::CorruptItem()
{
    if (!Internal_CheckIsServer(TEXT("CorruptItem")))
    {
        return false;
    }

    if (IsCorrupted())
    {
        UE_LOG(LogZfInventory, Warning, TEXT("UZfItemInstance::CorruptItem — " "Item GUID: %s já está corrompido."), *ItemGuid.ToString());
        return false;
    }

    // Marca como corrompido
    // O modifier de debuff será adicionado pelo ModifierSubsystem
    // que selecionará aleatoriamente um debuff compatível
    CorruptionState = EZfCorruptionState::Corrupted;

    UE_LOG(LogZfInventory, Log,
        TEXT("UZfItemInstance::CorruptItem — "
             "Item '%s' (GUID: %s) foi corrompido."),
        *GetItemName().ToString(), *ItemGuid.ToString());

    return true;
}

// ============================================================
// MISC
// ============================================================

void UZfItemInstance::SetIsRepairable(bool bRepairable)
{
    if (!Internal_CheckIsServer(TEXT("SetIsRepairable")))
    {
        return;
    }

    bIsRepairable = bRepairable;

    UE_LOG(LogZfInventory, Log,
        TEXT("UZfItemInstance::SetIsRepairable — " "GUID: %s | bIsRepairable: %s"),
        *ItemGuid.ToString(), bIsRepairable ? TEXT("true") : TEXT("false"));
}


void UZfItemInstance::SetRarity(EZfItemRarity NewRarity)
{
    ItemRarity = NewRarity;
}

void UZfItemInstance::SetTier(int32 NewTier)
{
    ItemTier = NewTier;
}

void UZfItemInstance::SetItemDefinition(UZfItemDefinition* InItemDefinition)
{
    ItemDefinition = InItemDefinition;
}

void UZfItemInstance::SetAppliedModifiers(const TArray<FZfAppliedModifier>& NewModifiers)
{
    AppliedModifiers = NewModifiers;
    RecalculateItemAttributes();
}

void UZfItemInstance::SetQuality(int32 NewQuality)
{
    CurrentQuality = NewQuality;
    RecalculateItemAttributes();
}

// ============================================================
// NOTIFICAÇÃO AOS FRAGMENTS
// ============================================================

void UZfItemInstance::NotifyFragments_ItemAddedToInventory(UZfInventoryComponent* InventoryComponent)
{
    if (!ItemDefinition)
    {
        return;
    }

    for (TObjectPtr<UZfItemFragment>& Fragment : ItemDefinition->Fragments)
    {
        if (Fragment)
        {
            Fragment->OnItemAddedToInventory(this, InventoryComponent);
        }
    }
}

void UZfItemInstance::NotifyFragments_ItemRemovedFromInventory(UZfInventoryComponent* InventoryComponent)
{
    if (!ItemDefinition)
    {
        return;
    }

    for (TObjectPtr<UZfItemFragment>& Fragment : ItemDefinition->Fragments)
    {
        if (Fragment)
        {
            Fragment->OnItemRemovedFromInventory(this, InventoryComponent);
        }
    }
}

void UZfItemInstance::NotifyFragments_ItemEquipped(UZfEquipmentComponent* EquipmentComponent, AActor* EquippingActor)
{
    if (!ItemDefinition)
    {
        return;
    }

    for (TObjectPtr<UZfItemFragment>& Fragment : ItemDefinition->Fragments)
    {
        if (Fragment)
        {
            Fragment->OnItemEquipped(this, EquipmentComponent, EquippingActor);
        }
    }
}

void UZfItemInstance::NotifyFragments_ItemUnequipped(UZfEquipmentComponent* EquipmentComponent, AActor* UnequippingActor)
{
    if (!ItemDefinition)
    {
        return;
    }

    for (TObjectPtr<UZfItemFragment>& Fragment : ItemDefinition->Fragments)
    {
        if (Fragment)
        {
            Fragment->OnItemUnequipped(this, EquipmentComponent, UnequippingActor);
        }
    }
}

void UZfItemInstance::NotifyFragments_ItemBroken()
{
    if (!ItemDefinition)
    {
        return;
    }

    for (TObjectPtr<UZfItemFragment>& Fragment : ItemDefinition->Fragments)
    {
        if (Fragment)
        {
            Fragment->OnItemBroken(this);
        }
    }
}

void UZfItemInstance::NotifyFragments_ItemRepaired()
{
    if (!ItemDefinition)
    {
        return;
    }

    for (TObjectPtr<UZfItemFragment>& Fragment : ItemDefinition->Fragments)
    {
        if (Fragment)
        {
            Fragment->OnItemRepaired(this);
        }
    }
}

// ============================================================
// FUNÇÕES INTERNAS
// ============================================================


void UZfItemInstance::Internal_InitializeUniqueModifiers()
{
    if (!ItemDefinition)
    {
        return;
    }

    // Busca o fragment de item único
    const UZfFragment_ItemUnique* UniqueFragment = ItemDefinition->FindFragment<UZfFragment_ItemUnique>();

    if (!UniqueFragment)
    {
        return;
    }

    // Itera cada handle selecionado no editor
    for (const FDataTableRowHandle& Handle : UniqueFragment->UniqueModifiers)
    {
        // Valida se o handle tem DataTable e linha configurados
        if (Handle.IsNull())
        {
            UE_LOG(LogZfInventory, Warning, TEXT("UZfItemInstance::Internal_InitializeUniqueModifiers — "
                "Handle nulo encontrado. Verifique o Fragment ItemUnique no editor."));
            continue;
        }

        // Busca os dados da linha no DataTable
        const FZfModifierDataTypes* ModifierRow = Handle.GetRow<FZfModifierDataTypes>(TEXT("Internal_InitializeUniqueModifiers"));

        if (!ModifierRow)
        {
            UE_LOG(LogZfInventory, Warning, TEXT("UZfItemInstance::Internal_InitializeUniqueModifiers — " "Linha '%s' não encontrada no DataTable '%s'."),
                *Handle.RowName.ToString(), Handle.DataTable ? *Handle.DataTable->GetName() : TEXT("None"));
            continue;
        }

        // Cria o FZfAppliedModifier a partir dos dados da linha
        FZfAppliedModifier NewModifier;
        NewModifier.ModifierRowName      = Handle.RowName;
        NewModifier.ModifierClass        = ModifierRow->ModifierClass;
        NewModifier.bIsDebuffModifier    = ModifierRow->bIsDebuffModifier;
        NewModifier.AffectedAttributeTag = ModifierRow->AffectedAttributeTag;
        NewModifier.GameplayEffect       = ModifierRow->GameplayEffect;
        NewModifier.TargetType           = ModifierRow->TargetType;
        NewModifier.ItemPropertyTag      = ModifierRow->ItemPropertyTag;
        
        // Itens únicos sempre usam o rank máximo disponível
        const int32 MaxRankIndex = ModifierRow->ArrayRanks.Num() - 1;
        if (ModifierRow->ArrayRanks.IsValidIndex(MaxRankIndex))
        {
            const FZfModifierRankData& RankData = ModifierRow->ArrayRanks[MaxRankIndex];

            NewModifier.CurrentRank           = RankData.RankLevel;
            NewModifier.CurrentValue          = RankData.RankRange.MaxValue;
            NewModifier.CurrentRollPercentage = 1.0f;
            NewModifier.MaxRollPercentage     = 1.0f;
            NewModifier.FinalValue   = NewModifier.CurrentValue;
            NewModifier.AppliedValue = 0.f;
        }
        else
        {
            UE_LOG(LogZfInventory, Warning, TEXT("UZfItemInstance::Internal_InitializeUniqueModifiers — "
                "Modifier '%s' não tem Ranks configurados no DataTable."),*Handle.RowName.ToString());
            continue;
        }

        // Adiciona o modifier ao item
        AppliedModifiers.Add(NewModifier);

        UE_LOG(LogZfInventory, Log, TEXT("UZfItemInstance::Internal_InitializeUniqueModifiers — "
                 "Modifier único '%s' | Rank: %d | Valor: %.2f | GUID: %s"),
            *Handle.RowName.ToString(), NewModifier.CurrentRank, NewModifier.CurrentValue, *ItemGuid.ToString());
    }
    
}

bool UZfItemInstance::Internal_CheckIsServer(const FString& FunctionName) const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogZfInventory, Error, TEXT("UZfItemInstance::%s — GetWorld() retornou nulo."), *FunctionName);
        return false;
    }

    // Verifica se está no servidor ou no cliente
    if (World->GetNetMode() == NM_Client)
    {
        UE_LOG(LogZfInventory, Warning, TEXT("UZfItemInstance::%s — " "Operação de servidor chamada no cliente! GUID: %s"),
            *FunctionName, *ItemGuid.ToString());
        return false;
    }

    return true;
}

void UZfItemInstance::Internal_RecalculateTotalMaxDurability()
{
    float BaseMaxDurability = 0.f;

    if (const UZfFragment_Durability* DurabilityFragment = GetFragment<UZfFragment_Durability>())
    {
        BaseMaxDurability = DurabilityFragment->MaxDurability;
    }

    TotalMaxDurability = BaseMaxDurability + BonusMaxDurability;

    UE_LOG(LogZfInventory, Log, TEXT("UZfItemInstance::Internal_RecalculateTotalMaxDurability — "
        "Base=%.1f | Bonus=%.1f | Total=%.1f | GUID: %s"),
        BaseMaxDurability, BonusMaxDurability, TotalMaxDurability,
        *ItemGuid.ToString());
}

// ============================================================
// APLICAÇÃO DE MODIFIER EM PROPRIEDADE DO ITEM
// ============================================================

void UZfItemInstance::ApplyPropertyModifier(const FGameplayTag& PropertyTag, float Value)
{
    if (!Internal_CheckIsServer(TEXT("ApplyPropertyModifier")))
    {
        return;
    }
    
    // "Item.Property.MaxDurability" — adiciona bônus ao teto máximo
    const FGameplayTag Tag_MaxDurability = ZfItemPropertyTags::Item_MaxDurability;

     if (PropertyTag == Tag_MaxDurability)
    {
         // Guard contra divisão por zero
         const float NormalizedNewDurability = (TotalMaxDurability > 0.f) ? CurrentDurability / TotalMaxDurability : 1.f; 
         
        BonusMaxDurability += Value;
         
        Internal_RecalculateTotalMaxDurability();
         
        const float NewDurability = NormalizedNewDurability * TotalMaxDurability;
        SetCurrentDurability(NewDurability);

        UE_LOG(LogZfInventory, Log, TEXT("UZfItemInstance::ApplyPropertyModifier — "
            "MaxDurability bonus +%.1f | Total Bonus: %.1f | GUID: %s"),
            Value, BonusMaxDurability, *ItemGuid.ToString());
    }
    else
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfItemInstance::ApplyPropertyModifier — "
            "Tag '%s' não reconhecida. Adicione o case correspondente. GUID: %s"),
            *PropertyTag.ToString(), *ItemGuid.ToString());
    }
}

void UZfItemInstance::RevertPropertyModifier(const FGameplayTag& PropertyTag, float AppliedValue)
{
    if (!Internal_CheckIsServer(TEXT("RevertPropertyModifier")))
    {
        return;
    }
    
    const FGameplayTag Tag_MaxDurability = ZfItemPropertyTags::Item_MaxDurability;
    
    if (PropertyTag == Tag_MaxDurability)
    {
        // Guard contra divisão por zero
        const float NormalizedNewDurability = (TotalMaxDurability > 0.f) ? CurrentDurability / TotalMaxDurability : 1.f; 
        
        // BonusMaxDurability nunca vai abaixo de zero
        BonusMaxDurability = FMath::Max(0.f, BonusMaxDurability - AppliedValue);
        
        Internal_RecalculateTotalMaxDurability();
        
        const float NewDurability = NormalizedNewDurability * TotalMaxDurability;
        SetCurrentDurability(NewDurability);

        UE_LOG(LogZfInventory, Log, TEXT("UZfItemInstance::RevertPropertyModifier — "
            "MaxDurability bonus -%.1f | Total Bonus: %.1f | GUID: %s"),
            AppliedValue, BonusMaxDurability, *ItemGuid.ToString());
    }
    else
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfItemInstance::RevertPropertyModifier — "
                 "Tag '%s' não reconhecida. GUID: %s"),
            *PropertyTag.ToString(), *ItemGuid.ToString());
    }
}


// ============================================================
// DEBUG
// ============================================================
