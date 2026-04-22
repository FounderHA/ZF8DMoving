// Fill out your copyright notice in the Description page of Project Settings.
// ZfCraftRequirement.h
// Classe base abstrata para requisitos polimorficos de receita.
//
// CONCEITO:
// Um UZfCraftRequirement encapsula uma condicao arbitraria que o
// contexto precisa satisfazer para que a receita possa ser craftada.
// Exemplos: nivel minimo, posse de tag no ASC, hora do dia, regiao,
// reputacao, quest state, etc.
//
// COMO E USADA:
// Cada UZfCraftRecipe tem um array Instanced de UZfCraftRequirement.
// O designer clica em "+" no editor e escolhe a subclasse concreta
// (dropdown aparece automaticamente gracas ao EditInlineNew).
//
// NO RUNTIME:
// O CraftingComponent itera o array e chama CheckRequirement(Context)
// em cada requisito. Se algum retornar false, o craft falha com
// Failed_RequirementNotMet e a UI recebe o FailureReason textual.
//
// COMO CRIAR UM NOVO REQUISITO:
// 1. Crie uma subclasse de UZfCraftRequirement em arquivo proprio
//    (padrao ZfCraftReq_<Nome>.h/.cpp)
// 2. Override CheckRequirement_Implementation — retorne true/false
// 3. Override GetFailureReason_Implementation — retorne FText explicativo
// 4. Exponha campos configuraveis no editor com UPROPERTY(EditAnywhere)

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CraftingSystem/ZfCraftTypes.h"
#include "ZfCraftRequirement.generated.h"


// ============================================================
// UZfCraftRequirement
// ============================================================
// Classe base abstrata. Nao instancie diretamente — use uma subclasse.
// Abstract + EditInlineNew + Blueprintable garantem que:
// - Nao aparece como opcao isolada em pickers
// - Aparece no dropdown quando adicionada a um array Instanced
// - Pode ser estendida em C++ ou Blueprint
// ============================================================

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class ZF8DMOVING_API UZfCraftRequirement : public UObject
{
	GENERATED_BODY()

public:

	// ----------------------------------------------------------
	// INTERFACE PRINCIPAL
	// ----------------------------------------------------------

	// Avalia se o requisito e satisfeito pelo contexto atual.
	// Chamado pelo CraftingComponent antes de executar o craft.
	// Tambem pode ser chamado pela UI (client-side, otimista) para
	// habilitar/desabilitar o botao de craft visualmente.
	//
	// @param Context — snapshot do estado (player, NPC, receita)
	// @return true se o requisito passa, false se falha
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Zf|CraftRequirement")
	bool CheckRequirement(const FZfCraftContext& Context) const;
	virtual bool CheckRequirement_Implementation(const FZfCraftContext& Context) const;

	// Retorna texto localizavel explicando por que o requisito falhou.
	// Chamado apenas quando CheckRequirement retorna false.
	// Deve ser curto e claro — vai ser exibido na UI pro jogador.
	//
	// Ex: "Requer nivel 15.", "Precisa ser noite.", "Requer Ferreiro Mestre."
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Zf|CraftRequirement")
	FText GetFailureReason() const;
	virtual FText GetFailureReason_Implementation() const;

	// ----------------------------------------------------------
	// DEBUG / EDITOR
	// ----------------------------------------------------------

#if WITH_EDITOR
	// Validacao no editor — subclasses devem sobrescrever para
	// detectar configuracao incompleta (ex: tag nao setada).
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};