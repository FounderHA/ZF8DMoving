// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Attributes/ZfAttributeSet.h"
#include "ZfMainAttributesSet.generated.h"

/**
 * 
 */
UCLASS()
class ZF8DMOVING_API UZfMainAttributesSet : public UZfAttributeSet
{
	GENERATED_BODY()
	
public:
	UZfMainAttributesSet(const FObjectInitializer& ObjectInitializer);
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override; 
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Strength, Category = "CharacterAttributes")
	FGameplayAttributeData Strength;
	ATTRIBUTE_ACCESSORS(UZfMainAttributesSet, Strength)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Dexterity, Category = "CharacterAttributes")
	FGameplayAttributeData Dexterity;
	ATTRIBUTE_ACCESSORS(UZfMainAttributesSet, Dexterity)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Intelligence, Category = "CharacterAttributes")
	FGameplayAttributeData Intelligence;
	ATTRIBUTE_ACCESSORS(UZfMainAttributesSet, Intelligence)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Constitution, Category = "CharacterAttributes")
	FGameplayAttributeData Constitution;
	ATTRIBUTE_ACCESSORS(UZfMainAttributesSet, Constitution)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Conviction, Category = "CharacterAttributes")
	FGameplayAttributeData Conviction;
	ATTRIBUTE_ACCESSORS(UZfMainAttributesSet, Conviction)
	
protected:
	
	UFUNCTION()
	virtual void OnRep_Strength(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	virtual void OnRep_Dexterity(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	virtual void OnRep_Intelligence(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	virtual void OnRep_Constitution(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	virtual void OnRep_Conviction(const FGameplayAttributeData& OldValue) const;
};