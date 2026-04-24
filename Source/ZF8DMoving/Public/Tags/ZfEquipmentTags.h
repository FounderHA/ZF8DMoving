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
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_OffHand)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Head)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Chest)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Legs)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Feet)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Hands)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Cape)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Backpack)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Ring)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Necklace)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Pickaxe)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Axe)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Shovel)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_FishingRod)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Consumable)
	}
}