// Copyright ZfGame Studio. All Rights Reserved.
// ZfGatherSkillCheckWidget.cpp

#include "GatheringSystem/ZfGatheringSkillCheckWidget.h"

#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GatheringSystem/ZfGatheringResourceData.h"
#include "Tags/ZfGatheringTags.h"

// ============================================================
// NativeConstruct
// ============================================================

void UZfGatheringSkillCheckWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Estado inicial — aguarda InitSkillCheck()
    State        = EZfSkillCheckState::Idle;
    CurrentAngle = 0.0f;
}

// ============================================================
// InitSkillCheck
// ============================================================

void UZfGatheringSkillCheckWidget::InitSkillCheck(
    UZfGatheringResourceData* InResourceData,
    const FGameplayTag&    InToolTag,
    AActor*                InOwnerActor)
{
    if (!InResourceData || !InOwnerActor)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ZfGatherSkillCheckWidget::InitSkillCheck — "
                 "ResourceData ou OwnerActor nulo."));
        return;
    }

    // Guarda referências
    ResourceData = InResourceData;
    OwnerActor   = InOwnerActor;
    ToolTag      = InToolTag;

    // Resolve a entrada da ferramenta
    ToolEntry = ResourceData->FindToolEntry(ToolTag);
    if (!ToolEntry)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ZfGatherSkillCheckWidget::InitSkillCheck — "
                 "ToolTag '%s' não encontrada no ResourceData."),
            *ToolTag.ToString());
        return;
    }

    // Guarda tamanhos das zonas da ferramenta
    CurrentGoodSize    = ToolEntry->GoodSize;
    CurrentPerfectSize = ToolEntry->PerfectSize;

    // Calcula velocidade: 1 volta por NeedleRotationTime
    AngularSpeed = 1.0f / FMath::Max(ResourceData->NeedleRotationTime, 0.1f);

    // Cria o Dynamic Material Instance a partir do ZoneImage
    if (ZoneImage)
    {
        SkillCheckMID = ZoneImage->GetDynamicMaterial();
        if (!SkillCheckMID)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("ZfGatherSkillCheckWidget::InitSkillCheck — "
                     "Falha ao criar DynamicMaterial. "
                     "Verifique se ZoneImage tem um material configurado."));
            return;
        }

        // Parâmetros fixos — não mudam entre rounds
        SkillCheckMID->SetScalarParameterValue(
            TEXT("AngleOffset"), AngleOffset);
    }

    State = EZfSkillCheckState::Idle;
}

// ============================================================
// StartRound
// ============================================================

void UZfGatheringSkillCheckWidget::StartRound()
{
    if (!SkillCheckMID || !ToolEntry)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ZfGatherSkillCheckWidget::StartRound — "
                 "Não inicializado. Chame InitSkillCheck() primeiro."));
        return;
    }

    // Reseta o ponteiro
    CurrentAngle = 0.0f;

    // Sorteia posição aleatória para a zona Good
    CurrentGoodStart = FMath::FRand();

    // Perfect centralizado dentro do Good
    CurrentPerfectStart = FMath::Fmod(
        CurrentGoodStart + (CurrentGoodSize - CurrentPerfectSize) * 0.5f,
        1.0f);

    // Atualiza o material com as novas zonas
    UpdateMaterialZones();

    // Começa a girar
    State = EZfSkillCheckState::Running;
}

// ============================================================
// OnInteractPressed
// ============================================================

void UZfGatheringSkillCheckWidget::OnInteractPressed()
{
    if (State != EZfSkillCheckState::Running)
        return;

    ProcessResult(EvaluateHit());
}

// ============================================================
// NativeTick
// ============================================================

void UZfGatheringSkillCheckWidget::NativeTick(
    const FGeometry& MyGeometry, float DeltaTime)
{
    Super::NativeTick(MyGeometry, DeltaTime);

    switch (State)
    {
        case EZfSkillCheckState::Running:
        {
            // Avança o ângulo
            CurrentAngle += AngularSpeed * DeltaTime;

            // Atualiza rotação do ponteiro
            if (PointerImage)
            {
                PointerImage->SetRenderTransformAngle(CurrentAngle * 360.f);
            }

            // Volta completa sem input = Missed
            if (CurrentAngle >= 1.0f)
            {
                CurrentAngle = 0.0f;
                ProcessResult(EZfGatherHitResult::Missed);
            }
            break;
        }

        case EZfSkillCheckState::ShowingResult:
        {
            // Conta o tempo de feedback e volta para Idle
            // (GA chama StartRound() quando quiser o próximo)
            ResultFeedbackTimer -= DeltaTime;
            if (ResultFeedbackTimer <= 0.0f)
            {
                State = EZfSkillCheckState::Idle;
            }
            break;
        }

        default:
            break;
    }
}

// ============================================================
// EvaluateHit
// ============================================================

EZfGatherHitResult UZfGatheringSkillCheckWidget::EvaluateHit() const
{
    // Aplica o AngleOffset para que a avaliação bata com o visual
    const float AdjustedAngle = FMath::Fmod(
        CurrentAngle + AngleOffset, 1.0f);

    if (IsAngleInRange(AdjustedAngle, CurrentPerfectStart, CurrentPerfectSize))
        return EZfGatherHitResult::Perfect;

    if (IsAngleInRange(AdjustedAngle, CurrentGoodStart, CurrentGoodSize))
        return EZfGatherHitResult::Good;

    return EZfGatherHitResult::Bad;
}

// ============================================================
// IsAngleInRange
// ============================================================

bool UZfGatheringSkillCheckWidget::IsAngleInRange(
    float Angle, float Start, float Size) const
{
    const float End = Start + Size;

    // Sem wrap
    if (End <= 1.0f)
        return Angle >= Start && Angle <= End;

    // Com wrap (zona cruza o ponto 0)
    return Angle >= Start || Angle <= (End - 1.0f);
}

// ============================================================
// UpdateMaterialZones
// ============================================================

void UZfGatheringSkillCheckWidget::UpdateMaterialZones() const
{
    if (!SkillCheckMID)
        return;

    SkillCheckMID->SetScalarParameterValue(
        TEXT("GoodStart"),    CurrentGoodStart);
    SkillCheckMID->SetScalarParameterValue(
        TEXT("GoodSize"),     CurrentGoodSize);
    SkillCheckMID->SetScalarParameterValue(
        TEXT("PerfectStart"), CurrentPerfectStart);
    SkillCheckMID->SetScalarParameterValue(
        TEXT("PerfectSize"),  CurrentPerfectSize);
}

// ============================================================
// ProcessResult
// ============================================================

void UZfGatheringSkillCheckWidget::ProcessResult(EZfGatherHitResult Result)
{
    // Para o ponteiro
    State = EZfSkillCheckState::ShowingResult;
    ResultFeedbackTimer = ResultFeedbackDuration;

    // Dispara feedback visual no Blueprint
    switch (Result)
    {
        case EZfGatherHitResult::Perfect: OnResultPerfect(); break;
        case EZfGatherHitResult::Good:    OnResultGood();    break;
        case EZfGatherHitResult::Bad:     OnResultBad();     break;
        case EZfGatherHitResult::Missed:  OnResultMissed();  break;
        default: break;
    }

    // Envia o evento para a GA
    BroadcastResultEvent(Result);
}

// ============================================================
// BroadcastResultEvent
// ============================================================

void UZfGatheringSkillCheckWidget::BroadcastResultEvent(
    EZfGatherHitResult Result) const
{
    if (!OwnerActor)
        return;

    FGameplayEventData Payload;
    Payload.EventTag      = GetTagForResult(Result);
    Payload.Instigator    = OwnerActor;
    Payload.EventMagnitude = 0.0f;

    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
        OwnerActor,
        Payload.EventTag,
        Payload);
}

// ============================================================
// GetTagForResult
// ============================================================

FGameplayTag UZfGatheringSkillCheckWidget::GetTagForResult(
    EZfGatherHitResult Result)
{
    switch (Result)
    {
        case EZfGatherHitResult::Perfect:
            return ZfGatheringTags::QTE::Gathering_QTE_Hit_Perfect;
        case EZfGatherHitResult::Good:
            return ZfGatheringTags::QTE::Gathering_QTE_Hit_Good;
        case EZfGatherHitResult::Bad:
            return ZfGatheringTags::QTE::Gathering_QTE_Hit_Bad;
        case EZfGatherHitResult::Missed:
            return ZfGatheringTags::QTE::Gathering_QTE_Hit_Missed;
        default:
            return ZfGatheringTags::QTE::Gathering_QTE_Hit_Missed;
    }
}