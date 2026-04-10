#include "Inventory/Modifiers/ZfRule_AttributeBase.h"
#include "AbilitySystemComponent.h"

void UZfRule_AttributeBase::BindToSource_Implementation(const FZfModifierRuleContext& InContext)
{
	Super::BindToSource_Implementation(InContext);

	if (!CachedContext.ASC) return;

	const TArray<FGameplayAttribute> Attributes = GetSourceAttributes();

	for (const FGameplayAttribute& Attr : Attributes)
	{
		if (!Attr.IsValid()) continue;

		FDelegateHandle Handle = CachedContext.ASC
			->GetGameplayAttributeValueChangeDelegate(Attr)
			.AddUObject(this, &UZfRule_AttributeBase::OnSourceAttributeChanged);

		AttributeDelegateHandles.Add(Handle);
	}
}

void UZfRule_AttributeBase::UnbindFromSource_Implementation()
{
	if (CachedContext.ASC)
	{
		const TArray<FGameplayAttribute> Attributes = GetSourceAttributes();

		for (int32 i = 0; i < Attributes.Num(); i++)
		{
			if (Attributes.IsValidIndex(i)  &&
				Attributes[i].IsValid()     &&
				AttributeDelegateHandles.IsValidIndex(i) &&
				AttributeDelegateHandles[i].IsValid())
			{
				CachedContext.ASC
					->GetGameplayAttributeValueChangeDelegate(Attributes[i])
					.Remove(AttributeDelegateHandles[i]);
			}
		}
	}

	AttributeDelegateHandles.Empty();
	Super::UnbindFromSource_Implementation();
}

void UZfRule_AttributeBase::OnSourceAttributeChanged(const FOnAttributeChangeData& Data)
{
	NotifyValueChanged();
}