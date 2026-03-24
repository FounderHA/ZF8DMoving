// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemReplicationProxyInterface.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "Inventory/ZfInventoryComponent.h"
#include "Inventory/ZfEquipmentComponent.h"
#include "Player/Class/ZfClassBaseSettings.h"
#include "ZfPlayerState.generated.h"

class UZfMainAttributeSet;
class UZfResourceAttributeSet;
class UZfProgressionAttributeSet;

UENUM(BlueprintType)
enum class EZfAttributeType : uint8
{
	Strength     UMETA(DisplayName = "Strength"),
	Dexterity    UMETA(DisplayName = "Dexterity"),
	Intelligence UMETA(DisplayName = "Intelligence"),
	Constitution UMETA(DisplayName = "Constitution"),
	Conviction UMETA(DisplayName = "Conviction")
	
};

UCLASS()
class ZF8DMOVING_API AZfPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	AZfPlayerState();
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UZfResourceAttributeSet* GetResourceAttributeSet() const;
	UZfMainAttributeSet* GetMainAttributeSet() const;
	UZfProgressionAttributeSet* GetProgressionAttributeSet() const;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category="Class")
	TObjectPtr<UZfPrimaryDataAssetClass> CharacterClassData;

	UFUNCTION(BlueprintCallable)
	void UpdateAttributePoints(float StrengthPointsToAdd, EZfAttributeType InAttributeType);

	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AvailablePoints, Category = "AllocatedPoints")
	float AvailablePoints = 0.f;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_StrengthPoints, Category = "AllocatedPoints")
	float StrengthPoints = 0.f;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IntelligencePoints, Category = "AllocatedPoints")
	float IntelligencePoints = 0.f;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DexterityPoints, Category = "AllocatedPoints")
	float DexterityPoints = 0.f;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ConstitutionPoints, Category = "AllocatedPoints")
	float ConstitutionPoints = 0.f;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ConvictionPoints, Category = "AllocatedPoints")
	float ConvictionPoints = 0.f;

	UFUNCTION(BlueprintImplementableEvent)
	void ApplyRecalculateAttribute(EZfAttributeType InAttributeType);

	UFUNCTION(BlueprintCallable)
	UZfInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }
	
protected:

	UFUNCTION()
	virtual void OnRep_AvailablePoints() const;

	UFUNCTION()
	virtual void OnRep_StrengthPoints();

	UFUNCTION()
	virtual void OnRep_IntelligencePoints() const;

	UFUNCTION()
	virtual void OnRep_DexterityPoints() const;

	UFUNCTION()
	virtual void OnRep_ConstitutionPoints() const;

	UFUNCTION()
	virtual void OnRep_ConvictionPoints() const;
	
	UPROPERTY()
	EZfAttributeType AttributeType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UZfInventoryComponent> InventoryComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UZfEquipmentComponent> EquipmentComponent;
	
	UPROPERTY()
	TObjectPtr<UZfResourceAttributeSet> ResourceAttributeSet;
	
	UPROPERTY()
	TObjectPtr<UZfMainAttributeSet> MainAttributeSet;

	UPROPERTY()
	TObjectPtr<UZfProgressionAttributeSet> ProgressionAttributeSet;

	
	
	//RPC
	UFUNCTION(Server, Reliable)
	void Server_UpdateAttributePoints(float StrengthPointsToAdd, EZfAttributeType InAttributeType);

public:
	// Debug inventory
	UFUNCTION(BlueprintCallable)
	void DebugAttribute();
	
};
