// Copyright ZfGame Studio. All Rights Reserved.
// ZfRefineryData.cpp

#include "Systems/RefinerySystem/ZfRefineryData.h"
#include "Systems/RefinerySystem/ZfRefineryRecipe.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif


// ============================================================
// PrimaryAssetType
// Registrar em DefaultGame.ini:
//   +PrimaryAssetTypesToScan=(PrimaryAssetType="ZfRefineryData", ...)
// ============================================================

const FPrimaryAssetType UZfRefineryData::PrimaryAssetType("ZfRefineryData");


// ============================================================
// GetPrimaryAssetId
// ============================================================

FPrimaryAssetId UZfRefineryData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}


// ============================================================
// IsItemAllowedInInput
// ============================================================

bool UZfRefineryData::IsItemAllowedInInput(const FGameplayTagContainer& ItemTags) const
{
	// Query vazia = aceita qualquer item (permissivo por padrão)
	if (AllowedInputTags.IsEmpty()) return true;
	return ItemTags.HasAny(AllowedInputTags);
}


// ============================================================
// IsItemAllowedAsCatalyst
// ============================================================

bool UZfRefineryData::IsItemAllowedAsCatalyst(const FGameplayTagContainer& ItemTags) const
{
	// Query vazia = aceita qualquer catalisador (permissivo por padrão)
	if (AllowedCatalystTags.IsEmpty()) return true;
	return ItemTags.HasAny(AllowedCatalystTags);
}


// ============================================================
// IsDataValid
// ============================================================

#if WITH_EDITOR
EDataValidationResult UZfRefineryData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	// ─── DisplayName ───────────────────────────────────────────────────
	if (DisplayName.IsEmpty())
	{
		Context.AddWarning(FText::FromString(
			TEXT("ZfRefineryData: DisplayName está vazio.")));
	}

	// ─── Capacidades ───────────────────────────────────────────────────
	if (InputSlotCapacity <= 0)
	{
		Context.AddError(FText::FromString(
			TEXT("ZfRefineryData: InputSlotCapacity deve ser maior que 0.")));
		Result = EDataValidationResult::Invalid;
	}

	if (OutputSlotCapacity <= 0)
	{
		Context.AddError(FText::FromString(
			TEXT("ZfRefineryData: OutputSlotCapacity deve ser maior que 0.")));
		Result = EDataValidationResult::Invalid;
	}

	if (CatalystSlotCount <= 0)
	{
		Context.AddError(FText::FromString(
			TEXT("ZfRefineryData: CatalystSlotCount deve ser maior que 0.")));
		Result = EDataValidationResult::Invalid;
	}

	// ─── Receitas ──────────────────────────────────────────────────────
	if (AvailableRecipes.IsEmpty())
	{
		Context.AddError(FText::FromString(
			TEXT("ZfRefineryData: AvailableRecipes está vazio — a máquina não tem receitas configuradas.")));
		Result = EDataValidationResult::Invalid;
	}

	for (int32 i = 0; i < AvailableRecipes.Num(); ++i)
	{
		if (AvailableRecipes[i].IsNull())
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("ZfRefineryData: AvailableRecipes[%d] tem referência nula."), i)));
			Result = EDataValidationResult::Invalid;
		}
	}

	return Result;
}
#endif


// ============================================================
// GetDebugString
// ============================================================

FString UZfRefineryData::GetDebugString() const
{
	return FString::Printf(
		TEXT("[ZfRefineryData] %s | Input: %d slots | Output: %d slots | Catalyst: %d slots | Recipes: %d"),
		*DisplayName.ToString(),
		InputSlotCapacity,
		OutputSlotCapacity,
		CatalystSlotCount,
		AvailableRecipes.Num());
}