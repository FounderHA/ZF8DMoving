#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZfItemPickup.generated.h"

class UZfItemInstance;
class USphereComponent;

UCLASS()
class ZF8DMOVING_API AZfItemPickup : public AActor
{
	GENERATED_BODY()

public:
	AZfItemPickup();

	void InitializeWithItem(UZfItemInstance* InItem);

	UFUNCTION(BlueprintCallable)
	UZfItemInstance* GetItem() const { return Item; }

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnRep_Item();

	UPROPERTY(ReplicatedUsing = OnRep_Item)
	TObjectPtr<UZfItemInstance> Item;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USphereComponent> SphereComponent;

private:
	void UpdateVisual();
};