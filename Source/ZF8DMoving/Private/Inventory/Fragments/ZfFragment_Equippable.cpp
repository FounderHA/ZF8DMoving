// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/Fragments/ZfFragment_Equippable.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Inventory/ZfItemInstance.h"

void UZfFragment_Equippable::OnItemEquipped(UZfItemInstance* OwningInstance, UZfEquipmentComponent* EquipmentComponent, AActor* EquippingActor)
{
	Super::OnItemEquipped(OwningInstance, EquipmentComponent, EquippingActor);

	if (!EquippingActor) return;

	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(EquippingActor);
	if (!ASCInterface) return;

	UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent();
	if (!ASC) return;

	for (const FZfAppliedModifier& Modifier : OwningInstance->GetAppliedModifiers())
	{
		TSubclassOf<UGameplayEffect> GEClass = Modifier.GameplayEffect.LoadSynchronous();

		if (!GEClass) continue;

		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		ASC->ApplyGameplayEffectToSelf(GEClass->GetDefaultObject<UGameplayEffect>(), 1.f, Context);
	}
}

void UZfFragment_Equippable::OnItemUnequipped(UZfItemInstance* OwningInstance, UZfEquipmentComponent* EquipmentComponent, AActor* UnequippingActor)
{
	Super::OnItemUnequipped(OwningInstance, EquipmentComponent, UnequippingActor);

	// Com Instant + Override o MMC recalcula sem o item
	// Só reaplicar o GE já resolve
	OnItemEquipped(OwningInstance, EquipmentComponent, UnequippingActor);
}
