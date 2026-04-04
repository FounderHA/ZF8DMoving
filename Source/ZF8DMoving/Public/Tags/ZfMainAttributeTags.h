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
namespace ZfMainAttributeTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_Strength)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_Dexterity)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_Intelligence)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_Constitution)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_Conviction)
}