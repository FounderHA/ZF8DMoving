// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ZfAttributeSet.h"
#include "ZfStrengthSet.generated.h"

/**
 * 
 */
UCLASS()
class ZF8DMOVING_API UZfStrengthSet : public UZfAttributeSet
{
	GENERATED_BODY()
	
public:
	UZfStrengthSet(const FObjectInitializer& ObjectInitializer);
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Strength, Category = "CharacterAttributes")
	FGameplayAttributeData Strength;
	ATTRIBUTE_ACCESSORS(UZfStrengthSet, Strength)
	
	
	
	
};
