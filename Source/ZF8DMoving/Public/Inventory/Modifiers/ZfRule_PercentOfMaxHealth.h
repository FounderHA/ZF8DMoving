// Copyright ZfGame Studio. All Rights Reserved.
// Fórmula: CurrentValue * MaxHealth / 100
// Exemplo: Roll = 0.15 | MaxHealth = 800 → FinalValue = 120
// Use: modifier de Força que escala com a vida máxima do personagem.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Modifiers/ZfRule_AttributeBase.h"
#include "ZfRule_PercentOfMaxHealth.generated.h"

UCLASS(DisplayName = "Rule: % of Max Health")
class ZF8DMOVING_API UZfRule_PercentOfMaxHealth : public UZfRule_AttributeBase
{
	GENERATED_BODY()

protected:
	virtual TArray<FGameplayAttribute> GetSourceAttributes() const override;
	virtual float Calculate_Implementation(float CurrentValue) const override;
};