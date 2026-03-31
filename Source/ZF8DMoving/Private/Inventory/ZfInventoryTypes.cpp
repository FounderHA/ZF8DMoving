// Copyright ZfGame Studio. All Rights Reserved.
// ZfInventoryTypes.cpp

#include "Inventory/ZfInventoryTypes.h"

#include "Inventory/ZfEquipmentComponent.h"
#include "Inventory/ZfInventoryComponent.h"

const TMap<EZfItemRarity, FZfModifierRange> ZfModifierRangeByRarity =
{
	{ EZfItemRarity::Common,    FZfModifierRange(0, 0) },
	{ EZfItemRarity::Uncommon,  FZfModifierRange(1, 2) },
	{ EZfItemRarity::Rare,      FZfModifierRange(1, 3) },
	{ EZfItemRarity::Epic,      FZfModifierRange(1, 4) },
	{ EZfItemRarity::Legendary, FZfModifierRange(1, 5) },
};

const TArray<FZfRarityWeight> GDefaultRarityWeights =
{
	{ EZfItemRarity::Common,    50.f },
	{ EZfItemRarity::Uncommon,  25.f },
	{ EZfItemRarity::Rare,      15.f },
	{ EZfItemRarity::Epic,       8.f },
	{ EZfItemRarity::Legendary,  2.f },
};

const TArray<FZfTierWeight> GDefaultTierWeights =
{
	{ 1, 30.f },
	{ 2, 15.f },
	{ 3, 10.f },
	{ 4,  4.f },
	{ 5,  1.f },
};