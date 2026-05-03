// Fill out your copyright notice in the Description page of Project Settings.

#include "Tags/ZfAbilityTreeTags.h"

namespace ZfAbilityTreeTags
{
	// =========================================================================
	// EVENT
	// =========================================================================

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SkillTree_Event_UnlockNode, "SkillTree.Event.UnlockNode",
		"Disparado por Server_UnlockAbilityNode. Escutado por GA_UnlockAbilityNode. "
		"Payload: OptionalObject = UZfUnlockNodeRequest.")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SkillTree_Event_UpgradeNode, "SkillTree.Event.UpgradeNode",
		"Disparado por Server_UpgradeAbilityNode. Escutado por GA_UpgradeAbilityNode. "
		"Payload: OptionalObject = UZfUpgradeNodeRequest.")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SkillTree_Event_UnlockSubEffect, "SkillTree.Event.UnlockSubEffect",
		"Disparado por Server_UnlockSubEffect. Escutado por GA_UnlockSubEffect. "
		"Payload: OptionalObject = UZfUnlockSubEffectRequest.")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SkillTree_Event_Respec, "SkillTree.Event.Respec",
		"Disparado por Server_RespecAbilityTree. Escutado por GA_RespecAbilityTree. "
		"Sem payload adicional.")
	
	UE_DEFINE_GAMEPLAY_TAG(SkillTree_AimMode_Active, "SkillTree.AimMode.Active")
	
	UE_DEFINE_GAMEPLAY_TAG(SkillTree_AimMode_Confirm, "SkillTree.AimMode.Confirm")
	
	
	// =========================================================================
	// DATA
	// =========================================================================

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SkillTree_Data_SkillPoints, "SkillTree.Data.SkillPoints",
		"Chave SetByCaller do GE_SpendSkillPoints. "
		"Negativo = gastar pontos. Positivo = devolver pontos (respec).")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SkillTree_Data_AbilityCost, "SkillTree.Data.AbilityCost",
		"Chave SetByCaller para passar custo base de recurso ao GE de custo. "
		"Lida pelo MMC de cada recurso para aplicar a reducao do atributo correspondente.")
	
	// =========================================================================
	// ABILITY
	// =========================================================================

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SkillTree_Ability_UnlockNode, "SkillTree.Ability.UnlockNode",
		"Tag de identificacao da GA_UnlockAbilityNode.")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SkillTree_Ability_UpgradeNode, "SkillTree.Ability.UpgradeNode",
		"Tag de identificacao da GA_UpgradeAbilityNode.")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SkillTree_Ability_UnlockSubEffect, "SkillTree.Ability.UnlockSubEffect",
		"Tag de identificacao da GA_UnlockSubEffect.")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SkillTree_Ability_Respec, "SkillTree.Ability.Respec",
		"Tag de identificacao da GA_RespecAbilityTree.")

	// =========================================================================
	// CLASS
	// =========================================================================

	UE_DEFINE_GAMEPLAY_TAG(SkillTree_Class_Novice,	"SkillTree.Class.Novice")
	UE_DEFINE_GAMEPLAY_TAG(SkillTree_Class_Mage,	"SkillTree.Class.Mage")
	UE_DEFINE_GAMEPLAY_TAG(SkillTree_Class_Warrior, "SkillTree.Class.Warrior")
	UE_DEFINE_GAMEPLAY_TAG(SkillTree_Class_ArcMage, "SkillTree.Class.ArcMage")
	
	// =========================================================================
	// Abilities
	// =========================================================================
	
	namespace ZfAbilityActiveTags
	{
		UE_DEFINE_GAMEPLAY_TAG(SkillTree_Ability_Active_Exemplo, "SkillTree.Ability.Active.Exemplo")
	}
	
	namespace ZfAbilityPassiveTags
	{
		UE_DEFINE_GAMEPLAY_TAG(SkillTree_Ability_Passive_MoveSpeedBoost,			"SkillTree.Ability.Passive.MoveSpeedBoost")
		UE_DEFINE_GAMEPLAY_TAG(SkillTree_Ability_Passive_MoveSpeedBoostTurbo,		"SkillTree.Ability.Passive.MoveSpeedBoostTurbo")
		UE_DEFINE_GAMEPLAY_TAG(SkillTree_Ability_Passive_MoveSpeedBoostTurboMaster, "SkillTree.Ability.Passive.MoveSpeedBoostTurboMaster")
	}
	
	// =========================================================================
	// NODE
	// =========================================================================
	namespace ZfNodeTags
	{
		// ── Novice ────────────────────────────────────────────────────────
		UE_DEFINE_GAMEPLAY_TAG(SkillTree_Node_Novice_MoveSpeedBoost,			"SkillTree.Node.Novice.MoveSpeedBoost")
		UE_DEFINE_GAMEPLAY_TAG(SkillTree_Node_Novice_MoveSpeedBoostTurbo,		"SkillTree.Node.Novice.MoveSpeedBoostTurbo")
		UE_DEFINE_GAMEPLAY_TAG(SkillTree_Node_Novice_MoveSpeedBoostTurboMaster, "SkillTree.Node.Novice.MoveSpeedBoostTurboMaster")
	}
	
	// =========================================================================
	// SUBEFFECT
	// =========================================================================
	namespace ZfSubEffectTags
	{
		// Sub-efeitos adicionados aqui conforme as abilities forem criadas
		UE_DEFINE_GAMEPLAY_TAG(SkillTree_SubEffect_Novice_Fireball_DoubleProjectile, "SkillTree.SubEffect.Novice.Fireball.DoubleProjectile")
	}
	
	// =========================================================================
	// COOLDOWN
	// =========================================================================

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Duration, "Cooldown.Duration",
		"Chave SetByCaller para passar duração do cooldown ao GE de cooldown. "
		"Lida pelo GE via Has Duration + SetByCaller.")

	namespace ZfCooldownTags
	{
		// ── Novice ────────────────────────────────────────────────────────
		UE_DEFINE_GAMEPLAY_TAG(Cooldown_Node_Novice_MoveSpeedBoost,				"Cooldown.Node.Novice.MoveSpeedBoost")
	}
	
	// =========================================================================
	// BUFF
	// =========================================================================
	
	namespace ZfBuffTags
	{
		// ── Novice ────────────────────────────────────────────────────────
		UE_DEFINE_GAMEPLAY_TAG(Buff_Node_Novice_MoveSpeedBoost,            "Buff.Node.Novice.MoveSpeedBoost")
	}
}