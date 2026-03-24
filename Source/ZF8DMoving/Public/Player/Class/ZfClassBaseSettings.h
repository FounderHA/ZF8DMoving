// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZfClassBaseSettings.generated.h"


UCLASS(BlueprintType)
class ZF8DMOVING_API UZfPrimaryDataAssetClass : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:

	// Main Attributes
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

	// Resources
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Resources")
	float BaseHealth;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Resources")
	float BaseMana;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Resources")
	float BaseStamina;

	// Combat Attributes - Resources

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Regens")
	float HealthRegen;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Regens")
	float ManaRegen;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Regens")
	float StaminaRegen;
	
	// Combat Attributes - Resistances
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resistences")
	float PhysicalResistance;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resistences")
	float MagicResistance;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resistences")
	float StunResistance;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resistences")
	float SlowResistance;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resistences")
	float CriticalResistance;

	// Attribute Modifiers
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Modifiers")
	float HealthModifier;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Modifiers")
	float StaminaModifier;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Modifiers")
	float ManaModifier;
	
};
