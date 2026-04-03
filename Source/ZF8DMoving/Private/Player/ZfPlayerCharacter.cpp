// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/ZfPlayerCharacter.h"
#include "Player/ZfPlayerState.h"

// Sets default values
AZfPlayerCharacter::AZfPlayerCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AZfPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AZfPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

// Called to bind functionality to input
void AZfPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AZfPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	InitAbilityActorInfo();
	
	// Concede as StartupAbilities configuradas no Blueprint deste Character.
	// GrantStartupAbilities usa FindAbilitySpecFromClass como guard —
	// seguro contra dupla concessão em respawns.
	GrantStartupAbilities();
}

void AZfPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	InitAbilityActorInfo();
}

UAbilitySystemComponent* AZfPlayerCharacter::GetAbilitySystemComponent() const
{
	AZfPlayerState* ZfPlayerState = GetPlayerState<AZfPlayerState>();
	
	return ZfPlayerState ? ZfPlayerState->GetAbilitySystemComponent() : nullptr;
}

void AZfPlayerCharacter::InitAbilityActorInfo()
{
	AZfPlayerState*	ZfPlayerState = GetPlayerState<AZfPlayerState>();
	if (!IsValid(ZfPlayerState))
	{
		return;
	}
	
	if (!HasAuthority())
	{
		return;
	}
	
	AbilitySystemComponent = ZfPlayerState->GetAbilitySystemComponent();
	ZfPlayerState->GetAbilitySystemComponent()->InitAbilityActorInfo(ZfPlayerState,this);
}

	