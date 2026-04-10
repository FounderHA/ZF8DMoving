// Copyright ZfGame Studio. All Rights Reserved.
// Base abstrata para Rules cuja fonte é a durabilidade do ItemInstance.
// Gerencia o bind/unbind do delegate OnDurabilityChanged.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Modifiers/ZfModifierRule.h"
#include "ZfRule_DurabilityBase.generated.h"

UCLASS(Abstract)
class ZF8DMOVING_API UZfRule_DurabilityBase : public UZfModifierRule
{
	GENERATED_BODY()

public:
	virtual void BindToSource_Implementation(const FZfModifierRuleContext& InContext) override;
	virtual void UnbindFromSource_Implementation() override;

private:
	// Callback disparado pelo ItemInstance quando a durabilidade muda.
	// @param NewDurability — novo valor de durabilidade
	UFUNCTION()
	void OnDurabilityChanged(float NewDurability);

	FDelegateHandle DurabilityDelegateHandle;
};