// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/ModMagnitudeCalculation/ZfStrengthModMagnitudeCalculation.h"
#include "AbilitySystemComponent.h"
#include "player/ZfPlayerState.h"



float UZfStrengthModMagnitudeCalculation::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	float Result = 0.f;

	AActor* AvatarActor = Spec.GetContext().GetOriginalInstigatorAbilitySystemComponent()->GetAvatarActor();

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
	//float AllocatedStrength = PS->StrengthPoints;

	// --- Itens equipados ---
	float ItemStrength = 0.f;

	UZfEquipmentComponent* EquipmentComponent = PS->FindComponentByClass<UZfEquipmentComponent>();

	if (EquipmentComponent)
	{
		const FGameplayTag StrengthTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.Type.AttributeSet.MainAttribute.Strength"));

		for (UZfItemInstance* Item : EquipmentComponent->GetAllEquippedItems())
		{
			if (!Item) continue;

			for (const FZfAppliedModifier& Modifier : Item->AppliedModifiers)
			{
				if (Modifier.AffectedAttributeTag == StrengthTag)
					ItemStrength += Modifier.CurrentValue;
			}
		}
	}

	// --- Resultado ---
	Result = BaseStrength + /*AllocatedStrength +*/ ItemStrength;

	UE_LOG(LogTemp, Log,
		TEXT("ZfStrengthMMC: Base=%.1f | Itens=%.1f | Total=%.1f"),
		BaseStrength, ItemStrength, Result);

	return Result;
}
