// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/GameplayAbility/LevelProgression/ZfGA_RefundAttributePoint.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ZfProgressionAttributeSet.h"
#include "Player/LevelProgression/ZfAttributeRefundRequest.h"
#include "Tags/ZfGameplayTags.h"

UZfGA_RefundAttributePoint::UZfGA_RefundAttributePoint()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag    = ZfProgressionTags::LevelProgression_Event_Character_RefundAttributePoint;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	FGameplayTagContainer Tags;
	Tags.AddTag(ZfProgressionTags::LevelProgression_Ability_Progression_RefundAttributePoint);
	SetAssetTags(Tags);
}

void UZfGA_RefundAttributePoint::ActivateAbility(
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

	const UZfAttributeRefundRequest* Request = GetValidatedRequest(TriggerEventData);
	if (!Request)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!ValidateRefund(ASC, Request))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!SpendEffectClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("UZfGA_RefundAttributePoint: SpendEffectClass não configurado."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FGameplayEffectSpecHandle Spec =
		MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, SpendEffectClass);

	if (!Spec.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("UZfGA_RefundAttributePoint: Falha ao criar EffectSpec."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const int32 Total = Request->GetTotalPointsToReturn();

	// Total positivo — incrementa AttributePoints (devolvendo ao pool)
	Spec.Data->SetSetByCallerMagnitude(ZfProgressionTags::LevelProgression_Data_Spend_Total,
		static_cast<float>(Total));

	// XxxPoints negativos — decrementa os pontos investidos
	Spec.Data->SetSetByCallerMagnitude(ZfProgressionTags::LevelProgression_Data_Spend_Strength,
		static_cast<float>(-Request->StrengthPointsToRemove));

	Spec.Data->SetSetByCallerMagnitude(ZfProgressionTags::LevelProgression_Data_Spend_Dexterity,
		static_cast<float>(-Request->DexterityPointsToRemove));

	Spec.Data->SetSetByCallerMagnitude(ZfProgressionTags::LevelProgression_Data_Spend_Intelligence,
		static_cast<float>(-Request->IntelligencePointsToRemove));

	Spec.Data->SetSetByCallerMagnitude(ZfProgressionTags::LevelProgression_Data_Spend_Constitution,
		static_cast<float>(-Request->ConstitutionPointsToRemove));

	Spec.Data->SetSetByCallerMagnitude(ZfProgressionTags::LevelProgression_Data_Spend_Conviction,
		static_cast<float>(-Request->ConvictionPointsToRemove));

	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

const UZfAttributeRefundRequest* UZfGA_RefundAttributePoint::GetValidatedRequest(
	const FGameplayEventData* TriggerEventData) const
{
	if (!TriggerEventData)
	{
		UE_LOG(LogTemp, Warning, TEXT("UZfGA_RefundAttributePoint: TriggerEventData nulo."));
		return nullptr;
	}

	const UZfAttributeRefundRequest* Request =
		Cast<UZfAttributeRefundRequest>(TriggerEventData->OptionalObject);

	if (!Request)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UZfGA_RefundAttributePoint: OptionalObject não é UZfAttributeRefundRequest."));
		return nullptr;
	}

	if (!Request->IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UZfGA_RefundAttributePoint: Request inválido."));
		return nullptr;
	}

	return Request;
}

bool UZfGA_RefundAttributePoint::ValidateRefund(
	UAbilitySystemComponent* ASC,
	const UZfAttributeRefundRequest* Request) const
{
	const UZfProgressionAttributeSet* ProgSet = ASC->GetSet<UZfProgressionAttributeSet>();
	if (!ProgSet)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UZfGA_RefundAttributePoint: ProgressionAttributeSet não encontrado."));
		return false;
	}

	// Verifica cada atributo individualmente
	struct FCheck { int32 ToRemove; float Current; FString Name; };
	TArray<FCheck> Checks =
	{
		{ Request->StrengthPointsToRemove,     ProgSet->GetStrengthPoints(),     TEXT("Strength")     },
		{ Request->DexterityPointsToRemove,    ProgSet->GetDexterityPoints(),    TEXT("Dexterity")    },
		{ Request->IntelligencePointsToRemove, ProgSet->GetIntelligencePoints(), TEXT("Intelligence") },
		{ Request->ConstitutionPointsToRemove, ProgSet->GetConstitutionPoints(), TEXT("Constitution") },
		{ Request->ConvictionPointsToRemove,   ProgSet->GetConvictionPoints(),   TEXT("Conviction")   },
	};

	for (const FCheck& Check : Checks)
	{
		if (Check.ToRemove > 0 && FMath::FloorToInt(Check.Current) < Check.ToRemove)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("UZfGA_RefundAttributePoint: %s insuficiente. "
				     "Atual: %.0f | Necessário para remover: %d"),
				*Check.Name, Check.Current, Check.ToRemove);
			return false;
		}
	}

	return true;
}