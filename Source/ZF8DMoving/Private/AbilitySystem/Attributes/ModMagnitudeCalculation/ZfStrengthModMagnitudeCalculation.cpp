// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/ModMagnitudeCalculation/ZfStrengthModMagnitudeCalculation.h"
#include "AbilitySystemComponent.h"
#include "player/ZfPlayerState.h"



float UZfStrengthModMagnitudeCalculation::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	
	float Result = 0.f;

// pegar avatar actor 
	
	AActor* AvatarActor = Spec.GetContext().GetOriginalInstigatorAbilitySystemComponent()->GetAvatarActor();
	
	/*UAbilitySystemComponent* MyASC = Spec.GetContext().GetOriginalInstigatorAbilitySystemComponent();
	const FActiveGameplayEffectsContainer& ActiveGameplayEffectsContainer = MyASC->GetActiveGameplayEffects();
	for (auto ActiveEffectIt = ActiveGameplayEffectsContainer.CreateConstIterator(); ActiveEffectIt; ++ActiveEffectIt)
	{
		const FActiveGameplayEffect ActiveEffect = *ActiveEffectIt;
		const FGameplayTagContainer TagContainer = ActiveEffect.Spec.GetDynamicAssetTags();
		if (TagContainer.HasTagExact(FGameplayTag::RequestGameplayTag("GameplayEffect.Type.Burning")))
		{
			
		}
	}*/

	if (!AvatarActor)
		return Result;

	APawn* Pawn = Cast<APawn>(AvatarActor);

	if (!Pawn)
		return Result;

	AZfPlayerState* PS = Pawn->GetPlayerState<AZfPlayerState>();
	
	if (!PS)
		return Result;

		// pegar valores
	float BaseStrength = 0.f;
	float AllocatedStrength = PS->AllocatedPoints.StrengthPoints;
	
	if (PS->CharacterClassData)
	{
			BaseStrength = PS->CharacterClassData->Strength;
	}

	

	// lógica final
	Result = BaseStrength + AllocatedStrength;

	return Result;
}
