// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Attributes/ModMagnitudeCalculation/ZfStaminaCostMMC.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ZfUtilityAttributeSet.h"
#include "Tags/ZfGameplayTags.h"
#include "Player/ZfPlayerState.h"

float UZfStaminaCostMMC::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
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

	float StaminaCostReduction = 0.f;
	if (const UZfUtilityAttributeSet* UtilSet = ASC->GetSet<UZfUtilityAttributeSet>())
	{
		StaminaCostReduction = FMath::Clamp(UtilSet->GetStaminaCostReduction(), 0.f, 100.f);
	}

	const float FinalCost = CostBase * (1.f - StaminaCostReduction / 100.f);

	UE_LOG(LogTemp, Log,
		TEXT("ZfMMC_StaminaCost: Base=%.1f | StaminaCostReduction=%.1f%% | Final=%.1f"),
		CostBase, StaminaCostReduction, FinalCost);

	return -FinalCost;
}