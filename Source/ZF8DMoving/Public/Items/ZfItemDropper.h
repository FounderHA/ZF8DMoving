// Copyright ZfGame Studio. All Rights Reserved.
// ZfItemDropper.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FunctionLibrary/ZfItemGeneratorLibrary.h"
#include "ZfItemDropper.generated.h"

class UZfItemDefinition;
class AZfItemPickup;
class USphereComponent;
class UStaticMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class ZF8DMOVING_API AZfItemDropper : public AActor
{
	GENERATED_BODY()

public:

	AZfItemDropper();

    // Impede que o drop seja chamado mais de uma vez
    UPROPERTY()
    bool bAlreadyDropped = false;
	
	// ItemDefinition do item que será dropado
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dropper")
	TObjectPtr<UZfItemDefinition> ItemDefinition = nullptr;

	// Implementado no Blueprint — executa o drop do item
	UFUNCTION(BlueprintImplementableEvent, Category = "Zf|Dropper")
	void DropItem();

protected:

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dropper")
	TObjectPtr<USphereComponent> TriggerSphere;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);
};