// Copyright ZfGame Studio. All Rights Reserved.
// ZfFragment_Quality.h
// Fragment que define se o item tem raridade

#pragma once

#include "CoreMinimal.h"
#include "ZfItemFragment.h"
#include "Inventory/ZfModifierDataTypes.h"
#include "ZfFragment_Modifiers.generated.h"

/**
 * 
 */
UCLASS(DisplayName = "Fragment: Modifiers")
class ZF8DMOVING_API UZfFragment_Modifiers : public UZfItemFragment
{
	GENERATED_BODY()

public:
	
	// ----------------------------------------------------------
	// MODIFIERS
	// Configuração de modifiers para este item específico.
	// Define quantos modifiers, quais classes e qual DataTable usar.
	// Deixe vazio para itens sem modifiers (consumíveis, quest items, etc.)
	// ----------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Modifiers")
	FZfItemModifierConfig ModifierConfig;

	// Retorna o DataTable de modifiers carregado e pronto para uso.
	// Carrega sincronamente se ainda não estiver em memória.
	// @return DataTable carregado ou nullptr se não configurado
	UFUNCTION(BlueprintCallable, Category = "Zf|Fragment|Modifiers")
	UDataTable* GetLoadedModifierDataTable() const;
	
};
