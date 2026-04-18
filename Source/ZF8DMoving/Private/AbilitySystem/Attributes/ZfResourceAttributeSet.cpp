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
	
	DOREPLIFETIME_CONDITION_NOTIFY(UZfResourceAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfResourceAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfResourceAttributeSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfResourceAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfResourceAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfResourceAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
}

void UZfResourceAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));

	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
		SetMaxHealth(FMath::Max(GetMaxHealth(), 0.f));

	else if (Data.EvaluatedData.Attribute == GetManaAttribute())
		SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));

	else if (Data.EvaluatedData.Attribute == GetMaxManaAttribute())
		SetMaxMana(FMath::Max(GetMaxMana(), 0.f));

	else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
		SetStamina(FMath::Clamp(GetStamina(), 0.f, GetMaxStamina()));

	else if (Data.EvaluatedData.Attribute == GetMaxStaminaAttribute())
		SetMaxStamina(FMath::Max(GetMaxStamina(), 0.f));
}

void UZfResourceAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
	
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UZfResourceAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResourceAttributeSet, Health, OldValue);
}

void UZfResourceAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResourceAttributeSet, MaxHealth, OldValue);
}

void UZfResourceAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResourceAttributeSet, Mana, OldValue);
}

void UZfResourceAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResourceAttributeSet, MaxMana, OldValue);
}

void UZfResourceAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResourceAttributeSet, Stamina, OldValue);
}

void UZfResourceAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfResourceAttributeSet, MaxStamina, OldValue);
}
