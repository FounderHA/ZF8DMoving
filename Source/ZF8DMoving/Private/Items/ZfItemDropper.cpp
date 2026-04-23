// Copyright ZfGame Studio. All Rights Reserved.

#include "Items/ZfItemDropper.h"
#include "Items/ZfItemPickup.h"
#include "Components/SphereComponent.h"

AZfItemDropper::AZfItemDropper()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
    SetRootComponent(SphereComponent);
    SphereComponent->SetSphereRadius(100.f);
    SphereComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void AZfItemDropper::BeginPlay()
{
    Super::BeginPlay();

    if (!HasAuthority()) return;
    if (bAlreadyDropped) return;

    bAlreadyDropped = true;

    RemainingDrops = AmountToDrop;

    HandleDropTick(); // dropa 1 imediatamente

    GetWorldTimerManager().SetTimer(
        DropTimerHandle,
        this,
        &AZfItemDropper::HandleDropTick,
        DropTime,
        true
    );
}

void AZfItemDropper::HandleDropTick()
{
    if (RemainingDrops <= 0)
    {
        GetWorldTimerManager().ClearTimer(DropTimerHandle);
        return;
    }

    if (HasAuthority())
    {
        DropItem(); // chama o Blueprint
    } 

    RemainingDrops--;
}
