// Copyright ZfGame Studio. All Rights Reserved.
// ZfRefineryRecipe.cpp

#include "Systems/RefinerySystem/ZfRefineryRecipe.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif


// ============================================================
// PrimaryAssetType
// Registrar em DefaultGame.ini:
//   +PrimaryAssetTypesToScan=(PrimaryAssetType="ZfRefineryRecipe", ...)
// ============================================================

const FPrimaryAssetType UZfRefineryRecipe::PrimaryAssetType("ZfRefineryRecipe");


// ============================================================
// Constructor
// ============================================================

UZfRefineryRecipe::UZfRefineryRecipe()
{
}


// ============================================================
// GetPrimaryAssetId
// ============================================================

FPrimaryAssetId UZfRefineryRecipe::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}


// ============================================================
// HasValidIngredients
// ============================================================

bool UZfRefineryRecipe::HasValidIngredients() const
{
	if (Ingredients.IsEmpty()) return false;

	for (const FZfRefineryIngredient& Ingredient : Ingredients)
	{
		if (Ingredient.ItemDefinition.IsNull()) return false;
		if (Ingredient.Quantity <= 0) return false;
	}

	return true;
}


// ============================================================
// HasValidOutputs
// ============================================================

bool UZfRefineryRecipe::HasValidOutputs() const
{
	if (Outputs.IsEmpty()) return false;

	for (const FZfRefineryOutput& Output : Outputs)
	{
		if (Output.ItemDefinition.IsNull()) return false;
		if (Output.Quantity <= 0) return false;
	}

	return true;
}


// ============================================================
// IsDataValid
// ============================================================

#if WITH_EDITOR
EDataValidationResult UZfRefineryRecipe::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	// ─── DisplayName obrigatório ──────────────────────────────────────
	if (DisplayName.IsEmpty())
	{
		Context.AddWarning(FText::FromString(
			TEXT("ZfRefineryRecipe: DisplayName está vazio.")));
	}

	// ─── Ingredientes ─────────────────────────────────────────────────
	if (Ingredients.IsEmpty())
	{
		Context.AddError(FText::FromString(
			TEXT("ZfRefineryRecipe: A receita deve ter pelo menos um ingrediente.")));
		Result = EDataValidationResult::Invalid;
	}

	for (int32 i = 0; i < Ingredients.Num(); ++i)
	{
		if (Ingredients[i].ItemDefinition.IsNull())
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("ZfRefineryRecipe: Ingrediente[%d] não tem ItemDefinition configurado."), i)));
			Result = EDataValidationResult::Invalid;
		}

		if (Ingredients[i].Quantity <= 0)
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("ZfRefineryRecipe: Ingrediente[%d] tem Quantity <= 0."), i)));
			Result = EDataValidationResult::Invalid;
		}
	}

	// ─── Outputs ──────────────────────────────────────────────────────
	if (Outputs.IsEmpty())
	{
		Context.AddError(FText::FromString(
			TEXT("ZfRefineryRecipe: A receita deve ter pelo menos um output.")));
		Result = EDataValidationResult::Invalid;
	}

	for (int32 i = 0; i < Outputs.Num(); ++i)
	{
		if (Outputs[i].ItemDefinition.IsNull())
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("ZfRefineryRecipe: Output[%d] não tem ItemDefinition configurado."), i)));
			Result = EDataValidationResult::Invalid;
		}

		if (Outputs[i].Quantity <= 0)
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("ZfRefineryRecipe: Output[%d] tem Quantity <= 0."), i)));
			Result = EDataValidationResult::Invalid;
		}
	}

	// ─── Tempo ────────────────────────────────────────────────────────
	if (BaseCraftTime <= 0.0f)
	{
		Context.AddError(FText::FromString(
			TEXT("ZfRefineryRecipe: BaseCraftTime deve ser maior que 0.")));
		Result = EDataValidationResult::Invalid;
	}

	// ─── Bônus ────────────────────────────────────────────────────────
	if (BonusOutputChance > 0.0f && BonusOutputAmount <= 0)
	{
		Context.AddError(FText::FromString(
			TEXT("ZfRefineryRecipe: BonusOutputChance > 0 mas BonusOutputAmount <= 0.")));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif