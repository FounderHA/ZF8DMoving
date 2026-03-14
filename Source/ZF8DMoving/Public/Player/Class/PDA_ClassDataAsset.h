// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PDA_ClassDataAsset.generated.h"


USTRUCT(BlueprintType)
struct FMainAttributes
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MainAttributes")
	float Strength;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MainAttributes")
	float Dexterity;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MainAttributes")
	float Intelligence;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MainAttributes")
	float Constitution;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MainAttributes")
	float Conviction;
};


USTRUCT(BlueprintType)
struct FResistences
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float PhysicalResistance;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MagicResistance;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float StunResistance;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float SlowResistance;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float CriticalResistance;
};

/**
 * 
 */
UCLASS(BlueprintType)
class ZF8DMOVING_API UPDA_ClassDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attributes")
	FMainAttributes MainAttributes;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Resistences")
	FResistences Resistences;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bases")
	float BaseHealth;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bases")
	float BaseMana;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bases")
	float BaseStamina;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Modifier")
	float HealthModifier;
	
};
