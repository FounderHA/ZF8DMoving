// Fill out your copyright notice in the Description page of Project Settings.

#include "Tags/ZfCraftTags.h"

namespace ZfCraftTags
{
	// =========================================================================
	// RECIPE ID
	// =========================================================================

	namespace RecipeId
	{
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Recipe_IronSword, "Crafting.Recipe.IronSword")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Recipe_IronPickaxe, "Crafting.Recipe.IronPickaxe")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Recipe_MithrilPickaxe, "Crafting.Recipe.MithrilPickaxe")
	}

	// =========================================================================
	// CATEGORY
	// =========================================================================

	namespace Category
	{
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Weapon, "Crafting.Category.Weapon")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Weapon_Sword, "Crafting.Category.Weapon.Sword")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Weapon_Sword_IronSword, "Crafting.Category.Weapon.Sword.IronSword")

		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Armor, "Crafting.Category.Armor")
		
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Shield, "Crafting.Category.Shield")

		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Jewelry, "Crafting.Category.Jewelry")

		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Consumable, "Crafting.Category.Consumable")

		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool, "Crafting.Category.Tool")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Pickaxe, "Crafting.Category.Tool.Pickaxe")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Pickaxe_BronzePickaxe, "Crafting.Category.Tool.Pickaxe.BronzePickaxe")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Pickaxe_IronPickaxe, "Crafting.Category.Tool.Pickaxe.IronPickaxe")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Pickaxe_SilverPickaxe, "Crafting.Category.Tool.Pickaxe.SilverPickaxe")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Pickaxe_MithrilPickaxe, "Crafting.Category.Tool.Pickaxe.MithrilPickaxe")

		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Axe_BronzeAxe, "Crafting.Category.Tool.Axe.BronzeAxe")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Axe_IronAxe, "Crafting.Category.Tool.Axe.IronAxe")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Axe_SilverAxe, "Crafting.Category.Tool.Axe.SilverAxe")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Axe_MithrilAxe, "Crafting.Category.Tool.Axe.MithrilAxe")

		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Shovel_BronzeShovel, "Crafting.Category.Tool.Shovel.BronzeShovel")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Shovel_IronShovel, "Crafting.Category.Tool.Shovel.IronShovel")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Shovel_SilverShovel, "Crafting.Category.Tool.Shovel.SilverShovel")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Shovel_MithrilShovel, "Crafting.Category.Tool.Shovel.MithrilShovel")

		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Material, "Crafting.Category.Material")
	}

	// =========================================================================
	// FLAG
	// =========================================================================

	namespace Flag
	{
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Flag_Hidden, "Crafting.Flag.Hidden")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Flag_Forbidden, "Crafting.Flag.Forbidden")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Flag_Dangerous, "Crafting.Flag.Dangerous")
	}

	// =========================================================================
	// CRAFTER - SPECIALIZATION
	// =========================================================================

	namespace CrafterSpecialization
	{
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Crafter_Specialization_Blacksmith, "Crafting.Crafter.Specialization.Blacksmith")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Crafter_Specialization_Alchemist, "Crafting.Crafter.Specialization.Alchemist")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Crafter_Specialization_Tailor, "Crafting.Crafter.Specialization.Tailor")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Crafter_Specialization_Carpenter, "Crafting.Crafter.Specialization.Carpenter")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Crafter_Specialization_Jeweler, "Crafting.Crafter.Specialization.Jeweler")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Crafter_Specialization_Cook, "Crafting.Crafter.Specialization.Cook")
	}

	// =========================================================================
	// CRAFTER - RANK
	// =========================================================================

	namespace CrafterRank
	{
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Crafter_Rank_Apprentice, "Crafting.Crafter.Rank.Apprentice")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Crafter_Rank_Journeyman, "Crafting.Crafter.Rank.Journeyman")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Crafter_Rank_Expert, "Crafting.Crafter.Rank.Expert")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Crafter_Rank_Master, "Crafting.Crafter.Rank.Master")
	}

	// =========================================================================
	// GAMEPLAY CUE
	// =========================================================================

	namespace Cue
	{
		UE_DEFINE_GAMEPLAY_TAG(Crafting_GameplayCue_Craft_Success, "Crafting.GameplayCue.Craft.Success")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_GameplayCue_Craft_Failure, "Crafting.GameplayCue.Craft.Failure")
	}
}