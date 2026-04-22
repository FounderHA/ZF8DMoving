// Fill out your copyright notice in the Description page of Project Settings.
// ZfCraftReq_CrafterHasTag.cpp

#include "CraftingSystem/Requirements/ZfCraftReq_CrafterHasTag.h"
#include "CraftingSystem/ZfCraftingComponent.h"
#include "CraftingSystem/ZfCraftTypes.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif


// ============================================================
// CheckRequirement
// ============================================================

bool UZfCraftReq_CrafterHasTag::CheckRequirement_Implementation(const FZfCraftContext& Context) const
{
	// Sem tags configuradas — requisito e trivialmente satisfeito.
	// (Evita bloquear crafts por config incompleta.)
	if (RequiredTags.IsEmpty())
	{
		return true;
	}

	// Busca o CraftingComponent no NPC crafter.
	AActor* CrafterActor = Context.CrafterActor;
	if (!CrafterActor)
	{
		UE_LOG(LogZfCraft, Warning,
			TEXT("Req_CrafterHasTag: CrafterActor nulo no contexto. Rejeitando."));
		return false;
	}

	UZfCraftingComponent* CraftingComp = CrafterActor->FindComponentByClass<UZfCraftingComponent>();
	if (!CraftingComp)
	{
		UE_LOG(LogZfCraft, Warning,
			TEXT("Req_CrafterHasTag: NPC '%s' nao tem CraftingComponent. Rejeitando."),
			*CrafterActor->GetName());
		return false;
	}

	const FGameplayTagContainer& CrafterTags = CraftingComp->CrafterTags;

	// Match com suporte hierarquico: HasAll/HasAny usa ancestrais implicitos.
	// Ex: CrafterTags tem "Crafting.Crafter.Rank.Master".
	//     RequiredTags pedindo "Crafting.Crafter.Rank" → passa (ancestral).
	switch (MatchMode)
	{
		case EZfCrafterTagMatchMode::MatchAll:
			return CrafterTags.HasAll(RequiredTags);

		case EZfCrafterTagMatchMode::MatchAny:
			return CrafterTags.HasAny(RequiredTags);
	}

	return false;
}


// ============================================================
// GetFailureReason
// ============================================================

FText UZfCraftReq_CrafterHasTag::GetFailureReason_Implementation() const
{
	if (RequiredTags.IsEmpty())
	{
		return NSLOCTEXT("ZfCraft", "CrafterTagEmpty",
			"Requisito de crafter invalido.");
	}

	const FString TagList = RequiredTags.ToStringSimple();

	switch (MatchMode)
	{
		case EZfCrafterTagMatchMode::MatchAll:
			return FText::Format(
				NSLOCTEXT("ZfCraft", "CrafterNeedsAll",
					"Este crafter nao tem todas as qualificacoes: [{0}]."),
				FText::FromString(TagList));

		case EZfCrafterTagMatchMode::MatchAny:
			return FText::Format(
				NSLOCTEXT("ZfCraft", "CrafterNeedsAny",
					"Este crafter nao e qualificado: requer uma de [{0}]."),
				FText::FromString(TagList));
	}

	return NSLOCTEXT("ZfCraft", "CrafterUnqualified", "Crafter nao qualificado.");
}


// ============================================================
// IsDataValid
// ============================================================

#if WITH_EDITOR
EDataValidationResult UZfCraftReq_CrafterHasTag::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (RequiredTags.IsEmpty())
	{
		Context.AddWarning(FText::FromString(TEXT(
			"Req_CrafterHasTag: RequiredTags esta vazio — requisito sera sempre satisfeito. "
			"Se isso nao e intencional, remova o requisito ou preencha as tags.")));
		Result = EDataValidationResult::Valid; // Warning nao bloqueia save.
	}

	return Result;
}
#endif