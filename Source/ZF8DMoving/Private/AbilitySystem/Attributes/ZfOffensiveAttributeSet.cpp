// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Attributes/ZfOffensiveAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

UZfOffensiveAttributeSet::UZfOffensiveAttributeSet(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UZfOffensiveAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UZfOffensiveAttributeSet, PhysicalDamage,    COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfOffensiveAttributeSet, MagicalDamage,     COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfOffensiveAttributeSet, PhysicalPain,      COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfOffensiveAttributeSet, MagicalPain,       COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfOffensiveAttributeSet, CriticalHitChance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfOffensiveAttributeSet, CriticalDamage,    COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfOffensiveAttributeSet, PoiseDamage,       COND_None, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UZfOffensiveAttributeSet, AttackSpeed,       COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfOffensiveAttributeSet, CastSpeed,         COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfOffensiveAttributeSet, CooldownReduction, COND_None, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UZfOffensiveAttributeSet, BackstabDamage,    COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfOffensiveAttributeSet, FirstHitDamage,    COND_None, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UZfOffensiveAttributeSet, BurnBuildup,       COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfOffensiveAttributeSet, FreezeBuildup,     COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfOffensiveAttributeSet, ShockBuildup,      COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfOffensiveAttributeSet, BleedBuildup,      COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfOffensiveAttributeSet, PoisonBuildup,     COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfOffensiveAttributeSet, StunBuildup,       COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfOffensiveAttributeSet, SlowBuildup,       COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfOffensiveAttributeSet, SleepBuildup,      COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfOffensiveAttributeSet, RootBuildup,       COND_None, REPNOTIFY_Always);
}

void UZfOffensiveAttributeSet::OnRep_PhysicalDamage(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfOffensiveAttributeSet, PhysicalDamage, OldValue);
}

void UZfOffensiveAttributeSet::OnRep_MagicalDamage(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfOffensiveAttributeSet, MagicalDamage, OldValue);
}

void UZfOffensiveAttributeSet::OnRep_PhysicalPain(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfOffensiveAttributeSet, PhysicalPain, OldValue);
}

void UZfOffensiveAttributeSet::OnRep_MagicalPain(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfOffensiveAttributeSet, MagicalPain, OldValue);
}

void UZfOffensiveAttributeSet::OnRep_CriticalHitChance(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfOffensiveAttributeSet, CriticalHitChance, OldValue);
}

void UZfOffensiveAttributeSet::OnRep_CriticalDamage(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfOffensiveAttributeSet, CriticalDamage, OldValue);
}

void UZfOffensiveAttributeSet::OnRep_PoiseDamage(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfOffensiveAttributeSet, PoiseDamage, OldValue);
}

void UZfOffensiveAttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfOffensiveAttributeSet, AttackSpeed, OldValue);
}

void UZfOffensiveAttributeSet::OnRep_CastSpeed(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfOffensiveAttributeSet, CastSpeed, OldValue);
}

void UZfOffensiveAttributeSet::OnRep_CooldownReduction(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfOffensiveAttributeSet, CooldownReduction, OldValue);
}

void UZfOffensiveAttributeSet::OnRep_BackstabDamage(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfOffensiveAttributeSet, BackstabDamage, OldValue);
}

void UZfOffensiveAttributeSet::OnRep_FirstHitDamage(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfOffensiveAttributeSet, FirstHitDamage, OldValue);
}

void UZfOffensiveAttributeSet::OnRep_BurnBuildup(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfOffensiveAttributeSet, BurnBuildup, OldValue);
}

void UZfOffensiveAttributeSet::OnRep_FreezeBuildup(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfOffensiveAttributeSet, FreezeBuildup, OldValue);
}

void UZfOffensiveAttributeSet::OnRep_ShockBuildup(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfOffensiveAttributeSet, ShockBuildup, OldValue);
}

void UZfOffensiveAttributeSet::OnRep_BleedBuildup(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfOffensiveAttributeSet, BleedBuildup, OldValue);
}

void UZfOffensiveAttributeSet::OnRep_PoisonBuildup(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfOffensiveAttributeSet, PoisonBuildup, OldValue);
}

void UZfOffensiveAttributeSet::OnRep_StunBuildup(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfOffensiveAttributeSet, StunBuildup, OldValue);
}

void UZfOffensiveAttributeSet::OnRep_SlowBuildup(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfOffensiveAttributeSet, SlowBuildup, OldValue);
}

void UZfOffensiveAttributeSet::OnRep_SleepBuildup(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfOffensiveAttributeSet, SleepBuildup, OldValue);
}

void UZfOffensiveAttributeSet::OnRep_RootBuildup(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UZfOffensiveAttributeSet, RootBuildup, OldValue);
}