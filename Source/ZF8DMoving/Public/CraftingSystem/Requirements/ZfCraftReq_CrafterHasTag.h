// Fill out your copyright notice in the Description page of Project Settings.
// ZfCraftReq_CrafterHasTag.h
// Requisito: o NPC crafter precisa ter uma ou mais tags especificas.
//
// CONCEITO:
// Consulta as CrafterTags do UZfCraftingComponent do NPC que esta
// atendendo o craft. Usado para restringir receitas a crafters
// especificos (ex: so ferreiros Master podem forjar Armadura Lendaria).
//
// EXEMPLOS:
// - Receita "Armadura Lendaria" → RequiredTags: Crafting.Crafter.Rank.Master
// - Receita "Pocao Real" → RequiredTags: Crafting.Crafter.Specialization.Alchemist
// - Receita combinada → RequiredTags: Crafting.Crafter.Specialization.Blacksmith
//                                     + Crafting.Crafter.Rank.Expert
//                       (precisa ser ferreiro E expert)
//
// MODOS DE MATCH:
// - MatchAll (default): NPC precisa ter TODAS as tags listadas
// - MatchAny:           NPC precisa ter PELO MENOS UMA das tags listadas
//
// Match hierarquico — uma CrafterTag Crafting.Crafter.Rank.Master
// tambem passa num requisito que pede Crafting.Crafter.Rank
// (porque Master e descendente de Rank).

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CraftingSystem/Requirements/ZfCraftRequirement.h"
#include "ZfCraftReq_CrafterHasTag.generated.h"


UENUM(BlueprintType)
enum class EZfCrafterTagMatchMode : uint8
{
	// NPC precisa ter TODAS as tags listadas.
	MatchAll UMETA(DisplayName = "Match All (requer todas)"),

	// NPC precisa ter PELO MENOS UMA das tags listadas.
	MatchAny UMETA(DisplayName = "Match Any (requer qualquer uma)")
};


UCLASS(DisplayName = "Req: Crafter Has Tag", meta = (ShortTooltip = "NPC crafter precisa ter tags especificas (rank, especialidade, etc)."))
class ZF8DMOVING_API UZfCraftReq_CrafterHasTag : public UZfCraftRequirement
{
	GENERATED_BODY()

public:

	// Tags que o NPC precisa ter (em CrafterTags do CraftingComponent).
	// Filtra pelas tags do namespace Crafting.Crafter.* no editor.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirement",
		meta = (GameplayTagFilter = "Crafting.Crafter"))
	FGameplayTagContainer RequiredTags;

	// Modo de match: todas ou qualquer uma.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirement")
	EZfCrafterTagMatchMode MatchMode = EZfCrafterTagMatchMode::MatchAll;

	// ------------------------------------------------------------
	// Override da base
	// ------------------------------------------------------------

	virtual bool CheckRequirement_Implementation(const FZfCraftContext& Context) const override;
	virtual FText GetFailureReason_Implementation() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};