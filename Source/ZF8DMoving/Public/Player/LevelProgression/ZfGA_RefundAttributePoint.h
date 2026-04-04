// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "Abilities/GameplayAbility.h"
#include "ZfGA_RefundAttributePoint.generated.h"

class UZfAttributeRefundRequest;

/**
 * GameplayAbility que devolve pontos investidos de volta ao pool de AttributePoints.
 *
 * Usa UZfAttributeRefundRequest como payload — mesmo padrão do SpendRequest
 * mas com campos que representam QUANTO remover de cada atributo (sempre positivos).
 * A ability converte internamente para negativos no SetByCaller.
 *
 * Fluxo:
 *   Widget monta UZfAttributeRefundRequest com os valores a remover
 *     → SendGameplayEventToActor(Event.Progression.RefundAttributePoint)
 *       → GA_RefundAttributePoint ativa (ServerOnly)
 *         → Valida: cada XxxPoints >= valor a remover
 *           → Aplica GE_SpendAttributePoints com valores negativos
 *             → XxxPoints decrementados, AttributePoints incrementado
 *               → PostGameplayEffectExecute recalcula atributos tocados
 */
UCLASS()
class ZF8DMOVING_API UZfGA_RefundAttributePoint : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UZfGA_RefundAttributePoint();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	/** Mesmo GE_SpendAttributePoints — reutilizado com valores negativos. */
	UPROPERTY(EditDefaultsOnly, Category = "Progression|AttributePoints")
	TSubclassOf<UGameplayEffect> SpendEffectClass;

private:

	const UZfAttributeRefundRequest* GetValidatedRequest(
		const FGameplayEventData* TriggerEventData) const;

	bool ValidateRefund(
		UAbilitySystemComponent* ASC,
		const UZfAttributeRefundRequest* Request) const;
};