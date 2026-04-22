// Fill out your copyright notice in the Description page of Project Settings.
// ZfFragment_RecipeScroll.cpp

#include "Inventory/Fragments/ZfFragment_RecipeScroll.h"
#include "Inventory/ZfItemInstance.h"
#include "Player/ZfPlayerState.h"
#include "CraftingSystem/ZfCraftTypes.h"  // LogZfCraft


// ============================================================
// TryLearnRecipeFromItem
// ============================================================
// Helper estatico chamado pelo sistema de consumo de item
// (ou qualquer outro gatilho que detecte o uso do scroll).
//
// Server-side: so age no servidor. Chamadas do cliente retornam
// false sem efeito colateral.
// ============================================================

bool UZfFragment_RecipeScroll::TryLearnRecipeFromItem(UZfItemInstance* ItemInstance, AZfPlayerState* PlayerState)
{
	if (!ItemInstance || !PlayerState)
	{
		UE_LOG(LogZfCraft, Warning,
			TEXT("UZfFragment_RecipeScroll::TryLearnRecipeFromItem: parametros invalidos."));
		return false;
	}

	if (!PlayerState->HasAuthority())
	{
		// Seguranca: aprender receitas e operacao server-only.
		UE_LOG(LogZfCraft, Warning,
			TEXT("UZfFragment_RecipeScroll::TryLearnRecipeFromItem: chamado sem authority."));
		return false;
	}

	const UZfFragment_RecipeScroll* Fragment = ItemInstance->GetFragment<UZfFragment_RecipeScroll>();
	if (!Fragment)
	{
		UE_LOG(LogZfCraft, Warning,
			TEXT("UZfFragment_RecipeScroll::TryLearnRecipeFromItem: item nao tem fragment de recipe scroll."));
		return false;
	}

	if (!Fragment->RecipeTag.IsValid())
	{
		UE_LOG(LogZfCraft, Warning,
			TEXT("UZfFragment_RecipeScroll::TryLearnRecipeFromItem: scroll com RecipeTag invalida."));
		return false;
	}

	// Ja conhece?
	if (PlayerState->HasLearnedRecipe(Fragment->RecipeTag))
	{
		UE_LOG(LogZfCraft, Log,
			TEXT("UZfFragment_RecipeScroll::TryLearnRecipeFromItem: player '%s' ja conhece '%s'."),
			*PlayerState->GetPlayerName(),
			*Fragment->RecipeTag.ToString());
		return false;
	}

	// Libera — isso ja propaga para os outros players online via Server_LearnRecipe.
	PlayerState->Server_LearnRecipe(Fragment->RecipeTag);

	UE_LOG(LogZfCraft, Log,
		TEXT("UZfFragment_RecipeScroll::TryLearnRecipeFromItem: player '%s' aprendeu '%s' via scroll."),
		*PlayerState->GetPlayerName(),
		*Fragment->RecipeTag.ToString());

	return true;
}