// Copyright ZfGame Studio. All Rights Reserved.
// Fórmula: CurrentValue * CurrentDurability * 0.01
// Quanto mais durabilidade restante, maior o FinalValue.
// Exemplo: Roll=2 | CurDur=75 → FinalValue = 2 * 75 * 0.01 = 1.5
// Use: modifier que enfraquece conforme o item vai sendo desgastado.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Modifiers/ZfRule_DurabilityBase.h"
#include "ZfRule_DurabilityRemaining.generated.h"

UCLASS(DisplayName = "Rule: Durability Remaining")
class ZF8DMOVING_API UZfRule_DurabilityRemaining : public UZfRule_DurabilityBase
{
	GENERATED_BODY()

public:
	virtual float Calculate_Implementation(float CurrentValue) const override;
};