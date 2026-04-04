// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/LevelProgression/ZfGA_SpendAttributePoints.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ZfProgressionAttributeSet.h"
#include "Player/LevelProgression/ZfAttributeSpendRequest.h"
#include "Tags/ZfGameplayTags.h"

UZfGA_SpendAttributePoints::UZfGA_SpendAttributePoints()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag    = ZfProgressionTags::LevelProgression_Event_Character_SpendAttributePoints;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	FGameplayTagContainer Tags;
	Tags.AddTag(ZfProgressionTags::LevelProgression_Ability_Progression_SpendAttributePoints);
	SetAssetTags(Tags);
}

void UZfGA_SpendAttributePoints::ActivateAbility(
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

	// ── Valida o request ──────────────────────────────────────────────────
	const UZfAttributeSpendRequest* Request = GetValidatedRequest(TriggerEventData);
	if (!Request)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ── Valida AttributePoints disponíveis ────────────────────────────────
	const UZfProgressionAttributeSet* ProgSet = ASC->GetSet<UZfProgressionAttributeSet>();
	if (!ProgSet)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UZfGA_SpendAttributePoints: ProgressionAttributeSet não encontrado."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const int32 Available = FMath::FloorToInt(ProgSet->GetAttributePoints());
	const int32 Required  = Request->GetTotalPointsToSpend();

	if (Available < Required)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UZfGA_SpendAttributePoints: AttributePoints insuficientes. "
			     "Disponíveis: %d | Necessários: %d"), Available, Required);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ── Valida GE configurado ─────────────────────────────────────────────
	if (!SpendEffectClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("UZfGA_SpendAttributePoints: SpendEffectClass não configurado."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ── Aplica o GE com todos os SetByCaller ──────────────────────────────
	FGameplayEffectSpecHandle Spec =
		MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, SpendEffectClass);

	if (!Spec.IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UZfGA_SpendAttributePoints: Falha ao criar EffectSpec."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Total negativo — decrementa AttributePoints
	Spec.Data->SetSetByCallerMagnitude(ZfProgressionTags::LevelProgression_Data_Spend_Total,
		static_cast<float>(-Required));

	Spec.Data->SetSetByCallerMagnitude(ZfProgressionTags::LevelProgression_Data_Spend_Strength,
		static_cast<float>(Request->StrengthPointsToAdd));

	Spec.Data->SetSetByCallerMagnitude(ZfProgressionTags::LevelProgression_Data_Spend_Dexterity,
		static_cast<float>(Request->DexterityPointsToAdd));

	Spec.Data->SetSetByCallerMagnitude(ZfProgressionTags::LevelProgression_Data_Spend_Intelligence,
		static_cast<float>(Request->IntelligencePointsToAdd));

	Spec.Data->SetSetByCallerMagnitude(ZfProgressionTags::LevelProgression_Data_Spend_Constitution,
		static_cast<float>(Request->ConstitutionPointsToAdd));

	Spec.Data->SetSetByCallerMagnitude(ZfProgressionTags::LevelProgression_Data_Spend_Conviction,
		static_cast<float>(Request->ConvictionPointsToAdd));

	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

const UZfAttributeSpendRequest* UZfGA_SpendAttributePoints::GetValidatedRequest(
	const FGameplayEventData* TriggerEventData) const
{
	if (!TriggerEventData)
	{
		UE_LOG(LogTemp, Warning, TEXT("UZfGA_SpendAttributePoints: TriggerEventData nulo."));
		return nullptr;
	}

	const UZfAttributeSpendRequest* Request =
		Cast<UZfAttributeSpendRequest>(TriggerEventData->OptionalObject);

	if (!Request)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UZfGA_SpendAttributePoints: OptionalObject não é UZfAttributeSpendRequest."));
		return nullptr;
	}

	if (!Request->IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UZfGA_SpendAttributePoints: Request inválido — valores negativos ou zerados."));
		return nullptr;
	}

	return Request;
}