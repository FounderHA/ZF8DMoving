// Fill out your copyright notice in the Description page of Project Settings.
// ZfCraftRecipe.h
// Primary Data Asset que representa uma receita de craft.
//
// CONCEITO:
// Cada receita e um asset individual criado no Content Browser.
// O designer preenche os campos, adiciona ingredientes/outputs/requisitos
// e salva. O UZfCraftingSubsystem descobre todas as receitas via
// AssetManager e as indexa no inicio do jogo.
//
// UMA RECEITA TEM:
// - Identificacao: tag unica, nome, descricao, icone
// - Categorizacao: tags de categoria (filtragem por NPC/UI)
// - Ingredientes: o que consumir do inventario
// - Outputs: o que produzir (deterministico)
// - Requisitos: TagQuery de estado de mundo + array polimorfico
// - Falha: chance de falhar + politica de ingredientes
// - Descoberta: se ja vem conhecida ou precisa ser liberada
//
// GERENCIAMENTO VIA ASSETMANAGER:
// Por ser UPrimaryDataAsset, e gerenciado pelo AssetManager.
// Registrar em DefaultGame.ini:
//   Primary Asset Types to Scan += ZfCraftRecipe
//
// COMO CRIAR UMA NOVA RECEITA NO EDITOR:
// 1. Content Browser -> Add -> Miscellaneous -> Data Asset
// 2. Selecione UZfCraftRecipe como classe
// 3. Configure RecipeTag (deve ser unica!), nome, descricao
// 4. Adicione ingredientes e outputs
// 5. Configure requisitos (opcional)
// 6. Salve o asset

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CraftingSystem/ZfCraftTypes.h"
#include "ZfCraftRecipe.generated.h"

class UTexture2D;
class UZfCraftRequirement;


// ============================================================
// UZfCraftRecipe
// ============================================================

UCLASS(BlueprintType, Blueprintable, Const)
class ZF8DMOVING_API UZfCraftRecipe : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UZfCraftRecipe();

	// ============================================================
	// IDENTIFICACAO
	// ============================================================

	// Tag unica que identifica esta receita.
	// E usada em PlayerState->KnownRecipeTags para marcar descoberta.
	// DEVE ser unica entre todas as receitas — IsDataValid nao
	// consegue detectar isso (e asset-local), mas o Subsystem loga
	// conflitos ao indexar.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe|Identity",
		meta = (GameplayTagFilter = "Crafting.Recipe"))
	FGameplayTag RecipeTag;

	// Nome legivel exibido na UI de craft.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe|Identity")
	FText DisplayName = FText::FromString(TEXT("None"));

	// Descricao curta — tooltip da receita.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe|Identity",
		meta = (MultiLine = true))
	FText Description;

	// Icone exibido na UI — pode ser o icone do output principal.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe|Identity")
	TSoftObjectPtr<UTexture2D> Icon;

	// ============================================================
	// CATEGORIZACAO
	// ============================================================

	// Tags de categoria da receita. Multiplas permitidas.
	// Usadas para:
	// - Filtro de receitas aceitas por NPC (AcceptedRecipesQuery)
	// - Agrupamento/filtragem na UI
	// - Queries no CraftingSubsystem
	//
	// Ex: uma espada poderia ter:
	//   Recipe.Category.Weapon
	//   Recipe.Category.Weapon.Sword (se existir sub-tag)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe|Identity",
		meta = (GameplayTagFilter = "Crafting.Category"))
	FGameplayTagContainer CategoryTags;

	// Flags de metadata (Hidden, Forbidden, Dangerous).
	// Consultadas pela UI e pela logica de execucao.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe|Identity",
		meta = (GameplayTagFilter = "Crafting.Flag"))
	FGameplayTagContainer Flags;

	// ============================================================
	// INGREDIENTES E OUTPUTS
	// ============================================================

	// Itens consumidos/requeridos do inventario do jogador.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe|Ingredients")
	TArray<FZfCraftIngredient> Ingredients;

	// Itens produzidos ao completar o craft com sucesso.
	// 100% deterministico — mesmo output sempre.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe|Outputs")
	TArray<FZfCraftOutput> Outputs;

	// ============================================================
	// REQUISITOS
	// ============================================================

	// Query sobre o estado de mundo (tags publicadas no ASC do mundo
	// ou no player). Usada para requisitos categoricos baratos:
	// clima, regiao, hora do dia.
	//
	// Deixe vazia se nenhum requisito de estado de mundo for necessario.
	// Avaliada ANTES dos ExtraRequirements (barato antes de caro).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe|Requirements")
	FGameplayTagQuery WorldStateQuery;

	// Array polimorfico de requisitos complexos/numericos.
	// Designer adiciona e escolhe a subclasse via dropdown no editor.
	// Ex: UZfCraftReq_Level, UZfCraftReq_HasGameplayTag, ...
	//
	// Avaliados em ordem. Primeiro a falhar define o FailureReason.
	UPROPERTY(EditAnywhere, Instanced, Category = "Recipe|Requirements")
	TArray<TObjectPtr<UZfCraftRequirement>> ExtraRequirements;

	// ============================================================
	// FALHA
	// ============================================================

	// Chance de o craft falhar por sorte — UNICA aleatoriedade do sistema.
	// 0.0 = nunca falha (deterministico puro)
	// 1.0 = sempre falha (receita bugada/proibida-por-design)
	//
	// Rolado uma unica vez no servidor DEPOIS de todas as outras
	// validacoes passarem. Se falhar, ingredientes sao tratados
	// segundo FailurePolicy.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe|Failure",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FailureChance = 0.0f;

	// Politica aplicada aos ingredientes consumiveis quando a
	// FailureChance rola contra o jogador.
	// Ignorado se FailureChance == 0.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe|Failure")
	EZfFailureIngredientPolicy FailurePolicy = EZfFailureIngredientPolicy::ConsumeHalf;

	// ============================================================
	// DESCOBERTA
	// ============================================================

	// Se true, a receita ja vem conhecida por todos os jogadores.
	// Se false, precisa ser adicionada a PlayerState->KnownRecipeTags
	// via algum gatilho (scroll, dialogo, quest, nivel, exploracao).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe|Discovery")
	bool bIsKnownByDefault = false;

	// ============================================================
	// ASSETMANAGER — UPrimaryDataAsset overrides
	// ============================================================

	// Tipo primario — registrado em DefaultGame.ini.
	// Usado para carregar receitas por categoria via AssetManager.
	static const FPrimaryAssetType PrimaryAssetType;

	// Override do PrimaryAssetId — usa RecipeTag como nome unico
	// se valido, senao cai no nome do asset.
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// ============================================================
	// HELPERS
	// ============================================================

	// Retorna true se a receita esta marcada com Recipe.Flag.Forbidden.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|CraftRecipe")
	bool IsForbidden() const;

	// Retorna true se a receita esta marcada com Recipe.Flag.Hidden.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|CraftRecipe")
	bool IsHidden() const;

	// Retorna true se a receita pode falhar (FailureChance > 0).
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|CraftRecipe")
	bool CanFail() const { return FailureChance > 0.0f; }

	// ============================================================
	// VALIDACAO NO EDITOR
	// ============================================================

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};