#include "Inventory/Modifiers/ZfRule_InversedVitalRatio.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ZfResourceAttributeSet.h"

TArray<FGameplayAttribute> UZfRule_InversedVitalRatio::GetSourceAttributes() const
{
	return {
		UZfResourceAttributeSet::GetHealthAttribute(),
		UZfResourceAttributeSet::GetMaxHealthAttribute()
	};
}

float UZfRule_InversedVitalRatio::Calculate_Implementation(float CurrentValue) const
{
	if (!CachedContext.ASC) return CurrentValue;

	bool bFound = false;
	const float CurrentHealth = CachedContext.ASC->GetGameplayAttributeValue(
		UZfResourceAttributeSet::GetHealthAttribute(), bFound);

	const float MaxHealth = CachedContext.ASC->GetGameplayAttributeValue(
		UZfResourceAttributeSet::GetMaxHealthAttribute(), bFound);

	if (!bFound || MaxHealth <= 0.f) return CurrentValue;

	const float Ratio         = FMath::Clamp(CurrentHealth / MaxHealth, 0.f, 1.f);
	const float InversedRatio = 1.f - Ratio;

	return CurrentValue * InversedRatio;
}