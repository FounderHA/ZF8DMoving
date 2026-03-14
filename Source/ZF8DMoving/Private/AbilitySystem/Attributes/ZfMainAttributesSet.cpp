// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/ZfMainAttributesSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

UZfMainAttributesSet::UZfMainAttributesSet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UZfMainAttributesSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UZfMainAttributesSet, Strength, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfMainAttributesSet, Dexterity, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfMainAttributesSet, Intelligence, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfMainAttributesSet, Constitution, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfMainAttributesSet, Conviction, COND_None, REPNOTIFY_Always);
}


void UZfMainAttributesSet::OnRep_Strength(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfMainAttributesSet, Strength, OldValue);
}

void UZfMainAttributesSet::OnRep_Dexterity(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfMainAttributesSet, Dexterity, OldValue);
}

void UZfMainAttributesSet::OnRep_Intelligence(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfMainAttributesSet, Intelligence, OldValue);
}

void UZfMainAttributesSet::OnRep_Constitution(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfMainAttributesSet, Constitution, OldValue);
}

void UZfMainAttributesSet::OnRep_Conviction(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfMainAttributesSet, Conviction, OldValue);
}
