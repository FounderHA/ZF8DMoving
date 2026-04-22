// Fill out your copyright notice in the Description page of Project Settings.
// ZfCraftRecipe.cpp

#include "CraftingSystem/ZfCraftRecipe.h"
#include "CraftingSystem/Requirements/ZfCraftRequirement.h"
#include "Tags/ZfGameplayTags.h"
#include "Inventory/ZfItemDefinition.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif


// ============================================================
// PrimaryAssetType
// Registrado em DefaultGame.ini, secao [/Script/Engine.AssetManagerSettings]:
//   +PrimaryAssetTypesToScan=(PrimaryAssetType="ZfCraftRecipe", ...)
// ============================================================

const FPrimaryAssetType UZfCraftRecipe::PrimaryAssetType("ZfCraftRecipe");


// ============================================================
// Constructor
// ============================================================

UZfCraftRecipe::UZfCraftRecipe()
{
	// Valores default configurados nas UPROPERTY acima.
}


// ============================================================
// GetPrimaryAssetId
// ============================================================
// Usa a RecipeTag como nome unico quando disponivel. Isso permite
// que o Subsystem localize receitas por tag de forma direta via
// AssetManager->GetPrimaryAssetIdList(PrimaryAssetType) + iteracao.
//
// Se RecipeTag estiver vazia (configuracao incompleta durante
// edicao), cai no nome do asset para nao quebrar o AssetManager.
// ============================================================

FPrimaryAssetId UZfCraftRecipe::GetPrimaryAssetId() const
{
	if (RecipeTag.IsValid())
	{
		return FPrimaryAssetId(PrimaryAssetType, FName(*RecipeTag.ToString()));
	}

	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}


// ============================================================
// Helpers
// ============================================================

bool UZfCraftRecipe::IsForbidden() const
{
	return Flags.HasTag(ZfCraftTags::Flag::Crafting_Flag_Forbidden);
}

bool UZfCraftRecipe::IsHidden() const
{
	return Flags.HasTag(ZfCraftTags::Flag::Crafting_Flag_Hidden);
}


// ============================================================
// IsDataValid
// ============================================================
// Validacoes rodadas ao salvar o asset no editor.
// Qualquer "erro" real trava o save; "warning" apenas avisa.
// ============================================================

#if WITH_EDITOR
EDataValidationResult UZfCraftRecipe::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	// ─── RecipeTag obrigatoria ─────────────────────────────────────────
	if (!RecipeTag.IsValid())
	{
		Context.AddError(FText::FromString(
			TEXT("ZfCraftRecipe: RecipeTag e obrigatoria — "
			     "a receita nao pode ser identificada sem ela.")));
		Result = EDataValidationResult::Invalid;
	}

	// ─── DisplayName nao pode estar vazio ──────────────────────────────
	if (DisplayName.IsEmpty())
	{
		Context.AddWarning(FText::FromString(
			TEXT("ZfCraftRecipe: DisplayName esta vazio.")));
	}

	// ─── Precisa ter pelo menos 1 ingrediente ──────────────────────────
	if (Ingredients.IsEmpty())
	{
		Context.AddError(FText::FromString(
			TEXT("ZfCraftRecipe: A receita nao tem ingredientes.")));
		Result = EDataValidationResult::Invalid;
	}

	// ─── Precisa ter pelo menos 1 output ───────────────────────────────
	if (Outputs.IsEmpty())
	{
		Context.AddError(FText::FromString(
			TEXT("ZfCraftRecipe: A receita nao tem outputs.")));
		Result = EDataValidationResult::Invalid;
	}

	// ─── Ingredientes: ItemDefinition obrigatoria, quantidade >= 1 ────
	for (int32 i = 0; i < Ingredients.Num(); i++)
	{
		const FZfCraftIngredient& Ingredient = Ingredients[i];

		if (Ingredient.ItemDefinition.IsNull())
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("ZfCraftRecipe: Ingrediente [%d] sem ItemDefinition."), i)));
			Result = EDataValidationResult::Invalid;
		}

		if (Ingredient.Quantity < 1)
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("ZfCraftRecipe: Ingrediente [%d] com Quantity < 1."), i)));
			Result = EDataValidationResult::Invalid;
		}
	}

	// ─── Outputs: ItemDefinition obrigatoria, quantidade >= 1 ─────────
	for (int32 i = 0; i < Outputs.Num(); i++)
	{
		const FZfCraftOutput& Output = Outputs[i];

		if (Output.ItemDefinition.IsNull())
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("ZfCraftRecipe: Output [%d] sem ItemDefinition."), i)));
			Result = EDataValidationResult::Invalid;
		}

		if (Output.Quantity < 1)
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("ZfCraftRecipe: Output [%d] com Quantity < 1."), i)));
			Result = EDataValidationResult::Invalid;
		}
	}

	// ─── ExtraRequirements: sem nulls + delega para IsDataValid das subclasses
	for (int32 i = 0; i < ExtraRequirements.Num(); i++)
	{
		const UZfCraftRequirement* Req = ExtraRequirements[i];

		if (!Req)
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("ZfCraftRecipe: ExtraRequirements [%d] esta nulo."), i)));
			Result = EDataValidationResult::Invalid;
			continue;
		}

		// Cada subclasse valida seus proprios campos.
		EDataValidationResult SubResult = Req->IsDataValid(Context);
		if (SubResult == EDataValidationResult::Invalid)
		{
			Result = EDataValidationResult::Invalid;
		}
	}

	// ─── Receita Forbidden nao deveria ter outputs/ingredientes? ──────
	// Escolha de design — nao bloqueia, so avisa.
	if (IsForbidden() && (!Ingredients.IsEmpty() || !Outputs.IsEmpty()))
	{
		Context.AddWarning(FText::FromString(
			TEXT("ZfCraftRecipe: Receita marcada como Forbidden "
			     "tem ingredientes/outputs — verifique se e intencional.")));
	}

	// ─── FailureChance so faz sentido se tiver ingredientes consumiveis ─
	if (FailureChance > 0.0f && FailurePolicy != EZfFailureIngredientPolicy::ConsumeNone)
	{
		bool bHasConsumable = false;
		for (const FZfCraftIngredient& Ingredient : Ingredients)
		{
			if (Ingredient.bConsume)
			{
				bHasConsumable = true;
				break;
			}
		}

		if (!bHasConsumable)
		{
			Context.AddWarning(FText::FromString(
				TEXT("ZfCraftRecipe: FailureChance > 0 mas nao ha ingredientes "
				     "consumiveis — FailurePolicy nao tera efeito.")));
		}
	}

	return Result;
}
#endif