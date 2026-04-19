// Fill out your copyright notice in the Description page of Project Settings.
 
#include "AbilitySystem/Attributes/ModMagnitudeCalculation/ZfMoveSpeedMMC.h"
#include "AbilitySystemComponent.h"
#include "player/ZfPlayerState.h"
 
float UZfMoveSpeedMMC::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	
	float Result = 0.f;
	
	// --- Obtém ASC, AvatarActor, Pawn e PlayerState ---
	UAbilitySystemComponent* ASC = Spec.GetContext().GetOriginalInstigatorAbilitySystemComponent();
	if (!ASC) return Result;
 
	AActor* AvatarActor = ASC->GetAvatarActor();
	if (!AvatarActor) return Result;
 
	APawn* Pawn = Cast<APawn>(AvatarActor);
	if (!Pawn) return Result;
 
	AZfPlayerState* PS = Pawn->GetPlayerState<AZfPlayerState>();
	if (!PS) return Result;
 
	// --- Base ---
	float BaseMoveSpeed = 0.f;
	if (PS->GetCharacterClassData())
		BaseMoveSpeed = PS->GetCharacterClassData()->BaseMoveSpeed;
	
	// --- Itens equipados ---
	float ItemMoveSpeed = 0.f;
	
	UZfEquipmentComponent* EquipmentComponent = PS->FindComponentByClass<UZfEquipmentComponent>();
	
	if (EquipmentComponent)
	{
		
	}
		
		
		
		
		
	// --- Resultado ---
	Result = BaseMoveSpeed + ItemMoveSpeed;
	return Result;
}
 