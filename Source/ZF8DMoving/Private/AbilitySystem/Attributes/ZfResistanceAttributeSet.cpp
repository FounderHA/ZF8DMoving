// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/ZfResistanceAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

UZfResistanceAttributeSet::UZfResistanceAttributeSet(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UZfResistanceAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UZfResistanceAttributeSet, PhysicalResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfResistanceAttributeSet, MagicResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfResistanceAttributeSet, StunResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfResistanceAttributeSet, SlowResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfResistanceAttributeSet, CriticalResistance, COND_None, REPNOTIFY_Always);
}


void UZfResistanceAttributeSet::OnRep_PhysicalResistance(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistanceAttributeSet, PhysicalResistance, OldValue);
}

void UZfResistanceAttributeSet::OnRep_MagicResistance(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistanceAttributeSet, MagicResistance, OldValue);
}

void UZfResistanceAttributeSet::OnRep_StunResistance(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistanceAttributeSet, StunResistance, OldValue);
}

void UZfResistanceAttributeSet::OnRep_SlowResistance(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistanceAttributeSet, SlowResistance, OldValue);
}

void UZfResistanceAttributeSet::OnRep_CriticalResistance(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistanceAttributeSet, CriticalResistance, OldValue);
}
