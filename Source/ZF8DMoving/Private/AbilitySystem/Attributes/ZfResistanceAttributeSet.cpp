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
    DOREPLIFETIME_CONDITION_NOTIFY(UZfResistanceAttributeSet, MagicalResistance,  COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfResistanceAttributeSet, Tenacity,           COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfResistanceAttributeSet, CriticalResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfResistanceAttributeSet, PoiseResistance,    COND_None, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UZfResistanceAttributeSet, BurnThreshold,      COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfResistanceAttributeSet, FreezeThreshold,    COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfResistanceAttributeSet, ShockThreshold,     COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfResistanceAttributeSet, BleedThreshold,     COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfResistanceAttributeSet, PoisonThreshold,    COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfResistanceAttributeSet, StunThreshold,      COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfResistanceAttributeSet, SlowThreshold,      COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfResistanceAttributeSet, SleepThreshold,     COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfResistanceAttributeSet, RootThreshold,      COND_None, REPNOTIFY_Always);
}

void UZfResistanceAttributeSet::OnRep_PhysicalResistance(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistanceAttributeSet, PhysicalResistance, OldValue);
}

void UZfResistanceAttributeSet::OnRep_MagicalResistance(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistanceAttributeSet, MagicalResistance, OldValue);
}

void UZfResistanceAttributeSet::OnRep_Tenacity(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistanceAttributeSet, Tenacity, OldValue);
}

void UZfResistanceAttributeSet::OnRep_CriticalResistance(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistanceAttributeSet, CriticalResistance, OldValue);
}

void UZfResistanceAttributeSet::OnRep_PoiseResistance(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistanceAttributeSet, PoiseResistance, OldValue);
}

void UZfResistanceAttributeSet::OnRep_BurnThreshold(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistanceAttributeSet, BurnThreshold, OldValue);
}

void UZfResistanceAttributeSet::OnRep_FreezeThreshold(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistanceAttributeSet, FreezeThreshold, OldValue);
}

void UZfResistanceAttributeSet::OnRep_ShockThreshold(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistanceAttributeSet, ShockThreshold, OldValue);
}

void UZfResistanceAttributeSet::OnRep_BleedThreshold(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistanceAttributeSet, BleedThreshold, OldValue);
}

void UZfResistanceAttributeSet::OnRep_PoisonThreshold(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistanceAttributeSet, PoisonThreshold, OldValue);
}

void UZfResistanceAttributeSet::OnRep_StunThreshold(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistanceAttributeSet, StunThreshold, OldValue);
}

void UZfResistanceAttributeSet::OnRep_SlowThreshold(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistanceAttributeSet, SlowThreshold, OldValue);
}

void UZfResistanceAttributeSet::OnRep_SleepThreshold(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistanceAttributeSet, SleepThreshold, OldValue);
}

void UZfResistanceAttributeSet::OnRep_RootThreshold(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResistanceAttributeSet, RootThreshold, OldValue);
}