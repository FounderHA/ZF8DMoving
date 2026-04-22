// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/ModMagnitudeCalculation/ZfDexterityMMC.h"
#include "AbilitySystemComponent.h"
#include "Tags/ZfGameplayTags.h"
#include "AbilitySystem/Attributes/ZfProgressionAttributeSet.h"
#include "player/ZfPlayerState.h"



float UZfDexterityMMC::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
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
	float BaseDexterity = 0.f;
	if (PS->GetCharacterClassData())
		BaseDexterity = PS->GetCharacterClassData()->Dexterity;

	// --- Alocado ---
	
	float AllocatedDexterity = 0.f;

	if (const UZfProgressionAttributeSet* ProgSet = ASC->GetSet<UZfProgressionAttributeSet>())
	{
		AllocatedDexterity = ProgSet->GetDexterityPoints();
	}

	// --- Itens equipados ---
	float ItemDexterity = 0.f;

	UZfEquipmentComponent* EquipmentComponent = PS->FindComponentByClass<UZfEquipmentComponent>();

	if (EquipmentComponent)
	{
		const FGameplayTag DexterityTag = ZfAttributeTags::ZfMainAttributeTags::Attribute_Dexterity;

		for (UZfItemInstance* Item : EquipmentComponent->GetAllEquippedItems())
		{
			if (!Item) continue;

			for (const FZfAppliedModifier& Modifier : Item->GetAppliedModifiers())
			{
				if (Modifier.AffectedAttributeTag == DexterityTag)
					ItemDexterity += Modifier.FinalValue;
			}
		}
	}

	// --- Resultado ---
	Result = BaseDexterity + AllocatedDexterity + ItemDexterity;

	UE_LOG(LogTemp, Log, TEXT("ZfDexterityMMC: Base=%.1f | Itens=%.1f | Total=%.1f"), BaseDexterity, ItemDexterity, Result);
	return Result;
}
