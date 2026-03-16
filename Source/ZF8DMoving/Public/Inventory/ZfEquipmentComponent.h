// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/ZfEquipSlot.h"
#include "ZfEquipmentComponent.generated.h"

class UZfItemInstance;
class UZfInventoryComponent;
class AZfItemEquipped;


USTRUCT(BlueprintType)
struct FZfEquipmentEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EZfEquipSlot Slot = EZfEquipSlot::None;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UZfItemInstance> Item = nullptr;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AZfItemEquipped> SpawnedActor = nullptr;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ZF8DMOVING_API UZfEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZfEquipmentComponent();

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Equipa um item vindo da Bag
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_EquipItem(UZfItemInstance* InItem, UZfInventoryComponent* FromInventory);

	// Desequipa um slot e devolve para a Bag
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_UnequipItem(EZfEquipSlot Slot, UZfInventoryComponent* ToInventory);

	// Retorna o item equipado em um slot
	UFUNCTION(BlueprintCallable, BlueprintPure)
	UZfItemInstance* GetEquippedItem(EZfEquipSlot Slot) const;

	// Verifica se um slot está ocupado
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsSlotOccupied(EZfEquipSlot Slot) const;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentChanged, 
		EZfEquipSlot, Slot, UZfItemInstance*, Item);

	UPROPERTY(BlueprintAssignable)
	FOnEquipmentChanged OnItemEquipped;

	UPROPERTY(BlueprintAssignable)
	FOnEquipmentChanged OnItemUnequipped;

private:

	// Array replicado em vez de TMap
	UPROPERTY(Replicated)
	TArray<FZfEquipmentEntry> EquippedItems;
};
