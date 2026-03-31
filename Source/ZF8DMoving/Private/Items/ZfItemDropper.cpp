// Copyright ZfGame Studio. All Rights Reserved.

#include "Items/ZfItemDropper.h"
#include "Inventory/ZfItemPickup.h"
#include "Components/SphereComponent.h"

AZfItemDropper::AZfItemDropper()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
    SetRootComponent(TriggerSphere);
    TriggerSphere->SetSphereRadius(100.f);
    TriggerSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void AZfItemDropper::BeginPlay()
{
    Super::BeginPlay();

    TriggerSphere->OnComponentBeginOverlap.AddDynamic(
        this, &AZfItemDropper::OnOverlapBegin);
}

void AZfItemDropper::OnOverlapBegin(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!HasAuthority()) return;
    if (!OtherActor || OtherActor == this) return;
    if (bAlreadyDropped) return;

    // Ignora se o ator que entrou for um pickup
    if (OtherActor->IsA<AZfItemPickup>()) return;

    bAlreadyDropped = true;
    DropItem();
}