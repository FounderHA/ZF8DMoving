// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ZfPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Inventory/ZfInventoryComponent.h"
#include "AbilitySystem/ZfAbilitySystemComponent.h"
#include "Inventory/ZfEquipmentComponent.h"
#include "AbilitySystem/Attributes/ZfResourceAttributeSet.h"
#include "AbilitySystem/Attributes/ZfMainAttributeSet.h"
#include "AbilitySystem/Attributes/ZfProgressionAttributeSet.h"
#include "AbilitySystem/Attributes/ZfDamageAttributeSet.h"
#include "AbilitySystem/Attributes/ZfResistanceAttributeSet.h"

AZfPlayerState::AZfPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UZfAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	InventoryComponent = CreateDefaultSubobject<UZfInventoryComponent>(TEXT("InventoryComponent"));
	InventoryComponent->SetIsReplicated(true);

	EquipmentComponent = CreateDefaultSubobject<UZfEquipmentComponent>(TEXT("EquipmentComponent"));
	EquipmentComponent->SetIsReplicated(true);
	
	SetNetUpdateFrequency(100.0f);
	
	ResourceAttributeSet = CreateDefaultSubobject<UZfResourceAttributeSet>(TEXT("ResourceAttributeSet"));
	MainAttributeSet = CreateDefaultSubobject<UZfMainAttributeSet>(TEXT("MainAttributeSet"));
	ProgressionAttributeSet = CreateDefaultSubobject<UZfProgressionAttributeSet>(TEXT("ProgressionAttributeSet"));
	DamageAttributeSet = CreateDefaultSubobject<UZfDamageAttributeSet>(TEXT("DamageAttributeSet"));
	ResistanceAttributeSet = CreateDefaultSubobject<UZfResistanceAttributeSet>(TEXT("ResistanceAttributeSet"));
}

UAbilitySystemComponent* AZfPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AZfPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AZfPlayerState, CharacterClassData);
}

UZfResourceAttributeSet* AZfPlayerState::GetResourceAttributeSet() const
{
	return ResourceAttributeSet;
}

UZfMainAttributeSet* AZfPlayerState::GetMainAttributeSet() const
{
	return MainAttributeSet;
}

UZfProgressionAttributeSet* AZfPlayerState::GetProgressionAttributeSet() const
{
	return ProgressionAttributeSet;
}

UZfDamageAttributeSet* AZfPlayerState::GetDamageAttributeSet() const
{
	return DamageAttributeSet;
}

UZfResistanceAttributeSet* AZfPlayerState::GetResistanceAttributeSet() const
{
	return ResistanceAttributeSet;
}


//
//============================ Debug Attribut ============================
//



