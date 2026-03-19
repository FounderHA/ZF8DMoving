// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ZfPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Inventory/ZfInventoryComponent.h"
#include "AbilitySystem/ZfAbilitySystemComponent.h"
#include "Inventory/ZfEquipmentComponent.h"
#include "AbilitySystem/Attributes/ZfResourceAttributeSet.h"
#include "AbilitySystem/Attributes/ZfMainAttributeSet.h"
#include "AbilitySystem/Attributes/ZfProgressionAttributeSet.h"

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
}

UAbilitySystemComponent* AZfPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AZfPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(AZfPlayerState, AvailablePoints, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(AZfPlayerState, StrengthPoints, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(AZfPlayerState, DexterityPoints, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(AZfPlayerState, IntelligencePoints, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(AZfPlayerState, ConstitutionPoints, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(AZfPlayerState, ConvictionPoints, COND_None, REPNOTIFY_Always);
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

// Notify
void AZfPlayerState::OnRep_AvailablePoints() const
{
}

void AZfPlayerState::OnRep_StrengthPoints()
{
}

void AZfPlayerState::OnRep_IntelligencePoints() const
{
}

void AZfPlayerState::OnRep_DexterityPoints() const
{
}

void AZfPlayerState::OnRep_ConstitutionPoints() const
{
}

void AZfPlayerState::OnRep_ConvictionPoints() const
{
}


void AZfPlayerState::UpdateAttributePoints(float AttributePointsToAdd, EZfAttributeType InAttributeType)
{
	//if (!HasAuthority()) return;
	//Server_UpdateAttributePoints_Implementation(AttributePointsToAdd, InAttributeType);

	if (HasAuthority())
	{
		// Se já é servidor, executa direto
		Server_UpdateAttributePoints_Implementation(AttributePointsToAdd, InAttributeType);
	}
	else
	{
		// Se é cliente, envia RPC para o servidor
		Server_UpdateAttributePoints(AttributePointsToAdd, InAttributeType);
	}
}

void AZfPlayerState::Server_UpdateAttributePoints_Implementation(float AttributePointsToAdd, EZfAttributeType InAttributeType)
{

	//if (AllocatedPoints.AvailablePoints <= 0.f) return;
	
	float NewAttributePoints;
	
	switch (InAttributeType)
	{
	case EZfAttributeType::Strength:
		// lógica para Strength
		NewAttributePoints = StrengthPoints + AttributePointsToAdd;
		StrengthPoints = NewAttributePoints;
		ApplyRecalculateAttribute(InAttributeType);
		OnRep_StrengthPoints();
		break;

	case EZfAttributeType::Dexterity:
		// lógica para Dexterity
		NewAttributePoints = DexterityPoints + AttributePointsToAdd;
		DexterityPoints = NewAttributePoints;
		ApplyRecalculateAttribute(InAttributeType);
		OnRep_DexterityPoints();
		break;

	case EZfAttributeType::Intelligence:
		// lógica para Intelligence
		NewAttributePoints = IntelligencePoints + AttributePointsToAdd;
		IntelligencePoints = NewAttributePoints;
		ApplyRecalculateAttribute(InAttributeType);
		OnRep_IntelligencePoints();
		break;

	case EZfAttributeType::Constitution:
		// lógica para Constitution
		NewAttributePoints = ConstitutionPoints + AttributePointsToAdd;
		ConstitutionPoints = NewAttributePoints;
		ApplyRecalculateAttribute(InAttributeType);
		OnRep_ConstitutionPoints();
		break;

	case EZfAttributeType::Conviction:
		// lógica para Conviction
		NewAttributePoints = ConvictionPoints + AttributePointsToAdd;
		ConvictionPoints = NewAttributePoints;
		ApplyRecalculateAttribute(InAttributeType);
		OnRep_ConvictionPoints();
		break;
		
	default:
		break;
	}

	AvailablePoints += AttributePointsToAdd * -1.f;
	
}

//
//============================ Debug Attribut ============================
//
void AZfPlayerState::DebugAttribute()
{
	float StrengthPointsDebug = StrengthPoints;
	float IntelligencePointsDebug = IntelligencePoints;
	float DexterityPointsDebug = DexterityPoints;
	float ConstitutionPointsDebug = ConstitutionPoints;
	float ConvictionPointsDebug = ConvictionPoints;
	float BaseClassStrengthDebug = CharacterClassData->Strength;
	float BaseClassIntelligenceDebug = CharacterClassData->Intelligence;
	float BaseClassDexterityDebug = CharacterClassData->Dexterity;
	float BaseClassConstitutionDebug = CharacterClassData->Constitution;
	float BaseClassConvictionDebug = CharacterClassData->Conviction;
	float TotalStrength = StrengthPointsDebug + BaseClassStrengthDebug;
	float TotalIntelligence = IntelligencePointsDebug + BaseClassIntelligenceDebug;
	float TotalDexterity = DexterityPointsDebug + BaseClassDexterityDebug;
	float TotalConstitution = ConstitutionPointsDebug + BaseClassConstitutionDebug;
	float TotalConviction = ConvictionPointsDebug + BaseClassConvictionDebug;
	
	
	
	APlayerController* PC = GetPlayerController();
	if (PC && PC->IsLocalController())
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Cyan,
		FString::Printf(TEXT("Conv = Base %.0f + Allocated %.0f Total: %.0f"),
		BaseClassConvictionDebug, ConvictionPointsDebug, TotalConviction));
		
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor(255, 100, 0),
		FString::Printf(TEXT("Const = Base %.0f + Allocated %.0f -> Total: %.0f"),
		BaseClassConstitutionDebug, ConstitutionPointsDebug, TotalConstitution));
		
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Cyan,
		FString::Printf(TEXT("Dex = Base %.0f + Allocated %.0f -> Total: %.0f"),
		BaseClassDexterityDebug, DexterityPointsDebug, TotalDexterity));
		
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor(255, 100, 0),
		FString::Printf(TEXT("Int = Base %.0f + Allocated %.0f -> Total: %.0f"),
		BaseClassIntelligenceDebug, IntelligencePointsDebug, TotalIntelligence));
		
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Cyan,
		FString::Printf(TEXT("Str = Base %.0f + Allocated %.0f -> Total: %.0f"),
		BaseClassStrengthDebug, StrengthPointsDebug, TotalStrength));
	
		
	
	}
}


