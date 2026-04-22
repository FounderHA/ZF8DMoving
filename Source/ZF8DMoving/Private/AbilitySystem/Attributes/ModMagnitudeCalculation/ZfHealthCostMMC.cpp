// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Attributes/ModMagnitudeCalculation/ZfHealthCostMMC.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ZfUtilityAttributeSet.h"
#include "Tags/ZfGameplayTags.h"
#include "Player/ZfPlayerState.h"

float UZfHealthCostMMC::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const float CostBase = Spec.GetSetByCallerMagnitude(
		ZfAbilityTreeTags::SkillTree_Data_AbilityCost, false, 0.f);

	if (CostBase <= 0.f) return 0.f;

	UAbilitySystemComponent* ASC = Spec.GetContext().GetOriginalInstigatorAbilitySystemComponent();
	if (!ASC) return -CostBase;

	AActor* AvatarActor = ASC->GetAvatarActor();
	if (!AvatarActor) return -CostBase;

	APawn* Pawn = Cast<APawn>(AvatarActor);
	if (!Pawn) return -CostBase;

	AZfPlayerState* PS = Pawn->GetPlayerState<AZfPlayerState>();
	if (!PS) return -CostBase;

	float HealthCostReduction = 0.f;
	if (const UZfUtilityAttributeSet* UtilSet = ASC->GetSet<UZfUtilityAttributeSet>())
	{
		HealthCostReduction = FMath::Clamp(UtilSet->GetHealthCostReduction(), 0.f, 100.f);
	}

	const float FinalCost = CostBase * (1.f - HealthCostReduction / 100.f);

	UE_LOG(LogTemp, Log,
		TEXT("ZfMMC_HealthCost: Base=%.1f | HealthCostReduction=%.1f%% | Final=%.1f"),
		CostBase, HealthCostReduction, FinalCost);

	return -FinalCost;
}