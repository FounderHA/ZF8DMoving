// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/ZfCharacter.h"
#include "ZfPlayerCharacter.generated.h"

UCLASS()
class ZF8DMOVING_API AZfPlayerCharacter : public AZfCharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AZfPlayerCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void InitAbilityActorInfo() override;

};


