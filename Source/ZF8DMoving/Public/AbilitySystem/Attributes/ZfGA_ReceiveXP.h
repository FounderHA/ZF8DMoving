// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ZfGA_ReceiveXP.generated.h"

/**
 * GameplayAbility responsável por receber XP de qualquer fonte e entregá-lo
 * ao UZfProgressionAttributeSet via GE_GiveXP.
 *
 * Fluxo:
 *   Fonte de XP (mob/quest/item)
 *     → HandleGameplayEvent(Event.XP.Gained, Payload{EventMagnitude = XPAmount})
 *       → GA_ReceiveXP ativa (ServerOnly)
 *         → Valida e lê EventMagnitude
 *           → Aplica GE_GiveXP com SetByCaller(Data.XP.Amount, XPAmount)
 *             → AttributeSet::PostGameplayEffectExecute processa IncomingXP
 *
 * Configuração:
 *   - NetExecutionPolicy  : ServerOnly  (validação sempre no servidor)
 *   - InstancingPolicy    : InstancedPerExecution  (suporta execuções sobrepostas)
 *   - Trigger             : Event.XP.Gained (configurado no construtor)
 *
 * Esta ability deve ser concedida ao personagem na inicialização do PlayerState
 * e nunca removida durante a sessão.
 *
 * Nota: se o projeto tiver uma classe base UZfGameplayAbility, substitua
 * UGameplayAbility na herança abaixo.
 */
UCLASS()
class ZF8DMOVING_API UZfGA_ReceiveXP : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UZfGA_ReceiveXP();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:

	/**
	 * Classe do GameplayEffect que modifica IncomingXP.
	 * Deve ser atribuído no CDO do Blueprint filho desta ability,
	 * apontando para o asset GE_GiveXP criado no editor.
	 *
	 * Deixado nulo em C++ intencionalmente — a lógica não depende
	 * de nenhuma implementação concreta de GE.
	 * Se não for atribuído, a ability loga erro e encerra sem aplicar XP.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Progression|XP")
	TSubclassOf<UGameplayEffect> GiveXPEffectClass;

private:

	/**
	 * Valida o payload do evento.
	 * Retorna false e encerra a ability se o valor for inválido.
	 */
	bool ValidateEventPayload(const FGameplayEventData* TriggerEventData, float& OutXPAmount) const;
};