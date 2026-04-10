#pragma once

#include "CoreMinimal.h"
#include "Inventory/Modifiers/ZfModifierRule.h"
#include "GameplayEffectTypes.h"
#include "ZfRule_AttributeBase.generated.h"

UCLASS(Abstract)
class ZF8DMOVING_API UZfRule_AttributeBase : public UZfModifierRule
{
	GENERATED_BODY()

public:
	virtual void BindToSource_Implementation(const FZfModifierRuleContext& InContext) override;
	virtual void UnbindFromSource_Implementation() override;

protected:
	// Retorna todos os atributos que esta Rule observa.
	// Cada mudança em qualquer um deles dispara NotifyValueChanged().
	// Subclasses com um único atributo retornam array com um elemento.
	// Subclasses com múltiplos (ex: CurrentHealth + MaxHealth) retornam todos.
	virtual TArray<FGameplayAttribute> GetSourceAttributes() const
		PURE_VIRTUAL(UZfRule_AttributeBase::GetSourceAttributes,
			return TArray<FGameplayAttribute>(););

private:
	void OnSourceAttributeChanged(const FOnAttributeChangeData& Data);

	// Um handle por atributo observado
	TArray<FDelegateHandle> AttributeDelegateHandles;
};