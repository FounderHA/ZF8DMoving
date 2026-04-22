// Fill out your copyright notice in the Description page of Project Settings.
// ZfCraftReq_Level.cpp

#include "CraftingSystem/Requirements/ZfCraftReq_Level.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/Attributes/ZfProgressionAttributeSet.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif


// ============================================================
// CheckRequirement
// ============================================================
// Busca o ASC do Pawn e le o atributo Level do AttributeSet
// de progressao. Comparacao direta com MinLevel.
// ============================================================

bool UZfCraftReq_Level::CheckRequirement_Implementation(const FZfCraftContext& Context) const
{
	if (!Context.InstigatorPawn)
	{
		UE_LOG(LogZfCraft, Warning, TEXT("CraftReq_Level: InstigatorPawn nulo."));
		return false;
	}

	// Pega ASC via interface (padrao do projeto).
	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Context.InstigatorPawn);
	if (!ASCInterface)
	{
		UE_LOG(LogZfCraft, Warning,
			TEXT("CraftReq_Level: Pawn nao implementa IAbilitySystemInterface."));
		return false;
	}

	UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogZfCraft, Warning, TEXT("CraftReq_Level: ASC nulo."));
		return false;
	}

	const UZfProgressionAttributeSet* ProgSet = ASC->GetSet<UZfProgressionAttributeSet>();
	if (!ProgSet)
	{
		UE_LOG(LogZfCraft, Warning,
			TEXT("CraftReq_Level: ProgressionAttributeSet nao encontrado no ASC."));
		return false;
	}

	const int32 CurrentLevel = FMath::FloorToInt(ProgSet->GetLevel());
	const bool bPasses = CurrentLevel >= MinLevel;

	UE_LOG(LogZfCraft, Verbose,
		TEXT("CraftReq_Level: Nivel atual=%d, MinLevel=%d, Passa=%s"),
		CurrentLevel, MinLevel, bPasses ? TEXT("SIM") : TEXT("NAO"));

	return bPasses;
}


// ============================================================
// GetFailureReason
// ============================================================

FText UZfCraftReq_Level::GetFailureReason_Implementation() const
{
	return FText::Format(
		NSLOCTEXT("ZfCraft", "RequirementFailed_Level", "Requer nivel {0}."),
		FText::AsNumber(MinLevel));
}


// ============================================================
// IsDataValid
// ============================================================

#if WITH_EDITOR
EDataValidationResult UZfCraftReq_Level::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (MinLevel < 1)
	{
		Context.AddError(FText::FromString(
			TEXT("ZfCraftReq_Level: MinLevel deve ser >= 1.")));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif