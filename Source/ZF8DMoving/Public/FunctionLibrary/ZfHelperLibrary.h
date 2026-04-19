// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AttributeSet.h"
#include "ZfHelperLibrary.generated.h"


/**
 * 
 */
UCLASS()
class ZF8DMOVING_API UZfHelperLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category="GAS")
	static TArray<FGameplayAttribute> GetAllAttributes(TSubclassOf<UAttributeSet> AttributeSetClass);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Inventory")
	static UZfInventoryComponent* FindInventoryComponent(AActor* Actor);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	static UZfEquipmentComponent* FindEquipmentComponent(AActor* Actor);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Gathering")
	static UZfGatheringComponent* FindGatheringComponent(AActor* Actor);

	// Converte um número inteiro para algarismos romanos.
	// @param Number — número a converter (1 a 3999)
	// @return número em algarismos romanos como FString
	UFUNCTION(BlueprintPure, Category = "Zf|Utility")
	static FString ToRomanNumeral(int32 Number);
};
