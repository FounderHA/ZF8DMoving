// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameplayAbility/SkillTreeSystem/ZfGameplayAbilitySkill.h"
#include "ZfAbility_Active.generated.h"

/**
 * Base para todas as Gameplay Abilities ativas do projeto Zf.
 *
 * Custo e cooldown usam o sistema nativo do GAS:
 *   CostGameplayEffectClass     → configurado no Blueprint filho
 *   CooldownGameplayEffectClass → configurado no Blueprint filho
 *
 * O custo é lido do NodeData (fonte única de verdade) e passado via
 * SetByCaller ao GE de custo — os MMCs aplicam a redução do atributo.
 *
 * Fluxo de execução:
 *   TryActivateAbility
 *     → CheckCost()     (verifica se tem recursos — usa NodeData)
 *     → CheckCooldown() (nativo)
 *     → ActivateAbility()
 *         → CommitAbility() → ApplyCost() + ApplyCooldown()
 *         → PlayMontage     (se AbilityMontage configurado)
 *         → OnAbilityActivated (Blueprint implementa aqui)
 *
 * Como usar:
 *   Crie um Blueprint filho (ex: BP_GA_Fireball) e configure:
 *     AbilityTag                  → Ability.Spell.Fireball
 *     NodeData                    → DA_Node_Fireball
 *     CostGameplayEffectClass     → GE_Cost_Fireball
 *     CooldownGameplayEffectClass → GE_Cooldown_Fireball
 *     AbilityMontage              → AM_Fireball (opcional)
 *   Implemente OnAbilityActivated no Blueprint.
 */
UCLASS(Abstract)
class ZF8DMOVING_API UZfAbility_Active : public UZfGameplayAbilitySkill
{
	GENERATED_BODY()

public:
	UZfAbility_Active();

	// ── Overrides nativos do GAS ──────────────────────────────────────────

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	/**
	 * Verifica se o personagem tem recursos suficientes para todos os custos.
	 * Lê NodeData->Costs e verifica cada recurso individualmente.
	 * Chamado automaticamente pelo GAS antes de ativar a ability.
	 */
	virtual bool CheckCost(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/**
	 * Aplica todos os GEs de custo configurados em NodeData->Costs.
	 * Seta o SetByCaller(SkillTree.Data.AbilityCost) com o valor do rank atual.
	 * Chamado por CommitAbility() dentro do ActivateAbility.
	 */
	virtual void ApplyCost(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	// ── Configuração ──────────────────────────────────────────────────────

	/**
	 * Montage reproduzida ao ativar a ability.
	 * Opcional — deixe nulo se a ability não tiver animação.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Animation")
	TObjectPtr<UAnimMontage> AbilityMontage;

protected:

	/**
	 * Chamado após CommitAbility (custo e cooldown aplicados).
	 * Implemente aqui a lógica da ability no Blueprint filho.
	 * EndAbility deve ser chamado ao final da lógica Blueprint.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Ability")
	void OnAbilityActivated(const FGameplayAbilitySpecHandle& Handle,
		const FGameplayAbilityActorInfo& ActorInfo,
		const FGameplayAbilityActivationInfo& ActivationInfo);
};