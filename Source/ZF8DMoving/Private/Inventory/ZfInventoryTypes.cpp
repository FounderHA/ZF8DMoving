// Copyright ZfGame Studio. All Rights Reserved.
// ZfInventoryTypes.cpp

#include "Inventory/ZfInventoryTypes.h"

#include "Inventory/ZfEquipmentComponent.h"
#include "Inventory/ZfInventoryComponent.h"


void FZfEquipmentSlotEntry::PreReplicatedRemove(
	const FZfEquipmentList& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent && ItemInstance)
	{
		InArraySerializer.OwnerComponent->OnItemUnequipped.Broadcast(
			ItemInstance, SlotType);
	}
}

void FZfEquipmentSlotEntry::PostReplicatedAdd(
	const FZfEquipmentList& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent && ItemInstance)
	{
		InArraySerializer.OwnerComponent->OnItemEquipped.Broadcast(
			ItemInstance, SlotType);
	}
}

void FZfEquipmentSlotEntry::PostReplicatedChange(
	const FZfEquipmentList& InArraySerializer)
{
	// Mudança genérica — equipamento atualizado
}

// Definição central da categoria de log do sistema de inventário.
// Declarada em ZfInventoryTypes.h com DECLARE_LOG_CATEGORY_EXTERN.
// Uso em qualquer arquivo: UE_LOG(LogZfInventory, Log, TEXT("..."));
DEFINE_LOG_CATEGORY(LogZfInventory);