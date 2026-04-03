// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/LevelProgression/ZfGA_LevelUp.h"

#include "AbilitySystemComponent.h"
#include "Player/LevelProgression/ZfLevelReward.h"
#include "Tags/ZfGameplayTags.h"

UZfGA_LevelUp::UZfGA_LevelUp()
{
	// ── Políticas de execução ─────────────────────────────────────────────

	// ServerOnly: recompensas e efeitos colaterais sempre validados no servidor.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// InstancedPerExecution: cada level-up é uma instância independente.
	// Permite que múltiplos level-ups consecutivos (mesmo frame) rodem sem conflito.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

	// ── Trigger: ativada exclusivamente por GameplayEvent ─────────────────
	// Nunca ativada por input — o AttributeSet é a única fonte válida.
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag    = ZfProgressionTags::Event_Character_LevelUp;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	// ── Tags de identificação ─────────────────────────────────────────────
	AbilityTags.AddTag(ZfProgressionTags::Ability_Progression_LevelUp);
}

// =============================================================================

void UZfGA_LevelUp::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// ── Validações ────────────────────────────────────────────────────────

	if (!TriggerEventData)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UZfGA_LevelUp: TriggerEventData nulo — ability encerrada sem executar recompensas."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("UZfGA_LevelUp: ASC inválido."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// EventMagnitude carrega o novo nível — definido em HandleIncomingXP do AttributeSet.
	const int32 NewLevel = FMath::Max(1, FMath::FloorToInt(TriggerEventData->EventMagnitude));

	// ── Executa recompensas ───────────────────────────────────────────────
	ExecuteRewards(ASC, NewLevel);

	// ── Dispara GameplayCue ───────────────────────────────────────────────
	ExecuteLevelUpCue(ASC, NewLevel);

	// Ability síncrona — encerra imediatamente após processar tudo.
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

// =============================================================================
// Privados
// =============================================================================

void UZfGA_LevelUp::ExecuteRewards(UAbilitySystemComponent* ASC, int32 NewLevel) const
{
	if (Rewards.IsEmpty())
	{
		UE_LOG(LogTemp, Verbose, TEXT("UZfGA_LevelUp: Lista de Rewards vazia no nível %d. "
			     "Adicione assets de recompensa no array do BP_ZfGA_LevelUp."), NewLevel);
		return;
	}

	for (UZfLevelReward* Reward : Rewards)
	{
		if (!IsValid(Reward))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("UZfGA_LevelUp: Asset nulo na lista de Rewards — ignorado."));
			continue;
		}

		Reward->GiveReward(ASC, NewLevel);
	}
}

void UZfGA_LevelUp::ExecuteLevelUpCue(UAbilitySystemComponent* ASC, int32 NewLevel) const
{
	// ExecuteGameplayCue: disparo único, replicado para todos os clientes.
	// Correto para eventos instantâneos como level-up (não use Add/Remove aqui).
	FGameplayCueParameters CueParams;
	CueParams.RawMagnitude  = static_cast<float>(NewLevel); // disponível no Blueprint do Cue
	CueParams.SourceObject  = ASC->GetOwnerActor();
	CueParams.Instigator    = ASC->GetAvatarActor();
	CueParams.EffectContext = ASC->MakeEffectContext();

	ASC->ExecuteGameplayCue(ZfProgressionTags::GameplayCue_Character_LevelUp, CueParams);
}