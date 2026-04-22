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
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_FlatLifeStealOnHit, Category = "UtiliteAttribute")
    FGameplayAttributeData FlatLifeStealOnHit;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, FlatLifeStealOnHit)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_FlatManaStealOnHit, Category = "UtiliteAttribute")
    FGameplayAttributeData FlatManaStealOnHit;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, FlatManaStealOnHit)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_FlatStaminaStealOnHit, Category = "UtiliteAttribute")
    FGameplayAttributeData FlatStaminaStealOnHit;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, FlatStaminaStealOnHit)

    // Percent On Hit
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PercentLifeStealOnHit, Category = "UtiliteAttribute")
    FGameplayAttributeData PercentLifeStealOnHit;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, PercentLifeStealOnHit)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PercentManaStealOnHit, Category = "UtiliteAttribute")
    FGameplayAttributeData PercentManaStealOnHit;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, PercentManaStealOnHit)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PercentStaminaStealOnHit, Category = "UtiliteAttribute")
    FGameplayAttributeData PercentStaminaStealOnHit;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, PercentStaminaStealOnHit)

    // Flat On Kill
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_FlatLifeStealOnKill, Category = "UtiliteAttribute")
    FGameplayAttributeData FlatLifeStealOnKill;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, FlatLifeStealOnKill)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_FlatManaStealOnKill, Category = "UtiliteAttribute")
    FGameplayAttributeData FlatManaStealOnKill;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, FlatManaStealOnKill)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_FlatStaminaStealOnKill, Category = "UtiliteAttribute")
    FGameplayAttributeData FlatStaminaStealOnKill;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, FlatStaminaStealOnKill)

    // Percent On Kill
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PercentLifeStealOnKill, Category = "UtiliteAttribute")
    FGameplayAttributeData PercentLifeStealOnKill;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, PercentLifeStealOnKill)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PercentManaStealOnKill, Category = "UtiliteAttribute")
    FGameplayAttributeData PercentManaStealOnKill;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, PercentManaStealOnKill)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PercentStaminaStealOnKill, Category = "UtiliteAttribute")
    FGameplayAttributeData PercentStaminaStealOnKill;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, PercentStaminaStealOnKill)
    
    // Regen
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_HealthRegen, Category = "UtiliteAttribute")
    FGameplayAttributeData HealthRegen;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, HealthRegen)
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_ManaRegen, Category = "UtiliteAttribute")
    FGameplayAttributeData ManaRegen;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, ManaRegen)
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_StaminaRegen, Category = "UtiliteAttribute")
    FGameplayAttributeData StaminaRegen;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, StaminaRegen)
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_HealthCostReduction, Category = "UtiliteAttribute")
    FGameplayAttributeData HealthCostReduction;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, HealthCostReduction)
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_ManaCostReduction, Category = "UtiliteAttribute")
    FGameplayAttributeData ManaCostReduction;
    ATTRIBUTE_ACCESSORS(UZfUtilityAttributeSet, ManaCostReduction)
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_StaminaCostReduction, Category = "UtiliteAttribute")
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