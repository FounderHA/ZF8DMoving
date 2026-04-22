// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameplayAbility/SkillTreeSystem/ZfGameplayAbilitySkill.h"
#include "ZfAbility_Passive.generated.h"

/**
 * Base para todas as Gameplay Abilities passivas do projeto Zf.
 *
 * Ativa automaticamente ao ser concedida ao ASC — sem input, sem custo,
 * sem cooldown. Permanece ativa enquanto a ability existir no ASC.
 * Quando o jogador respeçar, a ability é revogada e os efeitos somem.
 *
 * Como usar:
 *   Crie um Blueprint filho (ex: BP_GA_MoveSpeedBoost) e configure:
 *     AbilityTag       → tag de identificação (ex: Ability.Passive.MoveSpeedBoost)
 *     PassiveEffect    → GE Infinite que aplica o efeito desejado
 *   Não precisa implementar nenhuma lógica adicional no Blueprint.
 *
 * Fluxo:
 *   Ability concedida ao ASC (desbloqueio na skill tree)
 *     → OnGiveAbility chamado pelo GAS
 *       → TryActivateAbility chamado automaticamente
 *         → ActivateAbility aplica PassiveEffect (GE Infinite)
 *           → Efeito permanece até a ability ser revogada (respec)
 *
 * Escalamento por rank:
 *   O PassiveEffect é aplicado com EffectLevel = AbilityLevel (rank do nó).
 *   Configure o GE com FScalableFloat + CurveTable para escalar por rank.
 */
UCLASS(Abstract)
class ZF8DMOVING_API UZfAbility_Passive : public UZfGameplayAbilitySkill
{
	GENERATED_BODY()

public:
	UZfAbility_Passive();

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// ── Configuração ──────────────────────────────────────────────────────

	/**
	 * GameplayEffect Infinite aplicado ao ativar a ability.
	 * Permanece ativo enquanto a ability existir no ASC.
	 * Removido automaticamente quando a ability é revogada (respec).
	 *
	 * Configure com FScalableFloat para escalar por AbilityLevel (rank).
	 * Ex: MoveSpeed += 10 no Rank 1, += 20 no Rank 2, += 35 no Rank 3
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Passive")
	TSubclassOf<UGameplayEffect> PassiveEffect;

private:

	/** Handle do GE aplicado — necessário para remover ao revogar a ability. */
	FActiveGameplayEffectHandle PassiveEffectHandle;
};