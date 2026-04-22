// Fill out your copyright notice in the Description page of Project Settings.
// ZfCraftingSubsystem.h
// GameInstanceSubsystem que cataloga e indexa todas as receitas do jogo.
//
// CONCEITO:
// - Catalogo global IMUTAVEL apos Initialize.
// - Carrega todos os UZfCraftRecipe via AssetManager.
// - Mantem indices para lookup rapido (por tag, categoria, ingrediente, output).
// - Expoe queries publicas usadas pela UI e pelo CraftingComponent.
//
// POR QUE GameInstanceSubsystem:
// - Sobrevive entre level loads (receitas nao precisam recarregar).
// - Instancia unica compartilhada por todos os players no servidor.
// - Dedicated server e client tem sua propria instancia — cada um
//   monta seu proprio indice local (mesmo catalogo, mesma fonte).
//
// MULTI-PLAYER:
// Este subsystem e catalogo ESTATICO — nao tem estado por-jogador.
// O que cada jogador CONHECE vive no PlayerState (KnownRecipeTags).
// Queries que envolvem conhecimento do jogador (ex: GetAvailable...)
// recebem o PlayerState como parametro e fazem o filtro.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "CraftingSystem/ZfCraftTypes.h"
#include "ZfCraftingSubsystem.generated.h"

class UZfCraftRecipe;
class UZfItemDefinition;
class APlayerState;
class AZfPlayerState;


// ============================================================
// FZfRecipeList
// Wrapper para TArray<UZfCraftRecipe*> ser usavel como valor de UPROPERTY TMap.
// (TMap<FGameplayTag, TArray<T>> nao e serializavel direto como UPROPERTY.)
// ============================================================

USTRUCT()
struct FZfRecipeList
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<UZfCraftRecipe>> Recipes;
};


// ============================================================
// UZfCraftingSubsystem
// ============================================================

UCLASS(BlueprintType)
class ZF8DMOVING_API UZfCraftingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	// ============================================================
	// CICLO DE VIDA
	// ============================================================

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ============================================================
	// QUERIES PRINCIPAIS
	// ============================================================

	// Retorna a receita com a RecipeTag informada, ou nullptr se nao existir.
	// Lookup O(1) via mapa indexado.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|Crafting")
	UZfCraftRecipe* GetRecipeByTag(const FGameplayTag& RecipeTag) const;

	// Retorna todas as receitas cataloga — uso raro, prefira queries filtradas.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|Crafting")
	TArray<UZfCraftRecipe*> GetAllRecipes() const;

	// Retorna todas as receitas que contem a tag de categoria dada
	// (match hierarquico — tag pai pega sub-tags).
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|Crafting")
	TArray<UZfCraftRecipe*> GetRecipesByCategory(const FGameplayTag& CategoryTag) const;

	// Retorna todas as receitas que produzem a ItemDefinition dada.
	// Uso: tooltip "como obtenho este item?".
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|Crafting")
	TArray<UZfCraftRecipe*> GetRecipesProducingItem(UZfItemDefinition* ItemDefinition) const;

	// Retorna todas as receitas que consomem a ItemDefinition dada.
	// Uso: tooltip "para que serve este item?".
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|Crafting")
	TArray<UZfCraftRecipe*> GetRecipesConsumingItem(UZfItemDefinition* ItemDefinition) const;

	// ============================================================
	// QUERY COMBINADA — CASO DE USO DA UI DE CRAFT
	// ============================================================

	// Retorna as receitas disponiveis para exibicao na UI de craft
	// do jogador dado, interagindo com o NPC dado.
	//
	// Aplica os seguintes filtros em ordem:
	// 1. NPC aceita a categoria da receita (CrafterAcceptedQuery)
	// 2. Player conhece a receita (KnownRecipeTags) OU bIsKnownByDefault
	// 3. Receita nao marcada como Recipe.Flag.Hidden
	// 4. Receita nao marcada como Recipe.Flag.Forbidden (opcional)
	//
	// NOTA: NAO filtra por requisitos do tipo nivel/tag/etc.
	// A UI deve mostrar receitas que o jogador conhece mesmo que
	// ele nao atenda algum requisito — exibindo a razao. Filtragem
	// por requisito fica a cargo da UI, que itera os ExtraRequirements
	// para colorir/desabilitar o botao.
	//
	// @param PlayerState        — PlayerState do jogador (para KnownRecipeTags)
	// @param CrafterAcceptedQuery — query do NPC (AcceptedRecipesQuery)
	// @param bIncludeForbidden  — se true, inclui receitas Forbidden (debug)
	// @return lista filtrada
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|Crafting")
	TArray<UZfCraftRecipe*> GetAvailableRecipesForPlayer(
		APlayerState* PlayerState,
		const FGameplayTagQuery& CrafterAcceptedQuery,
		bool bIncludeForbidden = false) const;

	// ============================================================
	// DEBUG
	// ============================================================

	// Retorna quantas receitas foram catalogadas.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|Crafting|Debug")
	int32 GetRecipeCount() const { return RecipeByTag.Num(); }

	// Loga o catalogo inteiro — util no boot ou via console command.
	UFUNCTION(BlueprintCallable, Category = "Zf|Crafting|Debug")
	void LogCatalog() const;

protected:

	// ============================================================
	// INDEXACAO
	// ============================================================

	// Carrega todas as receitas via AssetManager e monta os indices.
	// Chamado uma unica vez no Initialize.
	void BuildCatalog();

	// Adiciona uma receita aos indices. Chamado por BuildCatalog.
	// Detecta colisao de RecipeTag e loga Error.
	void IndexRecipe(UZfCraftRecipe* Recipe);

	// Limpa todos os indices. Chamado no Deinitialize.
	void ClearCatalog();

private:

	// ============================================================
	// INDICES
	// ============================================================

	// Lookup principal: RecipeTag -> Recipe.
	UPROPERTY()
	TMap<FGameplayTag, TObjectPtr<UZfCraftRecipe>> RecipeByTag;

	// Lookup por categoria: CategoryTag -> lista de receitas.
	// Uma receita com multiplas CategoryTags aparece em varias entradas.
	UPROPERTY()
	TMap<FGameplayTag, FZfRecipeList> RecipesByCategory;

	// Lookup por item produzido: ItemDefinition path -> receitas.
	// Usa FSoftObjectPath para poder indexar por soft pointer.
	TMap<FSoftObjectPath, TArray<TWeakObjectPtr<UZfCraftRecipe>>> RecipesByOutput;

	// Lookup por item consumido: ItemDefinition path -> receitas.
	TMap<FSoftObjectPath, TArray<TWeakObjectPtr<UZfCraftRecipe>>> RecipesByIngredient;
};