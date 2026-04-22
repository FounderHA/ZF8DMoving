// Fill out your copyright notice in the Description page of Project Settings.

#include "Tags/ZfAttributeTags.h"

namespace ZfAttributeTags
{
	// -----------------------------------------------------------------------
	// Atributos principais (ZfMainAttributeSet)
	// -----------------------------------------------------------------------
	namespace ZfMainAttributeTags
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_Strength,     "Attribute.Main.Strength",     "Atributo principal Strength do personagem.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_Dexterity,    "Attribute.Main.Dexterity",    "Atributo principal Dexterity do personagem.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_Intelligence, "Attribute.Main.Intelligence", "Atributo principal Intelligence do personagem.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_Constitution, "Attribute.Main.Constitution", "Atributo principal Constitution do personagem.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_Conviction,   "Attribute.Main.Conviction",   "Atributo principal Conviction do personagem.")
	}

	// -----------------------------------------------------------------------
	// Recursos vitais (ZfResourceAttributeSet)
	// -----------------------------------------------------------------------
	namespace ZfResourceAttributeTags
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_Health,     "Attribute.Resource.Health",     "Vida atual do personagem.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_MaxHealth,  "Attribute.Resource.MaxHealth",  "Vida máxima do personagem.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_Mana,       "Attribute.Resource.Mana",       "Mana atual do personagem.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_MaxMana,    "Attribute.Resource.MaxMana",    "Mana máxima do personagem.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_Stamina,    "Attribute.Resource.Stamina",    "Stamina atual do personagem.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_MaxStamina, "Attribute.Resource.MaxStamina", "Stamina máxima do personagem.")
	}

	// -----------------------------------------------------------------------
	// Movimento (ZfMovementAttributeSet)
	// -----------------------------------------------------------------------
	namespace ZfMovementAttributeTags
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_MoveSpeed,    "Attribute.Movement.MoveSpeed",    "Velocidade de movimento do personagem.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_DashDistance, "Attribute.Movement.DashDistance", "Distância percorrida pelo dash do personagem.")
	}

	// -----------------------------------------------------------------------
	// Atributos ofensivos (ZfOffensiveAttributeSet)
	// -----------------------------------------------------------------------
	namespace ZfOffensiveAttributeTags
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_PhysicalDamage,    "Attribute.Offensive.PhysicalDamage",    "Dano físico base do personagem.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_MagicalDamage,     "Attribute.Offensive.MagicalDamage",     "Dano mágico base do personagem.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_PhysicalPain,      "Attribute.Offensive.PhysicalPain",      "Pain físico aplicado ao alvo.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_MagicalPain,       "Attribute.Offensive.MagicalPain",       "Pain mágico aplicado ao alvo.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_CriticalHitChance, "Attribute.Offensive.CriticalHitChance", "Chance de acerto crítico do personagem.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_CriticalDamage,    "Attribute.Offensive.CriticalDamage",    "Multiplicador de dano crítico do personagem.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_PoiseDamage,       "Attribute.Offensive.PoiseDamage",       "Dano de poise aplicado ao alvo.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_AttackSpeed,       "Attribute.Offensive.AttackSpeed",       "Velocidade de ataque do personagem.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_CastSpeed,         "Attribute.Offensive.CastSpeed",         "Velocidade de conjuração do personagem.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_CooldownReduction, "Attribute.Offensive.CooldownReduction", "Redução de cooldown das habilidades.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_BackstabDamage,    "Attribute.Offensive.BackstabDamage",    "Bônus de dano por ataque pelas costas.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_FirstHitDamage,    "Attribute.Offensive.FirstHitDamage",    "Bônus de dano no primeiro golpe.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_BurnBuildup,       "Attribute.Offensive.BurnBuildup",       "Acúmulo de status Burn aplicado ao alvo.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_FreezeBuildup,     "Attribute.Offensive.FreezeBuildup",     "Acúmulo de status Freeze aplicado ao alvo.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_ShockBuildup,      "Attribute.Offensive.ShockBuildup",      "Acúmulo de status Shock aplicado ao alvo.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_BleedBuildup,      "Attribute.Offensive.BleedBuildup",      "Acúmulo de status Bleed aplicado ao alvo.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_PoisonBuildup,     "Attribute.Offensive.PoisonBuildup",     "Acúmulo de status Poison aplicado ao alvo.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_StunBuildup,       "Attribute.Offensive.StunBuildup",       "Acúmulo de status Stun aplicado ao alvo.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_SlowBuildup,       "Attribute.Offensive.SlowBuildup",       "Acúmulo de status Slow aplicado ao alvo.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_SleepBuildup,      "Attribute.Offensive.SleepBuildup",      "Acúmulo de status Sleep aplicado ao alvo.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_RootBuildup,       "Attribute.Offensive.RootBuildup",       "Acúmulo de status Root aplicado ao alvo.")
	}

	// -----------------------------------------------------------------------
	// Resistências e limiares (ZfResistanceAttributeSet)
	// -----------------------------------------------------------------------
	namespace ZfDefensiveAttributeTags
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_PhysicalResistance, "Attribute.Resistance.PhysicalResistance", "Resistência a dano físico do personagem.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_MagicalResistance,  "Attribute.Resistance.MagicalResistance",  "Resistência a dano mágico do personagem.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_Tenacity,           "Attribute.Resistance.Tenacity",           "Redução de duração de status negativos.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_CriticalResistance, "Attribute.Resistance.CriticalResistance", "Redução de chance de receber crítico.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_PoiseResistance,    "Attribute.Resistance.PoiseResistance",    "Resistência a dano de poise.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_BurnThreshold,      "Attribute.Resistance.BurnThreshold",      "Limiar de acúmulo para infligir Burn.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_FreezeThreshold,    "Attribute.Resistance.FreezeThreshold",    "Limiar de acúmulo para infligir Freeze.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_ShockThreshold,     "Attribute.Resistance.ShockThreshold",     "Limiar de acúmulo para infligir Shock.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_BleedThreshold,     "Attribute.Resistance.BleedThreshold",     "Limiar de acúmulo para infligir Bleed.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_PoisonThreshold,    "Attribute.Resistance.PoisonThreshold",    "Limiar de acúmulo para infligir Poison.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_StunThreshold,      "Attribute.Resistance.StunThreshold",      "Limiar de acúmulo para infligir Stun.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_SlowThreshold,      "Attribute.Resistance.SlowThreshold",      "Limiar de acúmulo para infligir Slow.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_SleepThreshold,     "Attribute.Resistance.SleepThreshold",     "Limiar de acúmulo para infligir Sleep.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_RootThreshold,      "Attribute.Resistance.RootThreshold",      "Limiar de acúmulo para infligir Root.")
	}

	// -----------------------------------------------------------------------
	// Roubo de recursos (ZfStealAttributesSet)
	// -----------------------------------------------------------------------
	namespace ZfUtiliteAttributeTags
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_FlatLifeStealOnHit,        "Attribute.Steal.FlatLifeStealOnHit",        "Roubo de vida fixo por acerto.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_FlatManaStealOnHit,        "Attribute.Steal.FlatManaStealOnHit",        "Roubo de mana fixo por acerto.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_FlatStaminaStealOnHit,     "Attribute.Steal.FlatStaminaStealOnHit",     "Roubo de stamina fixo por acerto.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_PercentLifeStealOnHit,     "Attribute.Steal.PercentLifeStealOnHit",     "Roubo de vida percentual por acerto.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_PercentManaStealOnHit,     "Attribute.Steal.PercentManaStealOnHit",     "Roubo de mana percentual por acerto.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_PercentStaminaStealOnHit,  "Attribute.Steal.PercentStaminaStealOnHit",  "Roubo de stamina percentual por acerto.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_FlatLifeStealOnKill,       "Attribute.Steal.FlatLifeStealOnKill",       "Roubo de vida fixo por abate.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_FlatManaStealOnKill,       "Attribute.Steal.FlatManaStealOnKill",       "Roubo de mana fixo por abate.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_FlatStaminaStealOnKill,    "Attribute.Steal.FlatStaminaStealOnKill",    "Roubo de stamina fixo por abate.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_PercentLifeStealOnKill,    "Attribute.Steal.PercentLifeStealOnKill",    "Roubo de vida percentual por abate.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_PercentManaStealOnKill,    "Attribute.Steal.PercentManaStealOnKill",    "Roubo de mana percentual por abate.")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_PercentStaminaStealOnKill, "Attribute.Steal.PercentStaminaStealOnKill", "Roubo de stamina percentual por abate.")
	}

}