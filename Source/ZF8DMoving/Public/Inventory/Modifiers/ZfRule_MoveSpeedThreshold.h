// Copyright ZfGame Studio. All Rights Reserved.
// Fórmula: CurrentValue * floor(MoveSpeed / ThresholdStep)
// A cada ThresholdStep de MoveSpeed, acumula CurrentValue de bônus.
// Exemplo: Roll=5 | MoveSpeed=350 | Step=100 → FinalValue = 5 * 3 = 15
// Use: modifier de dano físico que escala em degraus com a velocidade.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Modifiers/ZfRule_AttributeBase.h"
#include "ZfRule_MoveSpeedThreshold.generated.h"

UCLASS(DisplayName = "Rule: MoveSpeed Threshold")
class ZF8DMOVING_API UZfRule_MoveSpeedThreshold : public UZfRule_AttributeBase
{
	GENERATED_BODY()

protected:
	virtual TArray<FGameplayAttribute> GetSourceAttributes() const override;
	virtual float Calculate_Implementation(float CurrentValue) const override;

	// Degrau de MoveSpeed necessário para acumular um bônus de CurrentValue.
	// Hardcoded por design — subclasse define o valor adequado ao balanceamento.
	virtual float GetThresholdStep() const { return 100.f; }
};