// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Attributes/ZfAttributeSet.h"
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
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override; 
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Strength, Category = "CharacterAttributes")
	FGameplayAttributeData Strength;
	ATTRIBUTE_ACCESSORS(UZfStrengthSet, Strength)
	
protected:
	
	UFUNCTION()
	virtual void OnRep_Strength(const FGameplayAttributeData& OldValue) const;
};