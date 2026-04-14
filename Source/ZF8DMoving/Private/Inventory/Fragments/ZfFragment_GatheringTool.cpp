// Copyright ZfGame Studio. All Rights Reserved.
// ZfFragment_GatherTool.cpp

#include "Inventory/Fragments/ZfFragment_GatheringTool.h"
#include "Inventory/ZfItemInstance.h"
#include "Inventory/ZfModifierDataTypes.h"
#include "Tags/ZfGameplayTags.h"
#include "Misc/DataValidation.h"

// ============================================================
// ResolveGatherStats
// ============================================================

FZfResolvedGatherStats UZfFragment_GatheringTool::ResolveGatherStats(const UZfItemInstance* ItemInstance) const
{
    FZfResolvedGatherStats Result;

    // ToolTag deve estar configurada — é a identidade da ferramenta
    if (!ToolTag.IsValid())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ZfFragment_GatherTool::ResolveGatherStats — ToolTag não configurada. "
                 "Verifique o ItemDefinition da ferramenta."));
        return Result; // bIsValid = false
    }

    // ItemInstance deve ser válido para ler os modifiers ativos
    if (!ItemInstance)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ZfFragment_GatherTool::ResolveGatherStats — ItemInstance nulo."));
        return Result; // bIsValid = false
    }

    // ----------------------------------------------------------
    // Popula com os valores BASE do fragment
    // ----------------------------------------------------------

    Result.ToolTag        = ToolTag;
    Result.BaseDamage     = BaseDamage;
    Result.DropMultiplier = BaseDropMultiplier;
    Result.ScoreBonus     = BaseScoreBonus;
    

    // ----------------------------------------------------------
    // Soma os modifiers ativos do ItemInstance
    // Filtra por TargetType == ItemProperty e tags Gather.*
    // ----------------------------------------------------------

    const FGameplayTag Tag_BonusDamage    = ZfGatheringTags::ItemProperties::Gathering_Property_BonusDamage;
    const FGameplayTag Tag_DropMultiplier = ZfGatheringTags::ItemProperties::Gathering_Property_DropMultiplier;
    const FGameplayTag Tag_ScoreBonus     = ZfGatheringTags::ItemProperties::Gathering_Property_ScoreBonus;

    for (const FZfAppliedModifier& Modifier : ItemInstance->GetAppliedModifiers())
    {
        // Só modifiers que afetam propriedades do item
        if (Modifier.TargetType != EZfModifierTargetType::ItemProperty)
        {
            continue;
        }

        if (Modifier.ItemPropertyTag == Tag_BonusDamage)
        {
            Result.BaseDamage += Modifier.FinalValue;
        }
        else if (Modifier.ItemPropertyTag == Tag_DropMultiplier)
        {
            Result.DropMultiplier += Modifier.FinalValue;
        }
        else if (Modifier.ItemPropertyTag == Tag_ScoreBonus)
        {
            Result.ScoreBonus += Modifier.FinalValue;
        }
    }

    // ----------------------------------------------------------
    // Clamp dos valores finais
    // ----------------------------------------------------------

    // Dano nunca abaixo de 1 — garante progresso mínimo
    Result.BaseDamage = FMath::Max(1.0f, Result.BaseDamage);

    // Multiplicador nunca abaixo de 1.0
    Result.DropMultiplier = FMath::Max(1.0f, Result.DropMultiplier);

    // ScoreBonus clampado em 1.0
    Result.ScoreBonus = FMath::Clamp(Result.ScoreBonus, 0.0f, 1.0f);

    Result.bIsValid = true;
    return Result;
}

// ============================================================
// IsDataValid
// ============================================================

#if WITH_EDITOR
EDataValidationResult UZfFragment_GatheringTool::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    if (!ToolTag.IsValid())
    {
        Context.AddError(FText::FromString(
            TEXT("ZfFragment_GatherTool: ToolTag não configurada. "
                 "Esta tag deve bater com as entradas no AllowedTools[] dos recursos.")));
        Result = EDataValidationResult::Invalid;
    }

    if (BaseDamage <= 0.0f)
    {
        Context.AddError(FText::FromString(
            TEXT("ZfFragment_GatherTool: BaseDamage deve ser maior que 0.")));
        Result = EDataValidationResult::Invalid;
    }

    return Result;
}
#endif

// ============================================================
// GetDebugString
// ============================================================

FString UZfFragment_GatheringTool::GetDebugString() const
{
    return FString::Printf(
        TEXT("ZfFragment_GatherTool | ToolTag: %s | BaseDamage: %.1f | DropMult: %.2f | ScoreBonus: %.2f"),
        *ToolTag.ToString(),
        BaseDamage,
        BaseDropMultiplier,
        BaseScoreBonus);
}