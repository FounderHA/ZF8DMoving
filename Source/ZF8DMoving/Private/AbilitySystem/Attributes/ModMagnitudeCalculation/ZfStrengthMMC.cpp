// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/ModMagnitudeCalculation/ZfStrengthMMC.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ZfProgressionAttributeSet.h"
#include "player/ZfPlayerState.h"



float UZfStrengthMMC::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	float Result = 0.f;

	// --- Obtém ASC, Pawn e PlayerState ---
	UAbilitySystemComponent* ASC = Spec.GetContext().GetOriginalInstigatorAbilitySystemComponent();
	if (!ASC) return Result;
 
	AActor* AvatarActor = ASC->GetAvatarActor();
	if (!AvatarActor) return Result;
 
	APawn* Pawn = Cast<APawn>(AvatarActor);
	if (!Pawn) return Result;
 
	AZfPlayerState* PS = Pawn->GetPlayerState<AZfPlayerState>();
	if (!PS) return Result;
	
	// --- Base ---
	float BaseStrength = 0.f;
	if (PS->CharacterClassData)
		BaseStrength = PS->CharacterClassData->Strength;

	// --- Alocado ---
	
	float AllocatedStrength = 0.f;

	if (const UZfProgressionAttributeSet* ProgSet = ASC->GetSet<UZfProgressionAttributeSet>())
	{
		AllocatedStrength = ProgSet->GetStrengthPoints();
	}

	// --- Itens equipados ---
	float ItemStrength = 0.f;

	UZfEquipmentComponent* EquipmentComponent = PS->FindComponentByClass<UZfEquipmentComponent>();

	if (EquipmentComponent)
	{
		const FGameplayTag StrengthTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.Type.AttributeSet.MainAttribute.Strength"));

		for (UZfItemInstance* Item : EquipmentComponent->GetAllEquippedItems())
		{
			if (!Item) continue;

			for (const FZfAppliedModifier& Modifier : Item->GetAppliedModifiers())
			{
				if (Modifier.AffectedAttributeTag == StrengthTag)
					ItemStrength += Modifier.FinalValue;
			}
		}
	}

	// --- Resultado ---
	Result = BaseStrength + AllocatedStrength + ItemStrength;

	UE_LOG(LogTemp, Log, TEXT("ZfStrengthMMC: Base=%.1f | Itens=%.1f | Total=%.1f"), BaseStrength, ItemStrength, Result);
	return Result;
}
