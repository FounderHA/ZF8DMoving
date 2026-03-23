// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/Fragments/ZfItemFragment.h"

void UZfItemFragment::OnInstanceCreated(UZfItemInstance* OwningInstance)
{
}

void UZfItemFragment::OnItemAddedToInventory(UZfItemInstance* OwningInstance, UZfInventoryComponent* InventoryComponent)
{
}

void UZfItemFragment::OnItemRemovedFromInventory(UZfItemInstance* OwningInstance,
	UZfInventoryComponent* InventoryComponent)
{
}

void UZfItemFragment::OnItemEquipped(UZfItemInstance* OwningInstance, UZfEquipmentComponent* EquipmentComponent,
	AActor* EquippingActor)
{
}

void UZfItemFragment::OnItemUnequipped(UZfItemInstance* OwningInstance, UZfEquipmentComponent* EquipmentComponent,
	AActor* UnequippingActor)
{
}

void UZfItemFragment::OnItemBroken(UZfItemInstance* OwningInstance)
{
}

void UZfItemFragment::OnItemRepaired(UZfItemInstance* OwningInstance)
{
}

void UZfItemFragment::OnItemDropped(UZfItemInstance* OwningInstance, const FVector& DropLocation)
{
}
