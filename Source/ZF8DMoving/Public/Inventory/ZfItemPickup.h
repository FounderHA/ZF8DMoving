#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZfItemPickup.generated.h"

class UZfItemInstance;
class UZfItemDefinition;
class USphereComponent;

UCLASS()
class ZF8DMOVING_API AZfItemPickup : public AActor
{
	GENERATED_BODY()

public:
	AZfItemPickup();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	void UpdateVisual();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnRep_ItemDefinition();
	
	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ItemDefinition, Category = "Item")
	UZfItemDefinition* ItemDefinition;

	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<USphereComponent> SphereComponent;
};