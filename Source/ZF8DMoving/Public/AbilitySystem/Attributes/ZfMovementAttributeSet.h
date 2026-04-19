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
	
	/**
	* Clampeia MoveSpeed e DashDistance após qualquer GE ser executado.
	* Único ponto de sanitização — garante que nenhum GE coloque valores
	* inválidos antes do delegate disparar para o CharacterMovementComponent.
	*/
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

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

private:

	/** Limites de sanitização do MoveSpeed. Ajuste conforme game design. */
	static constexpr float MinMoveSpeed  =   10.f;
	static constexpr float MaxMoveSpeed  = 3000.f;

	/** Limites de sanitização do DashDistance. */
	static constexpr float MinDashDistance =    0.f;
	static constexpr float MaxDashDistance = 5000.f;
	
};