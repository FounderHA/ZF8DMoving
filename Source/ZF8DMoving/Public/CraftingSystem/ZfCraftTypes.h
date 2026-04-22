// Fill out your copyright notice in the Description page of Project Settings.
// ZfCraftTypes.h
// Arquivo central de tipos do sistema de Craft.
//
// CONTEUDO:
// - Log category global (LogZfCraft)
// - Enums: EZfFailureIngredientPolicy, EZfCraftResult
// - Structs: FZfCraftIngredient, FZfCraftOutput, FZfCraftContext, FZfCraftResult
//
// Este arquivo e incluido por todos os demais arquivos do sistema
// (Recipe, Requirement, Subsystem, Component).

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ZfCraftTypes.generated.h"

// Forward declarations
class UZfCraftRecipe;
class UZfItemDefinition;
class UZfItemInstance;
class APlayerController;
class APlayerState;
class APawn;

// ============================================================
// LOG CATEGORY
// ============================================================

ZF8DMOVING_API DECLARE_LOG_CATEGORY_EXTERN(LogZfCraft, Log, All);


// ============================================================
// ENUMS
// ============================================================

// -----------------------------------------------------------
// EZfFailureIngredientPolicy
// Define o que acontece com os ingredientes consumiveis
// quando o craft falha pela FailureChance.
//
// A escolha e por receita — receitas "perigosas" podem punir
// com ConsumeAll, enquanto receitas tutoriais podem usar
// ConsumeNone para nao frustrar o jogador.
// -----------------------------------------------------------
UENUM(BlueprintType)
enum class EZfFailureIngredientPolicy : uint8
{
	// Consome todos os ingredientes (consumiveis) mesmo com a falha.
	// Punicao dura — recomendado para receitas de alto risco/recompensa.
	ConsumeAll    UMETA(DisplayName = "Consume All"),

	// Consome metade de cada stack consumivel (arredondado para cima).
	// Punicao media — padrao equilibrado.
	ConsumeHalf   UMETA(DisplayName = "Consume Half"),

	// Nao consome nenhum ingrediente ao falhar.
	// Falha suave — jogador perde apenas o tempo do clique.
	ConsumeNone   UMETA(DisplayName = "Consume None")
};


// -----------------------------------------------------------
// EZfCraftResult
// Codigos de resultado de uma tentativa de craft.
// Enviados para a UI via delegate OnCraftAttempted.
// -----------------------------------------------------------
UENUM(BlueprintType)
enum class EZfCraftResult : uint8
{
	// Craft concluido com sucesso, outputs entregues ao inventario.
	Success                    UMETA(DisplayName = "Success"),

	// FailureChance rolou contra o jogador.
	// Os ingredientes sao tratados de acordo com a FailurePolicy.
	Failed_ChanceRoll          UMETA(DisplayName = "Failed — Chance Roll"),

	// Receita nao conhecida pelo jogador (nao esta em KnownRecipeTags).
	Failed_NotLearned          UMETA(DisplayName = "Failed — Not Learned"),

	// NPC nao aceita essa categoria de receita (AcceptedRecipesQuery nao passa).
	Failed_RecipeNotAccepted   UMETA(DisplayName = "Failed — Recipe Not Accepted"),

	// Jogador esta fora do alcance do NPC crafter.
	Failed_OutOfRange          UMETA(DisplayName = "Failed — Out Of Range"),

	// Inventario nao possui ingredientes/quantidades suficientes.
	Failed_MissingIngredients  UMETA(DisplayName = "Failed — Missing Ingredients"),

	// Algum UZfCraftRequirement falhou (nivel, hora, regiao, etc.).
	// O texto do motivo vem em FZfCraftResult::FailureReason.
	Failed_RequirementNotMet   UMETA(DisplayName = "Failed — Requirement Not Met"),

	// Inventario sem espaco para receber o output.
	Failed_InventoryFull       UMETA(DisplayName = "Failed — Inventory Full"),

	// Receita marcada com Recipe.Flag.Forbidden.
	Failed_Forbidden           UMETA(DisplayName = "Failed — Forbidden"),

	// Dados invalidos na receita (null, config quebrada).
	Failed_InvalidRecipe       UMETA(DisplayName = "Failed — Invalid Recipe")
};


// ============================================================
// STRUCTS
// ============================================================

// -----------------------------------------------------------
// FZfCraftIngredient
// Um ingrediente requerido por uma receita.
//
// Referencia direta ao UZfItemDefinition — a receita exige
// esse item especifico (ferro = aquele asset de ferro).
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfCraftIngredient
{
	GENERATED_BODY()

	// ItemDefinition requerida pelo ingrediente.
	// Referencia suave — carregada sob demanda pelo subsystem.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UZfItemDefinition> ItemDefinition;

	// Quantidade necessaria deste item no inventario.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1", UIMin = "1"))
	int32 Quantity = 1;

	// Se true, a quantidade e removida do inventario ao craftar.
	// Se false, o item apenas precisa estar presente (ex: ferramenta
	// especial que e requisito mas nao e gasta).
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bConsume = true;
};


// -----------------------------------------------------------
// FZfCraftOutput
// Um item produzido pela receita ao completar com sucesso.
//
// Output e 100% deterministico — sem rolls, sem chance.
// O item sai direto da ItemDefinition com os defaults dela.
// Qualquer customizacao (raridade, modifiers) e feita em
// Blueprint pelo designer em um hook futuro.
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfCraftOutput
{
	GENERATED_BODY()

	// ItemDefinition que sera gerada como output.
	// Referencia suave — carregada sob demanda.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UZfItemDefinition> ItemDefinition;

	// Quantidade de itens produzidos.
	// Se o item for stackavel, pode gerar um stack unico.
	// Se nao for, serao N itens separados (respeitando slots livres).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1", UImin = "1"))
	int32 Quantity = 1;
};


// -----------------------------------------------------------
// FZfCraftContext
// Payload passado para UZfCraftRequirement::CheckRequirement
// e para validacoes do CraftingComponent.
//
// Contem as referencias essenciais para qualquer requisito
// poder consultar o estado do mundo e do jogador:
// - Quem esta craftando (Controller, Pawn, PlayerState)
// - Onde esta craftando (Crafter NPC)
// - O que esta craftando (Recipe)
//
// Requisitos que precisam de Inventory/ASC/AttributeSet
// acessam via InstigatorPawn/PlayerState.
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfCraftContext
{
	GENERATED_BODY()

	// Controller do jogador que iniciou o craft.
	UPROPERTY(BlueprintReadOnly, Category = "Craft|Context")
	TObjectPtr<APlayerController> InstigatorController;

	// Pawn do jogador (geralmente o AZfPlayerCharacter).
	// Usado para buscar componentes: InventoryComponent, ASC.
	UPROPERTY(BlueprintReadOnly, Category = "Craft|Context")
	TObjectPtr<APawn> InstigatorPawn;

	// PlayerState do jogador.
	// Usado para checar KnownRecipeTags, atributos persistentes,
	// reputacao, quest state, etc.
	UPROPERTY(BlueprintReadOnly, Category = "Craft|Context")
	TObjectPtr<APlayerState> InstigatorPlayerState;

	// Ator NPC que hospeda o UZfCraftingComponent.
	// Usado por requisitos do tipo "NPC precisa ter tag X" ou
	// para medir distancia ao instigador.
	UPROPERTY(BlueprintReadOnly, Category = "Craft|Context")
	TObjectPtr<AActor> CrafterActor;

	// Receita sendo avaliada/executada.
	UPROPERTY(BlueprintReadOnly, Category = "Craft|Context")
	TObjectPtr<UZfCraftRecipe> Recipe;

	// Helper de conveniencia — true se todos os campos essenciais estao preenchidos.
	bool IsValid() const
	{
		return InstigatorController && InstigatorPawn && CrafterActor && Recipe;
	}
};


// -----------------------------------------------------------
// FZfCraftResult
// Resultado de uma tentativa de craft.
// Enviado ao cliente via delegate OnCraftAttempted para
// a UI exibir feedback apropriado.
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfCraftResult
{
	GENERATED_BODY()

	// Codigo de resultado — sucesso ou motivo especifico da falha.
	UPROPERTY(BlueprintReadOnly, Category = "Craft|Result")
	EZfCraftResult ResultCode = EZfCraftResult::Success;

	// Texto exibivel ao usuario descrevendo o motivo da falha.
	// Vazio em caso de sucesso.
	// Em Failed_RequirementNotMet, vem do UZfCraftRequirement que falhou.
	UPROPERTY(BlueprintReadOnly, Category = "Craft|Result")
	FText FailureReason;

	// Receita que foi tentada.
	UPROPERTY(BlueprintReadOnly, Category = "Craft|Result")
	TObjectPtr<UZfCraftRecipe> Recipe;

	// Helper rapido.
	bool WasSuccessful() const
	{
		return ResultCode == EZfCraftResult::Success;
	}
};