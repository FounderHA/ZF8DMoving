// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ZfPlayerState.h"
#include "AbilitySystem/ZfAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ZfHealthSet.h"
#include "AbilitySystem/Attributes/MainAttributes/ZfStrengthSet.h"

AZfPlayerState::AZfPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UZfAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	SetNetUpdateFrequency(100.0f);
	
	HealthSet = CreateDefaultSubobject<UZfHealthSet>(TEXT("HealthSet"));
	StrengthSet = CreateDefaultSubobject<UZfStrengthSet>(TEXT("StrengthSet"));
}

UAbilitySystemComponent* AZfPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UZfHealthSet* AZfPlayerState::GetHealthSet() const
{
	return HealthSet;
}

UZfStrengthSet* AZfPlayerState::GetStrengthSet() const
{
	return StrengthSet;
}
