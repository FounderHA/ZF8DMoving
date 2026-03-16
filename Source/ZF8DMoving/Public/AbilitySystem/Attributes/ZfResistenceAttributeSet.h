// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Attributes/ZfAttributeSet.h"
#include "ZfResistenceAttributeSet.generated.h"

/**
 * 
 */
UCLASS()
class ZF8DMOVING_API UZfResistenceAttributeSet : public UZfAttributeSet
{
	GENERATED_BODY()
	
public:
	UZfResistenceAttributeSet(const FObjectInitializer& ObjectInitializer);
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override; 
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PhysicalResistance, Category = "CharacterAttributes")
	FGameplayAttributeData PhysicalResistance;
	ATTRIBUTE_ACCESSORS(UZfResistenceAttributeSet, PhysicalResistance)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_MagicResistance, Category = "CharacterAttributes")
	FGameplayAttributeData MagicResistance;
	ATTRIBUTE_ACCESSORS(UZfResistenceAttributeSet, MagicResistance)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_StunResistance, Category = "CharacterAttributes")
	FGameplayAttributeData StunResistance;
	ATTRIBUTE_ACCESSORS(UZfResistenceAttributeSet, StunResistance)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_SlowResistance, Category = "CharacterAttributes")
	FGameplayAttributeData SlowResistance;
	ATTRIBUTE_ACCESSORS(UZfResistenceAttributeSet, SlowResistance)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalResistance, Category = "CharacterAttributes")
	FGameplayAttributeData CriticalResistance;
	ATTRIBUTE_ACCESSORS(UZfResistenceAttributeSet, CriticalResistance)
	
protected:
	
	UFUNCTION()
	virtual void OnRep_PhysicalResistance(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	virtual void OnRep_MagicResistance(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	virtual void OnRep_StunResistance(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	virtual void OnRep_SlowResistance(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	virtual void OnRep_CriticalResistance(const FGameplayAttributeData& OldValue) const;
};