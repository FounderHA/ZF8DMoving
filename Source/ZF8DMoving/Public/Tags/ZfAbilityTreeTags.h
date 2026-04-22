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

	// =========================================================================
	// CLASS — Tags de classe concedidas ao personagem na progressão
	// Convenção: SkillTree.Class.<NomeDaClasse>
	//
	// Usadas em:
	//   FAbilityTreeRegion::RequiredClassTag  → acesso à região da classe
	//   FAbilityTreeNode::RequiredTags        → nós exclusivos de classe
	//   AZfPlayerState                        → concedida ao escolher/evoluir classe
	//
	// Tags de classes evoluídas seguem hierarquia de tag nativa do Unreal:
	//   SkillTree.Class.Mage inclui SkillTree.Class — HasTag funciona em hierarquia.
	//   Isso significa que um nó com RequiredTag = SkillTree.Class.Mage
	//   só é acessível por Magos, não por outras classes.
	// =========================================================================

	/** Tag da classe inicial — concedida a todos ao criar o personagem. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Class_Novice)

	namespace ZfAbilityActiveTags
	{
	}
	
	// ZfAbilityTreeTags.h
	namespace ZfAbilityPassiveTags
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Ability_Passive_MoveSpeedBoost)
	}
	
	
	// =========================================================================
	// NODE — Uma tag por nó da skill tree
	// Concedida ao personagem ao desbloquear o nó
	// Usada como pré-requisito de outros nós que dependem deste
	// Convenção: SkillTree.Node.<Região>.<NomeDaAbility>
	// =========================================================================
	namespace ZfNodeTags
	{
		// ── Novice ────────────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Node_Novice_MoveSpeedBoost)
	}

	
	// =========================================================================
	// SUBEFFECT — Uma tag por sub-efeito por habilidade
	// Concedida ao personagem ao desbloquear o sub-efeito
	// A ability consulta essas tags durante a execução para modificar comportamento
	// Convenção: SkillTree.SubEffect.<NomeDaAbility>.<NomeDoEfeito>
	// =========================================================================
	namespace ZfSubEffectTags
	{
		// Sub-efeitos adicionados aqui conforme as abilities forem criadas
		// Exemplo:
		//UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillTree_Node_Novice_Fireball_DoubleProjectile)
	}
}