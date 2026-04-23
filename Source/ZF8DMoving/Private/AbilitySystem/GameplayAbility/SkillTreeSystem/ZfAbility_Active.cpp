// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/GameplayAbility/SkillTreeSystem/ZfAbility_Active.h"

#include "AbilitySystemComponent.h"
#include "SkillTreeSystem/ZfSkillTreeNodeData.h"
#include "Tags/ZfGameplayTags.h"

UZfAbility_Active::UZfAbility_Active()
{
	// InstancedPerExecution — suporta execuções sobrepostas e estado por execução
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

	// ServerOnly — custo e efeitos sempre validados no servidor
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

// =============================================================================
// CheckCost — verifica recursos antes de ativar
// =============================================================================

bool UZfAbility_Active::CheckCost(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	// Sem NodeData — sem custo, libera ativação
	if (!NodeData) return true;

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC) return false;

	const int32 CurrentRank = GetAbilityLevel();

	for (const FAbilityCostData& Cost : NodeData->Costs)
	{
		if (!Cost.CostEffectClass) continue;

		const float CostValue = Cost.GetCostForRank(CurrentRank);
		if (CostValue <= 0.f) continue;

		// Cria spec temporário com o valor do rank atual via SetByCaller
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(
			Cost.CostEffectClass, static_cast<float>(CurrentRank), Context);

		if (!Spec.IsValid()) continue;

		Spec.Data->SetSetByCallerMagnitude(
			ZfAbilityTreeTags::SkillTree_Data_AbilityCost, CostValue);

		// CanApplyAttributeModifiers checa se o recurso é suficiente
		// respeitando os clamps do PreAttributeChange do AttributeSet
		if (!ASC->CanApplyAttributeModifiers(
			Cost.CostEffectClass->GetDefaultObject<UGameplayEffect>(),
			static_cast<float>(CurrentRank),
			Context))
		{
			UE_LOG(LogTemp, Verbose,
				TEXT("UZfAbility_Active::CheckCost: recurso insuficiente em '%s'."),
				*GetName());
			return false;
		}
	}

	return true;
}

// =============================================================================
// ApplyCost — aplica GEs de custo com valores do NodeData
// =============================================================================

void UZfAbility_Active::ApplyCost(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!NodeData) return;

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC) return;

	const int32 CurrentRank = GetAbilityLevel();

	for (const FAbilityCostData& Cost : NodeData->Costs)
	{
		if (!Cost.CostEffectClass) continue;

		const float CostValue = Cost.GetCostForRank(CurrentRank);
		if (CostValue <= 0.f) continue;

		FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(
			Handle, ActorInfo, ActivationInfo,
			Cost.CostEffectClass,
			static_cast<float>(CurrentRank));

		if (!Spec.IsValid())
		{
			UE_LOG(LogTemp, Warning,
				TEXT("UZfAbility_Active::ApplyCost: falha ao criar spec para '%s'."),
				*Cost.CostEffectClass->GetName());
			continue;
		}

		// Passa o custo base via SetByCaller — MMC aplica a redução do atributo
		Spec.Data->SetSetByCallerMagnitude(
			ZfAbilityTreeTags::SkillTree_Data_AbilityCost, CostValue);

		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
	}
}

// =============================================================================
// ActivateAbility — fluxo principal
// =============================================================================

void UZfAbility_Active::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ActorInfo)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// CommitAbility chama ApplyCost() + ApplyCooldown() — ambos implementados acima
	// Se falhar (recurso insuficiente ou cooldown ativo), cancela a ability
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Reproduz montage se configurada
	if (AbilityMontage)
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		if (ASC)
		{
			ASC->PlayMontage(this, ActivationInfo, AbilityMontage, 1.f);
		}
	}

	// Blueprint implementa a lógica da ability aqui
	OnAbilityActivated(Handle, *ActorInfo, ActivationInfo);
}