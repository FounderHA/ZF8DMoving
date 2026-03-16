// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ZfAttributeSet.h"
#include "ZfProgressionAttributeSet.generated.h"

/**
 * 
 */
UCLASS()
class ZF8DMOVING_API UZfProgressionAttributeSet : public UZfAttributeSet
{
	GENERATED_BODY()
	
public:
	UZfProgressionAttributeSet(const FObjectInitializer& ObjectInitializer);
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override; 
	
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override; 
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_AttributePoints, Category = "CharacterProgression")
	FGameplayAttributeData AttributePoints;
	ATTRIBUTE_ACCESSORS(UZfProgressionAttributeSet, AttributePoints)

protected:
	
	UFUNCTION()
	virtual void OnRep_AttributePoints(const FGameplayAttributeData& OldValue) const;

};
