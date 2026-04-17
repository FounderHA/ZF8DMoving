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
    // Tick desligado por padrão — ligado apenas durante um round ativo
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
// Roda apenas enquanto bRoundActive = true.
// Avança o ângulo, notifica a widget e detecta volta completa (Missed).
// ============================================================

void UZfGatheringComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bRoundActive) return;

    CurrentAngle += AngularSpeed * DeltaTime;

    // Notifica a widget para girar o ponteiro
    OnSkillCheckAngleUpdated.Broadcast(CurrentAngle);

    // Volta completa sem input = Missed
    if (CurrentAngle >= 1.0f)
    {
        bRoundActive = false;
        CurrentAngle = 0.0f;

        // Avalia como Missed, dispara delegates e envia GameplayEvent
        const EZfGatherHitResult Result = EZfGatherHitResult::Missed;
        OnSkillCheckHitEvaluated.Broadcast(Result);
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
}

// ============================================================
// GetMaxHP
// ============================================================

float UZfGatheringComponent::GetMaxHP() const
{
    return GatherResourceData ? GatherResourceData->ResourceHP : 0.0f;
}

// ============================================================
// GetHPPercent
// ============================================================

float UZfGatheringComponent::GetHPPercent() const
{
    const float MaxHP = GetMaxHP();
    if (MaxHP <= 0.0f) return 0.0f;
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
// Gera zonas aleatórias, liga o Tick e notifica a widget.
// ============================================================

void UZfGatheringComponent::BeginSkillCheckRound(
    float InGoodSize,
    float InPerfectSize,
    float InNeedleRotTime,
    AActor* InInstigator)
{
    CachedInstigator = InInstigator;

    RoundGoodSize    = InGoodSize;
    RoundPerfectSize = InPerfectSize;

    // Gera posição aleatória para a zona Good
    RoundGoodStart = FMath::FRand();

    // Perfect centralizado dentro do Good
    RoundPerfectStart = FMath::Fmod(
        RoundGoodStart + (RoundGoodSize - RoundPerfectSize) * 0.5f,
        1.0f);

    // Velocidade: 1 volta por NeedleRotTime segundos
    AngularSpeed = 1.0f / FMath::Max(InNeedleRotTime, 0.1f);

    // Reseta o ângulo e ativa o Tick
    CurrentAngle = 0.0f;
    bRoundActive = true;
    SetComponentTickEnabled(true);

    // Notifica a widget para posicionar as zonas
    OnSkillCheckRoundBegun.Broadcast(
        RoundGoodStart,
        RoundGoodSize,
        RoundPerfectStart,
        RoundPerfectSize);

    UE_LOG(LogTemp, Warning, TEXT("GatherComponent: Round iniciado | GoodStart=%.2f | PerfectStart=%.2f | Speed=%.2f"),
    RoundGoodStart, RoundPerfectStart, AngularSpeed);
}

// ============================================================
// EndSkillCheck
// Para o Tick — chamado pela GA no cleanup.
// ============================================================

void UZfGatheringComponent::EndSkillCheck()
{
    bRoundActive = false;
    CurrentAngle = 0.0f;
    CachedInstigator = nullptr;
    SetComponentTickEnabled(false);
}

// ============================================================
// RegisterHit
// Chamado via GA::RegisterGatherHit() quando o jogador clica.
// Usa o CurrentAngle interno — não precisa de parâmetros.
// ============================================================

void UZfGatheringComponent::RegisterHit()
{
    if (!bRoundActive) return;

    // Para o Tick imediatamente
    bRoundActive = false;
    SetComponentTickEnabled(false);

    const EZfGatherHitResult Result = EvaluateAngle(CurrentAngle);

    // Notifica a widget para exibir o feedback
    OnSkillCheckHitEvaluated.Broadcast(Result);

    // Envia o GameplayEvent para a GA processar o golpe
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

    UE_LOG(LogTemp, Warning,
        TEXT("GatherComponent: Avaliacao | Angulo=%.3f | Resultado=%d"),
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
// ============================================================

void UZfGatheringComponent::Internal_SendHitEvent(EZfGatherHitResult Result)
{
    if (!CachedInstigator) return;

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
    Payload.Instigator     = CachedInstigator;
    Payload.EventMagnitude = 0.0f;

    UE_LOG(LogTemp, Warning, TEXT("GatherComponent: GameplayEvent enviado | Tag=%s"), *ResultTag.ToString());
    
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
        CachedInstigator,
        ResultTag,
        Payload);
}

// ============================================================
// OnRep_IsDepleted
// ============================================================

void UZfGatheringComponent::OnRep_IsDepleted()
{
    if (bIsDepleted)
        OnResourceDepleted.Broadcast();
    else
        OnResourceRespawned.Broadcast();
}

// ============================================================
// OnRep_CurrentHP
// ============================================================

void UZfGatheringComponent::OnRep_CurrentHP()
{
    OnResourceHPChanged.Broadcast(CurrentHP, GetMaxHP());
}


// ============================================================
// TryRegisterHit
// Chamado pelo OnInteract do objeto — seguro chamar a qualquer momento.
// ============================================================

void UZfGatheringComponent::TryRegisterHit(AActor* InstigatorPawn)
{
    // Ignora se o round não estiver ativo
    if (!bRoundActive) return;

    // Ignora se não for o jogador que iniciou este round
    if (CachedInstigator != InstigatorPawn) return;

    UE_LOG(LogTemp, Warning, TEXT("GatherComponent: TryRegisterHit | bRoundActive=%d | InstigatorMatch=%d"),
    bRoundActive, CachedInstigator == InstigatorPawn);

    UE_LOG(LogTemp, Warning, TEXT("GatherComponent: Hit registrado | Angulo=%.3f"), CurrentAngle);
    
    RegisterHit();
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