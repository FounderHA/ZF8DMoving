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

namespace ZfQualityAttributesTags
{
	namespace AttributesWeapon
	{
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_PhysicalDamage)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_MagicalDamage)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_CriticalHitChance)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_CriticalDamage)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_AttackSpeed)
	}
	
	namespace AttributesArmor
	{
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Armor)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Armor_PhysicalResistance)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Armor_MagicalResistance)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Armor_StunResistance)
	}
}