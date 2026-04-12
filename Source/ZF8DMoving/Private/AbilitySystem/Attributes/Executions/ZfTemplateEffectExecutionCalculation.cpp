// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Attributes/Executions/ZfTemplateEffectExecutionCalculation.h"
#include "AbilitySystem/Attributes/ZfMainAttributeSet.h"
#include "AbilitySystem/Attributes/ZfResourceAttributeSet.h"
#include "Inventory/ZfEquipmentComponent.h"

struct FZfTemplateStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(Strength);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CurrentHealth);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MaxHealth);
	
	FZfTemplateStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UZfMainAttributeSet, Strength, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UZfResourceAttributeSet, CurrentHealth, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UZfResourceAttributeSet, MaxHealth, Source, false);
	}
};

static const FZfTemplateStatics& TemplateStatics()
{
	static FZfTemplateStatics DStatics;
	return DStatics;
}

UZfTemplateEffectExecutionCalculation::UZfTemplateEffectExecutionCalculation()
{
	RelevantAttributesToCapture.Add(TemplateStatics().StrengthDef);
	RelevantAttributesToCapture.Add(TemplateStatics().MaxHealthDef);
	RelevantAttributesToCapture.Add(TemplateStatics().CurrentHealthDef);
}

void UZfTemplateEffectExecutionCalculation::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
#if WITH_SERVER_CODE
	const UAbilitySystemComponent* TargetAbilitySystemComponent = ExecutionParams.GetTargetAbilitySystemComponent();
	const UAbilitySystemComponent* SourceAbilitySystemComponent = ExecutionParams.GetSourceAbilitySystemComponent();
	
	const AActor* SourceAvatarActor = SourceAbilitySystemComponent->GetAvatarActor();
	const AActor* SourceOwnerActor = SourceAbilitySystemComponent->GetOwnerActor();
	
	const AActor* TargetAvatarActor = TargetAbilitySystemComponent->GetAvatarActor();
	const AActor* TargetOwnerActor = TargetAbilitySystemComponent->GetOwnerActor();
	
	const FGameplayEffectSpec& EffectSpec = ExecutionParams.GetOwningSpec();
	const FGameplayEffectContextHandle& EffectContextHandle = EffectSpec.GetEffectContext();
	const FGameplayTagContainer& EffectAssetTags = EffectSpec.GetDynamicAssetTags();
	
	const FGameplayTagContainer* SourceTags = EffectSpec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = EffectSpec.CapturedTargetTags.GetAggregatedTags();
	
	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = SourceTags;
	EvaluateParameters.TargetTags = TargetTags;
	
	float SourceStrength;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(TemplateStatics().StrengthDef, EvaluateParameters, SourceStrength);
	SourceStrength = FMath::Max(0.0f, SourceStrength);
	
	float SourceHealth;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(TemplateStatics().CurrentHealthDef, EvaluateParameters, SourceHealth);
	SourceHealth = FMath::Max(0.0f, SourceHealth);
	
	float SourceMaxHealth;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(TemplateStatics().MaxHealthDef, EvaluateParameters, SourceMaxHealth);
	SourceMaxHealth = FMath::Max(0.0f, SourceMaxHealth);
	
	if (UZfEquipmentComponent* EquipmentComponent = SourceOwnerActor->FindComponentByClass<UZfEquipmentComponent>())
	{
		for (UZfItemInstance* Item : EquipmentComponent->GetAllEquippedItems())
		{
			Item->SetCurrentDurability(SourceStrength * 100.0f / (SourceMaxHealth * 0.5f));
		}
	}
#endif
}
