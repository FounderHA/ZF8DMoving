// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "ZfAttributeSet.h"
#include "ZfResourceAttributeSet.generated.h"

/**
 * 
 */
UCLASS()
class ZF8DMOVING_API UZfResourceAttributeSet : public UZfAttributeSet
{
	GENERATED_BODY()
	
public:
	UZfResourceAttributeSet(const FObjectInitializer& ObjectInitializer);
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override; 
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	
	// ── Health ────────────────────────────────────────────────────────────
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "HealthSet")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UZfResourceAttributeSet, Health)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "HealthSet")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UZfResourceAttributeSet, MaxHealth)
	
	// ── Mana ──────────────────────────────────────────────────────────────
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Mana, Category = "ManaSet")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UZfResourceAttributeSet, Mana)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMana, Category = "ManaSet")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UZfResourceAttributeSet, MaxMana)
	
	// ── Stamina ───────────────────────────────────────────────────────────
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, Category = "StaminaSet")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UZfResourceAttributeSet, Stamina)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStamina, Category = "StaminaSet")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UZfResourceAttributeSet, MaxStamina)
	
protected:
	
	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	virtual void OnRep_Mana(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	virtual void OnRep_MaxMana(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	virtual void OnRep_Stamina(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	virtual void OnRep_MaxStamina(const FGameplayAttributeData& OldValue) const;
};
