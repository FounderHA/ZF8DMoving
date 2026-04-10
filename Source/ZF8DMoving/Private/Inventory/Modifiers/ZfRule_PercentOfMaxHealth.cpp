#include "Inventory/Modifiers/ZfRule_PercentOfMaxHealth.h"
#include "AbilitySystemComponent.h"

// Substitua UZfMainAttributeSet pelo nome real do seu AttributeSet
// que contém o atributo MaxHealth.

#include "AbilitySystem/Attributes/ZfResourceAttributeSet.h"

// ZfRule_PercentOfMaxHealth
TArray<FGameplayAttribute> UZfRule_PercentOfMaxHealth::GetSourceAttributes() const
{
	return { UZfResourceAttributeSet::GetMaxHealthAttribute() };
}

float UZfRule_PercentOfMaxHealth::Calculate_Implementation(float CurrentValue) const
{
	if (!CachedContext.ASC) return CurrentValue;

	bool bFound = false;
	
	const float MaxHealth = CachedContext.ASC->GetGameplayAttributeValue(UZfResourceAttributeSet::GetMaxHealthAttribute(), bFound);

	if (!bFound || MaxHealth <= 0.f) return 0.f;

	return CurrentValue * MaxHealth / 100.f;
}