// Fill out your copyright notice in the Description page of Project Settings.

#include "Tags/ZfUniqueItemTags.h"

namespace ZfUniqueItemTags
{
	UE_DEFINE_GAMEPLAY_TAG(Item_Unique, "Item.Unique")

	// ----------------------------------------------------------
	// SCROLLS UNICOS
	// ----------------------------------------------------------
	namespace Scrolls
	{
		// Descomente e adicione conforme criar scrolls unicos:
		// UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Unique_Scroll_AncientKnowledge,
		//     "Item.Unique.Scroll.AncientKnowledge",
		//     "Pergaminho do Conhecimento Antigo — ensina receita lendaria.")
	}

	// ----------------------------------------------------------
	// RECIPES UNICOS
	// ----------------------------------------------------------
	namespace Scrolls
	{
		// Descomente e adicione conforme criarRecipes unicos:
		UE_DEFINE_GAMEPLAY_TAG(Item_Unique_Recipe_Tool_Pickaxe_Mithril, "Item.Unique.Recipe.Tool.Pickaxe.Mithril")
		UE_DEFINE_GAMEPLAY_TAG(Item_Unique_Recipe_Weapon_Sword_Iron, "Item.Unique.Recipe.Weapon.Sword.Iron")
	}

	// ----------------------------------------------------------
	// POCOES UNICAS
	// ----------------------------------------------------------
	namespace Potions
	{
		// Descomente e adicione conforme criar pocoes unicas:
		// UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Unique_Potion_DivineBless,
		//     "Item.Unique.Potion.DivineBless",
		//     "Pocao da Bencao Divina — concede buff permanente de Conviction.")
	}

	// ----------------------------------------------------------
	// ITENS DE QUEST UNICOS
	// ----------------------------------------------------------
	namespace Quest
	{
		// Descomente e adicione conforme criar itens de quest unicos:
		// UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Unique_Quest_SealedLetter,
		//     "Item.Unique.Quest.SealedLetter",
		//     "Carta Selada — entregue ao NPC do porto para iniciar a quest.")
	}

	// ----------------------------------------------------------
	// PROGRESSAO UNICA
	// ----------------------------------------------------------
	namespace Progression
	{
		// Descomente e adicione conforme criar itens de progressao unicos:
		// UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Unique_Progression_StrengthTome,
		//     "Item.Unique.Progression.StrengthTome",
		//     "Tomo da Forca — aumenta Strength base em 2 permanentemente.")
	}

	// ----------------------------------------------------------
	// ITEM EVENTS
	// ----------------------------------------------------------
	namespace ItemEvents
	{
		UE_DEFINE_GAMEPLAY_TAG(Item_Event_Use, "Item.Event.Use")
	}
}