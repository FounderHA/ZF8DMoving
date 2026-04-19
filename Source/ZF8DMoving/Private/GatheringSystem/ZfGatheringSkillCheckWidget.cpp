// Copyright ZfGame Studio. All Rights Reserved.
// ZfGatherSkillCheckWidget.cpp

#include "GatheringSystem/ZfGatheringSkillCheckWidget.h"
#include "GatheringSystem/ZfGatheringComponent.h"

// ============================================================
// InitSkillCheck
// ============================================================

void UZfGatheringSkillCheckWidget::InitSkillCheck(
    UZfGatheringComponent* InGatherComponent)
{
    if (!InGatherComponent)
    {
        UE_LOG(LogZfGathering, Warning,
            TEXT("ZfGatherSkillCheckWidget::InitSkillCheck — GatherComponent nulo."));
        return;
    }

    GatherComponent = InGatherComponent;

    GatherComponent->OnSkillCheckRoundBegun.AddDynamic(
        this, &UZfGatheringSkillCheckWidget::Internal_OnRoundBegun);

    GatherComponent->OnSkillCheckHitEvaluated.AddDynamic(
        this, &UZfGatheringSkillCheckWidget::Internal_OnHitEvaluated);

    BP_OnSkillCheckReady();

    // Inicialização tardia: se os dados de round já chegaram via replicação
    // antes da widget ser criada, exibe imediatamente sem esperar o próximo RepNotify.
    const FZfSkillCheckRoundData& Data = InGatherComponent->GetCurrentRoundData();
    if (Data.GoodSize > 0.f)
    {
        BP_OnRoundStarted(Data.GoodStart, Data.GoodSize, Data.PerfectStart, Data.PerfectSize);
        StartLocalTick(Data.NeedleRotTime);
    }
}

// ============================================================
// NativeDestruct
// ============================================================

void UZfGatheringSkillCheckWidget::NativeDestruct()
{
    StopLocalTick();

    if (GatherComponent)
    {
        GatherComponent->OnSkillCheckRoundBegun.RemoveDynamic(
            this, &UZfGatheringSkillCheckWidget::Internal_OnRoundBegun);

        GatherComponent->OnSkillCheckHitEvaluated.RemoveDynamic(
            this, &UZfGatheringSkillCheckWidget::Internal_OnHitEvaluated);
    }

    Super::NativeDestruct();
}

// ============================================================
// NativeTick — Tick visual local
// Avança LocalAngle e notifica o BP para girar o ponteiro.
// Este ângulo é puramente cosmético — o servidor usa o seu próprio.
// ============================================================

void UZfGatheringSkillCheckWidget::NativeTick(
    const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!bLocalTickActive) return;

    LocalAngle += LocalAngularSpeed * InDeltaTime;

    // Loop visual — reseta ao completar a volta sem parar o tick.
    // O servidor decide quando é Missed; a widget apenas continua girando
    // até receber o resultado via OnSkillCheckHitEvaluated.
    if (LocalAngle >= 1.0f)
        LocalAngle -= 1.0f;

    BP_OnAngleUpdated(LocalAngle);
}

// ============================================================
// StartLocalTick
// ============================================================

void UZfGatheringSkillCheckWidget::StartLocalTick(float InNeedleRotTime)
{
    LocalAngle        = 0.f;
    LocalAngularSpeed = 1.f / FMath::Max(InNeedleRotTime, 0.1f);
    bLocalTickActive  = true;
}

// ============================================================
// StopLocalTick
// ============================================================

void UZfGatheringSkillCheckWidget::StopLocalTick()
{
    bLocalTickActive = false;
}

// ============================================================
// Internal_OnRoundBegun
// Novo round chegou via RepNotify — posiciona zonas e inicia tick visual.
// ============================================================

void UZfGatheringSkillCheckWidget::Internal_OnRoundBegun(
    float GoodStart, float GoodSize,
    float PerfectStart, float PerfectSize,
    float NeedleRotTime)
{
    BP_OnRoundStarted(GoodStart, GoodSize, PerfectStart, PerfectSize);
    StartLocalTick(NeedleRotTime);
}

// ============================================================
// Internal_OnHitEvaluated
// Resultado chegou via RepNotify — para o tick visual e exibe feedback.
// ============================================================

void UZfGatheringSkillCheckWidget::Internal_OnHitEvaluated(EZfGatherHitResult Result)
{
    // NÃO para o tick local aqui — o servidor pode enviar RoundData e LastHitData
    // no mesmo frame, e a ordem de chegada no cliente é indeterminada.
    // Se StopLocalTick fosse chamado após StartLocalTick do próximo round,
    // a agulha pararia para sempre no round 2.
    //
    // O tick é reiniciado (e o ângulo resetado) automaticamente quando
    // Internal_OnRoundBegun chama StartLocalTick para o próximo round.
    // Se quiser pausar a agulha visualmente no hit, faça isso nos eventos
    // BP_OnResultX diretamente no Blueprint.

    switch (Result)
    {
        case EZfGatherHitResult::Perfect: BP_OnResultPerfect(); break;
        case EZfGatherHitResult::Good:    BP_OnResultGood();    break;
        case EZfGatherHitResult::Bad:     BP_OnResultBad();     break;
        default:                          BP_OnResultMissed();  break;
    }
}