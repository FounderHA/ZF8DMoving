// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/LevelProgression/LevelRewards/ZfLR_AttributePoints.h"

#include "AbilitySystemComponent.h"
#include "Tags/ZfGameplayTags.h"

UZfLR_AttributePoints::UZfLR_AttributePoints()
{
	// Padrão sensato — 1 ponto por level-up.
	// Altere no CDO do Blueprint filho ou diretamente no array de rewards da GA_LevelUp.
	PointsPerLevel  = 1;
	GrantEffectClass = nullptr; // Atribuir no editor — ver comentário no header.
}

void UZfLR_AttributePoints::GiveReward_Implementation(UAbilitySystemComponent* ASC, int32 FinalLevel, int32 LevelsGained)
{
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("UZfLR_AttributePoints: ASC inválido."));
		return;
	}

	if (!GrantEffectClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("UZfLR_AttributePoints: GrantEffectClass não configurado. "
			     "Atribua GE_GrantAttributePoints no CDO desta recompensa."));
		return;
	}

	// ── Diagnóstico ───────────────────────────────────────────────────────
	const FGameplayTag& CallerTag = ZfProgressionTags::LevelProgression_Data_Progression_AttributePoints;
	UE_LOG(LogTemp, Log,
		TEXT("UZfLR_AttributePoints: GiveReward chamado. "
		     "FinalLevel=%d LevelsGained=%d PointsPerLevel=%d "
		     "GE=%s CallerTag=%s TagValid=%s"),
		FinalLevel, LevelsGained, PointsPerLevel,
		*GrantEffectClass->GetName(),
		*CallerTag.ToString(),
		CallerTag.IsValid() ? TEXT("SIM") : TEXT("NAO — TAG NÃO REGISTRADA"));

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(ASC->GetOwnerActor());

	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(GrantEffectClass, 1.f, Context);
	if (!Spec.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("UZfLR_AttributePoints: Falha ao criar EffectSpec."));
		return;
	}

	const float TotalPoints = static_cast<float>(PointsPerLevel * LevelsGained);

	UE_LOG(LogTemp, Log,
		TEXT("UZfLR_AttributePoints: Aplicando SetByCaller. Tag=%s TotalPoints=%.1f"),
		*CallerTag.ToString(), TotalPoints);

	Spec.Data->SetSetByCallerMagnitude(CallerTag, TotalPoints);

	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

	UE_LOG(LogTemp, Log, TEXT("UZfLR_AttributePoints: GE aplicado com sucesso."));
}