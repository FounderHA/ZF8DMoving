// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ZfAttributeSet.h"
#include "ZfOffensiveAttributeSet.generated.h"

UCLASS()
class ZF8DMOVING_API UZfOffensiveAttributeSet : public UZfAttributeSet
{
    GENERATED_BODY()
    
public:
    UZfOffensiveAttributeSet(const FObjectInitializer& ObjectInitializer);
    
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Damage
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PhysicalDamage, Category = "OffensiveAttributes")
    FGameplayAttributeData PhysicalDamage;
    ATTRIBUTE_ACCESSORS(UZfOffensiveAttributeSet, PhysicalDamage)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_MagicalDamage, Category = "OffensiveAttributes")
    FGameplayAttributeData MagicalDamage;
    ATTRIBUTE_ACCESSORS(UZfOffensiveAttributeSet, MagicalDamage)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PhysicalPain, Category = "OffensiveAttributes")
    FGameplayAttributeData PhysicalPain;
    ATTRIBUTE_ACCESSORS(UZfOffensiveAttributeSet, PhysicalPain)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_MagicalPain, Category = "OffensiveAttributes")
    FGameplayAttributeData MagicalPain;
    ATTRIBUTE_ACCESSORS(UZfOffensiveAttributeSet, MagicalPain)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalHitChance, Category = "OffensiveAttributes")
    FGameplayAttributeData CriticalHitChance;
    ATTRIBUTE_ACCESSORS(UZfOffensiveAttributeSet, CriticalHitChance)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalDamage, Category = "OffensiveAttributes")
    FGameplayAttributeData CriticalDamage;
    ATTRIBUTE_ACCESSORS(UZfOffensiveAttributeSet, CriticalDamage)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PoiseDamage, Category = "OffensiveAttributes")
    FGameplayAttributeData PoiseDamage;
    ATTRIBUTE_ACCESSORS(UZfOffensiveAttributeSet, PoiseDamage)

    // Speed / Cooldown
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_AttackSpeed, Category = "OffensiveAttributes")
    FGameplayAttributeData AttackSpeed;
    ATTRIBUTE_ACCESSORS(UZfOffensiveAttributeSet, AttackSpeed)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_CastSpeed, Category = "OffensiveAttributes")
    FGameplayAttributeData CastSpeed;
    ATTRIBUTE_ACCESSORS(UZfOffensiveAttributeSet, CastSpeed)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_CooldownReduction, Category = "OffensiveAttributes")
    FGameplayAttributeData CooldownReduction;
    ATTRIBUTE_ACCESSORS(UZfOffensiveAttributeSet, CooldownReduction)

    // Positional
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_BackstabDamage, Category = "OffensiveAttributes")
    FGameplayAttributeData BackstabDamage;
    ATTRIBUTE_ACCESSORS(UZfOffensiveAttributeSet, BackstabDamage)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_FirstHitDamage, Category = "OffensiveAttributes")
    FGameplayAttributeData FirstHitDamage;
    ATTRIBUTE_ACCESSORS(UZfOffensiveAttributeSet, FirstHitDamage)

    // Buildup
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_BurnBuildup, Category = "OffensiveAttributes")
    FGameplayAttributeData BurnBuildup;
    ATTRIBUTE_ACCESSORS(UZfOffensiveAttributeSet, BurnBuildup)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_FreezeBuildup, Category = "OffensiveAttributes")
    FGameplayAttributeData FreezeBuildup;
    ATTRIBUTE_ACCESSORS(UZfOffensiveAttributeSet, FreezeBuildup)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_ShockBuildup, Category = "OffensiveAttributes")
    FGameplayAttributeData ShockBuildup;
    ATTRIBUTE_ACCESSORS(UZfOffensiveAttributeSet, ShockBuildup)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_BleedBuildup, Category = "OffensiveAttributes")
    FGameplayAttributeData BleedBuildup;
    ATTRIBUTE_ACCESSORS(UZfOffensiveAttributeSet, BleedBuildup)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PoisonBuildup, Category = "OffensiveAttributes")
    FGameplayAttributeData PoisonBuildup;
    ATTRIBUTE_ACCESSORS(UZfOffensiveAttributeSet, PoisonBuildup)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_StunBuildup, Category = "OffensiveAttributes")
    FGameplayAttributeData StunBuildup;
    ATTRIBUTE_ACCESSORS(UZfOffensiveAttributeSet, StunBuildup)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_SlowBuildup, Category = "OffensiveAttributes")
    FGameplayAttributeData SlowBuildup;
    ATTRIBUTE_ACCESSORS(UZfOffensiveAttributeSet, SlowBuildup)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_SleepBuildup, Category = "OffensiveAttributes")
    FGameplayAttributeData SleepBuildup;
    ATTRIBUTE_ACCESSORS(UZfOffensiveAttributeSet, SleepBuildup)

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_RootBuildup, Category = "OffensiveAttributes")
    FGameplayAttributeData RootBuildup;
    ATTRIBUTE_ACCESSORS(UZfOffensiveAttributeSet, RootBuildup)

protected:

    UFUNCTION()
    virtual void OnRep_PhysicalDamage(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_MagicalDamage(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_PhysicalPain(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_MagicalPain(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_CriticalHitChance(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_CriticalDamage(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_PoiseDamage(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_AttackSpeed(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_CastSpeed(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_CooldownReduction(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_BackstabDamage(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_FirstHitDamage(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_BurnBuildup(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_FreezeBuildup(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_ShockBuildup(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_BleedBuildup(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_PoisonBuildup(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_StunBuildup(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_SlowBuildup(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_SleepBuildup(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    virtual void OnRep_RootBuildup(const FGameplayAttributeData& OldValue) const;
};