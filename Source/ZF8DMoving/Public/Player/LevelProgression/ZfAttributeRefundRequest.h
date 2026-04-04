// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ZfAttributeRefundRequest.generated.h"

/**
 * Payload do evento Event.Progression.RefundAttributePoint.
 * Todos os campos representam QUANTOS pontos remover de cada atributo.
 * Sempre positivos — a GA_RefundAttributePoint converte para negativos internamente.
 *
 * Uso na widget (Blueprint):
 *   1. Construct Object from Class → ZfAttributeRefundRequest
 *   2. Preencha apenas os campos dos atributos que quer refundar (0 = não muda)
 *   3. Make Gameplay Event Data → OptionalObject = request
 *   4. Send Gameplay Event to Actor → Event.Progression.RefundAttributePoint
 */
UCLASS(BlueprintType)
class ZF8DMOVING_API UZfAttributeRefundRequest : public UObject
{
	GENERATED_BODY()

public:

	/** Pontos a remover de Strength. 0 = não muda. */
	UPROPERTY(BlueprintReadWrite, Category = "AttributeRefund")
	int32 StrengthPointsToRemove = 0;

	/** Pontos a remover de Dexterity. 0 = não muda. */
	UPROPERTY(BlueprintReadWrite, Category = "AttributeRefund")
	int32 DexterityPointsToRemove = 0;

	/** Pontos a remover de Intelligence. 0 = não muda. */
	UPROPERTY(BlueprintReadWrite, Category = "AttributeRefund")
	int32 IntelligencePointsToRemove = 0;

	/** Pontos a remover de Constitution. 0 = não muda. */
	UPROPERTY(BlueprintReadWrite, Category = "AttributeRefund")
	int32 ConstitutionPointsToRemove = 0;

	/** Pontos a remover de Conviction. 0 = não muda. */
	UPROPERTY(BlueprintReadWrite, Category = "AttributeRefund")
	int32 ConvictionPointsToRemove = 0;

	/** Total de pontos que voltam ao pool. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AttributeRefund")
	int32 GetTotalPointsToReturn() const
	{
		return StrengthPointsToRemove + DexterityPointsToRemove + IntelligencePointsToRemove
		     + ConstitutionPointsToRemove + ConvictionPointsToRemove;
	}

	/** Retorna true se todos os campos são >= 0 e há alguma mudança. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AttributeRefund")
	bool IsValid() const
	{
		return StrengthPointsToRemove     >= 0
		    && DexterityPointsToRemove    >= 0
		    && IntelligencePointsToRemove >= 0
		    && ConstitutionPointsToRemove >= 0
		    && ConvictionPointsToRemove   >= 0
		    && GetTotalPointsToReturn()   > 0;
	}
};