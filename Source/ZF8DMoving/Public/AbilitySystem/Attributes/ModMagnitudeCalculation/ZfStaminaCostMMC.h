// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "ZfStaminaCostMMC.generated.h"

/**
 * MMC que calcula o custo final de Stamina de uma ability.
 *
 * Fórmula:
 *   CustoFinal = CustoBase * (1 - StaminaCostReduction / 100)
 *
 * CustoBase            → valor passado via SetByCaller(SkillTree.Data.AbilityCost)
 * StaminaCostReduction → lido diretamente do UZfUtilityAttributeSet via ASC
 *
 * Como usar no editor:
 *   GE_ConsumeStamina
 *     Duration Policy : Instant
 *     Modifier
 *       Attribute  : ZfResourceAttributeSet.Stamina
 *       Operation  : Add
 *       Magnitude  : Custom Calculation Class → ZfMMC_StaminaCost
 */
UCLASS()
class ZF8DMOVING_API UZfStaminaCostMMC : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};