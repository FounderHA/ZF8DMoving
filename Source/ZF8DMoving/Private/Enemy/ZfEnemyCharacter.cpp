// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/ZfEnemyCharacter.h"
#include "AbilitySystem/ZfAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ZfHealthSet.h"


AZfEnemyCharacter::AZfEnemyCharacter()
{

	AbilitySystemComponent = CreateDefaultSubobject<UZfAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	HealthSet = CreateDefaultSubobject<UZfHealthSet>(TEXT("HealthSet"));
	
}
