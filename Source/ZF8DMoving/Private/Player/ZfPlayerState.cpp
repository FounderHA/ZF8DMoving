// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ZfPlayerState.h"

#include "AbilitySystem/ZfAbilitySystemComponent.h"

AZfPlayerState::AZfPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UZfAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	NetUpdateFrequency = 100.0f;
}

UAbilitySystemComponent* AZfPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
