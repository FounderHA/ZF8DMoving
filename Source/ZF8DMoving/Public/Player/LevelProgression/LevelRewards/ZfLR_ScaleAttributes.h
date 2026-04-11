// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/LevelProgression/LevelRewards/ZfLevelReward.h"
#include "GameplayEffect.h"
#include "ZfLR_ScaleAttributes.generated.h"

/**
 * Recompensa de level-up: escala atributos do personagem via GameplayEffect + CurveTable.
 *
 * Mecanismo:
 *   O GE é aplicado usando NewLevel como EffectLevel.
 *   Modifiers no GE referenciam linhas da CurveTable — o engine lê a coluna
 *   correspondente ao EffectLevel automaticamente.
 *   Isso permite que HP, Dano, Defesa, etc. escalem por nível sem nenhum código extra.
 *
 * Configuração no editor:
 *   ScaleEffectClass — apontar para GE_LevelScaleAttributes (ver abaixo)
 *
 * GE_LevelScaleAttributes (criar no editor):
 *   Duration Policy : Instant
 *   Modifiers (um por atributo a escalar):
 *     Attribute    : ex. MaxHealth
 *     Operation    : Override  (ou Add se quiser acumular sobre o base)
 *     Magnitude    : Scalable Float → apontar linha na CurveTable + coluna por nível
 *
 * Exemplo de CurveTable (CT_LevelScaling):
 *   Linha "Default.ZfResourceAttributeSet.MaxHealth" : 100, 120, 145, 175, 210...
 *   O engine interpola automaticamente entre as colunas.
 *
 * Nota sobre Override vs Add:
 *   Override — substitui o valor base pelo valor da curva naquele nível (mais previsível)
 *   Add      — soma sobre o valor atual (cuidado com acumulações indesejadas entre resets)
 */
UCLASS(meta = (DisplayName = "Scale Attributes"))
class ZF8DMOVING_API UZfLR_ScaleAttributes : public UZfLevelReward
{
	GENERATED_BODY()

public:
	UZfLR_ScaleAttributes();

	virtual void GiveReward_Implementation(UAbilitySystemComponent* ASC, int32 FinalLevel, int32 LevelsGained) override;

	/**
	 * GE que escala atributos usando CurveTable.
	 * O NewLevel é passado como EffectLevel — a CurveTable usa esse valor
	 * para determinar qual coluna ler em cada modifier.
	 * Criar no editor: GE_LevelScaleAttributes (Instant, Scalable Float por atributo).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|Rewards")
	TSubclassOf<UGameplayEffect> ScaleEffectClass;
};