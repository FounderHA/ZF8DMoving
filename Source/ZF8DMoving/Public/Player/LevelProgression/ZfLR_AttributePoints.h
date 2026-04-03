// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/LevelProgression/ZfLevelReward.h"
#include "GameplayEffect.h"
#include "ZfLR_AttributePoints.generated.h"

/**
 * Recompensa de level-up: concede AttributePoints distribuíveis ao jogador.
 *
 * Fluxo:
 *   GA_LevelUp chama GiveReward(ASC, NewLevel)
 *     → Aplica GE_GrantAttributePoints com SetByCaller(Data.Progression.AttributePoints, PointsPerLevel)
 *       → AttributeSet soma os pontos ao atributo AttributePoints
 *         → Jogador distribui via GA_SpendAttributePoint (etapa futura)
 *
 * Configuração no editor (CDO da GA_LevelUp ou DataAsset de rewards):
 *   PointsPerLevel       — quantos pontos por level-up (padrão: 1)
 *   GrantEffectClass     — apontar para o asset GE_GrantAttributePoints
 *
 * GE_GrantAttributePoints (criar no editor):
 *   Duration Policy : Instant
 *   Modifier        : AttributePoints | Add | SetByCaller (Data.Progression.AttributePoints)
 */
UCLASS(meta = (DisplayName = "Attribute Points"))
class ZF8DMOVING_API UZfLR_AttributePoints : public UZfLevelReward
{
	GENERATED_BODY()

public:
	UZfLR_AttributePoints();

	virtual void GiveReward_Implementation(UAbilitySystemComponent* ASC, int32 NewLevel) override;

	/** Quantidade de pontos distribuíveis concedidos a cada level-up. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|Rewards",
		meta = (ClampMin = "1", UIMin = "1"))
	int32 PointsPerLevel;

	/**
	 * GE que adiciona ao atributo AttributePoints.
	 * Criar no editor: GE_GrantAttributePoints (Instant, Add, SetByCaller).
	 * Atribuir no CDO desta classe ou no Blueprint filho.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|Rewards")
	TSubclassOf<UGameplayEffect> GrantEffectClass;
};