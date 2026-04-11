// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/LevelProgression/LevelRewards/ZfLR_ScaleAttributes.h"

#include "AbilitySystemComponent.h"

UZfLR_ScaleAttributes::UZfLR_ScaleAttributes()
{
	ScaleEffectClass = nullptr; // Atribuir no editor — ver comentário no header.
}

void UZfLR_ScaleAttributes::GiveReward_Implementation(UAbilitySystemComponent* ASC, int32 FinalLevel, int32 LevelsGained)
{
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("UZfLR_ScaleAttributes: ASC inválido."));
		return;
	}

	if (!ScaleEffectClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("UZfLR_ScaleAttributes: ScaleEffectClass não configurado. "
				 "Atribua GE_LevelScaleAttributes no CDO desta recompensa."));
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(ASC->GetOwnerActor());

	// FinalLevel como EffectLevel — a CurveTable lê a coluna correta
	// independente de quantos níveis foram ganhos de uma vez.
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(
		ScaleEffectClass,
		static_cast<float>(FinalLevel),
		Context
	);

	if (!Spec.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("UZfLR_ScaleAttributes: Falha ao criar EffectSpec."));
		return;
	}

	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}