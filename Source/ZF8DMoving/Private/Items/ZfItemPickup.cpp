// Copyright ZfGame Studio. All Rights Reserved.
// ZfItemPickup.cpp

#include "Items/ZfItemPickup.h"
#include "Inventory/ZfInventoryComponent.h"
#include "Inventory/ZfItemInstance.h"
#include "Inventory/ZfItemDefinition.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Player/ZfPlayerState.h"

// ============================================================
// Constructor
// ============================================================

AZfItemPickup::AZfItemPickup()
{
    // Pickup não precisa de tick
    PrimaryActorTick.bCanEverTick = false;

    // Habilita replicação do ator
    bReplicates = true;

    // Habilita replicação do movimento para que a posição
    // seja sincronizada em todos os clientes
    SetReplicateMovement(true);

    // ----------------------------------------------------------
    // Criação dos componentes
    // ----------------------------------------------------------

    // Mesh visual
    StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
    SetRootComponent(StaticMeshComponent);
    StaticMeshComponent->SetupAttachment(CollisionSphere);
    StaticMeshComponent->SetRelativeLocation(FVector::ZeroVector);

    // Física na mesh
    StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    StaticMeshComponent->SetCollisionObjectType(ECC_PhysicsBody);
    StaticMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
    StaticMeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    StaticMeshComponent->SetSimulatePhysics(true);
    
    // Sphere como root (interação)
    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    CollisionSphere->SetupAttachment(StaticMeshComponent);
    CollisionSphere->SetSphereRadius(16.0f);
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    
}

void AZfItemPickup::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    
    // Roda após o Blueprint aplicar seu delta no constructor,
    // garantindo que nossa configuração é a final
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionSphere->SetCollisionObjectType(ECC_GameTraceChannel2);
    CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionSphere->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap);
    CollisionSphere->SetGenerateOverlapEvents(true);
}

// ============================================================
// REPLICAÇÃO
// ============================================================

void AZfItemPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Replica o ItemInstance para todos os clientes
    // para que possam exibir as informações corretas do item
    DOREPLIFETIME(AZfItemPickup, ItemInstance);
}

// ============================================================
// CICLO DE VIDA
// ============================================================

void AZfItemPickup::BeginPlay()
{
    Super::BeginPlay();

    // Atualiza o raio da esfera com o valor configurado
    CollisionSphere->SetSphereRadius(PickupRadius);

    // Registra os callbacks de overlap
    CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AZfItemPickup::OnOverlapBegin);
    CollisionSphere->OnComponentEndOverlap.AddDynamic(this, &AZfItemPickup::OnOverlapEnd);
    
    // Inicia timer de auto-destruição se configurado
    if (HasAuthority() && AutoDestroyAfterSeconds > 0.0f)
    {
        Internal_StartAutoDestroyTimer();
    }
}

void AZfItemPickup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Cancela carregamento assíncrono de mesh se ainda em andamento
    if (MeshStreamingHandle.IsValid())
    {
        MeshStreamingHandle->CancelHandle();
        MeshStreamingHandle.Reset();
    }

    // Cancela timer de auto-destruição
    if (GetWorldTimerManager().IsTimerActive(AutoDestroyTimerHandle))
    {
        GetWorldTimerManager().ClearTimer(AutoDestroyTimerHandle);
    }

    Super::EndPlay(EndPlayReason);
}

// ============================================================
// INICIALIZAÇÃO
// ============================================================

void AZfItemPickup::InitializePickup(UZfItemInstance* InItemInstance)
{
    if (!Internal_CheckIsServer(TEXT("InitializePickup")))
    {
        return;
    }

    if (!InItemInstance)
    {
        UE_LOG(LogZfInventory, Error, TEXT("AZfItemPickup::InitializePickup — " "ItemInstance é nulo. Pickup não será inicializado."));
        return;
    }

    // Armazena o ItemInstance — será replicado automaticamente
    ItemInstance = InItemInstance;

    UE_LOG(LogZfInventory, Log, TEXT("AZfItemPickup::InitializePickup — " "Pickup inicializado com item '%s'. GUID: %s"),
        *InItemInstance->GetItemName().ToString(), *InItemInstance->GetItemGuid().ToString());
}

// ============================================================
// COLETA
// ============================================================

EZfItemMechanicResult AZfItemPickup::TryCollectItem(AActor* CollectorActor)
{
    if (!Internal_CheckIsServer(TEXT("TryCollectItem")))
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }
 
    if (!CollectorActor)
    {
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }
 
    if (!HasValidItem())
    {
        UE_LOG(LogZfInventory, Warning, TEXT("AZfItemPickup::TryCollectItem — " "Pickup não tem item válido."));
        return EZfItemMechanicResult::Failed_ItemNotFound;
    }
 
    // Obtém o InventoryComponent do coletor
    UZfInventoryComponent* CollectorInventory = nullptr;
    if (!Internal_GetCollectorInventory(CollectorActor, CollectorInventory))
    {
        UE_LOG(LogZfInventory, Warning, TEXT("AZfItemPickup::TryCollectItem — " "Ator '%s' não tem UZfInventoryComponent."), *CollectorActor->GetName());
        return EZfItemMechanicResult::Failed_InvalidOperation;
    }
 
    // Tenta adicionar o item ao inventário do coletor
    int32 OutSlotIndex = INDEX_NONE;
    const EZfItemMechanicResult Result = CollectorInventory->TryPickupItem(ItemInstance, this);
 
    if (Result == EZfItemMechanicResult::Success)
    {
        // Notifica todos os clientes da coleta
        // para efeitos visuais/sonoros
        MulticastOnItemCollected(CollectorActor);
 
        // Limpa a referência local
        UZfItemInstance* CollectedItem = ItemInstance;
        ItemInstance = nullptr;
 
        // Dispara delegate
        OnItemPickedUp.Broadcast(CollectorActor, CollectedItem);
 
        UE_LOG(LogZfInventory, Log, TEXT("AZfItemPickup::TryCollectItem — " "Item '%s' coletado por '%s'. Pickup será destruído."),
            *CollectedItem->GetItemName().ToString(), *CollectorActor->GetName());
 
        // Destrói o pickup após a coleta
        SetLifeSpan(0.01f);
    }
    else if (Result == EZfItemMechanicResult::Failed_InventoryFull)
    {
        ReDropItem(CollectorActor);
        UE_LOG(LogZfInventory, Warning, TEXT("AZfItemPickup::TryCollectItem — " "Inventario Cheio, RedropandoItem"));
    }
    else
    {
        UE_LOG(LogZfInventory, Warning, TEXT("AZfItemPickup::TryCollectItem — " "Falha ao coletar item: %s"), *UEnum::GetValueAsString(Result));
    }
 
    return Result;
}

// ============================================================
// DROP
// ============================================================

void AZfItemPickup::SetNewStack(int32 NewStack) const
{
    ItemInstance->SetCurrentStack(NewStack);
}

void AZfItemPickup::ReDropItem(const AActor* CollectorActor)
{
    if (!CollectorActor) return;

    // Posição na frente do ator
    const FVector ActorLocation  = CollectorActor->GetActorLocation();
    const FVector ForwardVector  = CollectorActor->GetActorForwardVector();
    const FVector DropLocation   = ActorLocation + ForwardVector * 100.0f + FVector(0, 0, 50.0f);

    SetActorLocation(DropLocation);

    if (StaticMeshComponent && StaticMeshComponent->IsSimulatingPhysics())
    {
        // Ângulo aleatório para frente (entre -30 e 30 graus horizontalmente)
        const float RandomAngle  = FMath::RandRange(-30.0f, 30.0f);
        const FRotator Rotation  = FRotator(0.0f, CollectorActor->GetActorRotation().Yaw + RandomAngle, 0.0f);
        const FVector ThrowDir   = Rotation.Vector();

        // Velocidade com componente para frente e para cima
        const float ForwardSpeed = FMath::RandRange(200.0f, 400.0f);
        const float UpSpeed      = FMath::RandRange(300.0f, 500.0f);
        const FVector ThrowVelocity = ThrowDir * ForwardSpeed + FVector(0, 0, UpSpeed);

        StaticMeshComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
        StaticMeshComponent->AddImpulse(ThrowVelocity, NAME_None, true);
    }

    // Reinicia o timer de auto-destruição
    if (HasAuthority() && AutoDestroyAfterSeconds > 0.0f)
    {
        GetWorldTimerManager().ClearTimer(AutoDestroyTimerHandle);
        Internal_StartAutoDestroyTimer();
    }
}

void AZfItemPickup::DropNewItem()
{
    
}

// ============================================================
// RPCs
// ============================================================

bool AZfItemPickup::ServerRequestPickup_Validate(AActor* RequestingActor)
{
    // Retorna false APENAS para input nulo — isso sim indica cheating
    // Verificação de distância foi movida para _Implementation
    // para evitar kick indevido por discrepância de predição de movimento
    return RequestingActor != nullptr;
}

void AZfItemPickup::ServerRequestPickup_Implementation(AActor* RequestingActor)
{
    if (!RequestingActor) return;
    TryCollectItem(RequestingActor);
}

void AZfItemPickup::MulticastOnItemCollected_Implementation(AActor* CollectorActor)
{
    // Executado em todos os clientes e no servidor
    // Aqui devem ser disparados efeitos visuais e sonoros de coleta

    UE_LOG(LogZfInventory, Log, TEXT("AZfItemPickup::MulticastOnItemCollected — " "Item coletado por '%s'. Efeitos visuais/sonoros aqui."),
        CollectorActor ? *CollectorActor->GetName() : TEXT("Unknown"));

    // Esconde os meshes e widget imediatamente nos clientes
    // O servidor destruirá o ator em 0.5 segundos
    if (StaticMeshComponent)
    {
        StaticMeshComponent->SetVisibility(false);
    }
}

// ============================================================
// REP NOTIFIES
// ============================================================



void AZfItemPickup::OnRep_ItemInstance()
{
    // Executado nos clientes quando o ItemInstance é replicado
    // Atualiza o mesh e o widget com os novos dados

    if (ItemInstance)
    {
        UE_LOG(LogZfInventory, Verbose, TEXT("AZfItemPickup::OnRep_ItemInstance — " "ItemInstance replicado: '%s'"),
            *ItemInstance->GetItemName().ToString());
    }
}

// ============================================================
// CALLBACKS DE OVERLAP
// ============================================================

void AZfItemPickup::OnOverlapBegin(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == this)
    {
        return;
    }

    // Se não requer interação, coleta automaticamente no servidor
    if (!bRequiresInteractionToPickup && HasAuthority())
    {
        TryCollectItem(OtherActor);
    }
}

void AZfItemPickup::OnOverlapEnd(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
    if (!OtherActor || OtherActor == this)
    {
        return;
    }
}

// ============================================================
// CONSULTA
// ============================================================

UZfItemDefinition* AZfItemPickup::GetItemDefinition() const
{
    if (!ItemInstance)
    {
        return nullptr;
    }
    return ItemInstance->GetItemDefinition();
}

// ============================================================
// FUNÇÕES INTERNAS
// ============================================================

void AZfItemPickup::Internal_StartAutoDestroyTimer()
{
    if (AutoDestroyAfterSeconds <= 0.0f)
    {
        return;
    }

    GetWorldTimerManager().SetTimer(
        AutoDestroyTimerHandle,
        this,
        &AZfItemPickup::Internal_OnAutoDestroyTimerExpired,
        AutoDestroyAfterSeconds,
        false);

    UE_LOG(LogZfInventory, Log, TEXT("AZfItemPickup::Internal_StartAutoDestroyTimer — " "Pickup será destruído em %.0f segundos."),
        AutoDestroyAfterSeconds);
}

void AZfItemPickup::Internal_OnAutoDestroyTimerExpired()
{
    UE_LOG(LogZfInventory, Log,
        TEXT("AZfItemPickup::Internal_OnAutoDestroyTimerExpired — "
             "Pickup '%s' destruído por timeout."),
        ItemInstance
            ? *ItemInstance->GetItemName().ToString()
            : TEXT("Unknown"));

    // Destrói o ator — o item é perdido
    Destroy();
}

bool AZfItemPickup::Internal_GetCollectorInventory(AActor* CollectorActor, UZfInventoryComponent*& OutInventory) const
{
    OutInventory = nullptr;

    if (!CollectorActor)
    {
        return false;
    }
    APawn* Pawn = Cast<APawn>(CollectorActor);
    
    if (!Pawn)
    {
        return false;
    }
    AZfPlayerState* PS = Pawn->GetPlayerState<AZfPlayerState>();
    
    if (!PS)
    {
        return false;
    }
    
    OutInventory = PS->FindComponentByClass<UZfInventoryComponent>();

    return OutInventory != nullptr;
}

bool AZfItemPickup::Internal_CheckIsServer(const FString& FunctionName) const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogZfInventory, Error, TEXT("AZfItemPickup::%s — GetWorld() retornou nulo."), *FunctionName);
        return false;
    }

    if (World->GetNetMode() == NM_Client)
    {
        UE_LOG(LogZfInventory, Warning, TEXT("AZfItemPickup::%s — " "Operação de servidor chamada no cliente!"), *FunctionName);
        return false;
    }

    return true;
}