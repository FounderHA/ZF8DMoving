// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/ZfDamageAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"


UZfDamageAttributeSet::UZfDamageAttributeSet(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UZfDamageAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UZfDamageAttributeSet, PhysicalDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfDamageAttributeSet, MagicalDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfDamageAttributeSet, CriticalDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfDamageAttributeSet, CriticalHitChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfDamageAttributeSet, AttackSpeed, COND_None, REPNOTIFY_Always);
}


void UZfDamageAttributeSet::OnRep_PhysicalDamage(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfDamageAttributeSet, PhysicalDamage, OldValue);
}

void UZfDamageAttributeSet::OnRep_MagicalDamage(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfDamageAttributeSet, MagicalDamage, OldValue);
}

void UZfDamageAttributeSet::OnRep_CriticalDamage(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfDamageAttributeSet, CriticalDamage, OldValue);
}

void UZfDamageAttributeSet::OnRep_CriticalHitChance(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfDamageAttributeSet, CriticalHitChance, OldValue);
}

void UZfDamageAttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfDamageAttributeSet, AttackSpeed, OldValue);
}
