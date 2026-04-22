// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Attributes/ZfUtilityAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

UZfUtilityAttributeSet::UZfUtilityAttributeSet(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UZfUtilityAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UZfUtilityAttributeSet, FlatLifeStealOnHit,       COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfUtilityAttributeSet, FlatManaStealOnHit,       COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfUtilityAttributeSet, FlatStaminaStealOnHit,    COND_None, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UZfUtilityAttributeSet, PercentLifeStealOnHit,    COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfUtilityAttributeSet, PercentManaStealOnHit,    COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfUtilityAttributeSet, PercentStaminaStealOnHit, COND_None, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UZfUtilityAttributeSet, FlatLifeStealOnKill,      COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfUtilityAttributeSet, FlatManaStealOnKill,      COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfUtilityAttributeSet, FlatStaminaStealOnKill,   COND_None, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UZfUtilityAttributeSet, PercentLifeStealOnKill,   COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfUtilityAttributeSet, PercentManaStealOnKill,   COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfUtilityAttributeSet, PercentStaminaStealOnKill,COND_None, REPNOTIFY_Always);
    
    DOREPLIFETIME_CONDITION_NOTIFY(UZfUtilityAttributeSet, HealthRegen,              COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfUtilityAttributeSet, ManaRegen,                COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfUtilityAttributeSet, StaminaRegen,             COND_None, REPNOTIFY_Always);
    
    DOREPLIFETIME_CONDITION_NOTIFY(UZfUtilityAttributeSet, HealthCostReduction,      COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfUtilityAttributeSet, ManaCostReduction,        COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfUtilityAttributeSet, StaminaCostReduction,     COND_None, REPNOTIFY_Always);
}

void UZfUtilityAttributeSet::OnRep_FlatLifeStealOnHit(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfUtilityAttributeSet, FlatLifeStealOnHit, OldValue); }

void UZfUtilityAttributeSet::OnRep_FlatManaStealOnHit(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfUtilityAttributeSet, FlatManaStealOnHit, OldValue); }

void UZfUtilityAttributeSet::OnRep_FlatStaminaStealOnHit(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfUtilityAttributeSet, FlatStaminaStealOnHit, OldValue); }

void UZfUtilityAttributeSet::OnRep_PercentLifeStealOnHit(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfUtilityAttributeSet, PercentLifeStealOnHit, OldValue); }

void UZfUtilityAttributeSet::OnRep_PercentManaStealOnHit(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfUtilityAttributeSet, PercentManaStealOnHit, OldValue); }

void UZfUtilityAttributeSet::OnRep_PercentStaminaStealOnHit(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfUtilityAttributeSet, PercentStaminaStealOnHit, OldValue); }

void UZfUtilityAttributeSet::OnRep_FlatLifeStealOnKill(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfUtilityAttributeSet, FlatLifeStealOnKill, OldValue); }

void UZfUtilityAttributeSet::OnRep_FlatManaStealOnKill(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfUtilityAttributeSet, FlatManaStealOnKill, OldValue); }

void UZfUtilityAttributeSet::OnRep_FlatStaminaStealOnKill(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfUtilityAttributeSet, FlatStaminaStealOnKill, OldValue); }

void UZfUtilityAttributeSet::OnRep_PercentLifeStealOnKill(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfUtilityAttributeSet, PercentLifeStealOnKill, OldValue); }

void UZfUtilityAttributeSet::OnRep_PercentManaStealOnKill(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfUtilityAttributeSet, PercentManaStealOnKill, OldValue); }

void UZfUtilityAttributeSet::OnRep_PercentStaminaStealOnKill(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfUtilityAttributeSet, PercentStaminaStealOnKill, OldValue); }

void UZfUtilityAttributeSet::OnRep_HealthRegen(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfUtilityAttributeSet, HealthRegen, OldValue); }

void UZfUtilityAttributeSet::OnRep_ManaRegen(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfUtilityAttributeSet, ManaRegen, OldValue); }

void UZfUtilityAttributeSet::OnRep_StaminaRegen(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfUtilityAttributeSet, StaminaRegen, OldValue); }

void UZfUtilityAttributeSet::OnRep_HealthCostReduction(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfUtilityAttributeSet, HealthCostReduction, OldValue); }

void UZfUtilityAttributeSet::OnRep_ManaCostReduction(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfUtilityAttributeSet, ManaCostReduction, OldValue); }

void UZfUtilityAttributeSet::OnRep_StaminaCostReduction(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfUtilityAttributeSet, StaminaCostReduction, OldValue); }