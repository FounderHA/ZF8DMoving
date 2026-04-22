// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/LevelProgression/LevelRewards/ZfLevelReward.h"
#include "GameplayEffect.h"
#include "ZfLR_SkillPoints.generated.h"

/**
 * Recompensa de level-up: concede SkillPoints para gastar na Skill Tree.
 *
 * Fluxo:
 *   GA_LevelUp chama GiveReward(ASC, FinalLevel, LevelsGained)
 *     → Aplica GE_GrantSkillPoints com SetByCaller(LevelProgression.Data.Progression.SkillPoints, PointsPerLevel * LevelsGained)
 *       → ProgressionAttributeSet soma os pontos ao atributo SkillPoints
 *         → Jogador gasta via Server_UnlockAbilityNode / Server_UpgradeAbilityNode
 *
 * Configuração no editor:
 *   PointsPerLevel   — quantos pontos por level-up
 *   GrantEffectClass — apontar para GE_GrantSkillPoints
 *
 * GE_GrantSkillPoints (criar no editor):
 *   Duration Policy : Instant
 *   Modifier        : SkillPoints | Add | SetByCaller (LevelProgression.Data.Progression.SkillPoints)
 */
UCLASS(meta = (DisplayName = "Skill Points"))
class ZF8DMOVING_API UZfLR_SkillPoints : public UZfLevelReward
{
	GENERATED_BODY()

public:
	UZfLR_SkillPoints();

	virtual void GiveReward_Implementation(UAbilitySystemComponent* ASC, int32 FinalLevel, int32 LevelsGained) override;

	/** Quantidade de SkillPoints concedidos a cada level-up. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|Rewards",
		meta = (ClampMin = "1", UIMin = "1"))
	int32 PointsPerLevel;

	/**
	 * GE que adiciona ao atributo SkillPoints.
	 * Criar no editor: GE_GrantSkillPoints (Instant, Add, SetByCaller).
	 * Atribuir no CDO desta classe ou no Blueprint filho.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|Rewards")
	TSubclassOf<UGameplayEffect> GrantEffectClass;
};