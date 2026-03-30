// Copyright ZfGame Studio. All Rights Reserved.
// ZfFragment_Quality.h
// Fragment que define se o item tem raridade


#include "Inventory/Fragments/ZfFragment_Modifiers.h"
#include "Misc/DataValidation.h"

EDataValidationResult UZfFragment_Modifiers::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	
	// Valida ModifierConfig — DataTable obrigatório se MaxTotalModifiers > 0
	if (ModifierConfig.MaxTotalModifiers > 0 &&
		ModifierConfig.ModifierDataTable.IsNull())
	{
		Context.AddError(FText::FromString(
			TEXT("ZfItemDefinition: ModifierConfig tem MaxTotalModifiers > 0 "
				 "mas ModifierDataTable não está configurado.")));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
