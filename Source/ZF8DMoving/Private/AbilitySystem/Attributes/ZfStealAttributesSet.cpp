// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Attributes/ZfStealAttributesSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

UZfStealAttributesSet::UZfStealAttributesSet(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UZfStealAttributesSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UZfStealAttributesSet, FlatLifeStealOnHit,       COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfStealAttributesSet, FlatManaStealOnHit,       COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfStealAttributesSet, FlatStaminaStealOnHit,    COND_None, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UZfStealAttributesSet, PercentLifeStealOnHit,    COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfStealAttributesSet, PercentManaStealOnHit,    COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfStealAttributesSet, PercentStaminaStealOnHit, COND_None, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UZfStealAttributesSet, FlatLifeStealOnKill,      COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfStealAttributesSet, FlatManaStealOnKill,      COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfStealAttributesSet, FlatStaminaStealOnKill,   COND_None, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UZfStealAttributesSet, PercentLifeStealOnKill,   COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfStealAttributesSet, PercentManaStealOnKill,   COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UZfStealAttributesSet, PercentStaminaStealOnKill,COND_None, REPNOTIFY_Always);
}

void UZfStealAttributesSet::OnRep_FlatLifeStealOnHit(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfStealAttributesSet, FlatLifeStealOnHit, OldValue); }

void UZfStealAttributesSet::OnRep_FlatManaStealOnHit(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfStealAttributesSet, FlatManaStealOnHit, OldValue); }

void UZfStealAttributesSet::OnRep_FlatStaminaStealOnHit(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfStealAttributesSet, FlatStaminaStealOnHit, OldValue); }

void UZfStealAttributesSet::OnRep_PercentLifeStealOnHit(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfStealAttributesSet, PercentLifeStealOnHit, OldValue); }

void UZfStealAttributesSet::OnRep_PercentManaStealOnHit(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfStealAttributesSet, PercentManaStealOnHit, OldValue); }

void UZfStealAttributesSet::OnRep_PercentStaminaStealOnHit(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfStealAttributesSet, PercentStaminaStealOnHit, OldValue); }

void UZfStealAttributesSet::OnRep_FlatLifeStealOnKill(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfStealAttributesSet, FlatLifeStealOnKill, OldValue); }

void UZfStealAttributesSet::OnRep_FlatManaStealOnKill(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfStealAttributesSet, FlatManaStealOnKill, OldValue); }

void UZfStealAttributesSet::OnRep_FlatStaminaStealOnKill(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfStealAttributesSet, FlatStaminaStealOnKill, OldValue); }

void UZfStealAttributesSet::OnRep_PercentLifeStealOnKill(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfStealAttributesSet, PercentLifeStealOnKill, OldValue); }

void UZfStealAttributesSet::OnRep_PercentManaStealOnKill(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfStealAttributesSet, PercentManaStealOnKill, OldValue); }

void UZfStealAttributesSet::OnRep_PercentStaminaStealOnKill(const FGameplayAttributeData& OldValue) const
{ GAMEPLAYATTRIBUTE_REPNOTIFY(UZfStealAttributesSet, PercentStaminaStealOnKill, OldValue); }