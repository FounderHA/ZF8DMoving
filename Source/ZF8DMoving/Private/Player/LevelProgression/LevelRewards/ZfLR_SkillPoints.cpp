// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/LevelProgression/LevelRewards/ZfLR_SkillPoints.h"

#include "AbilitySystemComponent.h"
#include "Tags/ZfGameplayTags.h"

UZfLR_SkillPoints::UZfLR_SkillPoints()
{
	PointsPerLevel   = 1;
	GrantEffectClass = nullptr; // Atribuir no editor — ver comentário no header.
}

void UZfLR_SkillPoints::GiveReward_Implementation(UAbilitySystemComponent* ASC, int32 FinalLevel, int32 LevelsGained)
{
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("UZfLR_SkillPoints: ASC inválido."));
		return;
	}

	if (!GrantEffectClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("UZfLR_SkillPoints: GrantEffectClass não configurado. "
				 "Atribua GE_GrantSkillPoints no CDO desta recompensa."));
		return;
	}

	const FGameplayTag& CallerTag = ZfProgressionTags::LevelProgression_Data_Progression_SkillPoints;

	UE_LOG(LogTemp, Log,
		TEXT("UZfLR_SkillPoints: GiveReward chamado. "
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
		UE_LOG(LogTemp, Warning, TEXT("UZfLR_SkillPoints: Falha ao criar EffectSpec."));
		return;
	}

	const float TotalPoints = static_cast<float>(PointsPerLevel * LevelsGained);

	UE_LOG(LogTemp, Log,
		TEXT("UZfLR_SkillPoints: Aplicando SetByCaller. Tag=%s TotalPoints=%.1f"),
		*CallerTag.ToString(), TotalPoints);

	Spec.Data->SetSetByCallerMagnitude(CallerTag, TotalPoints);
	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

	UE_LOG(LogTemp, Log, TEXT("UZfLR_SkillPoints: GE aplicado com sucesso."));
}