// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SkillTreeSystem/ZfSkillTreeNodeData.h"
#include "ZfGameplayAbilitySkill.generated.h"

/**
 * Classe base de todas as Gameplay Abilities do projeto Zf.
 *
 * Responsabilidades:
 *   - AbilityTag  → identificação única da ability no sistema
 *   - NodeData    → referência ao Data Asset do nó da skill tree
 *                   usado por ZfAbility_Active para ler custo e rank
 *                   usado pela UI para exibir ícone, nome e custo no slot
 *
 * Subclasses diretas:
 *   UZfAbility_Active  → abilities ativas com custo, cooldown e animação
 *   UZfAbility_Passive → abilities que ativam automaticamente ao serem concedidas
 */
UCLASS(Abstract)
class ZF8DMOVING_API UZfGameplayAbilitySkill : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UZfGameplayAbilitySkill();

	/**
	 * Tag de identificação única desta ability.
	 * Usada para localizar, bloquear ou cancelar a ability via ASC.
	 * Convenção: Ability.<Categoria>.<Nome>
	 * Ex: Ability.Spell.Fireball, Ability.Passive.MoveSpeedBoost
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	FGameplayTag AbilityTag;

	/**
	 * Data Asset do nó da skill tree ao qual esta ability pertence.
	 * Configure no Blueprint filho apontando para DA_Node_<NomeDaAbility>.
	 *
	 * Usado por:
	 *   ZfAbility_Active → ler Costs para CheckCost e ApplyCost
	 *   Widget           → ler Icon, DisplayName, Costs para exibir no slot
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	TObjectPtr<UZfSkillTreeNodeData> NodeData;
};