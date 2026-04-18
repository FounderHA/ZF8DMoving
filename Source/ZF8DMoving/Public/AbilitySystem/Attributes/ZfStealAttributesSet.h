// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ZfAttributeSet.h"
#include "ZfStealAttributesSet.generated.h"

UCLASS()
class ZF8DMOVING_API UZfStealAttributesSet : public UZfAttributeSet
{
    GENERATED_BODY()

public:
    UZfStealAttributesSet(const FObjectInitializer& ObjectInitializer);

    // Flat On Hit
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_FlatLifeStealOnHit, Category = "StealAttributes")
    FGameplayAttributeData FlatLifeStealOnHit;
    ATTRIBUTE_ACCESSORS(UZfStealAttributesSet, FlatLifeStealOnHit)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_FlatManaStealOnHit, Category = "StealAttributes")
    FGameplayAttributeData FlatManaStealOnHit;
    ATTRIBUTE_ACCESSORS(UZfStealAttributesSet, FlatManaStealOnHit)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_FlatStaminaStealOnHit, Category = "StealAttributes")
    FGameplayAttributeData FlatStaminaStealOnHit;
    ATTRIBUTE_ACCESSORS(UZfStealAttributesSet, FlatStaminaStealOnHit)

    // Percent On Hit
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PercentLifeStealOnHit, Category = "StealAttributes")
    FGameplayAttributeData PercentLifeStealOnHit;
    ATTRIBUTE_ACCESSORS(UZfStealAttributesSet, PercentLifeStealOnHit)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PercentManaStealOnHit, Category = "StealAttributes")
    FGameplayAttributeData PercentManaStealOnHit;
    ATTRIBUTE_ACCESSORS(UZfStealAttributesSet, PercentManaStealOnHit)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PercentStaminaStealOnHit, Category = "StealAttributes")
    FGameplayAttributeData PercentStaminaStealOnHit;
    ATTRIBUTE_ACCESSORS(UZfStealAttributesSet, PercentStaminaStealOnHit)

    // Flat On Kill
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_FlatLifeStealOnKill, Category = "StealAttributes")
    FGameplayAttributeData FlatLifeStealOnKill;
    ATTRIBUTE_ACCESSORS(UZfStealAttributesSet, FlatLifeStealOnKill)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_FlatManaStealOnKill, Category = "StealAttributes")
    FGameplayAttributeData FlatManaStealOnKill;
    ATTRIBUTE_ACCESSORS(UZfStealAttributesSet, FlatManaStealOnKill)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_FlatStaminaStealOnKill, Category = "StealAttributes")
    FGameplayAttributeData FlatStaminaStealOnKill;
    ATTRIBUTE_ACCESSORS(UZfStealAttributesSet, FlatStaminaStealOnKill)

    // Percent On Kill
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PercentLifeStealOnKill, Category = "StealAttributes")
    FGameplayAttributeData PercentLifeStealOnKill;
    ATTRIBUTE_ACCESSORS(UZfStealAttributesSet, PercentLifeStealOnKill)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PercentManaStealOnKill, Category = "StealAttributes")
    FGameplayAttributeData PercentManaStealOnKill;
    ATTRIBUTE_ACCESSORS(UZfStealAttributesSet, PercentManaStealOnKill)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PercentStaminaStealOnKill, Category = "StealAttributes")
    FGameplayAttributeData PercentStaminaStealOnKill;
    ATTRIBUTE_ACCESSORS(UZfStealAttributesSet, PercentStaminaStealOnKill)

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
};