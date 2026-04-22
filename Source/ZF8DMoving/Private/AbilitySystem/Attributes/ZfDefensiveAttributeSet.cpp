// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Attributes/ZfDefensiveAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

UZfDefensiveAttributeSet::UZfDefensiveAttributeSet(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UZfDefensiveAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UZfDefensiveAttributeSet, PhysicalResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfDefensiveAttributeSet, MagicalResistance,  COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfDefensiveAttributeSet, Tenacity,           COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfDefensiveAttributeSet, CriticalResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfDefensiveAttributeSet, PoiseResistance,    COND_None, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UZfDefensiveAttributeSet, BurnThreshold,      COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfDefensiveAttributeSet, FreezeThreshold,    COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfDefensiveAttributeSet, ShockThreshold,     COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfDefensiveAttributeSet, BleedThreshold,     COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfDefensiveAttributeSet, PoisonThreshold,    COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfDefensiveAttributeSet, StunThreshold,      COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfDefensiveAttributeSet, SlowThreshold,      COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfDefensiveAttributeSet, SleepThreshold,     COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfDefensiveAttributeSet, RootThreshold,      COND_None, REPNOTIFY_Always);
}

void UZfDefensiveAttributeSet::OnRep_PhysicalResistance(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfDefensiveAttributeSet, PhysicalResistance, OldValue);
}

void UZfDefensiveAttributeSet::OnRep_MagicalResistance(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfDefensiveAttributeSet, MagicalResistance, OldValue);
}

void UZfDefensiveAttributeSet::OnRep_Tenacity(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfDefensiveAttributeSet, Tenacity, OldValue);
}

void UZfDefensiveAttributeSet::OnRep_CriticalResistance(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfDefensiveAttributeSet, CriticalResistance, OldValue);
}

void UZfDefensiveAttributeSet::OnRep_PoiseResistance(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfDefensiveAttributeSet, PoiseResistance, OldValue);
}

void UZfDefensiveAttributeSet::OnRep_BurnThreshold(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfDefensiveAttributeSet, BurnThreshold, OldValue);
}

void UZfDefensiveAttributeSet::OnRep_FreezeThreshold(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfDefensiveAttributeSet, FreezeThreshold, OldValue);
}

void UZfDefensiveAttributeSet::OnRep_ShockThreshold(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfDefensiveAttributeSet, ShockThreshold, OldValue);
}

void UZfDefensiveAttributeSet::OnRep_BleedThreshold(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfDefensiveAttributeSet, BleedThreshold, OldValue);
}

void UZfDefensiveAttributeSet::OnRep_PoisonThreshold(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfDefensiveAttributeSet, PoisonThreshold, OldValue);
}

void UZfDefensiveAttributeSet::OnRep_StunThreshold(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfDefensiveAttributeSet, StunThreshold, OldValue);
}

void UZfDefensiveAttributeSet::OnRep_SlowThreshold(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfDefensiveAttributeSet, SlowThreshold, OldValue);
}

void UZfDefensiveAttributeSet::OnRep_SleepThreshold(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfDefensiveAttributeSet, SleepThreshold, OldValue);
}

void UZfDefensiveAttributeSet::OnRep_RootThreshold(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfDefensiveAttributeSet, RootThreshold, OldValue);
}