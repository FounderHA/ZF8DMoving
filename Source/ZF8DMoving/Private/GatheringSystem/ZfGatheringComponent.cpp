// Copyright ZfGame Studio. All Rights Reserved.
// ZfGatherableComponent.cpp

#include "GatheringSystem/ZfGatheringComponent.h"
#include "GatheringSystem/ZfGatheringResourceData.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Tags/ZfGatheringTags.h"
#include "Engine/World.h"

// ============================================================
// Constructor
// ============================================================

UZfGatheringComponent::UZfGatheringComponent()
{
    // Tick desligado por padrão — ligado apenas durante um round ativo.
    // Só processa no servidor (HasAuthority check no corpo do Tick).
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;

    SetIsReplicatedByDefault(true);
}

// ============================================================
// BeginPlay
// ============================================================

void UZfGatheringComponent::BeginPlay()
{
    Super::BeginPlay();

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
// TickComponent
// Roda apenas no servidor — é a fonte da verdade do ângulo.
// Avança CurrentAngle e detecta volta completa (Missed).
// ============================================================

void UZfGatheringComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Segurança: Tick só processa no servidor.
    // O cliente tem seu próprio tick visual local na widget — independente deste.
    if (!GetOwner()->HasAuthority()) return;

    if (!bRoundActive) return;

    CurrentAngle += AngularSpeed * DeltaTime;

    // Volta completa sem input = Missed
    if (CurrentAngle >= 1.0f)
    {
        bRoundActive = false;
        CurrentAngle = 0.0f;

        const EZfGatherHitResult Result = EZfGatherHitResult::Missed;

        // Atualiza LastHitData — replica ao cliente via OnRep_LastHitData
        LastHitData.Result = Result;
        LastHitData.HitIndex++;

        // Listen server: chama manualmente para o host receber o feedback visual
        OnRep_LastHitData();

        // Envia GameplayEvent para a GA processar o Missed
        Internal_SendHitEvent(Result);
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
    DOREPLIFETIME(UZfGatheringComponent, CurrentGatherer);
    DOREPLIFETIME(UZfGatheringComponent, RoundData);
    DOREPLIFETIME(UZfGatheringComponent, LastHitData);
}

// ============================================================
// GetMaxHP / GetHPPercent / GetRespawnTimeRemaining
// ============================================================

float UZfGatheringComponent::GetMaxHP() const
{
    return GatherResourceData ? GatherResourceData->ResourceHP : 0.0f;
}

float UZfGatheringComponent::GetHPPercent() const
{
    const float MaxHP = GetMaxHP();
    if (MaxHP <= 0.0f) return 0.0f;
    return FMath::Clamp(CurrentHP / MaxHP, 0.0f, 1.0f);
}

float UZfGatheringComponent::GetRespawnTimeRemaining() const
{
    if (!bIsDepleted) return 0.0f;
    UWorld* World = GetWorld();
    if (!World) return 0.0f;
    return World->GetTimerManager().GetTimerRemaining(RespawnTimerHandle);
}

// ============================================================
// StartGatheringLock
// Trava o recurso para o Instigator — apenas no servidor.
// ============================================================

void UZfGatheringComponent::StartGatheringLock(AActor* Instigator)
{
    if (!GetOwner()->HasAuthority()) return;

    CurrentGatherer = Instigator;

    UE_LOG(LogTemp, Log,
        TEXT("ZfGatherableComponent: Lock adquirido por '%s' em '%s'."),
        *GetNameSafe(Instigator),
        *GetOwner()->GetName());
}

// ============================================================
// Internal_ReleaseGatheringLock
// Libera o lock — chamado por EndSkillCheck.
// ============================================================

void UZfGatheringComponent::Internal_ReleaseGatheringLock()
{
    if (!GetOwner()->HasAuthority()) return;

    UE_LOG(LogTemp, Log,
        TEXT("ZfGatherableComponent: Lock liberado em '%s'."),
        *GetOwner()->GetName());

    CurrentGatherer = nullptr;
}

// ============================================================
// ApplyDamage
// ============================================================

void UZfGatheringComponent::ApplyDamage(float DamageAmount)
{
    if (!GetOwner()->HasAuthority()) return;
    if (bIsDepleted || DamageAmount <= 0.0f) return;

    CurrentHP = FMath::Max(0.0f, CurrentHP - DamageAmount);
    OnResourceHPChanged.Broadcast(CurrentHP, GetMaxHP());

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

    OnResourceDepleted.Broadcast();

    if (!GatherResourceData || !GatherResourceData->bCanRespawn) return;

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
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(RespawnTimerHandle);
    }
    Internal_OnRespawnTimerExpired();
}

// ============================================================
// BeginSkillCheckRound
// Gera zonas aleatórias no servidor, seta RoundData que replica ao cliente.
// O cliente recebe OnRep_RoundData → dispara OnSkillCheckRoundBegun → widget reage.
// ============================================================

void UZfGatheringComponent::BeginSkillCheckRound(
    float InGoodSize,
    float InPerfectSize,
    float InNeedleRotTime)
{
    // Este método deve ser chamado apenas pelo servidor
    if (!GetOwner()->HasAuthority()) return;

    // Armazena zonas locais (servidor) para avaliação
    RoundGoodSize    = InGoodSize;
    RoundPerfectSize = InPerfectSize;
    RoundGoodStart   = FMath::FRand();
    RoundPerfectStart = FMath::Fmod(
        RoundGoodStart + (RoundGoodSize - RoundPerfectSize) * 0.5f, 1.0f);

    // Velocidade: 1 volta por InNeedleRotTime segundos
    AngularSpeed = 1.0f / FMath::Max(InNeedleRotTime, 0.1f);

    // Reseta o ângulo e ativa o Tick
    CurrentAngle = 0.0f;
    bRoundActive = true;
    SetComponentTickEnabled(true);

    RoundData.GoodStart     = RoundGoodStart;
    RoundData.GoodSize      = RoundGoodSize;
    RoundData.PerfectStart  = RoundPerfectStart;
    RoundData.PerfectSize   = RoundPerfectSize;
    RoundData.NeedleRotTime = InNeedleRotTime;
    RoundData.RoundIndex++;

    // Listen server: RepNotify não dispara no próprio servidor.
    // Chamamos manualmente para que o host também receba o evento visual.
    // Clientes remotos recebem via replicação normalmente.
    OnRep_RoundData();

    UE_LOG(LogTemp, Log,
        TEXT("ZfGatherableComponent: Round %d iniciado | GoodStart=%.2f | PerfectStart=%.2f | Speed=%.2f"),
        RoundData.RoundIndex, RoundGoodStart, RoundPerfectStart, AngularSpeed);
}

// ============================================================
// EndSkillCheck
// Para o Tick e libera o lock. Chamado pela GA no cleanup.
// ============================================================

void UZfGatheringComponent::EndSkillCheck()
{
    bRoundActive = false;
    CurrentAngle = 0.0f;
    SetComponentTickEnabled(false);

    Internal_ReleaseGatheringLock();
}

// ============================================================
// RegisterHit
// Avalia o ângulo ATUAL DO SERVIDOR — sem dados do cliente.
// Chamado via GA::Server_RegisterHit_Implementation.
// ============================================================

void UZfGatheringComponent::RegisterHit()
{
    if (!bRoundActive) return;
    if (!GetOwner()->HasAuthority()) return;

    // Para o Tick imediatamente — impede avanço adicional do ângulo
    bRoundActive = false;
    SetComponentTickEnabled(false);

    // Avalia usando o ângulo interno do servidor — fonte da verdade
    const EZfGatherHitResult Result = EvaluateAngle(CurrentAngle);

    // Atualiza LastHitData — replica ao cliente via OnRep_LastHitData
    LastHitData.Result = Result;
    LastHitData.HitIndex++;

    // Listen server: chama manualmente para o host receber o feedback visual
    OnRep_LastHitData();

    // Envia o GameplayEvent para a GA processar o golpe no servidor
    Internal_SendHitEvent(Result);
}

// ============================================================
// EvaluateAngle
// ============================================================

EZfGatherHitResult UZfGatheringComponent::EvaluateAngle(float Angle) const
{
    EZfGatherHitResult Result = EZfGatherHitResult::Bad;

    if (IsAngleInRange(Angle, RoundPerfectStart, RoundPerfectSize))
        Result = EZfGatherHitResult::Perfect;
    else if (IsAngleInRange(Angle, RoundGoodStart, RoundGoodSize))
        Result = EZfGatherHitResult::Good;

    UE_LOG(LogTemp, Log,
        TEXT("ZfGatherableComponent: Avaliação | Ângulo=%.3f | Resultado=%d"),
        Angle, (int32)Result);

    return Result;
}

// ============================================================
// IsAngleInRange
// ============================================================

bool UZfGatheringComponent::IsAngleInRange(
    float Angle, float Start, float Size) const
{
    const float End = Start + Size;

    if (End <= 1.0f)
        return Angle >= Start && Angle <= End;

    // Wrap — zona cruza o ponto 0
    return Angle >= Start || Angle <= (End - 1.0f);
}

// ============================================================
// Internal_SendHitEvent
// Envia o GameplayEvent para o ASC do CurrentGatherer (instigador).
// ============================================================

void UZfGatheringComponent::Internal_SendHitEvent(EZfGatherHitResult Result)
{
    // CurrentGatherer é o pawn do jogador — usado como destino do GameplayEvent
    if (!CurrentGatherer) return;

    FGameplayTag ResultTag;
    switch (Result)
    {
        case EZfGatherHitResult::Perfect:
            ResultTag = ZfGatheringTags::QTE::Gathering_QTE_Hit_Perfect; break;
        case EZfGatherHitResult::Good:
            ResultTag = ZfGatheringTags::QTE::Gathering_QTE_Hit_Good;    break;
        case EZfGatherHitResult::Bad:
            ResultTag = ZfGatheringTags::QTE::Gathering_QTE_Hit_Bad;     break;
        default:
            ResultTag = ZfGatheringTags::QTE::Gathering_QTE_Hit_Missed;  break;
    }

    FGameplayEventData Payload;
    Payload.EventTag       = ResultTag;
    Payload.Instigator     = CurrentGatherer;
    Payload.EventMagnitude = 0.0f;

    UE_LOG(LogTemp, Log,
        TEXT("ZfGatherableComponent: GameplayEvent enviado | Tag=%s"),
        *ResultTag.ToString());

    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
        CurrentGatherer,
        ResultTag,
        Payload);
}

// ============================================================
// REP NOTIFIES
// ============================================================

void UZfGatheringComponent::OnRep_IsDepleted()
{
    if (bIsDepleted)
        OnResourceDepleted.Broadcast();
    else
        OnResourceRespawned.Broadcast();
}

void UZfGatheringComponent::OnRep_CurrentHP()
{
    OnResourceHPChanged.Broadcast(CurrentHP, GetMaxHP());
}

void UZfGatheringComponent::OnRep_CurrentGatherer()
{
    // Reservado para uso futuro.
    // Ex: ocultar widget de interação para outros jogadores quando ocupado.
}

void UZfGatheringComponent::OnRep_RoundData()
{
    // O cliente recebeu dados de novo round — notifica a widget.
    // A widget inicia o tick visual local com a velocidade correta.
    OnSkillCheckRoundBegun.Broadcast(
        RoundData.GoodStart,
        RoundData.GoodSize,
        RoundData.PerfectStart,
        RoundData.PerfectSize,
        RoundData.NeedleRotTime);

    UE_LOG(LogTemp, Log,
        TEXT("ZfGatherableComponent [Client]: OnRep_RoundData | Round=%d | GoodStart=%.2f"),
        RoundData.RoundIndex, RoundData.GoodStart);
}

void UZfGatheringComponent::OnRep_LastHitData()
{
    // O cliente recebeu o resultado do hit avaliado pelo servidor — notifica a widget.
    OnSkillCheckHitEvaluated.Broadcast(LastHitData.Result);

    UE_LOG(LogTemp, Log,
        TEXT("ZfGatherableComponent [Client]: OnRep_LastHitData | Hit=%d | Result=%d"),
        LastHitData.HitIndex, (int32)LastHitData.Result);
}

// ============================================================
// Internal_OnRespawnTimerExpired
// ============================================================

void UZfGatheringComponent::Internal_OnRespawnTimerExpired()
{
    bIsDepleted = false;

    if (GatherResourceData)
    {
        CurrentHP = GatherResourceData->ResourceHP;
    }

    OnResourceRespawned.Broadcast();
}