// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameplayAbility/SkillTreeSystem/ZfGameplayAbilitySkill.h"
#include "SkillTreeSystem/ZfSkillAimIndicator.h"
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

	/**
	 * Aplica o GE de cooldown com duração lida de NodeData->CooldownDurations.
	 * A duração escala por rank — sem CurveTable, tudo configurado no DA_Node.
	 * Usa SetByCaller com tag Cooldown.Duration para passar a duração ao GE.
	 * Chamado por CommitAbility() dentro do ActivateAbility.
	 */
	virtual void ApplyCooldown(
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

	// ── AimMode ───────────────────────────────────────────────────────────

	/**
	 * Classe do indicador visual spawned ao entrar no AimMode.
	 * Configure no Blueprint filho com o BP_AimIndicator correto.
	 *
	 * nullptr = skill sem AimMode (Instant ou NoTarget).
	 * A presença de um valor aqui é o que determina se a skill
	 * usa AimMode — sem necessidade de enum separado.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|AimMode")
	TSubclassOf<AZfSkillAimIndicator> AimIndicatorClass;

	/**
	 * Entra no modo de mira:
	 *   1. Adiciona tag SkillTree.AimMode.Active ao ASC
	 *   2. Spawna o AimIndicatorClass no mundo (cliente local)
	 *   3. Inicializa o indicador com MaxRange e AimRadius efetivos
	 *
	 * Chamado pelo Blueprint filho em ActivateAbility quando a skill
	 * precisa de targeting antes de executar.
	 * A GA permanece ativa aguardando confirmação ou cancelamento.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability|AimMode")
	void EnterAimMode();

	/**
	 * Sai do modo de mira:
	 *   1. Remove tag SkillTree.AimMode.Active do ASC
	 *   2. Destroi o AimIndicator se existir
	 *
	 * Chamado pelo Blueprint filho ao confirmar ou cancelar o cast.
	 * Deve ser chamado antes de EndAbility em qualquer caminho.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability|AimMode")
	void ExitAimMode();

	/**
	 * Retorna true se a GA está atualmente em AimMode.
	 * Consultado pelo Blueprint filho para decidir o comportamento
	 * ao receber o evento de confirmação ou cancelamento.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ability|AimMode")
	bool IsInAimMode() const { return IsValid(ActiveAimIndicator); }

	/**
	 * Retorna o AimIndicator ativo.
	 * Usado pelo Blueprint filho para ler GetCurrentHitLocation()
	 * ao confirmar o cast e montar os dados de targeting.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ability|AimMode")
	AZfSkillAimIndicator* GetActiveAimIndicator() const { return ActiveAimIndicator; }

	// ── Targeting em runtime ──────────────────────────────────────────────

	/**
	 * Retorna o raio de aim efetivo para o rank e sub-efeitos atuais.
	 * Implementação padrão lê do NodeData. Override no Blueprint filho
	 * para aplicar modificadores de sub-efeito em runtime.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Ability|Targeting")
	float GetEffectiveAimRadius() const;
	virtual float GetEffectiveAimRadius_Implementation() const;

	/**
	 * Retorna o alcance máximo efetivo para o rank e sub-efeitos atuais.
	 * Implementação padrão lê do NodeData. Override no Blueprint filho
	 * para aplicar modificadores de sub-efeito em runtime.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Ability|Targeting")
	float GetEffectiveMaxRange() const;
	virtual float GetEffectiveMaxRange_Implementation() const;

protected:

	/**
	 * Chamado após CommitAbility (custo e cooldown aplicados).
	 * Implemente aqui a lógica da ability no Blueprint filho.
	 * EndAbility deve ser chamado ao final da lógica Blueprint.
	 *
	 * Fluxo típico para skill com AimMode no Blueprint:
	 *   ActivateAbility → EnterAimMode → aguarda evento Confirm/Cancel
	 *   Confirm → ExitAimMode → aplica efeito → EndAbility
	 *   Cancel  → ExitAimMode → EndAbility(bWasCancelled=true)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Ability")
	void OnAbilityActivated(
		const FGameplayAbilitySpecHandle& Handle,
		const FGameplayAbilityActorInfo& ActorInfo,
		const FGameplayAbilityActivationInfo& ActivationInfo);

private:

	/**
	 * Referência ao indicador ativo.
	 * nullptr quando fora do AimMode.
	 * Não replicado — existe apenas no cliente local.
	 */
	UPROPERTY()
	TObjectPtr<AZfSkillAimIndicator> ActiveAimIndicator;};