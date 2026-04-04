// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "Abilities/GameplayAbility.h"
#include "ZfGA_SpendAttributePoints.generated.h"

class UZfAttributeSpendRequest;

/**
 * GameplayAbility que investe pontos de atributo acumulados pelo jogador.
 * Apenas adiciona pontos — valores sempre positivos.
 * Para remover pontos individualmente use GA_RefundAttributePoint.
 * Para resetar tudo use GA_ResetAttributePoints.
 */
UCLASS()
class ZF8DMOVING_API UZfGA_SpendAttributePoints : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UZfGA_SpendAttributePoints();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "Progression|AttributePoints")
	TSubclassOf<UGameplayEffect> SpendEffectClass;

private:

	const UZfAttributeSpendRequest* GetValidatedRequest(
		const FGameplayEventData* TriggerEventData) const;
};