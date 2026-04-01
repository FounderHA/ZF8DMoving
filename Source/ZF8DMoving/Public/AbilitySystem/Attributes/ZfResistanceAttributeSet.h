// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ZfAttributeSet.h"
#include "ZfResistanceAttributeSet.generated.h"

/**
 * 
 */
UCLASS()
class ZF8DMOVING_API UZfResistanceAttributeSet : public UZfAttributeSet
{
	GENERATED_BODY()
	
public:
	UZfResistanceAttributeSet(const FObjectInitializer& ObjectInitializer);
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override; 
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PhysicalResistance, Category = "CharacterAttributes")
	FGameplayAttributeData PhysicalResistance;
	ATTRIBUTE_ACCESSORS(UZfResistanceAttributeSet, PhysicalResistance)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_MagicResistance, Category = "CharacterAttributes")
	FGameplayAttributeData MagicResistance;
	ATTRIBUTE_ACCESSORS(UZfResistanceAttributeSet, MagicResistance)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_StunResistance, Category = "CharacterAttributes")
	FGameplayAttributeData StunResistance;
	ATTRIBUTE_ACCESSORS(UZfResistanceAttributeSet, StunResistance)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_SlowResistance, Category = "CharacterAttributes")
	FGameplayAttributeData SlowResistance;
	ATTRIBUTE_ACCESSORS(UZfResistanceAttributeSet, SlowResistance)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalResistance, Category = "CharacterAttributes")
	FGameplayAttributeData CriticalResistance;
	ATTRIBUTE_ACCESSORS(UZfResistanceAttributeSet, CriticalResistance)
	
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