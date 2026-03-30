// Copyright ZfGame Studio. All Rights Reserved.
// ZfInventoryTypes.cpp

#include "Inventory/ZfInventoryTypes.h"

#include "Inventory/ZfEquipmentComponent.h"
#include "Inventory/ZfInventoryComponent.h"


const TMap<EZfItemRarity, FZfModifierRange> ZfModifierRangeByRarity =
{
	{ EZfItemRarity::Common,    FZfModifierRange(1,  5)  },
	{ EZfItemRarity::Uncommon,  FZfModifierRange(5,  10) },
	{ EZfItemRarity::Rare,      FZfModifierRange(10, 20) },
	{ EZfItemRarity::Epic,      FZfModifierRange(20, 35) },
	{ EZfItemRarity::Legendary, FZfModifierRange(35, 50) },
};