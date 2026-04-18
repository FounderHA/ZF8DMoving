// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ZfAttributeSet.h"
#include "ZfMovementAttributeSet.generated.h"

UCLASS()
class ZF8DMOVING_API UZfMovementAttributeSet : public UZfAttributeSet
{
	GENERATED_BODY()

public:
	UZfMovementAttributeSet(const FObjectInitializer& ObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeed, Category = "MovementAttributes")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UZfMovementAttributeSet, MoveSpeed)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_DashDistance, Category = "MovementAttributes")
	FGameplayAttributeData DashDistance;
	ATTRIBUTE_ACCESSORS(UZfMovementAttributeSet, DashDistance)

protected:

	UFUNCTION()
	virtual void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	virtual void OnRep_DashDistance(const FGameplayAttributeData& OldValue) const;
};