// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "ZfCharacter.generated.h"

class UZfResourceAttributeSet;

UCLASS()
class ZF8DMOVING_API AZfCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AZfCharacter();
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	virtual void PossessedBy(AController* NewController) override;
	
	virtual void InitAbilityActorInfo();
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<UZfResourceAttributeSet> HealthSet;
};
