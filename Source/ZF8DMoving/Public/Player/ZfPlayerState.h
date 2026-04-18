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
class UZfOffensiveAttributeSet;
class UZfResistanceAttributeSet;
class UZfStealAttributesSet;
class UZfMovementAttributeSet;

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
	UZfOffensiveAttributeSet* GetDamageAttributeSet() const;
	UZfResistanceAttributeSet* GetResistanceAttributeSet() const;
	UZfStealAttributesSet* GetStealAttributeSet() const;
	UZfMovementAttributeSet* GetMovementAttributeSet() const;

	UFUNCTION(BlueprintCallable)
	UZfInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	/** Retorna o Data Asset de classe do personagem. */
	UFUNCTION(BlueprintCallable, Category = "Player|Class")
	UZfPrimaryDataAssetClass* GetCharacterClassData() const { return CharacterClassData; }
	
protected:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category="Class")
	TObjectPtr<UZfPrimaryDataAssetClass> CharacterClassData;

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
	TObjectPtr<UZfOffensiveAttributeSet> DamageAttributeSet;
	
	UPROPERTY()
	TObjectPtr<UZfResistanceAttributeSet> ResistanceAttributeSet;
	
	UPROPERTY()
	TObjectPtr<UZfStealAttributesSet> StealAttributeSet;
	
	UPROPERTY()
	TObjectPtr<UZfMovementAttributeSet> MovementAttributeSet;


private:
	
	/** 
	* Define se o personagem está sendo criado pela primeira vez.
	* true  → InitializeDefaults() será chamado após InitializeAttributes()
	* false → load do save, defaults ignorados.
	* Setado pelo GameMode antes da posse. Não replicado — servidor only.
	*/
	bool bIsNewCharacter = true;
	
public:
	

	/** Retorna true se o personagem está sendo criado pela primeira vez.
	*/
	UFUNCTION(BlueprintCallable, Category = "Player|Save")
	bool GetIsNewCharacter() const { return bIsNewCharacter; }
	
	/** Chamado pelo sistema de save quando um save existente é encontrado.
	* Após chamar este método, InitializeDefaults() não será executado.
	*/
	UFUNCTION(BlueprintCallable, Category = "Player|Save")
	void SetIsNewCharacterFalse() { bIsNewCharacter = false; }
	
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
