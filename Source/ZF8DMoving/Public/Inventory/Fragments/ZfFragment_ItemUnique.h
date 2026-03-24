// Copyright ZfGame Studio. All Rights Reserved.
// ZfFragment_Quality.h
// Fragment que define se o item é um item único

#pragma once

#include "CoreMinimal.h"
#include "ZfItemFragment.h"
#include "ZfFragment_ItemUnique.generated.h"

/**
 * 
 */
UCLASS(DisplayName = "Fragment: ItemUnique")
class ZF8DMOVING_API UZfFragment_ItemUnique : public UZfItemFragment
{
	GENERATED_BODY()
	
public:
	
	// ----------------------------------------------------------
	// UNIQUE ITEM
	// Configuração exclusiva para itens de raridade Unique.
	// não podem ser alterados por nenhuma mecânica do jogo
	// (Reroll, Extraction, Corruption, etc.)
	// Modifiers fixos do item Único — definidos aqui no PDA.
	// Não são rolados em runtime — são aplicados diretamente ao criar a instância.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|Unique")
	TArray<FDataTableRowHandle> UniqueModifiers;
};
