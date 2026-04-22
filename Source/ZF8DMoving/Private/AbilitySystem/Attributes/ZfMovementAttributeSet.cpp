// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Attributes/ZfMovementAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

UZfMovementAttributeSet::UZfMovementAttributeSet(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UZfMovementAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UZfMovementAttributeSet, MoveSpeed,    COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfMovementAttributeSet, DashDistance, COND_None, REPNOTIFY_Always);
}

void UZfMovementAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfMovementAttributeSet, MoveSpeed, OldValue);
}

void UZfMovementAttributeSet::OnRep_DashDistance(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfMovementAttributeSet, DashDistance, OldValue);
}

void UZfMovementAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Clamp(NewValue, MinMoveSpeed, MaxMoveSpeed);
		return;
	}

	if (Attribute == GetDashDistanceAttribute())
	{
		NewValue = FMath::Clamp(NewValue, MinDashDistance, MaxDashDistance);
	}
}

void UZfMovementAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
}