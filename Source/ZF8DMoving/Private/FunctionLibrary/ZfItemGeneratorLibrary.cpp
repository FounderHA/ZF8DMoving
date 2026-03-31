// Copyright ZfGame Studio. All Rights Reserved.

#include "FunctionLibrary/ZfItemGeneratorLibrary.h"

#include <cmath>

#include "Inventory/ZfItemInstance.h"
#include "Inventory/ZfItemDefinition.h"
#include "Inventory/Fragments/ZfFragment_Modifiers.h"

// ------------------------------------------------------------
// HELPER INTERNO
// ------------------------------------------------------------

class UZfFragment_Modifiers;
// Rola um índice aleatório baseado em um array de pesos
static int32 RollIndexByWeights(const TArray<float>& Weights)
{
    float Total = 0.f;
    for (float W : Weights) Total += W;

    float Roll = FMath::FRandRange(0.f, Total);
    float Acc  = 0.f;

    for (int32 i = 0; i < Weights.Num(); i++)
    {
        Acc += Weights[i];
        if (Roll <= Acc) return i;
    }

    return Weights.Num() - 1;
}

// ------------------------------------------------------------
// RARIDADE
// ------------------------------------------------------------

EZfItemRarity UZfItemGeneratorLibrary::RollRarity(const TArray<FZfRarityWeight>& RarityWeights)
{
    const TArray<FZfRarityWeight>& Table =
        RarityWeights.IsEmpty() ? GDefaultRarityWeights : RarityWeights;

    float TotalWeight = 0.f;
    for (const FZfRarityWeight& Entry : Table)
        TotalWeight += Entry.Weight;

    float Roll = FMath::FRandRange(0.f, TotalWeight);
    float Accumulated = 0.f;

    for (const FZfRarityWeight& Entry : Table)
    {
        Accumulated += Entry.Weight;
        if (Roll <= Accumulated)
            return Entry.Rarity;
    }

    return EZfItemRarity::Common;
}

// ------------------------------------------------------------
// TIER
// ------------------------------------------------------------

int32 UZfItemGeneratorLibrary::RollTier(const TArray<FZfTierWeight>& TierWeights)
{
    const TArray<FZfTierWeight>& Table =
        TierWeights.IsEmpty() ? GDefaultTierWeights : TierWeights;

    float TotalWeight = 0.f;
    for (const FZfTierWeight& Entry : Table)
        TotalWeight += Entry.Weight;

    float Roll = FMath::FRandRange(0.f, TotalWeight);
    float Accumulated = 0.f;

    for (const FZfTierWeight& Entry : Table)
    {
        Accumulated += Entry.Weight;
        if (Roll <= Accumulated)
            return Entry.Tier;
    }

    return 0;
}

// ------------------------------------------------------------
// MODIFIER COUNT
// ------------------------------------------------------------

int32 UZfItemGeneratorLibrary::RollModifierCount(EZfItemRarity Rarity)
{
    const FZfModifierRange* Range = ZfModifierRangeByRarity.Find(Rarity);
    if (!Range)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("RollModifierCount: Raridade não encontrada, retornando 0."));
        return 0;
    }

    // Começa no mínimo garantido
    int32 Count = Range->Min;

    UE_LOG(LogZfInventory, Log,
        TEXT("RollModifierCount: Raridade=%s | Min=%d | Max=%d | Iniciando com %d modifier(s)."),
        *UEnum::GetValueAsString(Rarity), Range->Min, Range->Max, Count);

    // Chance base de aumentar em 1
    float Chance = 0.75f;

    // Tenta aumentar até chegar no máximo
    while (Count < Range->Max)
    {
        const float Roll = FMath::FRandRange(0.f, 1.f);

        if (Roll <= Chance)
        {
            Count++;
            UE_LOG(LogZfInventory, Log,
                TEXT("RollModifierCount: Sucesso! Roll=%.4f <= Chance=%.4f | Total agora: %d"),
                Roll, Chance, Count);

            Chance *= 0.25f;
        }
        else
        {
            UE_LOG(LogZfInventory, Log,
                TEXT("RollModifierCount: Falhou. Roll=%.4f > Chance=%.4f | Parando em %d modifier(s)."),
                Roll, Chance, Count);
            break;
        }
    }

    if (Count == Range->Max)
    {
        UE_LOG(LogZfInventory, Log,
            TEXT("RollModifierCount: Atingiu o máximo de %d modifier(s)."), Count);
    }

    UE_LOG(LogZfInventory, Log,
        TEXT("RollModifierCount: Total final = %d modifier(s)."), Count);

    return Count;
}

// ------------------------------------------------------------
// RANK
// ------------------------------------------------------------

int32 UZfItemGeneratorLibrary::RollRankForTier(const FZfTierData& TierData)
{
    // Sem pesos configurados não é possível sortear um rank
    if (TierData.RankWeights.IsEmpty())
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("RollRankForTier: Tier %d sem RankWeights configurados, retornando rank 1."),
            TierData.TierLevel);
        return 1;
    }

    // Soma todos os pesos para definir o range do roll
    float TotalWeight = 0.f;
    for (const FZfTierRankWeight& Entry : TierData.RankWeights)
        TotalWeight += Entry.ProbabilityToAppear;

    // Sorteia um valor dentro do range total
    float Roll = FMath::FRandRange(0.f, TotalWeight);
    float Accumulated = 0.f;

    // Retorna o rank cujo peso acumulado supera o roll
    for (const FZfTierRankWeight& Entry : TierData.RankWeights)
    {
        Accumulated += Entry.ProbabilityToAppear;
        if (Roll <= Accumulated)
            return Entry.RankToAppear;
    }

    return TierData.RankWeights.Last().RankToAppear;
}

// ------------------------------------------------------------
// SINGLE MODIFIER
// ------------------------------------------------------------

FZfAppliedModifier UZfItemGeneratorLibrary::RollSingleModifier(const FZfModifierDataTypes& ModifierData, int32 ItemTier)
{
    FZfAppliedModifier Applied;

    // Cacheia informações do modifier para evitar lookup futuro no DataTable
    Applied.ModifierClass     = ModifierData.ModifierClass;
    Applied.bIsDebuffModifier = ModifierData.bIsDebuffModifier;

    // Busca os dados do tier do item neste modifier
    const FZfTierData* TierData = ModifierData.GetTierData(ItemTier);
    if (!TierData)
        return Applied;

    // Sorteia o rank baseado nas probabilidades do tier
    Applied.CurrentRank = RollRankForTier(*TierData);

    // Busca o range de valores do rank sorteado
    const FZfModifierRankData* RankData = ModifierData.GetRankData(Applied.CurrentRank);
    if (!RankData)
        return Applied;

    const float MinVal = RankData->RankRange.MinValue;
    const float MaxVal = RankData->RankRange.MaxValue * RankData->CurrentMaxPercentage;

    // Sorteia o valor dentro do range do rank
    Applied.CurrentValue = FMath::FRandRange(MinVal, MaxVal);
    
    // Arredonda para 1 casa decimal sem erro de precisão de float
    Applied.CurrentValue = std::round(FMath::FRandRange(MinVal, MaxVal) * 10.0) / 10.0;
    
    // Calcula o percentual dentro do range — usado futuramente pelo Modifier UP
    const float Range = MaxVal - MinVal;
    Applied.CurrentRollPercentage = (Range > 0.f)
        ? (Applied.CurrentValue - MinVal) / Range
        : 1.f;

    Applied.MaxRollPercentage = RankData->CurrentMaxPercentage;
    Applied.AwakeningCount    = 0;

    return Applied;
}

// ------------------------------------------------------------
// GENERATE ITEM DATA
// ------------------------------------------------------------

TArray<FZfAppliedModifier> UZfItemGeneratorLibrary::RollModifiers(UDataTable* ModifierDataTable,
        const FGameplayTagContainer& ItemTags, int32 ItemTier, int32 ModifierCount,
        const FZfItemModifierConfig& ModifierConfig)
{
    TArray<FZfAppliedModifier> Result;

    if (!ModifierDataTable)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("RollModifiers: ModifierDataTable nula."));
        return Result;
    }

    if (ModifierCount <= 0)
        return Result;

    // Filtra as linhas do DataTable compatíveis com as tags do item
    TArray<FName> CompatibleRows;
    for (const FName& RowName : ModifierDataTable->GetRowNames())
    {
        const FZfModifierDataTypes* Row =
            ModifierDataTable->FindRow<FZfModifierDataTypes>(RowName, TEXT("RollModifiers"));

        if (Row && Row->IsCompatibleWithAnyItemTag(ItemTags))
            CompatibleRows.Add(RowName);
    }

    if (CompatibleRows.IsEmpty())
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("RollModifiers: Nenhum modifier compatível com as tags do item."));
        return Result;
    }

    // Controle para evitar duplicatas e respeitar limites por classe
    TSet<FName> UsedRows;
    TMap<EZfModifierClass, int32> ClassCount;

    int32 Attempts = 0;
    const int32 MaxAttempts = ModifierCount * 10;

    while (Result.Num() < ModifierCount && Attempts < MaxAttempts)
    {
        Attempts++;

        // Sorteia uma linha aleatória da lista de compatíveis
        const FName& RowName =
            CompatibleRows[FMath::RandRange(0, CompatibleRows.Num() - 1)];

        // Evita duplicatas
        if (UsedRows.Contains(RowName))
            continue;

        const FZfModifierDataTypes* Row =
            ModifierDataTable->FindRow<FZfModifierDataTypes>(RowName, TEXT("RollModifiers"));
        if (!Row) continue;

        // Verifica limite por classe — definido no ModifierClassLimits do ItemDefinition
        const int32 ClassLimit = ModifierConfig.GetClassLimit(Row->ModifierClass);
        const int32 CurrentClassCount = ClassCount.FindOrAdd(Row->ModifierClass);
        if (CurrentClassCount >= ClassLimit)
            continue;

        // Rola o modifier
        FZfAppliedModifier Applied = RollSingleModifier(*Row, ItemTier);
        Applied.ModifierRowName    = RowName;

        Result.Add(Applied);
        UsedRows.Add(RowName);
        ClassCount[Row->ModifierClass]++;
    }

    if (Result.Num() < ModifierCount)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("RollModifiers: Solicitado %d modifiers mas apenas %d foram gerados. "
                 "Verifique os limites por classe ou a quantidade de modifiers compatíveis."),
            ModifierCount, Result.Num());
    }

    return Result;
}

// ------------------------------------------------------------
// APPLY TO INSTANCE
// ------------------------------------------------------------

void UZfItemGeneratorLibrary::ApplyGenerationToInstance(UZfItemInstance* ItemInstance,
    EZfItemRarity Rarity, int32 Tier, const TArray<FZfAppliedModifier>& Modifiers)
{
    if (!ItemInstance)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("ApplyGenerationToInstance: ItemInstance nulo."));
        return;
    }

    ItemInstance->SetRarity(Rarity);
    ItemInstance->SetTier(Tier);
    ItemInstance->SetAppliedModifiers(Modifiers);
}

UZfItemInstance* UZfItemGeneratorLibrary::GenerateItem(
    UObject* Outer,
    UZfItemDefinition* InItemDefinition,
    const TArray<FZfRarityWeight>& RarityWeights,
    const TArray<FZfTierWeight>& TierWeights)
{
    if (!Outer || !InItemDefinition)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("GenerateItem: Outer ou ItemDefinition nulo."));
        return nullptr;
    }

    // 1 — Sorteia raridade e tier
    const EZfItemRarity Rarity = RollRarity(RarityWeights);
    const int32 Tier           = RollTier(TierWeights);

    // 2 — Sorteia quantidade de modifiers baseado na raridade
    const int32 ModifierCount = RollModifierCount(Rarity);

    // 3 — Busca o DataTable de modifiers no Fragment_Modifiers do ItemDefinition
    TArray<FZfAppliedModifier> Modifiers;
    const UZfFragment_Modifiers* ModifierFragment =
        InItemDefinition->FindFragment<UZfFragment_Modifiers>();

    if (ModifierFragment)
    {
        UDataTable* ModifierTable = ModifierFragment->ModifierConfig.ModifierDataTable.LoadSynchronous();

        if (ModifierTable)
        {
            Modifiers = RollModifiers(ModifierTable, InItemDefinition->ItemTags, Tier,
                ModifierCount, ModifierFragment->ModifierConfig);
        }
        else
        {
            UE_LOG(LogZfInventory, Warning,
                TEXT("GenerateItem: ModifierDataTable nula ou não carregada."));
        }
    }

    // 4 — Cria e preenche o ItemInstance
    UZfItemInstance* NewInstance = NewObject<UZfItemInstance>(Outer);
    NewInstance->SetItemDefinition(InItemDefinition);
    ApplyGenerationToInstance(NewInstance, Rarity, Tier, Modifiers);

    return NewInstance;
}