// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/ZfProgressionAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

UZfProgressionAttributeSet::UZfProgressionAttributeSet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{ 
}

void UZfProgressionAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UZfProgressionAttributeSet, AttributePoints, COND_None, REPNOTIFY_Always);
}

void UZfProgressionAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
}

void UZfProgressionAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
	
}

void UZfProgressionAttributeSet::OnRep_AttributePoints(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfProgressionAttributeSet, AttributePoints, OldValue);
}

