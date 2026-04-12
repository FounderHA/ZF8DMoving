// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/ZfResourceAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

UZfResourceAttributeSet::UZfResourceAttributeSet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{ 
}

void UZfResourceAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UZfResourceAttributeSet, CurrentHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfResourceAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfResourceAttributeSet, CurrentMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfResourceAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfResourceAttributeSet, CurrentStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfResourceAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
}

void UZfResourceAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	if (Data.EvaluatedData.Attribute == GetCurrentHealthAttribute())
		SetCurrentHealth(FMath::Clamp(GetCurrentHealth(), 0.f, GetMaxHealth()));

	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
		SetMaxHealth(FMath::Max(GetMaxHealth(), 0.f));

	else if (Data.EvaluatedData.Attribute == GetCurrentManaAttribute())
		SetCurrentMana(FMath::Clamp(GetCurrentMana(), 0.f, GetMaxMana()));

	else if (Data.EvaluatedData.Attribute == GetMaxManaAttribute())
		SetMaxMana(FMath::Max(GetMaxMana(), 0.f));

	else if (Data.EvaluatedData.Attribute == GetCurrentStaminaAttribute())
		SetCurrentStamina(FMath::Clamp(GetCurrentStamina(), 0.f, GetMaxStamina()));

	else if (Data.EvaluatedData.Attribute == GetMaxStaminaAttribute())
		SetMaxStamina(FMath::Max(GetMaxStamina(), 0.f));
}

void UZfResourceAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
	
	if (Attribute == GetCurrentHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UZfResourceAttributeSet::OnRep_CurrentHealth(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResourceAttributeSet, CurrentHealth, OldValue);
}

void UZfResourceAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResourceAttributeSet, MaxHealth, OldValue);
}

void UZfResourceAttributeSet::OnRep_CurrentMana(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResourceAttributeSet, CurrentMana, OldValue);
}

void UZfResourceAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResourceAttributeSet, MaxMana, OldValue);
}

void UZfResourceAttributeSet::OnRep_CurrentStamina(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResourceAttributeSet, CurrentStamina, OldValue);
}

void UZfResourceAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResourceAttributeSet, MaxStamina, OldValue);
}
