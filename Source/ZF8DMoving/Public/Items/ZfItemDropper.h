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

	// Quantidade de Itens para dropar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dropper", meta = (ClampMin = "1", UIMin = "1"))
	int32 AmountToDrop = 1;

	// Tempo Entre Drops
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dropper", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float DropTime = 1.f;

	// Implementado no Blueprint — executa o drop do item
	UFUNCTION(BlueprintImplementableEvent, Category = "Zf|Dropper")
	void DropItem();

private:
	
	FTimerHandle DropTimerHandle;
	
	UPROPERTY()
	int32 RemainingDrops = 0;

	void HandleDropTick();

protected:

	virtual void BeginPlay() override;
	

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dropper")
	TObjectPtr<USphereComponent> SphereComponent;
};