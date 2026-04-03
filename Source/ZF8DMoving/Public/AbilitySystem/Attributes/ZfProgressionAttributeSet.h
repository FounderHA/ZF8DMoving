// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ZfAttributeSet.h"
#include "ZfProgressionAttributeSet.generated.h"

class UCurveTable;

/**
 * AttributeSet responsável por toda a economia de progressão do personagem.
 *
 * Atributos replicados:
 *   Level           — nível atual (mín. 1)
 *   XP              — XP relativo ao nível corrente (reseta com sobra ao subir)
 *   XPToNextLevel   — limiar para subir de nível (lido da CurveTable)
 *   TotalXP         — XP total acumulado desde o início (nunca reseta)
 *   AttributePoints — pontos distribuíveis gerados ao subir de nível
 *
 * Meta-atributo (servidor only, não replicado):
 *   IncomingXP      — caixa de entrada de XP; qualquer fonte de XP escreve
 *                     aqui via GE_GiveXP. PostGameplayEffectExecute consome
 *                     o valor, aplica a lógica de XP/level-up e zera o campo.
 *
 * Fluxo de level-up:
 *   IncomingXP chega → acumulado no XP corrente → while(XP >= threshold):
 *     Level++, XPToNextLevel atualizado via CurveTable inline,
 *     dispara GameplayEvent "Event.Character.LevelUp" para GA_LevelUp.
 *   Suporta múltiplos level-ups consecutivos na mesma chamada.
 *
 * Configuração:
 *   Crie um Blueprint filho desta classe (ex: BP_ZfProgressionAttributeSet),
 *   atribua LevelProgressionCurveTable no CDO e registre o Blueprint filho
 *   no PlayerState em vez da classe C++ diretamente.
 */
UCLASS()
class ZF8DMOVING_API UZfProgressionAttributeSet : public UZfAttributeSet
{
	GENERATED_BODY()

public:
	UZfProgressionAttributeSet(const FObjectInitializer& ObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	// -----------------------------------------------------------------------
	// Configuração
	// -----------------------------------------------------------------------

	/**
	 * CurveTable que define o XP necessário para cada nível.
	 * Linha esperada: "Progression.XPToNextLevel"
	 * Colunas: 1, 2, 3 ... N  (cada coluna = um nível)
	 *
	 * Configure no CDO do Blueprint filho desta classe.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CharacterProgression|Config")
	TObjectPtr<UCurveTable> LevelProgressionCurveTable;

	// -----------------------------------------------------------------------
	// Atributos replicados
	// -----------------------------------------------------------------------

	/** Nível atual do personagem. Mínimo: 1. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Level, Category = "CharacterProgression")
	FGameplayAttributeData Level;
	ATTRIBUTE_ACCESSORS(UZfProgressionAttributeSet, Level)

	/** XP relativo ao nível corrente. Reseta (com sobra) ao subir de nível. Use para barra de progresso na HUD. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_XP, Category = "CharacterProgression")
	FGameplayAttributeData XP;
	ATTRIBUTE_ACCESSORS(UZfProgressionAttributeSet, XP)

	
	 /** XP total acumulado desde a criação do personagem. Nunca reseta.
	 * Use para leaderboard, ranking, analytics ou qualquer comparação entre jogadores.
	 * Nunca é decrementado — nem por penalidades de morte nem por respec. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_TotalXP, Category = "CharacterProgression")
	FGameplayAttributeData TotalXP;
	ATTRIBUTE_ACCESSORS(UZfProgressionAttributeSet, TotalXP)

	/** Quantidade de XP necessária para sair do nível atual. Atualizado via CurveTable. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_XPToNextLevel, Category = "CharacterProgression")
	FGameplayAttributeData XPToNextLevel;
	ATTRIBUTE_ACCESSORS(UZfProgressionAttributeSet, XPToNextLevel)

	/** Pontos distribuíveis nos atributos principais. Concedidos pela GA_LevelUp via LR_AttributePoints. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttributePoints, Category = "CharacterProgression")
	FGameplayAttributeData AttributePoints;
	ATTRIBUTE_ACCESSORS(UZfProgressionAttributeSet, AttributePoints)

	// -----------------------------------------------------------------------
	// Meta-atributo (servidor only — NÃO replicado)
	// -----------------------------------------------------------------------

	/**
	 * Caixa de entrada de XP. Nunca leia este valor diretamente para UI.
	 * Fontes de XP (mobs, quests, itens) modificam este atributo via GE_GiveXP.
	 * PostGameplayEffectExecute consome e zera automaticamente.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "CharacterProgression|Meta")
	FGameplayAttributeData IncomingXP;
	ATTRIBUTE_ACCESSORS(UZfProgressionAttributeSet, IncomingXP)

protected:

	UFUNCTION()
	virtual void OnRep_Level(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	virtual void OnRep_XP(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	virtual void OnRep_TotalXP(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	virtual void OnRep_XPToNextLevel(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	virtual void OnRep_AttributePoints(const FGameplayAttributeData& OldValue) const;

private:

	/**
	 * Lógica central de XP e level-up.
	 * Chamado por PostGameplayEffectExecute quando IncomingXP é modificado.
	 * Suporta múltiplos level-ups consecutivos no mesmo frame.
	 */
	void HandleIncomingXP(const FGameplayEffectModCallbackData& Data);

	/**
	 * Consulta a CurveTable para obter o limiar de XP de um nível específico.
	 * Retorna 0 se a CurveTable não estiver configurada (sem level-up possível).
	 */
	float GetXPThresholdForLevel(int32 InLevel) const;
};