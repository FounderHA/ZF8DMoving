// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "ZfHealthCostMMC.generated.h"

/**
 * MMC que calcula o custo final de Health de uma ability.
 *
 * Fórmula:
 *   CustoFinal = CustoBase * (1 - HealthCostReduction / 100)
 *
 * CustoBase           → valor passado via SetByCaller(SkillTree.Data.AbilityCost)
 * HealthCostReduction → lido diretamente do UZfUtilityAttributeSet via ASC
 *
 * Como usar no editor:
 *   GE_ConsumeHealth
 *     Duration Policy : Instant
 *     Modifier
 *       Attribute  : ZfResourceAttributeSet.Health
 *       Operation  : Add
 *       Magnitude  : Custom Calculation Class → ZfMMC_HealthCost
 */
UCLASS()
class ZF8DMOVING_API UZfHealthCostMMC : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};