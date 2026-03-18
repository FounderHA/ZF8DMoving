#include "Inventory/ZfItemPickup.h"
#include "Inventory/ZfInventoryComponent.h"
#include "Inventory/ZfItemDefinition.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "Player/ZfPlayerState.h"
#include "Net/UnrealNetwork.h"

AZfItemPickup::AZfItemPickup()
{
    bReplicates = true;

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = MeshComponent;
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    
    SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
    SphereComponent->SetupAttachment(RootComponent);
    SphereComponent->SetSphereRadius(100.f);
    SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AZfItemPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AZfItemPickup, ItemDefinition);
}

void AZfItemPickup::BeginPlay()
{
    Super::BeginPlay();
    
    if (HasAuthority())
    {
        SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &AZfItemPickup::OnSphereOverlap);
    }

    SphereComponent->SetHiddenInGame(false);
}

void AZfItemPickup::OnRep_ItemDefinition()
{
    UpdateVisual();
}

void AZfItemPickup::UpdateVisual()
{
    // acessa a definição direto
    if (!ItemDefinition) return;

    if (ItemDefinition->StaticMesh)
    {
        MeshComponent->SetStaticMesh(ItemDefinition->StaticMesh);
    }
}

void AZfItemPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority() || !ItemDefinition || !OtherActor) return;

    ACharacter* Character = Cast<ACharacter>(OtherActor);
    if (!Character) return;

    AZfPlayerState* PS = Character->GetPlayerState<AZfPlayerState>();
    if (!PS) return;

    UZfInventoryComponent* Inventory = PS->GetInventoryComponent();
    if (!Inventory) return;

    Inventory->Server_AddItem(ItemDefinition);
    SetLifeSpan(0.01f);
}