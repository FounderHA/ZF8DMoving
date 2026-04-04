// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/LevelProgression/ZfGA_ReceiveXP.h"
#include "AbilitySystemComponent.h"
#include "Tags/ZfGameplayTags.h"

UZfGA_ReceiveXP::UZfGA_ReceiveXP()
{
	// ── Políticas de execução ─────────────────────────────────────────────

	// ServerOnly: XP e level-up nunca são calculados no cliente.
	// O resultado replica automaticamente via os atributos replicados.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// InstancedPerExecution: permite múltiplas execuções sobrepostas.
	// Necessário pois várias fontes de XP podem disparar o evento
	// em sequência rápida (ex: AoE que mata vários inimigos de uma vez).
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

	// ── Trigger: ativada por GameplayEvent, nunca por input ───────────────
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag    = ZfProgressionTags::LevelProgression_Event_Character_XP_Gained;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	// ── Tags de identificação ─────────────────────────────────────────────
	// Permite localizar e cancelar esta ability via ASC se necessário.
	FGameplayTagContainer AssetTagContainer;
	AssetTagContainer.AddTag(ZfProgressionTags::LevelProgression_Ability_Progression_ReceiveXP);
	SetAssetTags(AssetTagContainer);

	// ── GiveXPEffectClass ─────────────────────────────────────────────────
	// Deixado nulo intencionalmente — deve ser atribuído no CDO do Blueprint
	// filho desta ability (ex: BP_ZfGA_ReceiveXP) apontando para o asset
	// GE_GiveXP criado no editor.
	// Isso desacopla a lógica C++ da configuração de dados do GE.
}

void UZfGA_ReceiveXP::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// ── Validações ────────────────────────────────────────────────────────

	float XPAmount = 0.f;
	if (!ValidateEventPayload(TriggerEventData, XPAmount))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!GiveXPEffectClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("UZfGA_ReceiveXP: GiveXPEffectClass não está configurado. "
			     "Verifique o CDO da ability."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ── Aplica o GE com a quantidade de XP via SetByCaller ────────────────
	//
	// MakeOutgoingGameplayEffectSpec usa o Level da ability (1 por padrão).
	// O Level do personagem não afeta este GE — o valor vem do SetByCaller.
	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, GiveXPEffectClass);

	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("UZfGA_ReceiveXP: Falha ao criar EffectSpec para GiveXPEffectClass."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Define a magnitude via SetByCaller — lida em GE_GiveXP → IncomingXP.
	SpecHandle.Data->SetSetByCallerMagnitude(ZfProgressionTags::LevelProgression_Data_XP_Amount, XPAmount);

	// Aplica ao próprio dono da ability (o jogador que ganhou o XP).
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);

	// Ability instantânea — encerra imediatamente após aplicar o GE.
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool UZfGA_ReceiveXP::ValidateEventPayload(
	const FGameplayEventData* TriggerEventData, float& OutXPAmount) const
{
	if (!TriggerEventData)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UZfGA_ReceiveXP: TriggerEventData é nulo. "
			     "A fonte de XP deve fornecer um payload com EventMagnitude."));
		return false;
	}

	OutXPAmount = TriggerEventData->EventMagnitude;

	if (OutXPAmount <= 0.f)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UZfGA_ReceiveXP: EventMagnitude (XP) <= 0 (valor: %.2f). "
			     "Verifique o valor enviado pela fonte de XP."), OutXPAmount);
		return false;
	}

	return true;
}