// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/*
	// C++ 
	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(ZfInventoryTags::WeaponTypes::Weapon_Staff);
	FGameplayTag WeaponStaffTag = ZfInventoryTags::WeaponTypes::Weapon_Staff;

	// Blueprint
	FGameplayTag MinhaTagDaBlueprint = FGameplayTag::RequestGameplayTag("MinhaTag.Tag.MinhaTag");
	FGameplayTagContainer MeuTagContainerComTagDaBlueprint;
	MeuTagContainerComTagDaBlueprint.AddTag(FGameplayTag::RequestGameplayTag("MinhaTag.Tag.MinhaTag"));
*/

namespace ZfEquipmentTags
{
	namespace EquipmentSlots
	{
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_None)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_MainHand)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Backpack)
	}
}