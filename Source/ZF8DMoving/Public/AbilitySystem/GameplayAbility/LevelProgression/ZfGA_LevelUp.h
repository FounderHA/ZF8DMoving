// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ZfGA_LevelUp.generated.h"

class UZfLevelReward;
class UZfProgressionAttributeSet;

/**
 * GameplayAbility executada a cada level-up do personagem.
 *
 * Responsabilidades:
 *   1. Receber o evento Event.Character.LevelUp disparado pelo AttributeSet
 *   2. Iterar a lista de UZfLevelReward e executar cada recompensa
 *   3. Disparar o GameplayCue visual/sonoro (implementado em Blueprint)
 *
 * O que esta ability NÃO faz:
 *   - Não calcula XP nem thresholds (responsabilidade do AttributeSet)
 *   - Não define quais recompensas existem (responsabilidade das subclasses de UZfLevelReward)
 *   - Não implementa efeitos visuais (responsabilidade do GameplayCue Blueprint)
 *
 * Configuração:
 *   Crie um Blueprint filho (ex: BP_ZfGA_LevelUp) e configure:
 *     Rewards — adicione e configure instâncias de UZfLevelReward inline
 *
 *   Para adicionar uma recompensa:  Rewards → + → selecione o tipo → configure campos
 *   Para remover uma recompensa:    Rewards → selecione → Delete
 *   Zero recompilação necessária.
 *
 * Políticas:
 *   NetExecutionPolicy  : ServerOnly
 *   InstancingPolicy    : InstancedPerExecution
 *     Necessário pois o AttributeSet pode disparar múltiplos level-ups
 *     no mesmo frame — cada um cria uma instância independente.
 */
UCLASS()
class ZF8DMOVING_API UZfGA_LevelUp : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UZfGA_LevelUp();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// ─────────────────────────────────────────────────────────────────────
	// Lista de recompensas
	// ─────────────────────────────────────────────────────────────────────

	/**
	 * Recompensas concedidas a cada level-up, na ordem definida.
	 *
	 * Cada entrada é um asset de dados (Data Asset) criado no Content Browser:
	 *   botão direito → Miscellaneous → Data Asset → selecione o tipo de reward
	 *
	 * Ordem sugerida:
	 *   [0] DA_LR_ScaleAttributes  — escala atributos primeiro
	 *   [1] DA_LR_AttributePoints  — concede pontos após atributos atualizados
	 *
	 * Adicione, remova ou reordene assets sem tocar em código.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Progression|Rewards")
	TArray<TObjectPtr<UZfLevelReward>> Rewards;

private:

	/** Executa todas as recompensas da lista em ordem. */
	void ExecuteRewards(UAbilitySystemComponent* ASC, int32 FinalLevel, int32 LevelsGained) const;

	/** Dispara o GameplayCue de level-up (visual/som — implementado em Blueprint). */
	void ExecuteLevelUpCue(UAbilitySystemComponent* ASC, int32 FinalLevel, int32 LevelsGained) const;
};