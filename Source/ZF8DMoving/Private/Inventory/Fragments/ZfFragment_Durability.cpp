// Copyright ZfGame Studio. All Rights Reserved.
// ZfFragment_Durability.cpp

#include "Inventory/Fragments/ZfFragment_Durability.h"
#include "Inventory/ZfItemInstance.h"
#include "Inventory/ZfInventoryTypes.h"

void UZfFragment_Durability::OnItemBroken(UZfItemInstance* OwningInstance)
{
	if (!OwningInstance)
	{
		UE_LOG(LogZfInventory, Warning, TEXT("UZfFragment_Durability::OnItemBroken — OwningInstance é nulo."));
		return;
	}

	UE_LOG(LogZfInventory, Log, TEXT("UZfFragment_Durability::OnItemBroken — Item '%s' quebrou." "Bônus desativados pelo EquipmentComponent."),
		*OwningInstance->GetItemGuid().ToString());
}

void UZfFragment_Durability::OnItemRepaired(UZfItemInstance* OwningInstance)
{
	if (!OwningInstance)
	{
		UE_LOG(LogZfInventory, Warning,
			TEXT("UZfFragment_Durability::OnItemRepaired — OwningInstance é nulo."));
		return;
	}

	UE_LOG(LogZfInventory, Log,
		TEXT("UZfFragment_Durability::OnItemRepaired — Item '%s' reparado." "Bônus reativados pelo EquipmentComponent."),
		*OwningInstance->GetItemGuid().ToString());
}