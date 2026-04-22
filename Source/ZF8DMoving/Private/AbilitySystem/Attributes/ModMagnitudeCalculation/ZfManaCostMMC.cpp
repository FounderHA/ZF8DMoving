// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Attributes/ModMagnitudeCalculation/ZfManaCostMMC.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ZfUtilityAttributeSet.h"
#include "Tags/ZfGameplayTags.h"
#include "Player/ZfPlayerState.h"

float UZfManaCostMMC::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// Lê o custo base passado pela ability via SetByCaller
	const float CostBase = Spec.GetSetByCallerMagnitude(
		ZfAbilityTreeTags::SkillTree_Data_AbilityCost, false, 0.f);

	if (CostBase <= 0.f) return 0.f;

	// Obtém ASC, Pawn e PlayerState — mesmo padrão dos outros MMCs do projeto
	UAbilitySystemComponent* ASC = Spec.GetContext().GetOriginalInstigatorAbilitySystemComponent();
	if (!ASC) return -CostBase;

	AActor* AvatarActor = ASC->GetAvatarActor();
	if (!AvatarActor) return -CostBase;

	APawn* Pawn = Cast<APawn>(AvatarActor);
	if (!Pawn) return -CostBase;

	AZfPlayerState* PS = Pawn->GetPlayerState<AZfPlayerState>();
	if (!PS) return -CostBase;

	// Lê ManaCostReduction diretamente do AttributeSet
	float ManaCostReduction = 0.f;
	if (const UZfUtilityAttributeSet* UtilSet = ASC->GetSet<UZfUtilityAttributeSet>())
	{
		ManaCostReduction = FMath::Clamp(UtilSet->GetManaCostReduction(), 0.f, 100.f);
	}

	// Custo final com redução aplicada — negativo para subtrair Mana
	const float FinalCost = CostBase * (1.f - ManaCostReduction / 100.f);

	UE_LOG(LogTemp, Log,
		TEXT("ZfMMC_ManaCost: Base=%.1f | ManaCostReduction=%.1f%% | Final=%.1f"),
		CostBase, ManaCostReduction, FinalCost);

	return -FinalCost;
}