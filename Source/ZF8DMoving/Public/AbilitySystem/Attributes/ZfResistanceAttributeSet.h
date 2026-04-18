// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ZfAttributeSet.h"
#include "ZfResistanceAttributeSet.generated.h"

UCLASS()
class ZF8DMOVING_API UZfResistanceAttributeSet : public UZfAttributeSet
{
    GENERATED_BODY()
    
public:
    UZfResistanceAttributeSet(const FObjectInitializer& ObjectInitializer);
    
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Resistance
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PhysicalResistance, Category = "ResistanceAttributes")
    FGameplayAttributeData PhysicalResistance;
    ATTRIBUTE_ACCESSORS(UZfResistanceAttributeSet, PhysicalResistance)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_MagicalResistance, Category = "ResistanceAttributes")
    FGameplayAttributeData MagicalResistance;
    ATTRIBUTE_ACCESSORS(UZfResistanceAttributeSet, MagicalResistance)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Tenacity, Category = "ResistanceAttributes")
    FGameplayAttributeData Tenacity;
    ATTRIBUTE_ACCESSORS(UZfResistanceAttributeSet, Tenacity)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalResistance, Category = "ResistanceAttributes")
    FGameplayAttributeData CriticalResistance;
    ATTRIBUTE_ACCESSORS(UZfResistanceAttributeSet, CriticalResistance)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PoiseResistance, Category = "ResistanceAttributes")
    FGameplayAttributeData PoiseResistance;
    ATTRIBUTE_ACCESSORS(UZfResistanceAttributeSet, PoiseResistance)

    // Threshold
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_BurnThreshold, Category = "ResistanceAttributes")
    FGameplayAttributeData BurnThreshold;
    ATTRIBUTE_ACCESSORS(UZfResistanceAttributeSet, BurnThreshold)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_FreezeThreshold, Category = "ResistanceAttributes")
    FGameplayAttributeData FreezeThreshold;
    ATTRIBUTE_ACCESSORS(UZfResistanceAttributeSet, FreezeThreshold)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_ShockThreshold, Category = "ResistanceAttributes")
    FGameplayAttributeData ShockThreshold;
    ATTRIBUTE_ACCESSORS(UZfResistanceAttributeSet, ShockThreshold)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_BleedThreshold, Category = "ResistanceAttributes")
    FGameplayAttributeData BleedThreshold;
    ATTRIBUTE_ACCESSORS(UZfResistanceAttributeSet, BleedThreshold)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PoisonThreshold, Category = "ResistanceAttributes")
    FGameplayAttributeData PoisonThreshold;
    ATTRIBUTE_ACCESSORS(UZfResistanceAttributeSet, PoisonThreshold)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_StunThreshold, Category = "ResistanceAttributes")
    FGameplayAttributeData StunThreshold;
    ATTRIBUTE_ACCESSORS(UZfResistanceAttributeSet, StunThreshold)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_SlowThreshold, Category = "ResistanceAttributes")
    FGameplayAttributeData SlowThreshold;
    ATTRIBUTE_ACCESSORS(UZfResistanceAttributeSet, SlowThreshold)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_SleepThreshold, Category = "ResistanceAttributes")
    FGameplayAttributeData SleepThreshold;
    ATTRIBUTE_ACCESSORS(UZfResistanceAttributeSet, SleepThreshold)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_RootThreshold, Category = "ResistanceAttributes")
    FGameplayAttributeData RootThreshold;
    ATTRIBUTE_ACCESSORS(UZfResistanceAttributeSet, RootThreshold)

protected:

    UFUNCTION()
    virtual void OnRep_PhysicalResistance(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_MagicalResistance(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_Tenacity(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_CriticalResistance(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_PoiseResistance(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_BurnThreshold(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_FreezeThreshold(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_ShockThreshold(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_BleedThreshold(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_PoisonThreshold(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_StunThreshold(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_SlowThreshold(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_SleepThreshold(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_RootThreshold(const FGameplayAttributeData& OldValue) const;
};