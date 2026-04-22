// Fill out your copyright notice in the Description page of Project Settings.
// ZfFragment_RecipeScroll.h
// Fragment que transforma um ItemDefinition num pergaminho de receita.
//
// CONCEITO:
// Itens com este fragment sao "pergaminhos" — ao serem consumidos,
// liberam uma receita para o jogador (e para todos os players online
// via propagacao do PlayerState).
//
// COMO USAR:
// 1. Crie um novo ItemDefinition (ex: DA_RecipeScroll_IronSword)
// 2. Adicione UZfFragment_RecipeScroll no array Fragments
// 3. Configure RecipeTag com a tag da receita a ser liberada
// 4. Adicione tambem UZfFragment_Consumable (ou outro sistema de uso)
//    para que o item possa ser ativado pelo jogador
// 5. A logica de consumo chama LearnRecipeFromScroll() no servidor,
//    que dispara o aprendizado via PlayerState
//
// INTEGRACAO COM CONSUMABLE:
// Quando o sistema de consumo do Consumable processar o uso, ele deve
// verificar se o item tem UZfFragment_RecipeScroll e chamar
// TryLearnRecipeFromItem() passando o ItemInstance e o PlayerState do
// usuario. Se o player ja conhece a receita, o helper retorna false e
// o consumo pode ser cancelado (ou nao — design call).

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Inventory/Fragments/ZfItemFragment.h"
#include "ZfFragment_RecipeScroll.generated.h"

class AZfPlayerState;
class UZfItemInstance;


UCLASS(DisplayName = "Fragment: Recipe Scroll")
class ZF8DMOVING_API UZfFragment_RecipeScroll : public UZfItemFragment
{
	GENERATED_BODY()

public:

	// ----------------------------------------------------------
	// CONFIGURACAO (editor)
	// ----------------------------------------------------------

	// Tag da receita que este pergaminho libera ao ser consumido.
	// Deve bater com a RecipeTag de uma UZfCraftRecipe existente no catalogo.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fragment|RecipeScroll",
		meta = (GameplayTagFilter = "Recipe"))
	FGameplayTag RecipeTag;

	// Se true, o pergaminho pode ser consumido mesmo quando o jogador
	// ja conhece a receita (desperdica o scroll).
	// Se false, o sistema de consumo deve bloquear o uso quando
	// HasLearnedRecipe retornar true.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fragment|RecipeScroll")
	bool bAllowConsumeIfAlreadyKnown = false;

	// ----------------------------------------------------------
	// HELPER ESTATICO — chamada pelo sistema de consumo
	// ----------------------------------------------------------

	// Tenta liberar a receita no PlayerState. Server-side apenas.
	// Usado pelo fluxo de consumo do item (ou qualquer outro gatilho).
	//
	// @param ItemInstance — instancia do scroll sendo consumido
	// @param PlayerState  — PlayerState do jogador que consumiu
	// @return true se a receita foi liberada (nao conhecida ainda);
	//         false se ja conhecida ou dados invalidos
	UFUNCTION(BlueprintCallable, Category = "Zf|RecipeScroll")
	static bool TryLearnRecipeFromItem(UZfItemInstance* ItemInstance, AZfPlayerState* PlayerState);

	// ----------------------------------------------------------
	// DEBUG
	// ----------------------------------------------------------

	virtual FString GetDebugString() const override
	{
		return FString::Printf(
			TEXT("[Fragment_RecipeScroll] Recipe: %s | AllowIfKnown: %s"),
			*RecipeTag.ToString(),
			bAllowConsumeIfAlreadyKnown ? TEXT("Yes") : TEXT("No"));
	}
};