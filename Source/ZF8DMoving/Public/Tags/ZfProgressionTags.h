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
	// EVENT — GameplayEvents disparados entre sistemas Convenção: Event.<Domínio>.<Ação>
	// =========================================================================

	/** Disparado por qualquer fonte de XP (mob, quest, item) ao ASC do jogador. Escutado por GA_ReceiveXP.
	 * Payload: EventMagnitude = quantidade de XP a conceder. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Event_Character_XP_Gained)

	/** Disparado por UZfProgressionAttributeSet::HandleIncomingXP ao subir de nível. Escutado por GA_LevelUp.
	 * Payload: EventMagnitude = novo nível alcançado. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Event_Character_LevelUp)
	
	/** Disparado pela widget ao confirmar distribuição de pontos. Escutado por GA_SpendAttributePoints.
	* Payload: OptionalObject = UZfAttributeSpendRequest com os valores por atributo. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Event_Character_SpendAttributePoints)
	
	/** Disparado pela widget para refund individual. Payload: OptionalObject = UZfAttributeRefundRequest. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Event_Character_RefundAttributePoint)
	
	/** Disparado pelo botão "Reset All" na widget. Escutado por GA_ResetAttributePoints. Sem payload adicional. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Event_Character_ResetAttributePoints)

	// =========================================================================
	// DATA — SetByCaller para GEs de XP
	// =========================================================================

	/** Chave SetByCaller do GE_GiveXP. Define a quantidade exata de XP a ser adicionada ao IncomingXP.
	 * Uso: SpecHandle.Data->SetSetByCallerMagnitude(ZfProgressionTags::Data_XP_Amount, 150.f); */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Data_XP_Amount)
	
	/** Chave SetByCaller do GE_GrantAttributePoints. * Define quantos pontos distribuíveis conceder no level-up.
	* Usado por UZfLR_AttributePoints::GiveReward. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Data_Progression_AttributePoints)

	// =========================================================================
	// DATA — SetByCaller para GE_SpendAttributePoints Convenção: Data.Spend.<Atributo>
	// =========================================================================
 
	/** Total de pontos gastos — usado para decrementar AttributePoints. Calculado pela GA_SpendAttributePoints como soma de todos os campos abaixo.
	 * Valor sempre negativo no GE (ex: -3 para gastar 3 pontos). */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Data_Spend_Total)
 
	/** Pontos investidos em Strength. Positivo = adicionar, negativo = refund. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Data_Spend_Strength)
 
	/** Pontos investidos em Dexterity. Positivo = adicionar, negativo = refund. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Data_Spend_Dexterity)
 
	/** Pontos investidos em Intelligence. Positivo = adicionar, negativo = refund. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Data_Spend_Intelligence)
 
	/** Pontos investidos em Constitution. Positivo = adicionar, negativo = refund. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Data_Spend_Constitution)
 	
	/** Pontos investidos em Conviction. Positivo = adicionar, negativo = refund. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Data_Spend_Conviction)
	
	// =========================================================================
	// ABILITY — Tags de identificação das Gameplay Abilities
	// Convenção: Ability.<Sistema>.<Nome>
	// =========================================================================

	/** Tag de identificação da GA_ReceiveXP. Usada para localizar e cancelar a ability via ASC se necessário. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Ability_Progression_ReceiveXP)

	/** Tag de identificação da GA_LevelUp. Usada para localizar a ability e como guard contra ativações duplas. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Ability_Progression_LevelUp)
	
	/** Tag de identificação da GA_SpendAttributePoints. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Ability_Progression_SpendAttributePoints)
 
	/** Tag de identificação da GA_RefundAttributePoints. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Ability_Progression_RefundAttributePoint)
	
	/** Tag de identificação da GA_ResetAttributePoints. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_Ability_Progression_ResetAttributePoints)

	// =========================================================================
	// GAMEPLAYCUE — Efeitos visuais e sonoros replicados
	// Convenção: GameplayCue.<Domínio>.<Efeito>
	// Implementação: Blueprint filho de AGameplayCueNotify_Actor ou _Static
	// =========================================================================

	/** Cue de level-up do personagem.
	 * Dispare via ASC->ExecuteGameplayCue() dentro da GA_LevelUp.
	 * Implemente o visual/som no Blueprint — sem lógica de negócio aqui.
	 * Payload sugerido: RawMagnitude = novo nível (para escalar efeito por nível). */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelProgression_GameplayCue_Character_LevelUp)
}