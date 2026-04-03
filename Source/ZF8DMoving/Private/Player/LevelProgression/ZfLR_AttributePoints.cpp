// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/LevelProgression/ZfLR_AttributePoints.h"

#include "AbilitySystemComponent.h"
#include "Tags/ZfGameplayTags.h"

UZfLR_AttributePoints::UZfLR_AttributePoints()
{
	// Padrão sensato — 1 ponto por level-up.
	// Altere no CDO do Blueprint filho ou diretamente no array de rewards da GA_LevelUp.
	PointsPerLevel  = 1;
	GrantEffectClass = nullptr; // Atribuir no editor — ver comentário no header.
}

void UZfLR_AttributePoints::GiveReward_Implementation(UAbilitySystemComponent* ASC, int32 NewLevel)
{
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("UZfLR_AttributePoints: ASC inválido."));
		return;
	}

	if (!GrantEffectClass)
	{
		UE_LOG(LogTemp, Error, TEXT("UZfLR_AttributePoints: GrantEffectClass não configurado. "
			"Atribua GE_GrantAttributePoints no CDO desta recompensa."));
		return;
	}

	// Cria o spec usando Level 1 — a magnitude vem do SetByCaller, não do nível do GE.
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(ASC->GetOwnerActor());

	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(GrantEffectClass, 1.f, Context);
	if (!Spec.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("UZfLR_AttributePoints: Falha ao criar EffectSpec."));
		return;
	}

	// A magnitude é a quantidade de pontos configurada no PointsPerLevel.
	Spec.Data->SetSetByCallerMagnitude(
		ZfProgressionTags::LevelProgression_Data_Progression_AttributePoints,
		static_cast<float>(PointsPerLevel)
	);

	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}