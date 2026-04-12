// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/ModMagnitudeCalculation/ZfConvictionMMC.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ZfProgressionAttributeSet.h"
#include "player/ZfPlayerState.h"



float UZfConvictionMMC::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
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
	float BaseConviction = 0.f;
	if (PS->GetCharacterClassData())
		BaseConviction = PS->GetCharacterClassData()->Conviction;

	// --- Alocado ---
	
	float AllocatedConviction = 0.f;

	if (const UZfProgressionAttributeSet* ProgSet = ASC->GetSet<UZfProgressionAttributeSet>())
	{
		AllocatedConviction = ProgSet->GetConvictionPoints();
	}

	// --- Itens equipados ---
	float ItemConviction = 0.f;

	UZfEquipmentComponent* EquipmentComponent = PS->FindComponentByClass<UZfEquipmentComponent>();

	if (EquipmentComponent)
	{
		const FGameplayTag ConvictionTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.Type.AttributeSet.MainAttribute.Conviction"));

		for (UZfItemInstance* Item : EquipmentComponent->GetAllEquippedItems())
		{
			if (!Item) continue;

			for (const FZfAppliedModifier& Modifier : Item->GetAppliedModifiers())
			{
				if (Modifier.AffectedAttributeTag == ConvictionTag)
					ItemConviction += Modifier.FinalValue;
			}
		}
	}

	// --- Resultado ---
	Result = BaseConviction + AllocatedConviction + ItemConviction;

	UE_LOG(LogTemp, Log, TEXT("ZfConvictionMMC: Base=%.1f | Itens=%.1f | Total=%.1f"), BaseConviction, ItemConviction, Result);
	return Result;
}
