// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/*
	Tags nativas do sistema de Craft.

	TODAS as tags usam prefixo "Crafting.*" para ficarem agrupadas
	no editor de Gameplay Tags.

	SUBDIVISAO:
	- Crafting.Recipe.*            → ID UNICO de cada receita (RecipeTag)
	- Crafting.Category.*          → classificacao hierarquica (CategoryTags)
	- Crafting.Flag.*              → metadata (Hidden, Forbidden, Dangerous)
	- Crafting.Crafter.*           → identidade do NPC (specialization, rank)
	- Crafting.GameplayCue.Craft.* → cues de feedback VFX/SFX

	// C++
	FGameplayTag Tag = ZfCraftTags::RecipeId::Crafting_Recipe_IronSword;

	// Blueprint
	FGameplayTag::RequestGameplayTag("Crafting.Recipe.IronSword")
*/

namespace ZfCraftTags
{
	// ----------------------------------------------------------
	// RECIPE ID
	// Identificador UNICO de cada receita. Hierarquia plana.
	// Vai no campo RecipeTag do asset da receita.
	//
	// Serve para:
	// - Marcar em PlayerState->KnownRecipeTags ao aprender
	// - Lookup no CraftingSubsystem (GetRecipeByTag)
	// - Scroll de receita apontar para uma receita especifica
	// ----------------------------------------------------------
	namespace RecipeId
	{
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Recipe_IronSword)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Recipe_IronPickaxe)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Recipe_MithrilPickaxe)
	}

	// ----------------------------------------------------------
	// CATEGORY
	// Categorias hierarquicas. Vai no campo CategoryTags.
	// Multiplas receitas podem compartilhar a mesma categoria.
	// Match hierarquico — uma tag especifica tambem passa em queries
	// pelos niveis acima.
	//
	// Serve para:
	// - Filtro AcceptedRecipesQuery nos NPCs
	// - Agrupamento em abas na UI ("Armas", "Ferramentas")
	// - GetRecipesByCategory no subsystem
	// ----------------------------------------------------------
	namespace Category
	{
		// ── Weapon ────────────────────────────────────────────────
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Category_Weapon)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Category_Weapon_Sword)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Category_Weapon_Sword_IronSword)

		// ── Armor ─────────────────────────────────────────────────
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Category_Armor)

		// ── Shield ────────────────────────────────────────────────
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Category_Shield)

		// ── Jewelry ───────────────────────────────────────────────
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Category_Jewelry)

		// ── Consumable ────────────────────────────────────────────
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Category_Consumable)

		// ── Tool ──────────────────────────────────────────────────
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Category_Tool)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Category_Tool_Pickaxe)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Category_Tool_Pickaxe_BronzePickaxe)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Category_Tool_Pickaxe_IronPickaxe)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Category_Tool_Pickaxe_SilverPickaxe)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Category_Tool_Pickaxe_MithrilPickaxe)

		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Category_Tool_Axe_BronzeAxe)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Category_Tool_Axe_IronAxe)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Category_Tool_Axe_SilverAxe)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Category_Tool_Axe_MithrilAxe)

		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Category_Tool_Shovel_BronzeShovel)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Category_Tool_Shovel_IronShovel)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Category_Tool_Shovel_SilverShovel)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Category_Tool_Shovel_MithrilShovel)

		// ── Material ──────────────────────────────────────────────
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Category_Material)
	}

	// ----------------------------------------------------------
	// FLAG
	// Flags de metadata. Usadas pela UI e pela logica de execucao.
	// ----------------------------------------------------------
	namespace Flag
	{
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Flag_Hidden)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Flag_Forbidden)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Flag_Dangerous)
	}

	// ----------------------------------------------------------
	// CRAFTER - SPECIALIZATION
	// ----------------------------------------------------------
	namespace CrafterSpecialization
	{
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Crafter_Specialization_Blacksmith)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Crafter_Specialization_Alchemist)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Crafter_Specialization_Tailor)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Crafter_Specialization_Carpenter)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Crafter_Specialization_Jeweler)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Crafter_Specialization_Cook)
	}

	// ----------------------------------------------------------
	// CRAFTER - RANK
	// ----------------------------------------------------------
	namespace CrafterRank
	{
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Crafter_Rank_Apprentice)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Crafter_Rank_Journeyman)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Crafter_Rank_Expert)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_Crafter_Rank_Master)
	}

	// ----------------------------------------------------------
	// GAMEPLAY CUE
	// ----------------------------------------------------------
	namespace Cue
	{
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_GameplayCue_Craft_Success)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crafting_GameplayCue_Craft_Failure)
	}
}