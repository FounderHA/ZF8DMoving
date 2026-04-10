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
class UZfDamageAttributeSet;
class UZfResistanceAttributeSet;

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
	UZfDamageAttributeSet* GetDamageAttributeSet() const;
	UZfResistanceAttributeSet* GetResistanceAttributeSet() const;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category="Class")
	TObjectPtr<UZfPrimaryDataAssetClass> CharacterClassData;

	UFUNCTION(BlueprintCallable)
	UZfInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }
	
protected:
	

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|AttributeSets")
	TObjectPtr<UZfProgressionAttributeSet> ProgressionAttributeSet;
	
	UPROPERTY()
	TObjectPtr<UZfDamageAttributeSet> DamageAttributeSet;
	
	UPROPERTY()
	TObjectPtr<UZfResistanceAttributeSet> ResistanceAttributeSet;


public:
	
	// ─────────────────────────────────────────────────────────────────────
	// Server RPCs — ponte entre widget (cliente) e abilities (servidor)
	// Passa valores primitivos int32 para evitar problemas de replicação
	// de UObjects pela rede. O objeto Request é criado no servidor.
	// ─────────────────────────────────────────────────────────────────────
 
	/** Distribui pontos nos atributos. Valores >= 0. */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Progression")
	void Server_SpendAttributePoints(int32 Strength, int32 Dexterity, int32 Intelligence, int32 Constitution, int32 Conviction);
 
	/** Remove pontos individualmente. Valores >= 0 (quanto remover de cada). */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Progression")
	void Server_RefundAttributePoint(int32 Strength, int32 Dexterity, int32 Intelligence, int32 Constitution, int32 Conviction);
 
	/** Reseta todos os pontos distribuídos de volta ao pool. Sem parâmetros. */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Progression")
	void Server_ResetAttributePoints();

	
};
