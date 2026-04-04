// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/LevelProgression/ZfGA_LevelUp.h"
#include "AbilitySystemComponent.h"
#include "Player/LevelProgression/ZfLevelReward.h"
#include "AbilitySystem/Attributes/ZfProgressionAttributeSet.h"
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
	TriggerData.TriggerTag    = ZfProgressionTags::LevelProgression_Event_Character_LevelUp;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	// ── Tags de identificação ─────────────────────────────────────────────
	FGameplayTagContainer AssetTagContainer;
	AssetTagContainer.AddTag(ZfProgressionTags::LevelProgression_Ability_Progression_LevelUp);
	SetAssetTags(AssetTagContainer);
}

// =============================================================================

void UZfGA_LevelUp::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!TriggerEventData)
	{
		UE_LOG(LogTemp, Warning, TEXT("UZfGA_LevelUp: TriggerEventData nulo — ability encerrada sem executar recompensas."));
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

	// EventMagnitude → quantos níveis foram ganhos
	// FinalLevel     → lido diretamente do ProgressionAttributeSet via ASC
	const int32 LevelsGained = FMath::Max(1, FMath::FloorToInt(TriggerEventData->EventMagnitude));

	int32 FinalLevel = 1;
	if (const UZfProgressionAttributeSet* ProgSet =
		ASC->GetSet<UZfProgressionAttributeSet>())
	{
		FinalLevel = FMath::Max(1, FMath::FloorToInt(ProgSet->GetLevel()));
	}

	// Executa recompensas uma única vez com o contexto consolidado.
	ExecuteRewards(ASC, FinalLevel, LevelsGained);

	// Dispara UM único GameplayCue — independente de quantos níveis foram ganhos.
	ExecuteLevelUpCue(ASC, FinalLevel, LevelsGained);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

// =============================================================================
// Privados
// =============================================================================

void UZfGA_LevelUp::ExecuteRewards(UAbilitySystemComponent* ASC, int32 FinalLevel, int32 LevelsGained) const
{
	if (Rewards.IsEmpty())
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("UZfGA_LevelUp: Lista de Rewards vazia. "
			     "Adicione assets no array do BP_ZfGA_LevelUp."));
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

		// FinalLevel  → nível atual do personagem (usado por LR_ScaleAttributes)
		// LevelsGained → quantos níveis foram ganhos (usado por LR_AttributePoints)
		Reward->GiveReward(ASC, FinalLevel, LevelsGained);
	}
}

void UZfGA_LevelUp::ExecuteLevelUpCue(UAbilitySystemComponent* ASC, int32 FinalLevel, int32 LevelsGained) const
{
	FGameplayCueParameters CueParams;

	// RawMagnitude → LevelsGained: Blueprint usa para repetir a animação N vezes.
	CueParams.RawMagnitude = static_cast<float>(LevelsGained);

	// NormalizedMagnitude → FinalLevel: float livre de restrições de serialização.
	// AbilityLevel foi evitado propositalmente — é serializado como uint8
	// com limite MAX_LEVEL (~15), insuficiente para níveis altos do personagem.
	CueParams.NormalizedMagnitude = static_cast<float>(FinalLevel);

	CueParams.SourceObject  = ASC->GetOwnerActor();
	CueParams.Instigator    = ASC->GetAvatarActor();
	CueParams.EffectContext = ASC->MakeEffectContext();

	ASC->ExecuteGameplayCue(ZfProgressionTags::LevelProgression_GameplayCue_Character_LevelUp, CueParams);
}