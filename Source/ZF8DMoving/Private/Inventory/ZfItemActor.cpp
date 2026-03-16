#include "Inventory/ZfItemActor.h"
#include "Inventory/ZfItemInstance.h"
#include "Inventory/ZfInventoryComponent.h"
#include "Inventory/ZfItemDefinition.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "Player/ZfPlayerState.h"
#include "Net/UnrealNetwork.h"

AZfItemActor::AZfItemActor()
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

void AZfItemActor::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AZfItemActor, Item);
}

void AZfItemActor::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        SphereComponent->OnComponentBeginOverlap.AddDynamic(
            this, &AZfItemActor::OnSphereOverlap);
    }
}

void AZfItemActor::InitializeWithItem(UZfItemInstance* InItem)
{
    if (!InItem) return;
    Item = InItem;
    UpdateVisual();
}

void AZfItemActor::OnRep_Item()
{
    UpdateVisual();
}

void AZfItemActor::UpdateVisual()
{
    if (!Item) return;

    // ✅ acessa a definição direto
    UZfItemDefinition* Definition = Item->ItemDefinition;
    if (!Definition) return;

    if (Definition->StaticMesh)
    {
        MeshComponent->SetStaticMesh(Definition->StaticMesh);
    }
}

void AZfItemActor::OnSphereOverlap(UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority() || !Item || !OtherActor) return;

    ACharacter* Character = Cast<ACharacter>(OtherActor);
    if (!Character) return;

    AZfPlayerState* PS = Character->GetPlayerState<AZfPlayerState>();
    if (!PS) return;

    UZfInventoryComponent* Inventory = PS->GetInventoryComponent();
    if (!Inventory) return;

    Inventory->Server_AddItem(Item);
    Destroy();
}