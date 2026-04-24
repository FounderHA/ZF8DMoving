// Fill out your copyright notice in the Description page of Project Settings.
// ZfUniqueItemTags.h
// Tags de identificacao para itens com bIsSingleUsePerGame = true.
//
// CONCEITO:
// Cada item unico precisa de uma tag propria aqui.
// Quando o player usa o item, essa tag e salva em
// AZfPlayerState::UsedUniqueItemTags para todos os players online.
// Na proxima tentativa de uso, o sistema checa se a tag ja esta
// presente — se sim, bloqueia o uso.
//
// CONVENCAO DE NOME:
// Item.Unique.<Categoria>.<NomeDoItem>
// Ex:
//   Item.Unique.Scroll.AncientKnowledge
//   Item.Unique.Potion.DivineBless
//   Item.Unique.Quest.SealedLetter
//
// COMO USAR:
// 1. Declare a tag aqui (header + cpp).
// 2. No Fragment_Consumable do item, marque bIsSingleUsePerGame = true.
// 3. Configure UniqueItemTag com a tag declarada aqui.
// 4. O sistema cuida do resto automaticamente.
//
// PREFIXO:
// Todas as tags usam "Item.Unique.*" para ficarem agrupadas
// no editor e serem facilmente filtradas pelo GameplayTagFilter
// "Item.Unique" nos campos do Fragment_Consumable.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace ZfUniqueItemTags
{
	// ----------------------------------------------------------
	// TAG RAIZ
	// Nao use diretamente — serve como prefixo de filtro
	// no editor (GameplayTagFilter = "Item.Unique").
	// ----------------------------------------------------------
	ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Unique)

	// ----------------------------------------------------------
	// SCROLLS UNICOS
	// Pergaminhos de uso unico — normalmente ensinam receitas
	// raras ou concedem habilidades permanentes.
	// ----------------------------------------------------------
	namespace Scrolls
	{
		// Adicione aqui conforme criar scrolls unicos:
		// ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Unique_Scroll_AncientKnowledge)
		// ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Unique_Scroll_ForbiddenRitual)
	}

	// ----------------------------------------------------------
	// RECIPES UNICOS
	// Recipes de uso unico — normalmente ensinam receitas
	// raras ou concedem habilidades permanentes.
	// ----------------------------------------------------------
	namespace Recipes
	{
		// Adicione aqui conforme criar Recipes unicos:
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Unique_Recipe_Tool_Pickaxe_Mithril)
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Unique_Recipe_Weapon_Sword_Iron)
	}
	
	// ----------------------------------------------------------
	// POCOES UNICAS
	// Pocoes de uso unico — concedem buffs permanentes,
	// aumentam atributos base, etc.
	// ----------------------------------------------------------
	namespace Potions
	{
		// Adicione aqui conforme criar pocoes unicas:
		// ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Unique_Potion_DivineBless)
		// ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Unique_Potion_DragonBlood)
	}

	// ----------------------------------------------------------
	// ITENS DE QUEST UNICOS
	// Itens de quest consumiveis que so podem ser usados
	// uma vez por personagem (ex: cartas seladas, artefatos).
	// ----------------------------------------------------------
	namespace Quest
	{
		// Adicione aqui conforme criar itens de quest unicos:
		// ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Unique_Quest_SealedLetter)
		// ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Unique_Quest_AncientSeal)
	}

	// ----------------------------------------------------------
	// PROGRESSAO UNICA
	// Itens que concedem progressao permanente ao personagem
	// (ex: livros que aumentam atributos, cristais de poder).
	// ----------------------------------------------------------
	namespace Progression
	{
		// Adicione aqui conforme criar itens de progressao unicos:
		// ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Unique_Progression_StrengthTome)
		// ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Unique_Progression_PowerCrystal)
	}

	// ----------------------------------------------------------
	// ITEM EVENTS
	// Tags de evento usadas pelo sistema de uso de itens.
	// ZfGA_UseItem e ativada pelo evento Item.Event.Use.
	// Disparado por:
	//   - UZfInventoryComponent::UseItemAtSlot()   (context action no inventario)
	//   - UZfEquipmentComponent::UseQuickSlot()    (tecla de atalho 1/2/3)
	// ----------------------------------------------------------
	namespace ItemEvents
	{
		ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Event_Use)
	}
}