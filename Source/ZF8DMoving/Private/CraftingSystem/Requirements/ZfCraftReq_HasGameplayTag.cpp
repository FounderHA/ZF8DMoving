// Fill out your copyright notice in the Description page of Project Settings.
// ZfCraftReq_HasGameplayTag.cpp

#include "CraftingSystem/Requirements/ZfCraftReq_HasGameplayTag.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif


// ============================================================
// CheckRequirement
// ============================================================

bool UZfCraftReq_HasGameplayTag::CheckRequirement_Implementation(const FZfCraftContext& Context) const
{
	if (!RequiredTag.IsValid())
	{
		UE_LOG(LogZfCraft, Warning, TEXT("CraftReq_HasGameplayTag: RequiredTag nao setada."));
		return false;
	}

	if (!Context.InstigatorPawn)
	{
		UE_LOG(LogZfCraft, Warning, TEXT("CraftReq_HasGameplayTag: InstigatorPawn nulo."));
		return false;
	}

	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Context.InstigatorPawn);
	if (!ASCInterface)
	{
		UE_LOG(LogZfCraft, Warning,
			TEXT("CraftReq_HasGameplayTag: Pawn nao implementa IAbilitySystemInterface."));
		return false;
	}

	UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogZfCraft, Warning, TEXT("CraftReq_HasGameplayTag: ASC nulo."));
		return false;
	}

	const bool bHasTag = ASC->HasMatchingGameplayTag(RequiredTag);
	const bool bPasses = bInverted ? !bHasTag : bHasTag;

	UE_LOG(LogZfCraft, Verbose,
		TEXT("CraftReq_HasGameplayTag: Tag=%s, HasTag=%s, Inverted=%s, Passa=%s"),
		*RequiredTag.ToString(),
		bHasTag ? TEXT("SIM") : TEXT("NAO"),
		bInverted ? TEXT("SIM") : TEXT("NAO"),
		bPasses ? TEXT("SIM") : TEXT("NAO"));

	return bPasses;
}


// ============================================================
// GetFailureReason
// ============================================================

FText UZfCraftReq_HasGameplayTag::GetFailureReason_Implementation() const
{
	if (!CustomFailureReason.IsEmpty())
	{
		return CustomFailureReason;
	}

	if (bInverted)
	{
		return FText::Format(
			NSLOCTEXT("ZfCraft", "RequirementFailed_NotHasTag", "Nao pode ter o estado: {0}."),
			FText::FromString(RequiredTag.ToString()));
	}

	return FText::Format(
		NSLOCTEXT("ZfCraft", "RequirementFailed_HasTag", "Requer estado: {0}."),
		FText::FromString(RequiredTag.ToString()));
}


// ============================================================
// IsDataValid
// ============================================================

#if WITH_EDITOR
EDataValidationResult UZfCraftReq_HasGameplayTag::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (!RequiredTag.IsValid())
	{
		Context.AddError(FText::FromString(
			TEXT("ZfCraftReq_HasGameplayTag: RequiredTag nao esta setada.")));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif