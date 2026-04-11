// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ZfGA_ResetAttributePoints.generated.h"

/**
 * GameplayAbility que reseta todos os pontos distribuídos,
 * devolvendo-os ao pool de AttributePoints.
 *
 * Fluxo:
 *   Widget chama SendGameplayEventToActor(Event.Progression.ResetAttributePoints)
 *     → GA_ResetAttributePoints ativa (ServerOnly)
 *       → Lê valores atuais de XxxPoints no ProgressionAttributeSet
 *         → Aplica GE_SpendAttributePoints com todos os XxxPoints negativos
 *           → XxxPoints zerados, AttributePoints incrementado pelo total devolvido
 *             → PostGameplayEffectExecute recalcula todos os atributos tocados
 *
 * Usa o mesmo GE_SpendAttributePoints da GA_SpendAttributePoints —
 * não precisa de GE adicional.
 *
 * Configure no Blueprint filho (BP_ZfGA_ResetAttributePoints):
 *   SpendEffectClass → GE_SpendAttributePoints (o mesmo asset)
 */
UCLASS()
class ZF8DMOVING_API UZfGA_ResetAttributePoints : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UZfGA_ResetAttributePoints();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	/** Mesmo GE usado pela GA_SpendAttributePoints. */
	UPROPERTY(EditDefaultsOnly, Category = "Progression|AttributePoints")
	TSubclassOf<UGameplayEffect> SpendEffectClass;
};