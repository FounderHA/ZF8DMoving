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
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "HealthSet")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UZfResourceAttributeSet, Health)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "HealthSet")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UZfResourceAttributeSet, MaxHealth)

protected:
	
	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldValue) const;
};
