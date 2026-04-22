// Fill out your copyright notice in the Description page of Project Settings.
// ZfCraftRequirement.cpp

#include "CraftingSystem/Requirements/ZfCraftRequirement.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif


// ============================================================
// CheckRequirement — default
// ============================================================
// Default retorna true. Subclasses concretas devem sobrescrever
// para implementar a logica especifica. Classe base nunca deve
// ser usada diretamente no editor (Abstract impede).
// ============================================================

bool UZfCraftRequirement::CheckRequirement_Implementation(const FZfCraftContext& Context) const
{
	UE_LOG(LogZfCraft, Warning,
		TEXT("UZfCraftRequirement: CheckRequirement chamado na classe base. "
			 "A subclasse deveria ter sobrescrito esta funcao."));
	return true;
}


// ============================================================
// GetFailureReason — default
// ============================================================

FText UZfCraftRequirement::GetFailureReason_Implementation() const
{
	return NSLOCTEXT("ZfCraft", "RequirementFailed_Generic", "Requisito nao atendido.");
}


// ============================================================
// IsDataValid — default
// ============================================================
// Subclasses tipicamente sobrescrevem para validar campos proprios.
// ============================================================

#if WITH_EDITOR
EDataValidationResult UZfCraftRequirement::IsDataValid(FDataValidationContext& Context) const
{
	return Super::IsDataValid(Context);
}
#endif