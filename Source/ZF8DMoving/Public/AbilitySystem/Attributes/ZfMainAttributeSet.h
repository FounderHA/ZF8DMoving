// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ZfAttributeSet.h"
#include "ZfMainAttributeSet.generated.h"

/**
 * 
 */
UCLASS()
class ZF8DMOVING_API UZfMainAttributeSet : public UZfAttributeSet
{
	GENERATED_BODY()
	
public:
	UZfMainAttributeSet(const FObjectInitializer& ObjectInitializer);
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override; 
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Strength, Category = "CharacterAttributes")
	FGameplayAttributeData Strength;
	ATTRIBUTE_ACCESSORS(UZfMainAttributeSet, Strength)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Dexterity, Category = "CharacterAttributes")
	FGameplayAttributeData Dexterity;
	ATTRIBUTE_ACCESSORS(UZfMainAttributeSet, Dexterity)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Intelligence, Category = "CharacterAttributes")
	FGameplayAttributeData Intelligence;
	ATTRIBUTE_ACCESSORS(UZfMainAttributeSet, Intelligence)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Constitution, Category = "CharacterAttributes")
	FGameplayAttributeData Constitution;
	ATTRIBUTE_ACCESSORS(UZfMainAttributeSet, Constitution)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Conviction, Category = "CharacterAttributes")
	FGameplayAttributeData Conviction;
	ATTRIBUTE_ACCESSORS(UZfMainAttributeSet, Conviction)
	
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