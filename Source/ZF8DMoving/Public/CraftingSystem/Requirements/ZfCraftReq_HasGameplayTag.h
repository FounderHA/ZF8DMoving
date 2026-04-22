// Fill out your copyright notice in the Description page of Project Settings.
// ZfCraftReq_HasGameplayTag.h
// Requisito concreto: jogador precisa ter uma tag especifica no ASC.
//
// USO TIPICO:
// - Receitas gated por quest progress (tag temporaria no ASC)
// - Receitas que requerem buff/status ativo
// - Receitas que requerem descoberta de "skill tree" via tag
// - Receitas que requerem item consumido gera tag permanente

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CraftingSystem/Requirements/ZfCraftRequirement.h"
#include "ZfCraftReq_HasGameplayTag.generated.h"


UCLASS(DisplayName = "Req: Player Has Gameplay Tag")
class ZF8DMOVING_API UZfCraftReq_HasGameplayTag : public UZfCraftRequirement
{
	GENERATED_BODY()

public:

	// ----------------------------------------------------------
	// CONFIGURACAO (editor)
	// ----------------------------------------------------------

	// Tag que deve estar presente no ASC do jogador.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirement")
	FGameplayTag RequiredTag;

	// Se true, a logica inverte — requisito passa quando a tag
	// NAO esta presente. Util para "nao pode estar em combate",
	// "nao pode estar envenenado", etc.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirement")
	bool bInverted = false;

	// Texto customizado exibido na UI quando o requisito falha.
	// Se vazio, um texto generico e usado.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirement")
	FText CustomFailureReason;

	// ----------------------------------------------------------
	// INTERFACE
	// ----------------------------------------------------------

	virtual bool CheckRequirement_Implementation(const FZfCraftContext& Context) const override;
	virtual FText GetFailureReason_Implementation() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};