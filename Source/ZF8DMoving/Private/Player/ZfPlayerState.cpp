// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ZfPlayerState.h"

#include "Player/LevelProgression/ZfAttributeSpendRequest.h"
#include "Player/LevelProgression/ZfAttributeRefundRequest.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Inventory/ZfInventoryComponent.h"
#include "AbilitySystem/ZfAbilitySystemComponent.h"
#include "Inventory/ZfEquipmentComponent.h"
#include "Tags/ZfGameplayTags.h"
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






// =============================================================================
// Server RPCs — Progressão
// Valores passados como int32 — sem replicação de UObject pela rede.
// O Request é criado localmente no servidor após o RPC chegar.
// =============================================================================
 
void AZfPlayerState::Server_SpendAttributePoints_Implementation(
	int32 Strength,
	int32 Dexterity,
	int32 Intelligence,
	int32 Constitution,
	int32 Conviction)
{
	UZfAttributeSpendRequest* Request = NewObject<UZfAttributeSpendRequest>(this);
	Request->StrengthPointsToAdd     = Strength;
	Request->DexterityPointsToAdd    = Dexterity;
	Request->IntelligencePointsToAdd = Intelligence;
	Request->ConstitutionPointsToAdd = Constitution;
	Request->ConvictionPointsToAdd   = Conviction;
 
	if (!Request->IsValid()) return;
 
	APawn* Pawn = GetPawn();
	if (!Pawn) return;
 
	FGameplayEventData Payload;
	Payload.OptionalObject = static_cast<const UObject*>(Request);
 
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Pawn,
		ZfProgressionTags::LevelProgression_Event_Character_SpendAttributePoints,
		Payload
	);
}
 
void AZfPlayerState::Server_RefundAttributePoint_Implementation(
	int32 Strength,
	int32 Dexterity,
	int32 Intelligence,
	int32 Constitution,
	int32 Conviction)
{
	UZfAttributeRefundRequest* Request = NewObject<UZfAttributeRefundRequest>(this);
	Request->StrengthPointsToRemove     = Strength;
	Request->DexterityPointsToRemove    = Dexterity;
	Request->IntelligencePointsToRemove = Intelligence;
	Request->ConstitutionPointsToRemove = Constitution;
	Request->ConvictionPointsToRemove   = Conviction;
 
	if (!Request->IsValid()) return;
 
	APawn* Pawn = GetPawn();
	if (!Pawn) return;
 
	FGameplayEventData Payload;
	Payload.OptionalObject = static_cast<const UObject*>(Request);
 
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		Pawn,
		ZfProgressionTags::LevelProgression_Event_Character_RefundAttributePoint,
		Payload
	);
}
 
void AZfPlayerState::Server_ResetAttributePoints_Implementation()
{
	APawn* Pawn = GetPawn();
	if (!Pawn) return;
 
	FGameplayEventData Payload;
 
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		Pawn,
		ZfProgressionTags::LevelProgression_Event_Character_ResetAttributePoints,
		Payload
	);
}
