// Fill out your copyright notice in the Description page of Project Settings.
// ZfCraftingComponent.h
// Componente instalado no NPC crafter. Executa o craft server-authoritative.
//
// CONCEITO:
// - O NPC tem este componente ao lado do UZfInteractionComponent.
// - A UI do jogador chama Server_RequestCraft(PC, RecipeTag) neste
//   componente apos clicar em craftar.
// - O componente re-valida TUDO no servidor, consome ingredientes do
//   inventario do jogador, entrega outputs ao mesmo inventario.
// - Multiplos jogadores podem enviar requests em paralelo — cada request
//   e processado independente (cada um com seu PC e seu inventario).
//
// OWNERSHIP E RPC:
// O componente vive num NPC nao-possuido por nenhum player, entao
// Client RPC direto no componente nao funciona (sem NetConnection).
// O resultado e roteado de volta via Client RPC no PlayerState do
// instigator — PlayerStates tem conexao natural com seu cliente.
//
// CONFIGURACAO:
// - AcceptedRecipesQuery: query sobre CategoryTags das receitas.
//   Ex: ferreiro teria ANY(Recipe.Category.Weapon, Recipe.Category.Armor).
// - CrafterTags: tags que identificam o NPC (Blacksmith, Master, etc).
//   Consultadas por requisitos como UZfCraftReq_NPCRank futuros.
// - MaxCraftRange: distancia maxima do jogador ao NPC.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CraftingSystem/ZfCraftTypes.h"
#include "ZfCraftingComponent.generated.h"

class APlayerController;
class UZfCraftRecipe;
class UZfCraftingSubsystem;
class UZfInventoryComponent;
class UZfItemDefinition;


// ============================================================
// DELEGATES
// ============================================================

// Disparado no SERVIDOR quando qualquer craft nesse componente finaliza.
// Util para logging/analytics/achievements server-side.
// Clientes recebem o resultado via AZfPlayerState::OnCraftResultReceived.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCraftAttemptedOnServer, const FZfCraftResult&, Result);


// ============================================================
// UZfCraftingComponent
// ============================================================

UCLASS(ClassGroup=(Crafting), meta=(BlueprintSpawnableComponent))
class ZF8DMOVING_API UZfCraftingComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UZfCraftingComponent();

	// ============================================================
	// CONFIGURACAO (editor)
	// ============================================================

	// Query sobre CategoryTags das receitas. Define o que este NPC aceita.
	// Vazia = aceita qualquer receita (caso raro).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting|Config", meta = (GameplayTagFilter = "Crafting.Category"))
	FGameplayTagQuery AcceptedRecipesQuery;

	// Tags que identificam este NPC.
	// Ex: "Crafter.Specialization.Blacksmith" + "Crafter.Rank.Master".
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting|Config",
		meta = (GameplayTagFilter = "Crafting.Crafter"))
	FGameplayTagContainer CrafterTags;

	// Distancia maxima (cm) do jogador ao NPC para aceitar o craft.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting|Config",
		meta = (ClampMin = "50.0"))
	float MaxCraftRange = 400.0f;

	// ============================================================
	// DELEGATES PUBLICOS (server-side)
	// ============================================================

	UPROPERTY(BlueprintAssignable, Category = "Crafting|Events")
	FOnCraftAttemptedOnServer OnCraftAttemptedOnServer;
	
	// ============================================================
	// API PUBLICA
	// ============================================================

	// Executa um craft para o player dado. Deve ser chamado NO SERVIDOR.
	// Normalmente invocado pelo AZfPlayerState::Server_RequestCraft,
	// que e o RPC roteador (ver comentario la).
	//
	// Se voce chamar isto direto de um cliente, nao vai funcionar —
	// a chamada sera ignorada pelo guard de authority.
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void RequestCraft(APlayerController* PC, FGameplayTag RecipeTag);
	
	// ============================================================
	// RPCS PUBLICOS
	// ============================================================

	// ============================================================
	// QUERY AUXILIAR (cliente ou servidor)
	// ============================================================

	// Avalia se a receita poderia ser craftada agora por este jogador.
	// Usado pela UI client-side para habilitar/desabilitar o botao.
	// NAO substitui validacao server-side — apenas otimista.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Crafting|Query")
	bool CanPlayerCraft(APlayerController* PC, UZfCraftRecipe* Recipe, FText& OutReason) const;

protected:

	virtual void BeginPlay() override;

private:

	// ============================================================
	// PIPELINE DE EXECUCAO (servidor)
	// ============================================================

	// Orquestra a tentativa de craft — retorna o resultado completo.
	FZfCraftResult ExecuteCraftAttempt(APlayerController* PC, UZfCraftRecipe* Recipe);

	// Envia o resultado de volta ao cliente via PlayerState do instigator.
	// Tambem dispara OnCraftAttemptedOnServer localmente (para listeners
	// server-side como logging/analytics).
	void BroadcastResultToInstigator(APlayerController* PC, const FZfCraftResult& Result);

	// ─── Validadores ──────────────────────────────────────────────────
	// Cada retorna true se passa. Se falha, preenche OutResult.

	bool Validate_NotForbidden(UZfCraftRecipe* Recipe, FZfCraftResult& OutResult) const;
	bool Validate_Range(APlayerController* PC, FZfCraftResult& OutResult) const;
	bool Validate_NPCAcceptsRecipe(UZfCraftRecipe* Recipe, FZfCraftResult& OutResult) const;
	bool Validate_PlayerKnowsRecipe(APlayerController* PC, UZfCraftRecipe* Recipe, FZfCraftResult& OutResult) const;
	bool Validate_WorldStateQuery(APlayerController* PC, UZfCraftRecipe* Recipe, FZfCraftResult& OutResult) const;
	bool Validate_ExtraRequirements(const FZfCraftContext& Context, FZfCraftResult& OutResult) const;
	bool Validate_Ingredients(UZfInventoryComponent* Inventory, UZfCraftRecipe* Recipe, FZfCraftResult& OutResult) const;
	bool Validate_InventorySpace(UZfInventoryComponent* Inventory, UZfCraftRecipe* Recipe, FZfCraftResult& OutResult) const;

	// ─── Consumo e spawn ──────────────────────────────────────────────

	void ConsumeIngredients(UZfInventoryComponent* Inventory, UZfCraftRecipe* Recipe, bool bFailurePath);
	void SpawnOutputs(UZfInventoryComponent* Inventory, UZfCraftRecipe* Recipe);

	// ─── Helpers ──────────────────────────────────────────────────────

	int32 CountItemsInInventory(UZfInventoryComponent* Inventory, UZfItemDefinition* ItemDef) const;
	void RemoveItemsFromInventory(UZfInventoryComponent* Inventory, UZfItemDefinition* ItemDef, int32 Amount);

	FZfCraftContext BuildContext(APlayerController* PC, UZfCraftRecipe* Recipe) const;
	UZfInventoryComponent* GetInstigatorInventory(APlayerController* PC) const;
	UZfCraftingSubsystem* GetCraftingSubsystem() const;

	void FireCueOnCrafter(const FGameplayTag& CueTag) const;
};