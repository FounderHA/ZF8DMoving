// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ZfCharacter.h"
#include "AbilitySystemComponent.h"

// Sets default values
AZfCharacter::AZfCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

UAbilitySystemComponent* AZfCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AZfCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	InitAbilityActorInfo();
}

void AZfCharacter::InitAbilityActorInfo()
{
	if (!IsValid(AbilitySystemComponent) || !HasAuthority())
	{
		return;
	}
	
	AbilitySystemComponent->InitAbilityActorInfo(this,this);
}


