// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/ModMagnitudeCalculation/ZfHealthNormalization.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ZfProgressionAttributeSet.h"
#include "AbilitySystem/Attributes/ZfResourceAttributeSet.h"
#include "player/ZfPlayerState.h"


float UZfHealthNormalization::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	float Result = 0.f;
	
	// --- Obtém ASC, Pawn e PlayerState ---
	UAbilitySystemComponent* ASC = Spec.GetContext().GetOriginalInstigatorAbilitySystemComponent();
	if (!ASC) return Result;
	
	// --- CurrentHealth ---
	float CurrentHealth = 0.f;
	if (const UZfResourceAttributeSet* ProgSet = ASC->GetSet<UZfResourceAttributeSet>())
	{
		CurrentHealth = ProgSet->GetHealth();
	}
	
	// --- MaxHealth ---
	float MaxHealth = 0.f;
	if (const UZfResourceAttributeSet* ProgSet = ASC->GetSet<UZfResourceAttributeSet>())
	{
		MaxHealth = ProgSet->GetMaxHealth();
	}
	
	// --- Resultado ---
	Result = CurrentHealth / MaxHealth;
	
	return Result;
}
