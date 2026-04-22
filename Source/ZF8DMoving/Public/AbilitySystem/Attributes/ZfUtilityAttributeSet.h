// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ZfAttributeSet.h"
#include "ZfUtilityAttributeSet.generated.h"

UCLASS()
class ZF8DMOVING_API UZfUtilityAttributeSet : public UZfAttributeSet
{
    GENERATED_BODY()

public:
    UZfUtilityAttributeSet(const FObjectInitializer& ObjectInitializer);

    // Flat On Hit
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_FlatLifeStealOnHit, Category = "UtilityAttribute")
    FGameplayAttributeData FlatLifeStealOnHit;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, FlatLifeStealOnHit)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_FlatManaStealOnHit, Category = "UtilityAttribute")
    FGameplayAttributeData FlatManaStealOnHit;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, FlatManaStealOnHit)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_FlatStaminaStealOnHit, Category = "UtilityAttribute")
    FGameplayAttributeData FlatStaminaStealOnHit;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, FlatStaminaStealOnHit)

    // Percent On Hit
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PercentLifeStealOnHit, Category = "UtilityAttribute")
    FGameplayAttributeData PercentLifeStealOnHit;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, PercentLifeStealOnHit)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PercentManaStealOnHit, Category = "UtilityAttribute")
    FGameplayAttributeData PercentManaStealOnHit;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, PercentManaStealOnHit)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PercentStaminaStealOnHit, Category = "UtilityAttribute")
    FGameplayAttributeData PercentStaminaStealOnHit;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, PercentStaminaStealOnHit)

    // Flat On Kill
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_FlatLifeStealOnKill, Category = "UtilityAttribute")
    FGameplayAttributeData FlatLifeStealOnKill;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, FlatLifeStealOnKill)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_FlatManaStealOnKill, Category = "UtilityAttribute")
    FGameplayAttributeData FlatManaStealOnKill;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, FlatManaStealOnKill)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_FlatStaminaStealOnKill, Category = "UtilityAttribute")
    FGameplayAttributeData FlatStaminaStealOnKill;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, FlatStaminaStealOnKill)

    // Percent On Kill
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PercentLifeStealOnKill, Category = "UtilityAttribute")
    FGameplayAttributeData PercentLifeStealOnKill;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, PercentLifeStealOnKill)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PercentManaStealOnKill, Category = "UtilityAttribute")
    FGameplayAttributeData PercentManaStealOnKill;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, PercentManaStealOnKill)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PercentStaminaStealOnKill, Category = "UtilityAttribute")
    FGameplayAttributeData PercentStaminaStealOnKill;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, PercentStaminaStealOnKill)
    
    // Regen
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_HealthRegen, Category = "UtilityAttribute")
    FGameplayAttributeData HealthRegen;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, HealthRegen)
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_ManaRegen, Category = "UtilityAttribute")
    FGameplayAttributeData ManaRegen;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, ManaRegen)
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_StaminaRegen, Category = "UtilityAttribute")
    FGameplayAttributeData StaminaRegen;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, StaminaRegen)
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_HealthCostReduction, Category = "UtilityAttribute")
    FGameplayAttributeData HealthCostReduction;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, HealthCostReduction)
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_ManaCostReduction, Category = "UtilityAttribute")
    FGameplayAttributeData ManaCostReduction;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, ManaCostReduction)
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_StaminaCostReduction, Category = "UtilityAttribute")
    FGameplayAttributeData StaminaCostReduction;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, StaminaCostReduction)

protected:

    UFUNCTION() virtual void OnRep_FlatLifeStealOnHit(const FGameplayAttributeData& OldValue) const;
    UFUNCTION() virtual void OnRep_FlatManaStealOnHit(const FGameplayAttributeData& OldValue) const;
    UFUNCTION() virtual void OnRep_FlatStaminaStealOnHit(const FGameplayAttributeData& OldValue) const;

    UFUNCTION() virtual void OnRep_PercentLifeStealOnHit(const FGameplayAttributeData& OldValue) const;
    UFUNCTION() virtual void OnRep_PercentManaStealOnHit(const FGameplayAttributeData& OldValue) const;
    UFUNCTION() virtual void OnRep_PercentStaminaStealOnHit(const FGameplayAttributeData& OldValue) const;

    UFUNCTION() virtual void OnRep_FlatLifeStealOnKill(const FGameplayAttributeData& OldValue) const;
    UFUNCTION() virtual void OnRep_FlatManaStealOnKill(const FGameplayAttributeData& OldValue) const;
    UFUNCTION() virtual void OnRep_FlatStaminaStealOnKill(const FGameplayAttributeData& OldValue) const;

    UFUNCTION() virtual void OnRep_PercentLifeStealOnKill(const FGameplayAttributeData& OldValue) const;
    UFUNCTION() virtual void OnRep_PercentManaStealOnKill(const FGameplayAttributeData& OldValue) const;
    UFUNCTION() virtual void OnRep_PercentStaminaStealOnKill(const FGameplayAttributeData& OldValue) const;
    
    UFUNCTION() virtual void OnRep_HealthRegen(const FGameplayAttributeData& OldValue) const;
    UFUNCTION() virtual void OnRep_ManaRegen(const FGameplayAttributeData& OldValue) const;
    UFUNCTION() virtual void OnRep_StaminaRegen(const FGameplayAttributeData& OldValue) const;
    
    UFUNCTION() virtual void OnRep_HealthCostReduction(const FGameplayAttributeData& OldValue) const;
    UFUNCTION() virtual void OnRep_ManaCostReduction(const FGameplayAttributeData& OldValue) const;
    UFUNCTION() virtual void OnRep_StaminaCostReduction(const FGameplayAttributeData& OldValue) const;
};