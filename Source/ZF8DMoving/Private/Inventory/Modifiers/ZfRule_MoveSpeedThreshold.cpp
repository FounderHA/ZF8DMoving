#include "Inventory/Modifiers/ZfRule_MoveSpeedThreshold.h"
#include "AbilitySystemComponent.h"

// Substitua pelo seu AttributeSet real
#include "AbilitySystem/Attributes/ZfMainAttributeSet.h"


/*
TArray<FGameplayAttribute> UZfRule_MoveSpeedThreshold::GetSourceAttributes() const
{
	return { UZfMainAttributeSet::GetMoveSpeedAttribute() };
}

float UZfRule_MoveSpeedThreshold::Calculate_Implementation(float CurrentValue) const
{
	if (!CachedContext.ASC) return CurrentValue;

	bool bFound = false;
	const float MoveSpeed = CachedContext.ASC->GetGameplayAttributeValue(
		UZfMainAttributeSet::GetMoveSpeedAttribute(), bFound);  // ← direto, sem GetSourceAttribute()

	if (!bFound || MoveSpeed <= 0.f) return CurrentValue;

	const float Step       = FMath::Max(1.f, GetThresholdStep());
	const float Multiplier = FMath::FloorToFloat(MoveSpeed / Step);

	return CurrentValue * Multiplier;
}
*/


TArray<FGameplayAttribute> UZfRule_MoveSpeedThreshold::GetSourceAttributes() const
{
	return Super::GetSourceAttributes();
}

float UZfRule_MoveSpeedThreshold::Calculate_Implementation(float CurrentValue) const
{
	return Super::Calculate_Implementation(CurrentValue);
}
