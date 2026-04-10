#pragma once

#include "CoreMinimal.h"
#include "Inventory/Modifiers/ZfRule_AttributeBase.h"
#include "ZfRule_InversedVitalRatio.generated.h"

UCLASS(DisplayName = "Rule: Inversed Vital Ratio")
class ZF8DMOVING_API UZfRule_InversedVitalRatio : public UZfRule_AttributeBase
{
	GENERATED_BODY()

protected:
	virtual TArray<FGameplayAttribute> GetSourceAttributes() const override;
	virtual float Calculate_Implementation(float CurrentValue) const override;
};