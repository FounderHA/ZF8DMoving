// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/MainAttributes/ZfStrengthSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

UZfStrengthSet::UZfStrengthSet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UZfStrengthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UZfStrengthSet, Strength, COND_None, REPNOTIFY_Always);
}


void UZfStrengthSet::OnRep_Strength(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfStrengthSet, Strength, OldValue);
}
