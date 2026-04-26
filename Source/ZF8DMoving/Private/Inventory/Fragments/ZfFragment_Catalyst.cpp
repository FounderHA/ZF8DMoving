// Copyright ZfGame Studio. All Rights Reserved.
// ZfFragment_Catalyst.cpp

#include "Inventory/Fragments/ZfFragment_Catalyst.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif


// ============================================================
// IsDataValid
// ============================================================

#if WITH_EDITOR
EDataValidationResult UZfFragment_Catalyst::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (SpeedMultiplier < 1.0f)
	{
		Context.AddError(FText::FromString(
			TEXT("ZfFragment_Catalyst: SpeedMultiplier deve ser >= 1.0. "
				 "Valores abaixo de 1.0 desacelerariam o refino.")));
		Result = EDataValidationResult::Invalid;
	}

	if (BoostDuration <= 0.0f)
	{
		Context.AddError(FText::FromString(
			TEXT("ZfFragment_Catalyst: BoostDuration deve ser maior que 0.")));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif


// ============================================================
// GetDebugString
// ============================================================

FString UZfFragment_Catalyst::GetDebugString() const
{
	return FString::Printf(
		TEXT("[Fragment_Catalyst] SpeedMultiplier: %.2fx | BoostDuration: %.1fs"),
		SpeedMultiplier,
		BoostDuration);
}