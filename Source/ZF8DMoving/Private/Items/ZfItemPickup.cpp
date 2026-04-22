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

    // Esfera de colisão como root — define área de coleta
    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    SetRootComponent(CollisionSphere);
    CollisionSphere->SetSphereRadius(16.0f);
    

    // Mesh estática — exibida quando item tem StaticMesh no Definition
    StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
    StaticMeshComponent->SetupAttachment(CollisionSphere);
    StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    StaticMeshComponent->SetRelativeLocation(FVector::ZeroVector);
    
    // Mesh esquelética — alternativa para itens com SkeletalMesh
    SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
    SkeletalMeshComponent->SetupAttachment(CollisionSphere);
    SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SkeletalMeshComponent->SetVisibility(false);

    // Widget 3D com informações do item
    ItemInfoWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("ItemInfoWidget"));
    ItemInfoWidget->SetupAttachment(CollisionSphere);
    ItemInfoWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
    ItemInfoWidget->SetWidgetSpace(EWidgetSpace::World);
    ItemInfoWidget->SetDrawSize(FVector2D(200.0f, 60.0f));
    ItemInfoWidget->SetVisibility(false);
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

    // Atualiza o widget com informações do item
    Internal_UpdateItemInfoWidget();

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
    const EZfItemMechanicResult Result = CollectorInventory->TryPickupItem(ItemInstance);
 
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
    else
    {
        UE_LOG(LogZfInventory, Warning, TEXT("AZfItemPickup::TryCollectItem — " "Falha ao coletar item: %s"), *UEnum::GetValueAsString(Result));
    }
 
    return Result;
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
    if (SkeletalMeshComponent)
    {
        SkeletalMeshComponent->SetVisibility(false);
    }
    if (ItemInfoWidget)
    {
        ItemInfoWidget->SetVisibility(false);
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
        Internal_UpdateItemInfoWidget();

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

    // Mostra o widget de interação ao entrar no raio
    if (ItemInfoWidget)
    {
        ItemInfoWidget->SetVisibility(true);
    }

    // Se não requer interação, coleta automaticamente no servidor
    if (!bRequiresInteractionToPickup && HasAuthority())
    {
        //TryCollectItem(OtherActor);
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

    // Esconde o widget ao sair do raio
    if (ItemInfoWidget)
    {
        ItemInfoWidget->SetVisibility(false);
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

void AZfItemPickup::Internal_UpdateItemInfoWidget()
{
    // O Widget Blueprint deve bindar ao GetItemInstance()
    // para exibir nome, ícone e raridade.
    // A implementação visual é feita no Widget Blueprint — aqui
    // apenas garantimos que o widget está atualizado.
    if (ItemInfoWidget && ItemInstance)
    {
        // Força o widget a atualizar — o Blueprint pode bindar
        // a GetItemInstance() diretamente
        ItemInfoWidget->RequestRedraw();
    }
}

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

// ============================================================
// DEBUG
// ============================================================

void AZfItemPickup::DrawDebugPickupInfo() const
{
    if (!GetWorld())
    {
        return;
    }

    const FVector Location = GetActorLocation();

    // Desenha o raio de coleta
    DrawDebugSphere(
        GetWorld(),
        Location,
        PickupRadius,
        16,
        FColor::Yellow,
        false,
        3.0f);

    // Exibe informações do item acima do pickup
    if (ItemInstance)
    {
        const FString InfoText = FString::Printf(
            TEXT("[Pickup]\n%s\nTier: %d | Quality: %d\nGUID: %s"),
            *ItemInstance->GetItemName().ToString(),
            ItemInstance->GetItemTier(),
            ItemInstance->GetItemQuality(),
            *ItemInstance->GetItemGuid().ToString());

        DrawDebugString(
            GetWorld(),
            Location + FVector(0.0f, 0.0f, 120.0f),
            InfoText,
            nullptr,
            FColor::Yellow,
            3.0f,
            false,
            1.0f);
    }
    else
    {
        DrawDebugString(
            GetWorld(),
            Location + FVector(0.0f, 0.0f, 80.0f),
            TEXT("[Pickup] Sem item"),
            nullptr,
            FColor::Red,
            3.0f,
            false,
            1.0f);
    }
}