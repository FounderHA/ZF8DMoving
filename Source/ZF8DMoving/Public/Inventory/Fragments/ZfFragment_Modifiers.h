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

#if WITH_EDITOR
	// Validação no editor — garante consistência dos dados configurados.
	// Chamado ao salvar o asset no editor.
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;

#endif
};
