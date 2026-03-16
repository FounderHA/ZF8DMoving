// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/ZfResistenceAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

UZfResistenceAttributeSet::UZfResistenceAttributeSet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UZfResistenceAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UZfResistenceAttributeSet, PhysicalResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfResistenceAttributeSet, MagicResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfResistenceAttributeSet, StunResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfResistenceAttributeSet, SlowResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfResistenceAttributeSet, CriticalResistance, COND_None, REPNOTIFY_Always);
}


void UZfResistenceAttributeSet::OnRep_PhysicalResistance(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistenceAttributeSet, PhysicalResistance, OldValue);
}

void UZfResistenceAttributeSet::OnRep_MagicResistance(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistenceAttributeSet, MagicResistance, OldValue);
}

void UZfResistenceAttributeSet::OnRep_StunResistance(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistenceAttributeSet, StunResistance, OldValue);
}

void UZfResistenceAttributeSet::OnRep_SlowResistance(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistenceAttributeSet, SlowResistance, OldValue);
}

void UZfResistenceAttributeSet::OnRep_CriticalResistance(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistenceAttributeSet, CriticalResistance, OldValue);
}
