// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ZfPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystem/ZfAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ZfHealthSet.h"
#include "AbilitySystem/Attributes/ZfMainAttributesSet.h"

AZfPlayerState::AZfPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UZfAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	SetNetUpdateFrequency(100.0f);
	
	HealthSet = CreateDefaultSubobject<UZfHealthSet>(TEXT("HealthSet"));
	StrengthSet = CreateDefaultSubobject<UZfMainAttributesSet>(TEXT("StrengthSet"));
}

UAbilitySystemComponent* AZfPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AZfPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(AZfPlayerState, AllocatedPoints, COND_None, REPNOTIFY_Always);
}

UZfHealthSet* AZfPlayerState::GetHealthSet() const
{
	return HealthSet;
}

UZfMainAttributesSet* AZfPlayerState::GetStrengthSet() const
{
	return StrengthSet;
}
