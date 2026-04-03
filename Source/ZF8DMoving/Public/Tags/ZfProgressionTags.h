// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

/**
 * Tags nativas do sistema de Progressão (XP / Level).
 *
 * Como usar:
 *   #include "ZfGameplayTags.h"
 *   ASC->HandleGameplayEvent(ZfProgressionTags::Event_Character_LevelUp, &Payload);
 */
namespace ZfProgressionTags
{
	// =========================================================================
	// EVENT — GameplayEvents disparados entre sistemas
	// Convenção: Event.<Domínio>.<Ação>
	// =========================================================================

	/**
	 * Disparado por qualquer fonte de XP (mob, quest, item) ao ASC do jogador.
	 * Escutado por GA_ReceiveXP.
	 * Payload: EventMagnitude = quantidade de XP a conceder.
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Event_XP_Gained)

	/**
	 * Disparado por UZfProgressionAttributeSet::HandleIncomingXP ao subir de nível.
	 * Escutado por GA_LevelUp.
	 * Payload: EventMagnitude = novo nível alcançado.
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Event_Character_LevelUp)

	// =========================================================================
	// DATA — SetByCaller: chaves usadas em GameplayEffects com magnitude dinâmica
	// Convenção: Data.<Sistema>.<Campo>
	// =========================================================================

	/**
	 * Chave SetByCaller do GE_GiveXP.
	 * Define a quantidade exata de XP a ser adicionada ao IncomingXP.
	 * Uso: SpecHandle.Data->SetSetByCallerMagnitude(ZfProgressionTags::Data_XP_Amount, 150.f);
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Data_XP_Amount)
	
	/**
	* Chave SetByCaller do GE_GrantAttributePoints.
	* Define quantos pontos distribuíveis conceder no level-up.
	* Usado por UZfLR_AttributePoints::GiveReward.
	* Uso: Spec.Data->SetSetByCallerMagnitude(ZfProgressionTags::Data_Progression_AttributePoints, PointsPerLevel);
	*/
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Data_Progression_AttributePoints)

	// =========================================================================
	// ABILITY — Tags de identificação das Gameplay Abilities
	// Convenção: Ability.<Sistema>.<Nome>
	// =========================================================================

	/**
	 * Tag de identificação da GA_ReceiveXP.
	 * Usada para localizar e cancelar a ability via ASC se necessário.
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Ability_Progression_ReceiveXP)

	/**
	 * Tag de identificação da GA_LevelUp.
	 * Usada para localizar a ability e como guard contra ativações duplas.
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Ability_Progression_LevelUp)

	// =========================================================================
	// GAMEPLAYCUE — Efeitos visuais e sonoros replicados
	// Convenção: GameplayCue.<Domínio>.<Efeito>
	// Implementação: Blueprint filho de AGameplayCueNotify_Actor ou _Static
	// =========================================================================

	/**
	 * Cue de level-up do personagem.
	 * Dispare via ASC->ExecuteGameplayCue() dentro da GA_LevelUp.
	 * Implemente o visual/som no Blueprint — sem lógica de negócio aqui.
	 * Payload sugerido: RawMagnitude = novo nível (para escalar efeito por nível).
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_GameplayCue_Character_LevelUp)
}