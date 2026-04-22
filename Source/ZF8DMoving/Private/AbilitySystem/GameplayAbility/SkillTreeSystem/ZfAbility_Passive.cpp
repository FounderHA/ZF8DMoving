// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/GameplayAbility/SkillTreeSystem/ZfAbility_Passive.h"

#include "AbilitySystemComponent.h"

UZfAbility_Passive::UZfAbility_Passive()
{
	// Passive abilities mantêm estado persistente — InstancedPerActor
	// garante que a mesma instância existe durante toda a vida da ability no ASC.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// ServerOnly — efeitos passivos sempre aplicados no servidor.
	// Resultado replica via atributos e GEs automaticamente.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

// =============================================================================
// OnGiveAbility — chamado pelo GAS ao conceder a ability ao ASC
// =============================================================================

void UZfAbility_Passive::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	UE_LOG(LogTemp, Warning, TEXT("OnGiveAbility: %s | IsNetAuthority: %s | IsActive: %s"),
		*GetName(),
		ActorInfo->IsNetAuthority() ? TEXT("SERVER") : TEXT("CLIENT"),
		IsActive() ? TEXT("YES") : TEXT("NO"));

	if (!ActorInfo) return;
	if (!ActorInfo->IsNetAuthority()) return;

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC) return;

	if (!IsActive())
	{
		ASC->TryActivateAbility(Spec.Handle);
	}
}

// =============================================================================
// ActivateAbility — aplica o GE Infinite
// =============================================================================

void UZfAbility_Passive::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!PassiveEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("ActivateAbility: %s | IsNetAuthority: %s | PassiveEffectHandle valid: %s"),
			*GetName(),
				ActorInfo->IsNetAuthority() ? TEXT("SERVER") : TEXT("CLIENT"),
					PassiveEffectHandle.IsValid() ? TEXT("YES") : TEXT("NO"));
		
		UE_LOG(LogTemp, Warning,
			TEXT("UZfAbility_Passive '%s': PassiveEffect não configurado. "
				 "Configure no Blueprint filho."), *GetName());

		// Encerra mas não cancela — ability permanece concedida no ASC
		// mesmo sem efeito configurado, para não quebrar o estado da tree.
		EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Aplica o GE Infinite com AbilityLevel como EffectLevel
	// O GE usa FScalableFloat para escalar por rank automaticamente
	FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(
		Handle, ActorInfo, ActivationInfo,
		PassiveEffect,
		static_cast<float>(GetAbilityLevel()));

	if (!Spec.IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UZfAbility_Passive '%s': falha ao criar EffectSpec."), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PassiveEffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);

	if (!PassiveEffectHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UZfAbility_Passive '%s': falha ao aplicar PassiveEffect."), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Passive não encerra — permanece ativa enquanto a ability existir no ASC.
	// EndAbility NÃO é chamado aqui intencionalmente.
}