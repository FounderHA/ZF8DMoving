// Fill out your copyright notice in the Description page of Project Settings.
 
#pragma once
 
#include "NativeGameplayTags.h"
 
/**
 * Tags nativas dos atributos principais do personagem (ZfMainAttributeSet).
 * Usadas como chaves no TMap RecalculateEffects do ZfProgressionAttributeSet
 * para mapear cada atributo ao seu GE de recalculo correspondente.
 *
 * Como usar:
 *   #include "ZfGameplayTags.h"
 *   RecalculateEffects.Find(ZfMainAttributeTags::Attribute_Strength);
 */
namespace ZfItemPropertyTags
{
	namespace ItemProperties
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Durability)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_MaxDurability)
	}
		
	namespace ToolProperties
	{
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Gathering_ScoreBonus)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Gathering_DamageBonus)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Gathering_GoodSizeBonus)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Gathering_PerfectSizeBonus)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Gathering_NeedleSpeedBonus)
	}
}

