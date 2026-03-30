// Copyright ZfGame Studio. All Rights Reserved.
// ZfModifierRollSystem.cpp

#include "Items/ZfModifierRollSystem.h"
#include "Inventory/ZfItemInstance.h"
#include "Inventory/ZfItemDefinition.h"
#include "Inventory/Fragments/ZfFragment_Modifiers.h"
#include "Engine/DataTable.h"

// ============================================================
// FUNÇÃO PRINCIPAL
// ============================================================

bool UZfModifierRollSystem::RollModifiersForItem(UZfItemInstance* ItemInstance)
{
    if (!ItemInstance)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfModifierRollSystem::RollModifiersForItem — "
                 "ItemInstance é nulo."));
        return false;
    }

    // Monta o contexto de roll
    FZfModifierRollContext Context;
    if (!Internal_BuildRollContext(ItemInstance, Context))
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfModifierRollSystem::RollModifiersForItem — "
                 "Falha ao montar contexto. Item: '%s'"),
            *ItemInstance->GetItemName().ToString());
        return false;
    }

    // Determina quantos modifiers este item vai receber
    const int32 ModifierCount = RollModifierCountForRarity(Context.ItemRarity);

    if (ModifierCount <= 0)
    {
        UE_LOG(LogZfInventory, Log,
            TEXT("UZfModifierRollSystem::RollModifiersForItem — "
                 "Item '%s' não recebe modifiers para raridade '%s'."),
            *ItemInstance->GetItemName().ToString(),
            *UEnum::GetValueAsString(Context.ItemRarity));
        return true;
    }

    UE_LOG(LogZfInventory, Log,
        TEXT("UZfModifierRollSystem::RollModifiersForItem — "
             "Rolando %d modifier(s) para '%s' | Tier: %d | Rarity: %s"),
        ModifierCount,
        *ItemInstance->GetItemName().ToString(),
        Context.ItemTier,
        *UEnum::GetValueAsString(Context.ItemRarity));

    int32 AppliedCount = 0;

    // Tenta rolar a quantidade determinada de modifiers
    // Limite de tentativas para evitar loop infinito
    // caso não haja modifiers suficientes compatíveis
    const int32 MaxAttempts = ModifierCount * 3;
    int32 Attempts = 0;

    while (AppliedCount < ModifierCount && Attempts < MaxAttempts)
    {
        Attempts++;

        // Busca linhas compatíveis sem filtro de classe
        TArray<FName> CompatibleRows =
            GetCompatibleModifierRows(Context);

        if (CompatibleRows.IsEmpty())
        {
            UE_LOG(LogZfInventory, Warning,
                TEXT("UZfModifierRollSystem::RollModifiersForItem — "
                     "Sem modifiers compatíveis disponíveis após %d aplicados."),
                AppliedCount);
            break;
        }

        // Seleciona uma linha aleatória
        const FName SelectedRow = Internal_SelectRandomRow(CompatibleRows);
        if (SelectedRow == NAME_None)
        {
            continue;
        }

        // Rola e aplica o modifier
        if (Internal_RollAndApplyModifier(Context, SelectedRow))
        {
            AppliedCount++;
        }
    }

    UE_LOG(LogZfInventory, Log,
        TEXT("UZfModifierRollSystem::RollModifiersForItem — "
             "Item '%s' recebeu %d/%d modifier(s)."),
        *ItemInstance->GetItemName().ToString(),
        AppliedCount,
        ModifierCount);

    return AppliedCount > 0;
}

// ============================================================
// FUNÇÕES DE CONSULTA
// ============================================================

int32 UZfModifierRollSystem::RollModifierCountForRarity(EZfItemRarity Rarity)
{
    // Busca o range configurado em ZfInventoryTypes
    const FZfModifierRange* Range = ZfModifierRangeByRarity.Find(Rarity);

    if (!Range)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfModifierRollSystem::RollModifierCountForRarity — "
                 "Raridade '%s' não configurada em ZfModifierRangeByRarity."),
            *UEnum::GetValueAsString(Rarity));
        return 0;
    }

    // Garante que Min <= Max
    const int32 Min = FMath::Min(Range->Min, Range->Max);
    const int32 Max = FMath::Max(Range->Min, Range->Max);

    return FMath::RandRange(Min, Max);
}

int32 UZfModifierRollSystem::RollRankForTier(
    const FZfModifierDataTypes& ModifierRow,
    int32 ItemTier)
{
    // Busca o TierData para o tier do item
    const FZfTierData* TierData = ModifierRow.GetTierData(ItemTier);

    if (!TierData || TierData->RankWeights.IsEmpty())
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfModifierRollSystem::RollRankForTier — "
                 "Tier %d não configurado para modifier '%s'. "
                 "Usando Rank 1 como fallback."),
            ItemTier,
            *ModifierRow.DisplayName.ToString());
        return 1;
    }

    // Normaliza os pesos para garantir soma = 1.0
    const TArray<float> NormalizedWeights =
        Internal_NormalizeTierWeights(*TierData);

    // Rola um valor entre 0.0 e 1.0
    const float Roll = FMath::FRand();

    // Determina qual rank foi sorteado baseado nos pesos
    float AccumulatedWeight = 0.0f;
    for (int32 i = 0; i < TierData->RankWeights.Num(); i++)
    {
        AccumulatedWeight += NormalizedWeights[i];
        if (Roll <= AccumulatedWeight)
        {
            return TierData->RankWeights[i].RankToAppear;
        }
    }

    // Fallback — retorna o último rank configurado
    return TierData->RankWeights.Last().RankToAppear;
}

float UZfModifierRollSystem::RollValueForRank(
    const FZfModifierRankData& RankData)
{
    const float Min = RankData.RankRange.MinValue;
    const float Max = RankData.RankRange.MaxValue;

    if (Min >= Max)
    {
        return Min;
    }

    return FMath::FRandRange(Min, Max);
}

TArray<FName> UZfModifierRollSystem::GetCompatibleModifierRows(
    const FZfModifierRollContext& Context,
    EZfModifierClass ModifierClass)
{
    TArray<FName> CompatibleRows;

    if (!Context.ModifierDataTable)
    {
        return CompatibleRows;
    }

    // Itera todas as linhas do DataTable
    for (const TPair<FName, uint8*>& RowPair :
         Context.ModifierDataTable->GetRowMap())
    {
        const FName RowName = RowPair.Key;
        const FZfModifierDataTypes* Row = reinterpret_cast<const FZfModifierDataTypes*>(RowPair.Value);

        if (!Row)
        {
            continue;
        }

        // Filtra por classe se especificada
        if (ModifierClass != EZfModifierClass::None &&
            Row->ModifierClass != ModifierClass)
        {
            continue;
        }

        // Filtra por compatibilidade de tag com o item
        if (!Row->IsCompatibleWithAnyItemTag(Context.ItemTags))
        {
            continue;
        }

        // Filtra duplicatas — modifier já aplicado
        if (Internal_IsModifierAlreadyApplied(Context, RowName))
        {
            continue;
        }

        // Filtra por limite de classe
        if (Internal_IsClassLimitReached(Context, Row->ModifierClass))
        {
            continue;
        }

        // Filtra modifiers de debuff — só aparecem via Corruption
        if (Row->bIsDebuffModifier)
        {
            continue;
        }

        // Filtra modifiers que requerem corrupção
        if (Row->bRequiresCorruption)
        {
            continue;
        }

        CompatibleRows.Add(RowName);
    }

    return CompatibleRows;
}

// ============================================================
// FUNÇÕES INTERNAS
// ============================================================

bool UZfModifierRollSystem::Internal_BuildRollContext(
    UZfItemInstance* ItemInstance,
    FZfModifierRollContext& OutContext)
{
    if (!ItemInstance || !ItemInstance->GetItemDefinition())
    {
        return false;
    }

    // Busca o Fragment de modifiers
    const UZfFragment_Modifiers* ModifierFragment =
        ItemInstance->GetFragment<UZfFragment_Modifiers>();

    if (!ModifierFragment)
    {
        UE_LOG(LogZfInventory, Verbose,
            TEXT("UZfModifierRollSystem::Internal_BuildRollContext — "
                 "Item '%s' não tem UZfFragment_Modifiers. "
                 "Sem modifiers para rolar."),
            *ItemInstance->GetItemName().ToString());
        return false;
    }

    // Valida o DataTable
    if (ModifierFragment->ModifierConfig.ModifierDataTable.IsNull())
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfModifierRollSystem::Internal_BuildRollContext — "
                 "ModifierDataTable não configurado no Fragment de '%s'."),
            *ItemInstance->GetItemName().ToString());
        return false;
    }

    // Carrega o DataTable sincronamente
    // Aceitável aqui pois ocorre durante a criação do item
    // que já está num contexto de carregamento
    UDataTable* LoadedTable =
        ModifierFragment->ModifierConfig.ModifierDataTable.LoadSynchronous();

    if (!LoadedTable)
    {
        UE_LOG(LogZfInventory, Error,
            TEXT("UZfModifierRollSystem::Internal_BuildRollContext — "
                 "Falha ao carregar ModifierDataTable de '%s'."),
            *ItemInstance->GetItemName().ToString());
        return false;
    }

    // Preenche o contexto
    OutContext.ItemInstance     = ItemInstance;
    OutContext.ModifierDataTable = LoadedTable;
    OutContext.ItemTags          = ItemInstance->GetItemTags();
    OutContext.ItemTier          = ItemInstance->ItemTier;
    OutContext.ItemRarity        = ItemInstance->ItemRarity;
    OutContext.ClassLimits       =
        ModifierFragment->ModifierConfig.ModifierClassLimits;

    return true;
}

bool UZfModifierRollSystem::Internal_RollAndApplyModifier(
    FZfModifierRollContext& Context,
    const FName& RowName)
{
    if (!Context.ModifierDataTable)
    {
        return false;
    }

    // Busca a linha no DataTable
    const FZfModifierDataTypes* Row = Context.ModifierDataTable->FindRow<FZfModifierDataTypes>(
    RowName, TEXT("Internal_RollAndApplyModifier"));

    if (!Row)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfModifierRollSystem::Internal_RollAndApplyModifier — "
                 "Linha '%s' não encontrada no DataTable."),
            *RowName.ToString());
        return false;
    }

    // Determina o rank baseado no tier do item
    const int32 RolledRank = RollRankForTier(*Row, Context.ItemTier);

    // Busca os dados do rank sorteado
    const FZfModifierRankData* RankData = Row->GetRankData(RolledRank);
    if (!RankData)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfModifierRollSystem::Internal_RollAndApplyModifier — "
                 "Rank %d não encontrado para modifier '%s'."),
            RolledRank, *RowName.ToString());
        return false;
    }

    // Rola o valor dentro do range do rank
    const float RolledValue = RollValueForRank(*RankData);

    // Calcula o percentual dentro do range
    const float RangeSize =
        RankData->RankRange.MaxValue - RankData->RankRange.MinValue;

    const float RollPercentage = RangeSize > 0.0f
        ? (RolledValue - RankData->RankRange.MinValue) / RangeSize
        : 0.0f;

    // Monta o FZfAppliedModifier
    FZfAppliedModifier NewModifier;
    NewModifier.ModifierRowName       = RowName;
    NewModifier.ModifierClass         = Row->ModifierClass;
    NewModifier.CurrentRank           = RolledRank;
    NewModifier.CurrentValue          = RolledValue;
    NewModifier.CurrentRollPercentage = RollPercentage;
    NewModifier.MaxRollPercentage     = 1.0f;
    NewModifier.bIsDebuffModifier     = Row->bIsDebuffModifier;

    // Adiciona ao ItemInstance
    Context.ItemInstance->AppliedModifiers.Add(NewModifier);

    UE_LOG(LogZfInventory, Log,
        TEXT("UZfModifierRollSystem::Internal_RollAndApplyModifier — "
             "Modifier '%s' aplicado | Rank: %d | Valor: %.2f (%.0f%%)"),
        *RowName.ToString(),
        RolledRank,
        RolledValue,
        RollPercentage * 100.0f);

    return true;
}

bool UZfModifierRollSystem::Internal_IsModifierAlreadyApplied(
    const FZfModifierRollContext& Context,
    const FName& RowName)
{
    if (!Context.ItemInstance)
    {
        return false;
    }

    return Context.ItemInstance->AppliedModifiers.ContainsByPredicate(
        [&RowName](const FZfAppliedModifier& Modifier)
        {
            return Modifier.ModifierRowName == RowName;
        });
}

bool UZfModifierRollSystem::Internal_IsClassLimitReached(
    const FZfModifierRollContext& Context,
    EZfModifierClass ModifierClass)
{
    // Busca o limite configurado para esta classe
    const FZfModifierClassLimit* ClassLimit = Context.ClassLimits.FindByPredicate(
        [ModifierClass](const FZfModifierClassLimit& Limit)
        {
            return Limit.ModifierClass == ModifierClass;
        });

    // Sem limite configurado = sem restrição
    if (!ClassLimit)
    {
        return false;
    }

    // Conta quantos modifiers desta classe já foram aplicados
    const int32 CurrentCount =
        Internal_CountAppliedModifiersOfClass(Context, ModifierClass);

    return CurrentCount >= ClassLimit->MaxCount;
}

int32 UZfModifierRollSystem::Internal_CountAppliedModifiersOfClass(
    const FZfModifierRollContext& Context,
    EZfModifierClass ModifierClass)
{
    if (!Context.ItemInstance)
    {
        return 0;
    }

    int32 Count = 0;
    for (const FZfAppliedModifier& Modifier :
         Context.ItemInstance->AppliedModifiers)
    {
        if (Modifier.ModifierClass == ModifierClass)
        {
            Count++;
        }
    }
    return Count;
}

FName UZfModifierRollSystem::Internal_SelectRandomRow(
    const TArray<FName>& RowNames)
{
    if (RowNames.IsEmpty())
    {
        return NAME_None;
    }

    // Seleciona um índice aleatório
    const int32 RandomIndex = FMath::RandRange(0, RowNames.Num() - 1);
    return RowNames[RandomIndex];
}

TArray<float> UZfModifierRollSystem::Internal_NormalizeTierWeights(
    const FZfTierData& TierData)
{
    TArray<float> NormalizedWeights;

    if (TierData.RankWeights.IsEmpty())
    {
        return NormalizedWeights;
    }

    // Soma total dos pesos
    float TotalWeight = 0.0f;
    for (const FZfTierRankWeight& Weight : TierData.RankWeights)
    {
        TotalWeight += Weight.ProbabilityToAppear;
    }

    // Normaliza cada peso
    for (const FZfTierRankWeight& Weight : TierData.RankWeights)
    {
        const float Normalized = TotalWeight > 0.0f
            ? Weight.ProbabilityToAppear / TotalWeight
            : 0.0f;
        NormalizedWeights.Add(Normalized);
    }

    return NormalizedWeights;
}