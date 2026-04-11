// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "ZfIntelligenceMMC.generated.h"

/**
 * 
 */
UCLASS()
class ZF8DMOVING_API UZfIntelligenceMMC : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
	
public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
