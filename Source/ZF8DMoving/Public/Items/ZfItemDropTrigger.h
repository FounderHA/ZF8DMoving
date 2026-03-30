// Copyright ZfGame Studio. All Rights Reserved.
// ZfItemDropTrigger.h
// Ator de teste que spawna um pickup ao fazer overlap.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inventory/ZfInventoryTypes.h"
#include "ZfItemDropTrigger.generated.h"

class USphereComponent;
class UZfItemDefinition;

UCLASS()
class ZF8DMOVING_API AZfItemDropTrigger : public AActor
{
	GENERATED_BODY()

public:

	AZfItemDropTrigger();

	// Sphere de colisão que detecta o overlap
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> TriggerSphere;

	// ItemDefinition do item que será spawnado
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop|Config")
	TSoftObjectPtr<UZfItemDefinition> ItemDefinition;

	// Tier do item a spawnar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop|Config",
		meta = (ClampMin = "0", ClampMax = "5"))
	int32 ItemTier = 0;

	// Raridade do item a spawnar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop|Config")
	EZfItemRarity ItemRarity = EZfItemRarity::Common;

	// Se verdadeiro, só spawna uma vez e desativa o trigger
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop|Config")
	bool bDropOnce = true;

	// Distância à frente do ator que fez overlap
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop|Config",
		meta = (ClampMin = "0.0"))
	float SpawnDistanceAhead = 100.0f;

protected:

	virtual void BeginPlay() override;

private:

	// Já dropou o item (usado com bDropOnce)
	bool bHasDropped = false;

	UFUNCTION()
	void OnOverlapBegin(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
};