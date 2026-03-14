// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZfPrimaryDataAssetClass.generated.h"


UCLASS(BlueprintType)
class ZF8DMOVING_API UZfPrimaryDataAssetClass : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	
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
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MainResistences")
	float PhysicalResistance;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MainResistences")
	float MagicResistance;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MainResistences")
	float StunResistance;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MainResistences")
	float SlowResistance;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MainResistences")
	float CriticalResistance;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BasesValues")
	float BaseHealth;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BasesValues")
	float BaseMana;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BasesValues")
	float BaseStamina;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Modifier")
	float HealthModifier;
	
};
