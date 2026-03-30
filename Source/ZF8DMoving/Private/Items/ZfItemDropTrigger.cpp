// Copyright ZfGame Studio. All Rights Reserved.
// ZfItemDropTrigger.cpp

#include "Items/ZfItemDropTrigger.h"
#include "Items/ZfItemSpawner.h"
#include "Inventory/ZfItemDefinition.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"

AZfItemDropTrigger::AZfItemDropTrigger()
{
    PrimaryActorTick.bCanEverTick = false;

    // Habilita replicação do ator
    bReplicates = true;

    // Cria a sphere de colisão
    TriggerSphere = CreateDefaultSubobject<USphereComponent>(
        TEXT("TriggerSphere"));
    SetRootComponent(TriggerSphere);
    TriggerSphere->SetSphereRadius(100.0f);
    TriggerSphere->SetCollisionProfileName(TEXT("Trigger"));
}

void AZfItemDropTrigger::BeginPlay()
{
    Super::BeginPlay();

    // Registra o callback de overlap apenas no servidor
    if (HasAuthority())
    {
        TriggerSphere->OnComponentBeginOverlap.AddDynamic(
            this, &AZfItemDropTrigger::OnOverlapBegin);
    }
}

void AZfItemDropTrigger::OnOverlapBegin(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    // Só executa no servidor
    if (!HasAuthority())
    {
        return;
    }

    // Verifica se já dropou (bDropOnce)
    if (bDropOnce && bHasDropped)
    {
        return;
    }

    // Só reage a Characters
    if (!Cast<ACharacter>(OtherActor))
    {
        return;
    }

    // Carrega o ItemDefinition
    UZfItemDefinition* LoadedDefinition =
        ItemDefinition.LoadSynchronous();

    if (!LoadedDefinition)
    {
        UE_LOG(LogZfInventory, Error,
            TEXT("AZfItemDropTrigger::OnOverlapBegin — "
                 "ItemDefinition não configurado ou falhou ao carregar."));
        return;
    }

    // Spawna o pickup à frente do ator que fez overlap
    const FZfSpawnItemResult Result =
        UZfItemSpawner::SpawnItemPickupInFrontOfActor(
            OtherActor,
            LoadedDefinition,
            ItemTier,
            ItemRarity,
            SpawnDistanceAhead);

    if (Result.IsSuccess())
    {
        UE_LOG(LogZfInventory, Log,
            TEXT("AZfItemDropTrigger::OnOverlapBegin — "
                 "Item '%s' spawnado por overlap de '%s'."),
            *LoadedDefinition->ItemName.ToString(),
            *OtherActor->GetName());

        // Marca como dropado se bDropOnce
        if (bDropOnce)
        {
            bHasDropped = true;

            // Desativa a colisão para não dropar novamente
            TriggerSphere->SetCollisionEnabled(
                ECollisionEnabled::NoCollision);
        }
    }
}