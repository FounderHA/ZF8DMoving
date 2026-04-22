// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/ModMagnitudeCalculation/ZfMaxHealthMMC.h"
#include "AbilitySystemComponent.h"
#include "Tags/ZfGameplayTags.h"
#include "AbilitySystem/Attributes/ZfMainAttributeSet.h"
#include "player/ZfPlayerState.h"



float UZfMaxHealthMMC::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
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
	float BaseMaxHealth = 0.f;
	if (PS->GetCharacterClassData())
		BaseMaxHealth = PS->GetCharacterClassData()->BaseMaxHealth;

	// --- Attribute ---
	
	float Constitution = 0.f;
	if (const UZfMainAttributeSet* ProgSet = ASC->GetSet<UZfMainAttributeSet>())
	{
		Constitution = ProgSet->GetConstitution();
	}
	
	float ModifierHealth = 0.f;
	if (PS->GetCharacterClassData())
		ModifierHealth = PS->GetCharacterClassData()->HealthModifier;
	
	
	float MaxHealthAttribute = 0.f;
	MaxHealthAttribute = ModifierHealth * Constitution;
	
	
	// --- Itens equipados ---
	float ItemMaxHealth = 0.f;

	UZfEquipmentComponent* EquipmentComponent = PS->FindComponentByClass<UZfEquipmentComponent>();

	if (EquipmentComponent)
	{
		const FGameplayTag MaxHealthTag = ZfAttributeTags::ZfResourceAttributeTags::Attribute_MaxHealth;

		for (UZfItemInstance* Item : EquipmentComponent->GetAllEquippedItems())
		{
			if (!Item) continue;

			for (const FZfAppliedModifier& Modifier : Item->GetAppliedModifiers())
			{
				if (Modifier.AffectedAttributeTag == MaxHealthTag)
					ItemMaxHealth += Modifier.FinalValue;
			}
		}
	}

	// --- Resultado ---
	Result = BaseMaxHealth + MaxHealthAttribute + ItemMaxHealth;

	UE_LOG(LogTemp, Log, TEXT("ZfMaxHealthMMC: Base=%.1f | Attribute=%.1f | Itens=%.1f | Total=%.1f"), BaseMaxHealth, MaxHealthAttribute, ItemMaxHealth, Result);
	return Result;
}
