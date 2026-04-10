// Copyright ZfGame Studio. All Rights Reserved.

#include "Inventory/Modifiers/ZfModifierRule.h"

void UZfModifierRule::BindToSource_Implementation(const FZfModifierRuleContext& InContext)
{
	// Implementação base apenas armazena o contexto.
	// Subclasses fazem override para conectar ao delegate da fonte.
	CachedContext = InContext;
}

void UZfModifierRule::UnbindFromSource_Implementation()
{
	// Implementação base limpa o contexto.
	// Subclasses fazem override para desconectar delegates específicos.
	CachedContext = FZfModifierRuleContext{};
}

float UZfModifierRule::Calculate_Implementation(float CurrentValue) const
{
	// Implementação base: modifier estático — FinalValue == CurrentValue.
	// Subclasses fazem override para aplicar a fórmula dinâmica.
	return CurrentValue;
}

void UZfModifierRule::NotifyValueChanged()
{
	OnRuleValueChanged.Broadcast();
}