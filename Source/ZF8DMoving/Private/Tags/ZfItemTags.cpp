// Fill out your copyright notice in the Description page of Project Settings.

#include "Tags/ZfItemsTags.h"

namespace ZfItemPropertyTags
{
	namespace ItemProperties
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Durability,     "Item.Property.Durability",     "Durabilidade Atual do item")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_MaxDurability,  "Item.Property.MaxDurability",  "Maxima Durabilidade do Item")
	}

	namespace ToolProperties
	{
		UE_DEFINE_GAMEPLAY_TAG(Item_Gathering_ScoreBonus,        "Item.Gathering.ScoreBonus")
		UE_DEFINE_GAMEPLAY_TAG(Item_Gathering_DamageBonus,       "Item.Gathering.DamageBonus")
		UE_DEFINE_GAMEPLAY_TAG(Item_Gathering_GoodSizeBonus,    "Item.Gathering.GoodSizeBonus")
		UE_DEFINE_GAMEPLAY_TAG(Item_Gathering_PerfectSizeBonus, "Item.Gathering.PerfectSizeBonus")
		UE_DEFINE_GAMEPLAY_TAG(Item_Gathering_NeedleSpeedBonus,"Item.Gathering.NeedleSpeedBonus")
	}
}