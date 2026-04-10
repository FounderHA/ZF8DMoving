#include "Inventory/Modifiers/ZfRule_DurabilityRemaining.h"
#include "Inventory/ZfItemInstance.h"

float UZfRule_DurabilityRemaining::Calculate_Implementation(float CurrentValue) const
{
	if (!CachedContext.ItemInstance) return CurrentValue;

	const float CurrentDurability = CachedContext.ItemInstance->GetCurrentDurability();

	return CurrentDurability * CurrentValue;
}