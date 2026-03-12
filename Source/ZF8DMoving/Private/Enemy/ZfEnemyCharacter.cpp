// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/ZfEnemyCharacter.h"

#include "AbilitySystem/ZfAbilitySystemComponent.h"


AZfEnemyCharacter::AZfEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	AbilitySystemComponent = CreateDefaultSubobject<UZfAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
}
