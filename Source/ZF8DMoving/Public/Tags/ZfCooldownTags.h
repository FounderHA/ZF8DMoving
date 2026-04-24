// Fill out your copyright notice in the Description page of Project Settings.
// ZfCooldownTags.h
// Tags de cooldown usadas pelo sistema de consumo de itens.
//
// ORGANIZACAO:
// - Cooldown.Item.Global  → cooldown compartilhado entre TODOS os consumiveis.
//                           Quando qualquer item e usado, essa tag e aplicada no ASC
//                           do player por alguns segundos. Impede usar outro item
//                           enquanto estiver ativa.
//
// - Cooldown.Item.PerItem → cooldown especifico por item. Cada item define o seu
//                           proprio tempo via ConsumptionCooldownSeconds no Fragment.
//                           A tag e aplicada no ASC do player identificando aquele
//                           tipo de item especifico.
//
// COMO O GAS USA ESSAS TAGS:
// A ZfGA_UseItem define um CooldownGameplayEffect que aplica essas tags no ASC.
// Antes de ativar o consumo, a GA checa se as tags estao presentes — se sim, bloqueia.
//
// PREFIXO:
// Todas usam "Cooldown.*" pra ficarem agrupadas no editor de tags
// e serem facilmente filtradas como CooldownTags no GAS.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace ZfCooldownTags
{
	// ----------------------------------------------------------
	// PER ITEM
	// Aplicado apos consumir um item especifico. Impede reusar
	// o mesmo tipo de item enquanto ativo.
	// Cada ItemDefinition referencia essa tag base — o GE usa
	// GrantedTags com a tag base pra identificar o bloqueio.
	// ----------------------------------------------------------
	ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Item_Consumable)
}