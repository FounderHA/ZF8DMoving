// Fill out your copyright notice in the Description page of Project Settings.

#include "Tags/ZfProgressionTags.h"

namespace ZfProgressionTags
{
	// =========================================================================
	// EVENT
	// =========================================================================

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_XP_Gained, "Event.XP.Gained",
		"Disparado por fontes de XP (mob, quest, item). Escutado por GA_ReceiveXP. " "Payload: EventMagnitude = quantidade de XP.")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Character_LevelUp, "Event.Character.LevelUp",
		"Disparado por UZfProgressionAttributeSet ao subir de nivel. Escutado por GA_LevelUp." "Payload: EventMagnitude = novo nivel alcancado.")

	// =========================================================================
	// DATA — SetByCaller
	// =========================================================================

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_XP_Amount, "Data.XP.Amount",
		"Chave SetByCaller do GE_GiveXP. Define a quantidade de XP a conceder.")
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Progression_AttributePoints, "Data.Progression.AttributePoints",
		"Chave SetByCaller do GE_GrantAttributePoints. Define pontos distribuiveis a conceder no level-up.")

	// =========================================================================
	// ABILITY
	// =========================================================================

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Progression_ReceiveXP, "Ability.Progression.ReceiveXP",
		"Tag de identificacao da GA_ReceiveXP.")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Progression_LevelUp, "Ability.Progression.LevelUp",
		"Tag de identificacao da GA_LevelUp.")

	// =========================================================================
	// GAMEPLAYCUE
	// =========================================================================

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Character_LevelUp, "GameplayCue.Character.LevelUp",
		"Cue de level-up. Implemente visual e som no Blueprint filho de GameplayCueNotify.")
}