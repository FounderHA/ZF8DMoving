// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/LevelProgression/ZfGA_ResetAttributePoints.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ZfProgressionAttributeSet.h"
#include "Tags/ZfGameplayTags.h"

UZfGA_ResetAttributePoints::UZfGA_ResetAttributePoints()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

	// Ativada por evento — botão "Reset All" na widget
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag    = ZfProgressionTags::LevelProgression_Event_Character_ResetAttributePoints;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	FGameplayTagContainer Tags;
	Tags.AddTag(ZfProgressionTags::LevelProgression_Ability_Progression_ResetAttributePoints);
	SetAssetTags(Tags);
}

void UZfGA_ResetAttributePoints::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!SpendEffectClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("UZfGA_ResetAttributePoints: SpendEffectClass não configurado."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const UZfProgressionAttributeSet* ProgSet = ASC->GetSet<UZfProgressionAttributeSet>();
	if (!ProgSet)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UZfGA_ResetAttributePoints: ProgressionAttributeSet não encontrado."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ── Lê os valores atuais de cada XxxPoints ────────────────────────────
	const int32 CurrStrength     = FMath::FloorToInt(ProgSet->GetStrengthPoints());
	const int32 CurrDexterity    = FMath::FloorToInt(ProgSet->GetDexterityPoints());
	const int32 CurrIntelligence = FMath::FloorToInt(ProgSet->GetIntelligencePoints());
	const int32 CurrConstitution = FMath::FloorToInt(ProgSet->GetConstitutionPoints());
	const int32 CurrConviction   = FMath::FloorToInt(ProgSet->GetConvictionPoints());

	const int32 TotalToReturn = CurrStrength + CurrDexterity + CurrIntelligence
	                          + CurrConstitution + CurrConviction;

	// Nada a resetar — encerra sem aplicar GE
	if (TotalToReturn <= 0)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("UZfGA_ResetAttributePoints: Nenhum ponto distribuído para resetar."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// ── Aplica GE_SpendAttributePoints com todos os valores negativos ─────
	// Zera cada XxxPoints individualmente e devolve o total ao AttributePoints.
	// Reutiliza o mesmo GE — o PostGameplayEffectExecute detecta as mudanças
	// e recalcula apenas os atributos que efetivamente tinham pontos.
	FGameplayEffectSpecHandle Spec =
		MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, SpendEffectClass);

	if (!Spec.IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UZfGA_ResetAttributePoints: Falha ao criar EffectSpec."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Total positivo = devolve pontos ao pool
	Spec.Data->SetSetByCallerMagnitude(ZfProgressionTags::LevelProgression_Data_Spend_Total,
		static_cast<float>(TotalToReturn));

	// Cada XxxPoints negativo — zera o que estava investido
	Spec.Data->SetSetByCallerMagnitude(ZfProgressionTags::LevelProgression_Data_Spend_Strength,
		static_cast<float>(-CurrStrength));

	Spec.Data->SetSetByCallerMagnitude(ZfProgressionTags::LevelProgression_Data_Spend_Dexterity,
		static_cast<float>(-CurrDexterity));

	Spec.Data->SetSetByCallerMagnitude(ZfProgressionTags::LevelProgression_Data_Spend_Intelligence,
		static_cast<float>(-CurrIntelligence));

	Spec.Data->SetSetByCallerMagnitude(ZfProgressionTags::LevelProgression_Data_Spend_Constitution,
		static_cast<float>(-CurrConstitution));

	Spec.Data->SetSetByCallerMagnitude(ZfProgressionTags::LevelProgression_Data_Spend_Conviction,
		static_cast<float>(-CurrConviction));

	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}