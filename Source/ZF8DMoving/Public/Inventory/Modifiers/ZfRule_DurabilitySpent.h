// Copyright ZfGame Studio. All Rights Reserved.
// Fórmula: CurrentValue * (MaxDurability - CurrentDurability) * 0.01
// Quanto mais durabilidade gasta, maior o FinalValue.
// Exemplo: Roll=2 | MaxDur=100 | CurDur=30 → FinalValue = 2 * 70 * 0.01 = 1.4
// Use: modifier que cresce conforme o item vai sendo desgastado.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Modifiers/ZfRule_DurabilityBase.h"
#include "ZfRule_DurabilitySpent.generated.h"

UCLASS(DisplayName = "Rule: Durability Spent")
class ZF8DMOVING_API UZfRule_DurabilitySpent : public UZfRule_DurabilityBase
{
	GENERATED_BODY()

public:
	virtual float Calculate_Implementation(float CurrentValue) const override;
};