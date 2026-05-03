// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/Fragments/ZfFragment_WeaponSkill.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/ZfItemInstance.h"
#include "Inventory/ZfEquipmentComponent.h"
#include "Player/ZfPlayerState.h"
#include "SkillTreeSystem/ZfSkillTreeComponent.h"

// =============================================================================
// OnItemEquipped
// =============================================================================

void UZfFragment_WeaponSkill::OnItemEquipped(
	UZfItemInstance* OwningInstance,
	UZfEquipmentComponent* EquipmentComponent,
	AActor* EquippingActor)
{
	if (!EquippingActor) return;

	UAbilitySystemComponent* ASC = GetASC(EquippingActor);
	if (!ASC) return;

	// Concede abilities ao ASC e armazena os handles para revogação futura
	if (PrimaryAbilityClass)
	{
		PrimaryHandle = ASC->GiveAbility(FGameplayAbilitySpec(PrimaryAbilityClass, 1));
	}

	if (SecondaryAbilityClass)
	{
		SecondaryHandle = ASC->GiveAbility(FGameplayAbilitySpec(SecondaryAbilityClass, 1));
	}

	// Redistribui os WeaponSlots com base nas armas agora equipadas
	UZfSkillTreeComponent* TreeComp = GetSkillTreeComponent(EquippingActor);
	if (TreeComp && EquipmentComponent)
	{
		TreeComp->RecalculateWeaponSlots(EquipmentComponent);
	}
}

// =============================================================================
// OnItemUnequipped
// =============================================================================

void UZfFragment_WeaponSkill::OnItemUnequipped(
	UZfItemInstance* OwningInstance,
	UZfEquipmentComponent* EquipmentComponent,
	AActor* UnequippingActor)
{
	if (!UnequippingActor) return;

	UAbilitySystemComponent* ASC = GetASC(UnequippingActor);
	if (ASC)
	{
		if (PrimaryHandle.IsValid())
		{
			ASC->ClearAbility(PrimaryHandle);
			PrimaryHandle = FGameplayAbilitySpecHandle();
		}

		if (SecondaryHandle.IsValid())
		{
			ASC->ClearAbility(SecondaryHandle);
			SecondaryHandle = FGameplayAbilitySpecHandle();
		}
	}

	// Redistribui os WeaponSlots — arma removida, recalcula com o que sobrou
	UZfSkillTreeComponent* TreeComp = GetSkillTreeComponent(UnequippingActor);
	if (TreeComp && EquipmentComponent)
	{
		TreeComp->RecalculateWeaponSlots(EquipmentComponent);
	}
}

// =============================================================================
// Helpers privados
// =============================================================================

UAbilitySystemComponent* UZfFragment_WeaponSkill::GetASC(AActor* Actor) const
{
	// Tenta direto no Actor
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Actor))
	{
		return ASI->GetAbilitySystemComponent();
	}

	// Tenta via PlayerState — AZfPlayerCharacter tem ASC no PlayerState
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		if (const APlayerState* PS = Pawn->GetPlayerState())
		{
			if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PS))
			{
				return ASI->GetAbilitySystemComponent();
			}
		}
	}

	return nullptr;
}

UZfSkillTreeComponent* UZfFragment_WeaponSkill::GetSkillTreeComponent(AActor* Actor) const
{
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		if (const AZfPlayerState* PS = Pawn->GetPlayerState<AZfPlayerState>())
		{
			return PS->GetSkillTreeComponent();
		}
	}
	return nullptr;
}