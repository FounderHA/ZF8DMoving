// Copyright ZfGame Studio. All Rights Reserved.
// ZfFragment_Stackable.h
// Fragment que define o comportamento de stack do item.
// Itens com este fragment compartilham 1 ItemInstance com
// a quantidade como variável replicada.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Fragments/ZfItemFragment.h"
#include "ZfFragment_Stackable.generated.h"

UCLASS(DisplayName = "Fragment: Stackable")
class ZF8DMOVING_API UZfFragment_Stackable : public UZfItemFragment
{
	GENERATED_BODY()

public:

	// Quantidade máxima de itens que podem existir em um único slot.
	// Ex: Poção de vida = 20, Minério de ferro = 64, Moeda = 9999
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|Stackable", meta = (ClampMin = "1"))
	int32 MaxStackSize = 1;

	// Se verdadeiro, ao adicionar ao inventário, tenta empilhar
	// com stacks existentes do mesmo item antes de ocupar novo slot.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|Stackable")
	bool bAutoStackOnPickup = true;

// ----------------------------------------------------------
// DEBUG
// ----------------------------------------------------------	
	
	virtual FString GetDebugString() const override
	{
		return FString::Printf(
			TEXT("[Fragment_Stackable] MaxStack: %d | AutoStack: %s"),
			MaxStackSize,
			bAutoStackOnPickup ? TEXT("Yes") : TEXT("No"));
	}
};