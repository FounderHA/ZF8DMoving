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
        UE_LOG(LogTemp, Warning,
            TEXT("ZfGatherSkillCheckWidget::InitSkillCheck — GatherComponent nulo."));
        return;
    }

    GatherComponent = InGatherComponent;

    GatherComponent->OnSkillCheckRoundBegun.AddDynamic(
        this, &UZfGatheringSkillCheckWidget::Internal_OnRoundBegun);

    GatherComponent->OnSkillCheckAngleUpdated.AddDynamic(
        this, &UZfGatheringSkillCheckWidget::Internal_OnAngleUpdated);

    GatherComponent->OnSkillCheckHitEvaluated.AddDynamic(
        this, &UZfGatheringSkillCheckWidget::Internal_OnHitEvaluated);

    BP_OnSkillCheckReady();
}

// ============================================================
// NativeDestruct
// Remove os bindings quando a widget é destruída.
// ============================================================

void UZfGatheringSkillCheckWidget::NativeDestruct()
{
    if (GatherComponent)
    {
        GatherComponent->OnSkillCheckRoundBegun.RemoveDynamic(
            this, &UZfGatheringSkillCheckWidget::Internal_OnRoundBegun);

        GatherComponent->OnSkillCheckAngleUpdated.RemoveDynamic(
            this, &UZfGatheringSkillCheckWidget::Internal_OnAngleUpdated);

        GatherComponent->OnSkillCheckHitEvaluated.RemoveDynamic(
            this, &UZfGatheringSkillCheckWidget::Internal_OnHitEvaluated);
    }

    Super::NativeDestruct();
}

// ============================================================
// Internal_OnRoundBegun
// ============================================================

void UZfGatheringSkillCheckWidget::Internal_OnRoundBegun(
    float GoodStart, float GoodSize,
    float PerfectStart, float PerfectSize)
{
    BP_OnRoundStarted(GoodStart, GoodSize, PerfectStart, PerfectSize);
}

// ============================================================
// Internal_OnAngleUpdated
// ============================================================

void UZfGatheringSkillCheckWidget::Internal_OnAngleUpdated(float NormalizedAngle)
{
    BP_OnAngleUpdated(NormalizedAngle);
}

// ============================================================
// Internal_OnHitEvaluated
// ============================================================

void UZfGatheringSkillCheckWidget::Internal_OnHitEvaluated(EZfGatherHitResult Result)
{
    switch (Result)
    {
        case EZfGatherHitResult::Perfect: BP_OnResultPerfect(); break;
        case EZfGatherHitResult::Good:    BP_OnResultGood();    break;
        case EZfGatherHitResult::Bad:     BP_OnResultBad();     break;
        default:                          BP_OnResultMissed();  break;
    }
}