// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/ModMagnitudeCalculation/ZfIntelligenceModMagnitudeCalculation.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ZfProgressionAttributeSet.h"
#include "player/ZfPlayerState.h"



float UZfIntelligenceModMagnitudeCalculation::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
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
	float BaseIntelligence = 0.f;
	if (PS->CharacterClassData)
		BaseIntelligence = PS->CharacterClassData->Intelligence;

	// --- Alocado ---
	
	float AllocatedIntelligence = 0.f;

	if (const UZfProgressionAttributeSet* ProgSet = ASC->GetSet<UZfProgressionAttributeSet>())
	{
		AllocatedIntelligence = ProgSet->GetIntelligencePoints();
	}

	// --- Itens equipados ---
	float ItemIntelligence = 0.f;

	UZfEquipmentComponent* EquipmentComponent = PS->FindComponentByClass<UZfEquipmentComponent>();

	if (EquipmentComponent)
	{
		const FGameplayTag IntelligenceTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.Type.AttributeSet.MainAttribute.Intelligence"));

		for (UZfItemInstance* Item : EquipmentComponent->GetAllEquippedItems())
		{
			if (!Item) continue;

			for (const FZfAppliedModifier& Modifier : Item->AppliedModifiers)
			{
				if (Modifier.AffectedAttributeTag == IntelligenceTag)
					ItemIntelligence += Modifier.FinalValue;
			}
		}
	}

	// --- Resultado ---
	Result = BaseIntelligence + AllocatedIntelligence + ItemIntelligence;

	UE_LOG(LogTemp, Log, TEXT("ZfIntelligenceMMC: Base=%.1f | Itens=%.1f | Total=%.1f"), BaseIntelligence, AllocatedIntelligence, Result);
	return Result;
}
