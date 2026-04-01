// Copyright ZfGame Studio. All Rights Reserved.
// ZfFragment_Quality.h
// Fragment que define se o item tem raridade

#pragma once

#include "CoreMinimal.h"
#include "ZfItemFragment.h"
#include "ZfFragment_ItemRarity.generated.h"

/**
 * 
 */
UCLASS(DisplayName = "Fragment: ItemRarity")
class ZF8DMOVING_API UZfFragment_ItemRarity : public UZfItemFragment
{
	GENERATED_BODY()
	
public:
	
	// Array para adicionar todas Raridades que o item pode ter
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Rarity")
	TArray<FZfRarityWeight> RarityProbability;
	
};