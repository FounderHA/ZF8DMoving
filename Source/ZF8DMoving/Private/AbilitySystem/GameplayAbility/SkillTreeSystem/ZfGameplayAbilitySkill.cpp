// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/GameplayAbility/SkillTreeSystem/ZfGameplayAbilitySkill.h"
#include "AbilitySystemComponent.h"
#include "SkillTreeSystem/ZfSkillTreeNodeData.h"

UZfGameplayAbilitySkill::UZfGameplayAbilitySkill()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UZfGameplayAbilitySkill::AreAttributeRequirementsMet(const FGameplayAbilityActorInfo* ActorInfo) const
{
	// Sem NodeData ou sem requisitos — sempre atendido
	if (!NodeData || NodeData->AttributeRequirements.IsEmpty()) return true;

	if (!ActorInfo) return false;

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC) return false;

	for (const FAttributeRequirement& Req : NodeData->AttributeRequirements)
	{
		if (!Req.Attribute.IsValid()) continue;

		const float CurrentValue = ASC->GetNumericAttribute(Req.Attribute);
		if (CurrentValue < Req.MinValue)
		{
			return false;
		}
	}

	return true;
}