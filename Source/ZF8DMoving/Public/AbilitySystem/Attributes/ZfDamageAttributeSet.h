// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ZfAttributeSet.h"
#include "ZfDamageAttributeSet.generated.h"

/**
 * 
 */
UCLASS()
class ZF8DMOVING_API UZfDamageAttributeSet : public UZfAttributeSet
{
	GENERATED_BODY()
	
public:
	UZfDamageAttributeSet(const FObjectInitializer& ObjectInitializer);
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PhysicalDamage, Category = "DamageAttributes")
	FGameplayAttributeData PhysicalDamage;
	ATTRIBUTE_ACCESSORS(UZfDamageAttributeSet, PhysicalDamage)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_MagicalDamage, Category = "DamageAttributes")
	FGameplayAttributeData MagicalDamage;
	ATTRIBUTE_ACCESSORS(UZfDamageAttributeSet, MagicalDamage)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalDamage, Category = "DamageAttributes")
	FGameplayAttributeData CriticalDamage;
	ATTRIBUTE_ACCESSORS(UZfDamageAttributeSet, CriticalDamage)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalHitChance, Category = "DamageAttributes")
	FGameplayAttributeData CriticalHitChance;
	ATTRIBUTE_ACCESSORS(UZfDamageAttributeSet, CriticalHitChance)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_AttackSpeed, Category = "DamageAttributes")
	FGameplayAttributeData AttackSpeed;
	ATTRIBUTE_ACCESSORS(UZfDamageAttributeSet, AttackSpeed)

	
protected:
	
	UFUNCTION()
	virtual void OnRep_PhysicalDamage(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	virtual void OnRep_MagicalDamage(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	virtual void OnRep_CriticalDamage(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	virtual void OnRep_CriticalHitChance(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	virtual void OnRep_AttackSpeed(const FGameplayAttributeData& OldValue) const;
	
};
