// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "ZfDexterityModMagnitudeCalculation.generated.h"

/**
 * 
 */
UCLASS()
class ZF8DMOVING_API UZfDexterityModMagnitudeCalculation : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
	
public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
