// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Fragments/ZfItemFragment.h"
#include "ZfStackableFragment.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, meta=(DisplayName="Fragment: Stackable"))
class ZF8DMOVING_API UZfStackableFragment : public UZfItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stack")
	int32 MaxStackSize = 99;

	UPROPERTY(BlueprintReadOnly, Category = "Stack")
	int32 CurrentStackSize = 1;
};
