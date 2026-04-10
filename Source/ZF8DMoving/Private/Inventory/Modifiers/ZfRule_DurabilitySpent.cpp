#include "Inventory/Modifiers/ZfRule_DurabilitySpent.h"
#include "Inventory/ZfItemInstance.h"

// Substitua pelo seu Fragment de Durabilidade real
#include "Inventory/Fragments/ZfFragment_Durability.h"

float UZfRule_DurabilitySpent::Calculate_Implementation(float CurrentValue) const
{
	if (!CachedContext.ItemInstance) return CurrentValue;

	const float CurrentDurability = CachedContext.ItemInstance->GetCurrentDurability();

	// Obtém a durabilidade máxima do fragment
	float MaxDurability = 0.f;
	if (const UZfFragment_Durability* DurabilityFragment =
		CachedContext.ItemInstance->GetFragment<UZfFragment_Durability>())
	{
		// Soma o bônus de modifier ao teto base do fragment
		MaxDurability = DurabilityFragment->MaxDurability
					  + CachedContext.ItemInstance->BonusMaxDurability;
	}

	if (MaxDurability <= 0.f) return CurrentValue;

	const float DurabilitySpent = FMath::Max(0.f, MaxDurability - CurrentDurability);

	return CurrentValue * DurabilitySpent * 0.01f;
}