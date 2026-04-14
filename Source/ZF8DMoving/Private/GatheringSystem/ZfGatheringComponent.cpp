// Copyright ZfGame Studio. All Rights Reserved.
// ZfGatherableComponent.cpp

#include "GatheringSystem/ZfGatheringComponent.h"
#include "GatheringSystem/ZfGatheringResourceData.h"
#include "Engine/World.h"

// ============================================================
// Constructor
// ============================================================

UZfGatheringComponent::UZfGatheringComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    // Component precisa replicar para o HP e estado funcionarem em multiplayer
    SetIsReplicatedByDefault(true);
}

// ============================================================
// BeginPlay
// ============================================================

void UZfGatheringComponent::BeginPlay()
{
    Super::BeginPlay();

    // Inicializa o HP com o valor do DataAsset
    // Feito aqui para garantir que o DataAsset já está carregado
    if (GatherResourceData)
    {
        CurrentHP = GatherResourceData->ResourceHP;
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ZfGatherableComponent::BeginPlay — GatherResourceData não configurado em '%s'."),
            *GetOwner()->GetName());
    }
}

// ============================================================
// GetLifetimeReplicatedProps
// ============================================================

void UZfGatheringComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UZfGatheringComponent, bIsDepleted);
    DOREPLIFETIME(UZfGatheringComponent, CurrentHP);
}

// ============================================================
// GetMaxHP
// ============================================================

float UZfGatheringComponent::GetMaxHP() const
{
    if (!GatherResourceData)
    {
        return 0.0f;
    }
    return GatherResourceData->ResourceHP;
}

// ============================================================
// GetHPPercent
// ============================================================

float UZfGatheringComponent::GetHPPercent() const
{
    const float MaxHP = GetMaxHP();
    if (MaxHP <= 0.0f)
    {
        return 0.0f;
    }
    return FMath::Clamp(CurrentHP / MaxHP, 0.0f, 1.0f);
}

// ============================================================
// GetRespawnTimeRemaining
// ============================================================

float UZfGatheringComponent::GetRespawnTimeRemaining() const
{
    if (!bIsDepleted) return 0.0f;

    UWorld* World = GetWorld();
    if (!World) return 0.0f;

    return World->GetTimerManager().GetTimerRemaining(RespawnTimerHandle);
}

// ============================================================
// ApplyDamage
// ============================================================

void UZfGatheringComponent::ApplyDamage(float DamageAmount)
{
    // Apenas o servidor aplica dano — replicado para os clientes
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    if (bIsDepleted || DamageAmount <= 0.0f)
    {
        return;
    }

    // Aplica o dano e garante que não vai abaixo de zero
    CurrentHP = FMath::Max(0.0f, CurrentHP - DamageAmount);

    // Dispara o delegate de HP no servidor
    // Clientes recebem via OnRep_CurrentHP
    OnResourceHPChanged.Broadcast(CurrentHP, GetMaxHP());

    // Se HP chegou a zero, esgota o recurso
    if (CurrentHP <= 0.0f)
    {
        Deplete();
    }
}

// ============================================================
// Deplete
// ============================================================

void UZfGatheringComponent::Deplete()
{
    if (bIsDepleted) return;

    bIsDepleted = true;
    CurrentHP   = 0.0f;

    // Servidor dispara o delegate localmente
    // Clientes recebem via OnRep_IsDepleted
    OnResourceDepleted.Broadcast();

    if (!GatherResourceData || !GatherResourceData->bCanRespawn)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World) return;

    World->GetTimerManager().SetTimer(
        RespawnTimerHandle,
        this,
        &UZfGatheringComponent::Internal_OnRespawnTimerExpired,
        GatherResourceData->RespawnTime,
        false);
}

// ============================================================
// ForceRespawn
// ============================================================

void UZfGatheringComponent::ForceRespawn()
{
    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(RespawnTimerHandle);
    }

    Internal_OnRespawnTimerExpired();
}

// ============================================================
// OnRep_IsDepleted
// ============================================================

void UZfGatheringComponent::OnRep_IsDepleted()
{
    // Clientes reagem visualmente à mudança de estado
    if (bIsDepleted)
    {
        OnResourceDepleted.Broadcast();
    }
    else
    {
        OnResourceRespawned.Broadcast();
    }
}

// ============================================================
// OnRep_CurrentHP
// ============================================================

void UZfGatheringComponent::OnRep_CurrentHP()
{
    // Clientes disparam o delegate de HP para atualizar UI
    OnResourceHPChanged.Broadcast(CurrentHP, GetMaxHP());
}

// ============================================================
// Internal_OnRespawnTimerExpired
// ============================================================

void UZfGatheringComponent::Internal_OnRespawnTimerExpired()
{
    bIsDepleted = false;

    // Restaura o HP ao máximo quando o recurso respawna
    if (GatherResourceData)
    {
        CurrentHP = GatherResourceData->ResourceHP;
    }

    // Servidor dispara localmente, clientes recebem via OnRep
    OnResourceRespawned.Broadcast();
}