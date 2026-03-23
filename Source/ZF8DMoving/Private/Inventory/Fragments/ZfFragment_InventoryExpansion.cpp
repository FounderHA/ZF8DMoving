// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/Fragments/ZfFragment_InventoryExpansion.h"

void UZfFragment_InventoryExpansion::OnItemEquipped(UZfItemInstance* OwningInstance,
	UZfEquipmentComponent* EquipmentComponent, AActor* EquippingActor)
{
	Super::OnItemEquipped(OwningInstance, EquipmentComponent, EquippingActor);
}

void UZfFragment_InventoryExpansion::OnItemUnequipped(UZfItemInstance* OwningInstance,
	UZfEquipmentComponent* EquipmentComponent, AActor* UnequippingActor)
{
	Super::OnItemUnequipped(OwningInstance, EquipmentComponent, UnequippingActor);
}
