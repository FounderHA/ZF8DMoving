// Fill out your copyright notice in the Description page of Project Settings.
 
#pragma once
 
#include "NativeGameplayTags.h"
 
/**
 * Tags nativas dos atributos do personagem.
 * Usadas como chaves em TMaps de GEs de recalculo e em qualquer outro
 * sistema que precise identificar atributos por tag em runtime.
 *
 * Como usar:
 *   #include "Tags/ZfGamePlayTags" -- include geral para todas as tags do jogo
 *   RecalculateEffects.Find(ZfAttributeTags::ZfMainAttributeTags::Attribute_Strength);
 */

namespace ZfAttributeTags
{
	// -----------------------------------------------------------------------
	// Atributos principais (ZfMainAttributeSet)
	// -----------------------------------------------------------------------
	namespace ZfMainAttributeTags
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_Strength)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_Dexterity)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_Intelligence)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_Constitution)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_Conviction)
	}

	// -----------------------------------------------------------------------
	// Recursos vitais (ZfResourceAttributeSet)
	// -----------------------------------------------------------------------
	namespace ZfResourceAttributeTags
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_Health)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_MaxHealth)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_Mana)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_MaxMana)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_Stamina)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_MaxStamina)
	}

	// -----------------------------------------------------------------------
	// Movimento (ZfMovementAttributeSet)
	// -----------------------------------------------------------------------
	namespace ZfMovementAttributeTags
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_MoveSpeed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_DashDistance)
	}

	// -----------------------------------------------------------------------
	// Atributos ofensivos (ZfOffensiveAttributeSet)
	// -----------------------------------------------------------------------
	namespace ZfOffensiveAttributeTags
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_PhysicalDamage)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_MagicalDamage)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_PhysicalPain)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_MagicalPain)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_CriticalHitChance)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_CriticalDamage)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_PoiseDamage)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_AttackSpeed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_CastSpeed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_CooldownReduction)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_BackstabDamage)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_FirstHitDamage)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_BurnBuildup)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_FreezeBuildup)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_ShockBuildup)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_BleedBuildup)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_PoisonBuildup)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_StunBuildup)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_SlowBuildup)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_SleepBuildup)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_RootBuildup)
	}

	// -----------------------------------------------------------------------
	// Resistências e limiares (ZfResistanceAttributeSet)
	// -----------------------------------------------------------------------
	namespace ZfDefensiveAttributeTags
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_PhysicalResistance)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_MagicalResistance)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_Tenacity)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_CriticalResistance)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_PoiseResistance)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_BurnThreshold)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_FreezeThreshold)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_ShockThreshold)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_BleedThreshold)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_PoisonThreshold)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_StunThreshold)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_SlowThreshold)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_SleepThreshold)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_RootThreshold)
	}

	// -----------------------------------------------------------------------
	// Roubo de recursos (ZfStealAttributesSet)
	// -----------------------------------------------------------------------
	namespace ZfUtilityAttributeTags
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_FlatLifeStealOnHit)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_FlatManaStealOnHit)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_FlatStaminaStealOnHit)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_PercentLifeStealOnHit)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_PercentManaStealOnHit)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_PercentStaminaStealOnHit)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_FlatLifeStealOnKill)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_FlatManaStealOnKill)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_FlatStaminaStealOnKill)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_PercentLifeStealOnKill)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_PercentManaStealOnKill)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_PercentStaminaStealOnKill)
	}
	
}