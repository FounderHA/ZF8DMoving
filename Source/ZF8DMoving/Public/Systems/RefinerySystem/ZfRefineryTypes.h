// Copyright ZfGame Studio. All Rights Reserved.
// ZfRefineryTypes.h
// Tipos centrais do sistema de Refinaria.
//
// CONTEUDO:
// - FZfRefineryIngredient  — um ingrediente requerido pela receita
// - FZfRefineryOutput      — um item produzido pela receita
// - FZfActiveCatalystState — estado do catalisador ativo no componente
// - FZfRefinerySlotFilter  — configuração de filtro de um slot

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ZfRefineryTypes.generated.h"

// Forward declarations
class UZfItemDefinition;


// ============================================================
// LOG CATEGORY
// ============================================================

ZF8DMOVING_API DECLARE_LOG_CATEGORY_EXTERN(LogZfRefinery, Log, All);




// ============================================================
// TIPO DE SLOT — identifica o SlotList e seus efeitos colaterais
// ============================================================

UENUM(BlueprintType)
enum class EZfRefinerySlotType : uint8
{
	None,
	Input,
	Output,
	Catalyst
};

// ============================================================
// STRUCTS
// ============================================================

// -----------------------------------------------------------
// FZfRefineryIngredient
// Um ingrediente requerido por uma receita de refinaria.
// O refinador verifica se o slot de input possui a quantidade
// mínima deste item antes de iniciar o refino.
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfRefineryIngredient
{
	GENERATED_BODY()

	// ItemDefinition requerida.
	// Referência suave — carregada sob demanda.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refinery|Ingredient")
	TSoftObjectPtr<UZfItemDefinition> ItemDefinition;

	// Quantidade mínima necessária no slot de input para
	// esta receita poder ser executada.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refinery|Ingredient", meta = (ClampMin = "1", UIMin = "1"))
	int32 Quantity = 1;
};


// -----------------------------------------------------------
// FZfRefineryOutput
// Um item produzido ao completar um ciclo de refino com sucesso.
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfRefineryOutput
{
	GENERATED_BODY()

	// ItemDefinition que será gerada como output.
	// Referência suave — carregada sob demanda.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refinery|Output")
	TSoftObjectPtr<UZfItemDefinition> ItemDefinition;

	// Quantidade base produzida por ciclo.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refinery|Output", meta = (ClampMin = "1", UIMin = "1"))
	int32 Quantity = 1;
};


// -----------------------------------------------------------
// FZfActiveCatalystState
// Estado do catalisador atualmente ativo no refinador.
// Armazenado no ZfRefineryComponent durante a sessão.
// Resetado quando o boost acaba e não há próximo catalisador.
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfActiveCatalystState
{
	GENERATED_BODY()

	// Multiplicador de velocidade do catalisador ativo.
	// 1.0 = sem boost (estado padrão sem catalisador).
	UPROPERTY(BlueprintReadOnly, Category = "Refinery|Catalyst")
	float SpeedMultiplier = 1.0f;

	// Quanto tempo de boost ainda resta em segundos efetivos de refino.
	// Desconta enquanto o refinador processa. Ao chegar a 0,
	// o componente tenta consumir o próximo catalisador disponível.
	UPROPERTY(BlueprintReadOnly, Category = "Refinery|Catalyst")
	float RemainingBoostDuration = 0.0f;

	// True se há um catalisador ativo com boost válido.
	bool IsActive() const
	{
		return SpeedMultiplier > 1.0f && RemainingBoostDuration > 0.0f;
	}

	// Reseta para o estado padrão (sem boost).
	void Reset()
	{
		SpeedMultiplier       = 1.0f;
		RemainingBoostDuration = 0.0f;
	}
};