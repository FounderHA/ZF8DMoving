// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Fragments/ZfItemFragment.h"
#include "ZfBackpackFragment.generated.h"

UCLASS(BlueprintType, meta=(DisplayName="Fragment: Backpack"))
class ZF8DMOVING_API UZfBackpackFragment : public UZfItemFragment
{
	GENERATED_BODY()

public:
	
	// Quantos slots extras essa mochila adiciona
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Backpack")
	int32 ExtraSlots = 10;
};