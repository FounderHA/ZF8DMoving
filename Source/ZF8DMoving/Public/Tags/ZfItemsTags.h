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
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Durability)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_MaxDurability)
}