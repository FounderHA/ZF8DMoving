// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AttributeSet.h"
#include "ZfAttributeHelperLibrary.generated.h"

/**
 * 
 */
UCLASS()
class ZF8DMOVING_API UZfAttributeHelperLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category="GAS")
	static TArray<FGameplayAttribute> GetAllAttributes(TSubclassOf<UAttributeSet> AttributeSetClass);
	
};
