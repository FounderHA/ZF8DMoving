// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/ModMagnitudeCalculation/ZfConstitutionModMagnitudeCalculation.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ZfProgressionAttributeSet.h"
#include "player/ZfPlayerState.h"



float UZfConstitutionModMagnitudeCalculation::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
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
	float BaseConstitution = 0.f;
	if (PS->CharacterClassData)
		BaseConstitution = PS->CharacterClassData->Constitution;

	// --- Alocado ---
	
	float AllocatedConstitution = 0.f;

	if (const UZfProgressionAttributeSet* ProgSet = ASC->GetSet<UZfProgressionAttributeSet>())
	{
		AllocatedConstitution = ProgSet->GetConstitutionPoints();
	}

	// --- Itens equipados ---
	float ItemConstitution = 0.f;

	UZfEquipmentComponent* EquipmentComponent = PS->FindComponentByClass<UZfEquipmentComponent>();

	if (EquipmentComponent)
	{
		const FGameplayTag ConstitutionTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.Type.AttributeSet.MainAttribute.Constitution"));

		for (UZfItemInstance* Item : EquipmentComponent->GetAllEquippedItems())
		{
			if (!Item) continue;

			for (const FZfAppliedModifier& Modifier : Item->AppliedModifiers)
			{
				if (Modifier.AffectedAttributeTag == ConstitutionTag)
					ItemConstitution += Modifier.FinalValue;
			}
		}
	}

	// --- Resultado ---
	Result = BaseConstitution + AllocatedConstitution + ItemConstitution;

	UE_LOG(LogTemp, Log, TEXT("ZfConstitutionMMC: Base=%.1f | Itens=%.1f | Total=%.1f"), BaseConstitution, ItemConstitution, Result);
	return Result;
}
