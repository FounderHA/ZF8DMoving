// Fill out your copyright notice in the Description page of Project Settings.
// ZfCraftReq_Level.h
// Requisito concreto: jogador precisa ter nivel >= MinLevel.
//
// FONTE DO DADO:
// Le o atributo Level de UZfProgressionAttributeSet do ASC do jogador,
// obtido via Context.InstigatorPawn->GetAbilitySystemComponent().

#pragma once

#include "CoreMinimal.h"
#include "CraftingSystem/Requirements/ZfCraftRequirement.h"
#include "ZfCraftReq_Level.generated.h"


UCLASS(DisplayName = "Req: Player Level")
class ZF8DMOVING_API UZfCraftReq_Level : public UZfCraftRequirement
{
	GENERATED_BODY()

public:

	// ----------------------------------------------------------
	// CONFIGURACAO (editor)
	// ----------------------------------------------------------

	// Nivel minimo necessario para o jogador craftar.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirement",
		meta = (ClampMin = "1"))
	int32 MinLevel = 1;

	// ----------------------------------------------------------
	// INTERFACE
	// ----------------------------------------------------------

	virtual bool CheckRequirement_Implementation(const FZfCraftContext& Context) const override;
	virtual FText GetFailureReason_Implementation() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};