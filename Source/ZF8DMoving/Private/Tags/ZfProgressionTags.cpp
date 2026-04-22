// Fill out your copyright notice in the Description page of Project Settings.

#include "Tags/ZfProgressionTags.h"

namespace ZfProgressionTags
{
	// =========================================================================
	// EVENT
	// =========================================================================

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LevelProgression_Event_Character_XP_Gained, "LevelProgression.Event.Character.XP.Gained",
		"Disparado por fontes de XP (mob, quest, item). Escutado por GA_ReceiveXP. " "Payload: EventMagnitude = quantidade de XP.")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LevelProgression_Event_Character_LevelUp, "LevelProgression.Event.Character.LevelUp",
		"Disparado por UZfProgressionAttributeSet ao subir de nivel. Escutado por GA_LevelUp." "Payload: EventMagnitude = novo nivel alcancado.")
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LevelProgression_Event_Character_SpendAttributePoints, "LevelProgression.Event.Character.SpendAttributePoints",
		"Disparado pela widget ao confirmar pontos. Payload: OptionalObject = UZfAttributeSpendRequest.")
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LevelProgression_Event_Character_RefundAttributePoint, "LevelProgression.Event.Character.RefundAttributePoint", 
		"Disparado pela widget para refund individual. Payload: OptionalObject = UZfAttributeRefundRequest.")
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LevelProgression_Event_Character_ResetAttributePoints, "LevelProgression.Event.Character.ResetAttributePoints",
		"Disparado pelo botao Reset All. Sem payload adicional.")

	// =========================================================================
	// DATA — SetByCaller
	// =========================================================================

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LevelProgression_Data_XP_Amount, "LevelProgression.Data.XP.Amount",
		"Chave SetByCaller do GE_GiveXP. Define a quantidade de XP a conceder.")
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LevelProgression_Data_Progression_AttributePoints, "LevelProgression.Data.Progression.AttributePoints",
		"Chave SetByCaller do GE_GrantAttributePoints. Define pontos distribuiveis a conceder no level-up.")
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LevelProgression_Data_Progression_SkillPoints, "LevelProgression.Data.Progression.SkillPoints",
		"Chave SetByCaller do GE_GrantSkillPoints. Define pontos de skill tree a conceder no level-up.")

	// =========================================================================
	// DATA — SetByCaller
	// =========================================================================
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LevelProgression_Data_Spend_Total,         "LevelProgression.Data.Spend.Total",
		"Total de pontos gastos. Negativo no GE para decrementar AttributePoints.")
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LevelProgression_Data_Spend_Strength,      "LevelProgression.Data.Spend.Strength",
		"Pontos investidos em Strength.")
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LevelProgression_Data_Spend_Dexterity,     "LevelProgression.Data.Spend.Dexterity",
		"Pontos investidos em Dexterity.")
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LevelProgression_Data_Spend_Intelligence,  "LevelProgression.Data.Spend.Intelligence",
		"Pontos investidos em Intelligence.")
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LevelProgression_Data_Spend_Constitution,  "LevelProgression.Data.Spend.Constitution",
		"Pontos investidos em Constitution.")
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LevelProgression_Data_Spend_Conviction,    "LevelProgression.Data.Spend.Conviction",
		"Pontos investidos em Conviction.")
	
	// =========================================================================
	// ABILITY
	// =========================================================================

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LevelProgression_Ability_Progression_ReceiveXP, "LevelProgression.Ability.Progression.ReceiveXP",
		"Tag de identificacao da GA_ReceiveXP.")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LevelProgression_Ability_Progression_LevelUp, "LevelProgression.Ability.Progression.LevelUp",
		"Tag de identificacao da GA_LevelUp.")
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LevelProgression_Ability_Progression_SpendAttributePoints,"LevelProgression.Ability.Progression.SpendAttributePoints",
		"Tag da GA_SpendAttributePoints.")
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LevelProgression_Ability_Progression_RefundAttributePoint, "LevelProgression.Ability.Progression.RefundAttributePoint",
		"Tag da GA_RefundAttributePoint.")
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LevelProgression_Ability_Progression_ResetAttributePoints,"LevelProgression.Ability.Progression.ResetAttributePoints",
		"Tag da GA_ResetAttributePoints.")

	// =========================================================================
	// GAMEPLAYCUE
	// =========================================================================

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LevelProgression_GameplayCue_Character_LevelUp, "LevelProgression.GameplayCue.Character.LevelUp",
		"Cue de level-up. Implemente visual e som no Blueprint filho de GameplayCueNotify.")
}