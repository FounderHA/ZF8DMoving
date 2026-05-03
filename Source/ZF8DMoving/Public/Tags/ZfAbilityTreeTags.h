// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

/**
 * Tags nativas do sistema de Skill Tree.
 *
 * Como usar:
 *   #include "Tags/ZfGameplayTags.h"
 *   ASC->HandleGameplayEvent(ZfAbilityTreeTags::SkillTree_Event_UnlockNode, &Payload);
 *
 * O que está definido aqui:
 *   EVENT  → eventos disparados pelos Server RPCs, escutados pelas GAs
 *   DATA   → chaves SetByCaller para GEs que consomem/concedem SkillPoints
 *   ABILITY → tags de identificação das GAs do sistema
 *   CLASS  → tags de classe concedidas ao personagem na progressão
 *
 * O que NÃO está aqui:
 *   SkillTree.Node.<X>       → definidas por nó no Data Asset, registradas via DefaultGameplayTags.ini
 *   SubEffect.<Ability>.<X>  → idem — pertencem ao designer de conteúdo, não ao sistema
 */
namespace ZfAbilityTreeTags
{
	// =========================================================================
	// EVENT — GameplayEvents disparados pelos Server RPCs
	// Escutados pelas GAs via AbilityTriggers
	// Convenção: SkillTree.Event.<Ação>
	// =========================================================================

	/** Disparado por Server_UnlockAbilityNode. Escutado por GA_UnlockAbilityNode.
	 * Payload: OptionalObject = UZfUnlockNodeRequest com o NodeID. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Event_UnlockNode)

	/** Disparado por Server_UpgradeAbilityNode. Escutado por GA_UpgradeAbilityNode.
	 * Payload: OptionalObject = UZfUpgradeNodeRequest com o NodeID. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Event_UpgradeNode)

	/** Disparado por Server_UnlockSubEffect. Escutado por GA_UnlockSubEffect.
	 * Payload: OptionalObject = UZfUnlockSubEffectRequest com NodeID e SubEffectIndex. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Event_UnlockSubEffect)

	/** Disparado por Server_RespecAbilityTree. Escutado por GA_RespecAbilityTree.
	 * Sem payload adicional — o componente lê o estado atual da tree. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Event_Respec)

	// =========================================================================
	// DATA — SetByCaller para GEs de SkillPoints
	// Convenção: SkillTree.Data.<Chave>
	// =========================================================================

	/** Chave SetByCaller do GE_SpendSkillPoints.
	 * Valor negativo = gastar pontos. Valor positivo = devolver pontos (respec).
	 * Uso: Spec.Data->SetSetByCallerMagnitude(ZfAbilityTreeTags::SkillTree_Data_SkillPoints, -1.f); */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Data_SkillPoints)
	
	/**
	* Chave SetByCaller usada pelas abilities ativas para passar o custo base ao GE de custo.
	* O MMC de cada recurso (Mana, Stamina, Health) lê este valor e aplica
	* a redução do atributo correspondente (ManaCostReduction, etc.).
	*
	* Uso na ability:
	*   Spec.Data->SetSetByCallerMagnitude(ZfAbilityTreeTags::SkillTree_Data_AbilityCost, CustoBase);
	*/
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Data_AbilityCost)

	// =========================================================================
	// ABILITY — Tags de identificação das Gameplay Abilities
	// Convenção: SkillTree.Ability.<Nome>
	// =========================================================================

	/** Tag de identificação da GA_UnlockAbilityNode. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Ability_UnlockNode)

	/** Tag de identificação da GA_UpgradeAbilityNode. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Ability_UpgradeNode)

	/** Tag de identificação da GA_UnlockSubEffect. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Ability_UnlockSubEffect)

	/** Tag de identificação da GA_RespecAbilityTree. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Ability_Respec)
	
	/** Tag de identificação do modo de mira */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_AimMode_Active)
	
	/** Tag de confirmação do modo de mira */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_AimMode_Confirm)

	// =========================================================================
	// CLASS — Tags de classe concedidas ao personagem na progressão
	// Concedida ao personagem ao Escolher ou evoluir a Class
	//
	// Usadas em:
	//   FAbilityTreeRegion::RequiredClassTag  → acesso à região da classe
	//   FAbilityTreeNode::RequiredTags        → nós exclusivos de classe
	//   AZfPlayerState                        → concedida ao escolher/evoluir classe
	//
	// Convenção: SkillTree.Class.<NomeDaClasse>
	// =========================================================================

	/** Tag da classe inicial — concedida a todos ao criar o personagem. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Class_Novice)
	/** Tag da classe Escolhida — Escolhida em jogo */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Class_Mage)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Class_Warrior)
	/** Tag da classe Evoluida — Escolhida em jogo */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Class_ArcMage)
	
	
	// =========================================================================
	// AbilityTag — Tags da Abilidade equipada
	// Configurada no GA da abilidade
	// Convenção: SkillTree.Ability.<Type>.<NomeDaAbility> (mesmo nome do Node)
	// =========================================================================
	
	namespace ZfAbilityActiveTags
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Ability_Active_Exemplo)
	}
	
	// ZfAbilityTreeTags.h
	namespace ZfAbilityPassiveTags
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Ability_Passive_MoveSpeedBoost)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Ability_Passive_MoveSpeedBoostTurbo)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Ability_Passive_MoveSpeedBoostTurboMaster)
	}
	
	
	// =========================================================================
	// NODE — Uma tag por nó da skill tree
	// Concedida ao personagem ao desbloquear o nó
	// Configurada no DA da Skill em Blueprint
	// Convenção: SkillTree.Node.<Região>.<NomeDaAbility>
	// =========================================================================
	namespace ZfNodeTags
	{
		// ── Novice ────────────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Node_Novice_MoveSpeedBoost)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Node_Novice_MoveSpeedBoostTurbo)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Node_Novice_MoveSpeedBoostTurboMaster)
	}
	
	// =========================================================================
	// SUBEFFECT — Uma tag por sub-efeito por habilidade
	// Concedida ao personagem ao desbloquear o sub-efeito
	// Configurada no DA da Skill em Blueprint
	// A ability consulta essas tags durante a execução para modificar comportamento
	// Convenção: SkillTree.SubEffect.<Região>.<NomeDaAbility>.<NomeDoEfeito>
	// =========================================================================
	namespace ZfSubEffectTags
	{
		// Sub-efeitos adicionados aqui conforme as abilities forem criadas
		// Exemplo:
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_SubEffect_Novice_Fireball_DoubleProjectile)
	}
	
	// =========================================================================
	// COOLDOWN — Tags de cooldown por nó
	// Usadas pela widget do slot para detectar cooldown ativo
	// Configuradas no GE de cooldown em GrantedTags
	// Convenção: Cooldown.Node.<Região>.<NomeDaSkill>
	// =========================================================================
 
	/** Chave SetByCaller para passar duração do cooldown ao GE.
	 * Uso: Spec.Data->SetSetByCallerMagnitude(ZfAbilityTreeTags::Cooldown_Duration, Duration); */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Duration)
 
	namespace ZfCooldownTags
	{
		// ── Novice ────────────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Node_Novice_MoveSpeedBoost)
	}
	
	// =========================================================================
	// BUFF — Tags de buff por nó
	// Usadas pela widget do slot para detectar buff ativo
	// Configuradas no GE de buff em GrantedTags
	// Convenção: Buff.Node.<Região>.<NomeDaSkill>
	// =========================================================================
 
	namespace ZfBuffTags
	{
		// ── Novice ────────────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Buff_Node_Novice_MoveSpeedBoost)
	}
}