// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemReplicationProxyInterface.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "Player/Class/ZfPrimaryDataAssetClass.h"
#include "ZfPlayerState.generated.h"

class UZfHealthSet;
class UZfMainAttributesSet;

USTRUCT(BlueprintType)
struct FPlayerAttributePoints
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintReadOnly)
	float StrengthPoints;

	UPROPERTY(BlueprintReadOnly)
	float IntelligencePoints;

	UPROPERTY(BlueprintReadOnly)
	float DexterityPoints;
	
	UPROPERTY(BlueprintReadOnly)
	float ConstitutionPoints;
	
	UPROPERTY(BlueprintReadOnly)
	float ConvictionPoints;
};


UCLASS()
class ZF8DMOVING_API AZfPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	AZfPlayerState();
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override; 
	
	UZfHealthSet* GetHealthSet() const;
	UZfMainAttributesSet* GetStrengthSet() const;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category="Class")
	TObjectPtr<UZfPrimaryDataAssetClass> CharacterClassData;
	
	UPROPERTY(BlueprintReadOnly, Replicated)
	FPlayerAttributePoints AllocatedPoints;
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<UZfHealthSet> HealthSet;
	
	UPROPERTY()
	TObjectPtr<UZfMainAttributesSet> StrengthSet;
	
};
