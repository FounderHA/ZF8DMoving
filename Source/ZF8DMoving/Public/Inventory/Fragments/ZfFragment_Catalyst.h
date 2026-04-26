// Copyright ZfGame Studio. All Rights Reserved.
// ZfFragment_Catalyst.h
// Fragment que transforma um item em um catalisador de refinaria.
//
// CONCEITO:
// Itens com este fragment podem ser inseridos nos slots de catalisador
// de uma ZfRefineryComponent. Enquanto ativo, o catalisador acelera
// o tempo de refino pelo SpeedMultiplier configurado.
//
// FUNCIONAMENTO:
// - Quando o refinador começa a processar, ele consome o catalisador
//   do slot de menor índice disponível e armazena o estado do boost
//   (multiplicador + duração restante) internamente no componente.
// - O tempo de boost só desconta enquanto o refinador está ativamente
//   processando itens — pausar o refino congela o boost também.
// - Quando a duração acaba, o próximo catalisador disponível é consumido
//   automaticamente.
// - O item é removido do slot no momento do consumo, não ao inserir.
//
// COMO USAR NO EDITOR:
// 1. Crie um novo ItemDefinition (ex: DA_Catalyst_Ore_Common)
// 2. Adicione UZfFragment_Catalyst no array Fragments
// 3. Configure SpeedMultiplier e BoostDuration
// 4. No ItemTags do ItemDefinition, adicione a tag correta:
//    ex: ItemType.Catalyst.Ore para catalisadores de refinadora de minério
// 5. Na DA da refinadora, configure o CatalystSlotQuery para aceitar
//    a tag correspondente

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Fragments/ZfItemFragment.h"
#include "ZfFragment_Catalyst.generated.h"


// ============================================================
// UZfFragment_Catalyst
// ============================================================

UCLASS(DisplayName = "Fragment: Catalyst")
class ZF8DMOVING_API UZfFragment_Catalyst : public UZfItemFragment
{
	GENERATED_BODY()

public:

	// ----------------------------------------------------------
	// BOOST
	// ----------------------------------------------------------

	// Multiplicador de velocidade aplicado ao tempo de refino.
	// 1.5 significa que 1 segundo real equivale a 1.5 segundos
	// de progresso de refino.
	// Mínimo de 1.0 — valores abaixo de 1.0 desacelerariam o refino.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fragment|Catalyst", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float SpeedMultiplier = 1.5f;

	// Duração total do boost em segundos de tempo de refino efetivo.
	// O boost só desconta enquanto o refinador está processando — 
	// se o refino parar, a duração congela.
	// Ex: 60.0 = o catalisador acelera o refino por 60 segundos ativos.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fragment|Catalyst", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float BoostDuration = 60.0f;

	// ----------------------------------------------------------
	// VALIDAÇÃO NO EDITOR
	// ----------------------------------------------------------

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	// ----------------------------------------------------------
	// DEBUG
	// ----------------------------------------------------------

	virtual FString GetDebugString() const override;
};