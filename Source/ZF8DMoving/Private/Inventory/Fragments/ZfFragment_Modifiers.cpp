// Copyright ZfGame Studio. All Rights Reserved.
// ZfFragment_Quality.h
// Fragment que define se o item tem raridade


#include "Inventory/Fragments/ZfFragment_Modifiers.h"
#include "Misc/DataValidation.h"

UDataTable* UZfFragment_Modifiers::GetLoadedModifierDataTable() const
{
	return ModifierConfig.ModifierDataTable.LoadSynchronous();
}

