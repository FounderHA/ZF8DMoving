// Fill out your copyright notice in the Description page of Project Settings.

#include "Tags/ZfCraftTags.h"

namespace ZfCraftTags
{
	// =========================================================================
	// RECIPE ID
	// =========================================================================

	namespace RecipeId
	{
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Recipe_Sword_Iron, "Crafting.Recipe.Sword.Iron")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Recipe_Sword_Mithril, "Crafting.Recipe.Sword.Mithril")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Recipe_Pickaxe_Iron, "Crafting.Recipe.Pickaxe.Iron")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Recipe_Pickaxe_Mithril, "Crafting.Recipe.Pickaxe.Mithril")
	}

	// =========================================================================
	// CATEGORY
	// =========================================================================

	namespace Category
	{
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Weapon, "Crafting.Category.Weapon")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Weapon_Sword, "Crafting.Category.Weapon.Sword")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Weapon_Sword_Iron, "Crafting.Category.Weapon.Sword.Iron")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Weapon_Sword_Mithril, "Crafting.Category.Weapon.Sword.Mithril")

		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Armor, "Crafting.Category.Armor")
		
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Shield, "Crafting.Category.Shield")

		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Jewelry, "Crafting.Category.Jewelry")

		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Consumable, "Crafting.Category.Consumable")

		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool, "Crafting.Category.Tool")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Pickaxe, "Crafting.Category.Tool.Pickaxe")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Pickaxe_Bronze, "Crafting.Category.Tool.Pickaxe.Bronze")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Pickaxe_Iron, "Crafting.Category.Tool.Pickaxe.Iron")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Pickaxe_Silver, "Crafting.Category.Tool.Pickaxe.Silver")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Pickaxe_Mithril, "Crafting.Category.Tool.Pickaxe.Mithril")

		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Axe_Bronze, "Crafting.Category.Tool.Axe.Bronze")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Axe_Iron, "Crafting.Category.Tool.Axe.Iron")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Axe_Silver, "Crafting.Category.Tool.Axe.Silver")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Axe_Mithril, "Crafting.Category.Tool.Axe.Mithril")

		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Shovel_Bronze, "Crafting.Category.Tool.Shovel.Bronze")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Shovel_Iron, "Crafting.Category.Tool.Shovel.Iron")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Shovel_Silver, "Crafting.Category.Tool.Shovel.Silver")
		UE_DEFINE_GAMEPLAY_TAG(Crafting_Category_Tool_Shovel_Mithril, "Crafting.Category.Tool.Shovel.Mithril")

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