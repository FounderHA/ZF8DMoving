// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "ZfManaCostMMC.generated.h"

/**
 * MMC que calcula o custo final de Mana de uma ability.
 *
 * Fórmula:
 *   CustoFinal = CustoBase * (1 - ManaCostReduction / 100)
 *
 * CustoBase         → valor passado via SetByCaller(SkillTree.Data.AbilityCost)
 * ManaCostReduction → lido diretamente do UZfUtilityAttributeSet via ASC
 *
 * Como usar no editor:
 *   GE_ConsumeMana
 *     Duration Policy : Instant
 *     Modifier
 *       Attribute  : ZfResourceAttributeSet.Mana
 *       Operation  : Add
 *       Magnitude  : Custom Calculation Class → ZfMMC_ManaCost
 */
UCLASS()
class ZF8DMOVING_API UZfManaCostMMC : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};