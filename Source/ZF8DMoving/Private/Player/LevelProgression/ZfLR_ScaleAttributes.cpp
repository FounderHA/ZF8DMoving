// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/LevelProgression/ZfLR_ScaleAttributes.h"

#include "AbilitySystemComponent.h"

UZfLR_ScaleAttributes::UZfLR_ScaleAttributes()
{
	ScaleEffectClass = nullptr; // Atribuir no editor — ver comentário no header.
}

void UZfLR_ScaleAttributes::GiveReward_Implementation(UAbilitySystemComponent* ASC, int32 NewLevel)
{
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("UZfLR_ScaleAttributes: ASC inválido."));
		return;
	}

	if (!ScaleEffectClass)
	{
		UE_LOG(LogTemp, Error, TEXT("UZfLR_ScaleAttributes: ScaleEffectClass não configurado. "
			"Atribua GE_LevelScaleAttributes no CDO desta recompensa."));
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(ASC->GetOwnerActor());

	// NewLevel é passado como EffectLevel.
	// Este é o valor que o engine usa para indexar a CurveTable nos modifiers do GE.
	// Ex: nível 5 → engine lê a coluna 5 de cada linha da CurveTable referenciada.
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(ScaleEffectClass, static_cast<float>(NewLevel),
		Context
	);

	if (!Spec.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("UZfLR_ScaleAttributes: Falha ao criar EffectSpec."));
		return;
	}

	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}