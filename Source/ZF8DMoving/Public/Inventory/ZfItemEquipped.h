// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZfItemEquipped.generated.h"

class UZfItemInstance;

UCLASS()
class ZF8DMOVING_API AZfItemEquipped : public AActor
{
	GENERATED_BODY()

public:
	AZfItemEquipped();

	void InitializeWithItem(UZfItemInstance* InItem, USceneComponent* AttachTarget, FName SocketName);

	UFUNCTION(BlueprintCallable)
	UZfItemInstance* GetItem() const { return Item; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY()
	TObjectPtr<UZfItemInstance> Item;

};

