// Copyright ZfGame Studio. All Rights Reserved.
// ZfGatherResourceData.cpp

#include "GatheringSystem/ZfGatheringResourceData.h"
#include "Misc/DataValidation.h"

// ============================================================
// AssetManager
// ============================================================

const FPrimaryAssetType UZfGatheringResourceData::PrimaryAssetType("ZfGatherResourceData");

// ============================================================
// FindToolEntry
// ============================================================

const FZfGatherToolEntry* UZfGatheringResourceData::FindToolEntry(const FGameplayTag& ToolTag) const
{
    for (const FZfGatherToolEntry& Entry : AllowedTools)
    {
        if (Entry.ToolTag == ToolTag)
        {
            return &Entry;
        }
    }
    return nullptr;
}

// ============================================================
// IsToolAllowed
// ============================================================

bool UZfGatheringResourceData::IsToolAllowed(const FGameplayTag& ToolTag) const
{
    return FindToolEntry(ToolTag) != nullptr;
}

// ============================================================
// IsDataValid
// ============================================================

#if WITH_EDITOR
EDataValidationResult UZfGatheringResourceData::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    if (!ResourceTag.IsValid())
    {
        Context.AddWarning(FText::FromString(
            TEXT("ZfGatherResourceData: ResourceTag não configurada.")));
        Result = EDataValidationResult::Invalid;
    }

    if (ResourceName.IsEmpty())
    {
        Context.AddWarning(FText::FromString(
            TEXT("ZfGatherResourceData: ResourceName não pode estar vazio.")));
        Result = EDataValidationResult::Invalid;
    }

    if (ResourceHP <= 0.0f)
    {
        Context.AddError(FText::FromString(
            TEXT("ZfGatherResourceData: ResourceHP deve ser maior que 0.")));
        Result = EDataValidationResult::Invalid;
    }

    if (AllowedTools.IsEmpty())
    {
        Context.AddWarning(FText::FromString(
            TEXT("ZfGatherResourceData: AllowedTools está vazio. "
                 "Adicione ao menos uma ferramenta.")));
        Result = EDataValidationResult::Invalid;
    }

    for (int32 i = 0; i < AllowedTools.Num(); i++)
    {
        const FZfGatherToolEntry& Entry = AllowedTools[i];

        if (!Entry.ToolTag.IsValid())
        {
            Context.AddError(FText::FromString(FString::Printf(
                TEXT("ZfGatherResourceData: AllowedTools[%d] tem ToolTag inválida."), i)));
            Result = EDataValidationResult::Invalid;
        }

        if (Entry.DamageMultiplier <= 0.0f)
        {
            Context.AddError(FText::FromString(FString::Printf(
                TEXT("ZfGatherResourceData: AllowedTools[%d] tem DamageMultiplier <= 0."), i)));
            Result = EDataValidationResult::Invalid;
        }

        if (Entry.GoodSize <= 0.0f || Entry.GoodSize > 1.0f)
        {
            Context.AddError(FText::FromString(FString::Printf(
                TEXT("ZfGatherResourceData: AllowedTools[%d] tem ZoneSize fora do range (0.01 - 1.0)."), i)));
            Result = EDataValidationResult::Invalid;
        }
    }

    if (LootTable.IsEmpty())
    {
        Context.AddWarning(FText::FromString(
            TEXT("ZfGatherResourceData: LootTable está vazia. "
                 "O recurso não irá dropar nenhum item.")));
    }

    for (int32 i = 0; i < LootTable.Num(); i++)
    {
        const FZfGatherLootEntry& Entry = LootTable[i];

        if (Entry.ItemDefinition.IsNull())
        {
            Context.AddError(FText::FromString(FString::Printf(
                TEXT("ZfGatherResourceData: LootTable[%d] tem ItemDefinition nulo."), i)));
            Result = EDataValidationResult::Invalid;
        }

        if (Entry.QuantityMin > Entry.QuantityMax)
        {
            Context.AddError(FText::FromString(FString::Printf(
                TEXT("ZfGatherResourceData: LootTable[%d] tem QuantityMin (%d) maior que QuantityMax (%d)."),
                i, Entry.QuantityMin, Entry.QuantityMax)));
            Result = EDataValidationResult::Invalid;
        }

        if (Entry.DropChance <= 0.0f)
        {
            Context.AddWarning(FText::FromString(FString::Printf(
                TEXT("ZfGatherResourceData: LootTable[%d] tem DropChance <= 0. "
                     "Este item nunca irá dropar."), i)));
        }
    }

    if (NeedleRotationTime < 0.3f)
    {
        Context.AddWarning(FText::FromString(
            TEXT("ZfGatherResourceData: QTEWindowSeconds muito baixo (< 0.3s).")));
    }

    return Result;
}
#endif

// ============================================================
// GetDebugString
// ============================================================

FString UZfGatheringResourceData::GetDebugString() const
{
    FString Debug = FString::Printf(
        TEXT("=== ZfGatherResourceData Debug ===\n")
        TEXT("Name: %s\n")
        TEXT("Tag: %s\n")
        TEXT("ResourceHP: %.1f\n")
        TEXT("QTE Window: %.2fs\n")
        TEXT("Respawn: %s (%.0fs)\n")
        TEXT("AllowedTools (%d):\n"),
        *ResourceName.ToString(),
        *ResourceTag.ToString(),
        ResourceHP,
        NeedleRotationTime,
        bCanRespawn ? TEXT("Sim") : TEXT("Não"),
        RespawnTime,
        AllowedTools.Num());

    for (int32 i = 0; i < AllowedTools.Num(); i++)
    {
        Debug += FString::Printf(
            TEXT("  [%d] Tag: %s | DamageMult: %.2f | ZoneSize: %.2f\n"),
            i,
            *AllowedTools[i].ToolTag.ToString(),
            AllowedTools[i].DamageMultiplier,
            AllowedTools[i].GoodSize);
    }

    Debug += FString::Printf(TEXT("LootTable (%d):\n"), LootTable.Num());

    for (int32 i = 0; i < LootTable.Num(); i++)
    {
        Debug += FString::Printf(
            TEXT("  [%d] Item: %s | Qtd: %d~%d | Chance: %.0f%% | ScoreMin: %.2f\n"),
            i,
            *LootTable[i].ItemDefinition.GetAssetName(),
            LootTable[i].QuantityMin,
            LootTable[i].QuantityMax,
            LootTable[i].DropChance * 100.0f,
            LootTable[i].ScoreMinimum);
    }

    return Debug;
}