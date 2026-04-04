// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ZfAttributeSpendRequest.generated.h"

/**
 * Payload do evento Event.Progression.SpendAttributePoints.
 * Transporta quantos pontos o jogador quer investir em cada atributo
 * da widget até a GA_SpendAttributePoints no servidor.
 *
 * Uso na widget (Blueprint):
 *   1. Construct Object from Class → ZfAttributeSpendRequest
 *   2. Preencha os campos com os valores do formulário
 *   3. Make Gameplay Event Data → OptionalObject = request criado
 *   4. Send Gameplay Event to Actor com tag Event.Progression.SpendAttributePoints
 *
 * O jogador pode distribuir 0 em qualquer atributo — a ability ignora
 * campos zerados e não aplica SetByCaller desnecessário.
 */
UCLASS(BlueprintType)
class ZF8DMOVING_API UZfAttributeSpendRequest : public UObject
{
	GENERATED_BODY()

public:

	/** Pontos a investir em Strength. 0 = não muda. */
	UPROPERTY(BlueprintReadWrite, Category = "AttributeSpend")
	int32 StrengthPointsToAdd = 0;

	/** Pontos a investir em Dexterity. 0 = não muda. */
	UPROPERTY(BlueprintReadWrite, Category = "AttributeSpend")
	int32 DexterityPointsToAdd = 0;

	/** Pontos a investir em Intelligence. 0 = não muda. */
	UPROPERTY(BlueprintReadWrite, Category = "AttributeSpend")
	int32 IntelligencePointsToAdd = 0;

	/** Pontos a investir em Constitution. 0 = não muda. */
	UPROPERTY(BlueprintReadWrite, Category = "AttributeSpend")
	int32 ConstitutionPointsToAdd = 0;

	/** Pontos a investir em Conviction. 0 = não muda. */
	UPROPERTY(BlueprintReadWrite, Category = "AttributeSpend")
	int32 ConvictionPointsToAdd = 0;

	/** Retorna a soma total de todos os pontos a gastar. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AttributeSpend")
	int32 GetTotalPointsToSpend() const
	{
		return StrengthPointsToAdd + DexterityPointsToAdd + IntelligencePointsToAdd
		     + ConstitutionPointsToAdd + ConvictionPointsToAdd;
	}

	/** Retorna true se todos os campos são >= 0 (sem valores negativos). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AttributeSpend")
	bool IsValid() const
	{
		return StrengthPointsToAdd     >= 0
		    && DexterityPointsToAdd    >= 0
		    && IntelligencePointsToAdd >= 0
		    && ConstitutionPointsToAdd >= 0
		    && ConvictionPointsToAdd   >= 0;
	}
};