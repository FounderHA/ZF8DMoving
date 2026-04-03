// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZfLevelReward.generated.h"

class UAbilitySystemComponent;

/**
 * Classe base para recompensas concedidas ao subir de nível.
 *
 * Herda de UDataAsset — cada recompensa é um asset no Content Browser,
 * exatamente como os itens do inventário. Crie assets filhos para cada
 * tipo de recompensa e configure os campos no editor.
 *
 * Como criar uma nova recompensa:
 *   Content Browser → botão direito → Miscellaneous → Data Asset
 *   → selecione a subclasse desejada (ex: ZfLR_AttributePoints)
 *   → configure os campos → arraste para o array Rewards em BP_ZfGA_LevelUp
 *
 * Como adicionar um novo tipo de recompensa ao sistema:
 *   1. Crie UZfLR_NomeDaRecompensa : public UZfLevelReward
 *   2. Override GiveReward_Implementation
 *   3. Crie o asset no editor e adicione ao array — zero impacto no restante
 */
UCLASS(BlueprintType, Blueprintable)
class ZF8DMOVING_API UZfLevelReward : public UDataAsset
{
	GENERATED_BODY()

public:

	/**
	 * Executa a recompensa para o personagem que subiu de nível.
	 *
	 * @param ASC      AbilitySystemComponent do jogador
	 * @param NewLevel Nível recém-alcançado
	 *
	 * BlueprintNativeEvent: C++ implementa GiveReward_Implementation,
	 * Blueprint pode fazer override para recompensas customizadas.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Progression|Rewards")
	void GiveReward(UAbilitySystemComponent* ASC, int32 NewLevel);
	virtual void GiveReward_Implementation(UAbilitySystemComponent* ASC, int32 NewLevel);
};