#include "Inventory/Modifiers/ZfRule_DurabilityBase.h"
#include "Inventory/ZfItemInstance.h"

void UZfRule_DurabilityBase::BindToSource_Implementation(const FZfModifierRuleContext& InContext)
{
	Super::BindToSource_Implementation(InContext);

	if (!CachedContext.ItemInstance) return;

	DurabilityDelegateHandle = CachedContext.ItemInstance->OnDurabilityChanged
		.AddUObject(this, &UZfRule_DurabilityBase::OnDurabilityChanged);
}

void UZfRule_DurabilityBase::UnbindFromSource_Implementation()
{
	if (CachedContext.ItemInstance && DurabilityDelegateHandle.IsValid())
	{
		CachedContext.ItemInstance->OnDurabilityChanged.Remove(DurabilityDelegateHandle);
		DurabilityDelegateHandle.Reset();
	}

	Super::UnbindFromSource_Implementation();
}

void UZfRule_DurabilityBase::OnDurabilityChanged(float NewDurability)
{
	NotifyValueChanged();
}