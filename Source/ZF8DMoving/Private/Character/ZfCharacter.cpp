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
	// Caminho NPC/AI: ASC vive no próprio Character.
	// Não chamado por AZfPlayerCharacter — ele tem seu próprio caminho
	// via PossessedBy/OnRep_PlayerState apontando para o ASC do PlayerState.
	if (!IsValid(AbilitySystemComponent) || !HasAuthority())
	{
		return;
	}
 
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
 
	// Concede as startup abilities após o ActorInfo estar configurado.
	GrantStartupAbilities();
}

void AZfCharacter::GrantStartupAbilities()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !HasAuthority()) return;
 
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : StartupAbilities)
	{
		if (!AbilityClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("AZfCharacter: Entrada nula em StartupAbilities em '%s' — ignorada."),
				*GetName());
			continue;
		}
 
		// Guard contra dupla concessão: se a ability já existe no ASC, pula.
		// Cobre respawns (novo pawn, ASC no PlayerState persiste) e
		// re-chamadas acidentais de InitAbilityActorInfo.
		if (ASC->FindAbilitySpecFromClass(AbilityClass))
		{
			continue;
		}
 
		ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
	}
}

void AZfCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


