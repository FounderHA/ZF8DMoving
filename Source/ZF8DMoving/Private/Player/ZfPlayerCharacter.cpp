// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/ZfPlayerCharacter.h"
#include "Player/ZfPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AZfPlayerCharacter::AZfPlayerCharacter()
{
	InteractionComponent = CreateDefaultSubobject<UZfInteractionComponent>(TEXT("InteractionComponent"));
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
	
	
	// Concede as StartupAbilities configuradas no Blueprint deste Character.
	// GrantStartupAbilities usa FindAbilitySpecFromClass como guard —
	// seguro contra dupla concessão em respawns.
	GrantStartupAbilities();
	
	// Roda todos os GEs para inicialização dos Attributos
	InitializeAttributes();
	
	RegisterAttributeRefreshDelegates();
	RegisterResourceSyncDelegates();
	InitializeDependentAttributes();
	
	// InitializeDefaults apenas para personagem novo.
	// No load, o save restaura Health/Mana/Stamina antes deste ponto,
	// e esta chamada é ignorada.
	AZfPlayerState* PS = GetPlayerState<AZfPlayerState>();
	if (PS && PS->GetIsNewCharacter())
	{
		InitializeDefaultsAttributes();
	}
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
	
	// Concessão de abilities e aplicação de GEs continuam exclusivos do
	// servidor (já estão em PossessedBy, que só roda com HasAuthority).
	AbilitySystemComponent = ZfPlayerState->GetAbilitySystemComponent();
	ZfPlayerState->GetAbilitySystemComponent()->InitAbilityActorInfo(ZfPlayerState,this);
	
	// Registra o delegate de MoveSpeed → CMC em ambos os caminhos:
	//   Servidor      : chamado via PossessedBy → Super → AZfCharacter::PossessedBy
	//   Owning Client : chamado via OnRep_PlayerState
	// Sem HasAuthority — comportamento diferente dos outros delegates de sync.
	RegisterMovementSyncDelegate();
	
	// Servidor   → escuta tags quando o nó é desbloqueado localmente
	// Cliente    → escuta tags quando chegam replicadas do servidor
	if (UZfSkillTreeComponent* TreeComp = ZfPlayerState->GetSkillTreeComponent())
	{
		//
		UE_LOG(LogTemp, Warning, TEXT("InitializeTagListeners chamado. Authority: %s"),
			HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
		//
		TreeComp->InitializeTagListeners(ZfPlayerState->GetAbilitySystemComponent());
	}
}