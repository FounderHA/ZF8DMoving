#include "InteractionSystem/ZFInteractionComponent.h"
#include "InteractionSystem/ZfInteractionInterface.h"
#include "InteractionSystem/ZfInteractionWidget.h"
#include "InteractionSystem/ZfIndicatorWidget.h"
#include "Components/SphereComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"

// ─────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────

UZfInteractionComponent::UZfInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(false);
}

// ─────────────────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────────────────

void UZfInteractionComponent::BeginPlay()
{
    Super::BeginPlay();
    if (!IsLocallyControlled()) return;

    SetupAwarenessSphere();
    ReconcileCandidates();
}

void UZfInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearFocus(false);
    UnbindCurrentInput();

    for (auto& Pair : IndicatorWidgets)
        if (IsValid(Pair.Value)) Pair.Value->RemoveFromParent();
    IndicatorWidgets.Empty();

    if (IsValid(InteractionWidget))
        InteractionWidget->RemoveFromParent();

    Super::EndPlay(EndPlayReason);
}

void UZfInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!IsLocallyControlled()) return;

    ReconciliationTimer += DeltaTime;
    if (ReconciliationTimer >= ReconciliationInterval)
    {
        ReconciliationTimer = 0.f;
        ReconcileCandidates();
    }

    Candidates.RemoveAll([](const FZfCandidateInfo& C) { return !IsValid(C.Actor); });

    UpdateFocus(DeltaTime);

    if (ActiveHold.bActive)
        UpdateHold(DeltaTime);

    UpdateWidgetsPositions();
}

// ─────────────────────────────────────────────────────────────────────────
// Setup
// ─────────────────────────────────────────────────────────────────────────

void UZfInteractionComponent::SetupAwarenessSphere()
{
    AActor* Owner = GetOwner();
    if (!IsValid(Owner)) return;

    AwarenessSphere = NewObject<USphereComponent>(Owner, TEXT("AwarenessSphere"));
    AwarenessSphere->SetupAttachment(Owner->GetRootComponent());
    AwarenessSphere->InitSphereRadius(AwarenessRadius);

    // NÃO usar SetCollisionProfileName — ele reseta o ObjectType para WorldDynamic
    AwarenessSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    AwarenessSphere->SetCollisionObjectType(ECC_GameTraceChannel2); // InteractionSensor
    AwarenessSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    AwarenessSphere->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap);
    AwarenessSphere->SetGenerateOverlapEvents(true);
    AwarenessSphere->SetHiddenInGame(true); //Debug
    AwarenessSphere->SetVisibility(false); //Debug

    AwarenessSphere->RegisterComponent();

    AwarenessSphere->OnComponentBeginOverlap.AddDynamic(this, &UZfInteractionComponent::OnSphereBeginOverlap);
    AwarenessSphere->OnComponentEndOverlap.AddDynamic(this, &UZfInteractionComponent::OnSphereEndOverlap);
    
    AwarenessSphere->UpdateOverlaps();
}

void UZfInteractionComponent::ReconcileCandidates()
{
    if (!IsValid(AwarenessSphere)) return;

    TArray<AActor*> Overlapping;
    AwarenessSphere->GetOverlappingActors(Overlapping);

    for (AActor* Actor : Overlapping)
        if (IsValid(Actor) && Actor->Implements<UZfInteractionInterface>() && !HasCandidate(Actor))
            AddCandidate(Actor);

    TArray<AActor*> ToRemove;
    for (const FZfCandidateInfo& C : Candidates)
        if (!IsValid(C.Actor) || !Overlapping.Contains(C.Actor))
            ToRemove.Add(C.Actor);

    for (AActor* Actor : ToRemove)
        RemoveCandidate(Actor);
}

// ─────────────────────────────────────────────────────────────────────────
// Overlap
// ─────────────────────────────────────────────────────────────────────────

void UZfInteractionComponent::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (IsValid(OtherActor) && OtherActor->Implements<UZfInteractionInterface>() && !HasCandidate(OtherActor))
        AddCandidate(OtherActor);
}

void UZfInteractionComponent::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (IsValid(OtherActor))
        RemoveCandidate(OtherActor);
}

// ─────────────────────────────────────────────────────────────────────────
// Candidatos
// ─────────────────────────────────────────────────────────────────────────

void UZfInteractionComponent::AddCandidate(AActor* Actor)
{
    if (!IsValid(Actor)) return;

    FZfCandidateInfo Info;
    Info.Actor        = Actor;
    Info.Interactions = IZfInteractionInterface::Execute_GetInteractionDataArray(Actor);
    Candidates.Add(Info);

    IZfInteractionInterface::Execute_OnPlayerEnterRange(Actor, GetOwningController());
    CreateIndicatorFor(Actor);
}

void UZfInteractionComponent::RemoveCandidate(AActor* Actor)
{
    if (!IsValid(Actor)) return;

    const int32 Index = Candidates.IndexOfByPredicate(
        [Actor](const FZfCandidateInfo& C) { return C.Actor == Actor; });
    if (Index == INDEX_NONE) return;

    if (CurrentFocus == Actor)
        ClearFocus(true);

    IZfInteractionInterface::Execute_OnPlayerExitRange(Actor, GetOwningController());
    DestroyIndicatorFor(Actor);
    Candidates.RemoveAt(Index);
}

bool UZfInteractionComponent::HasCandidate(AActor* Actor) const
{
    return Candidates.ContainsByPredicate(
        [Actor](const FZfCandidateInfo& C) { return C.Actor == Actor; });
}

// ─────────────────────────────────────────────────────────────────────────
// Foco e Score
// ─────────────────────────────────────────────────────────────────────────

void UZfInteractionComponent::UpdateFocus(float DeltaTime)
{
    AActor*               BestActor = nullptr;
    float                 BestScore = -1.f;
    TArray<FInteractionData> BestInteractions;

    const FVector OwnerLocation = GetOwner()->GetActorLocation();

    // FIX: O filtro de ângulo agora usa o forward da câmera em vez do forward
    // do ator. Em third-person, o jogador interage com o que a câmera aponta,
    // não necessariamente com o que o personagem está voltado.
    APlayerController* PC = GetOwningController();
    if (!IsValid(PC)) return;

    FVector CamLoc; FRotator CamRot;
    PC->GetPlayerViewPoint(CamLoc, CamRot);
    const FVector CamForward = CamRot.Vector();

    for (FZfCandidateInfo& Candidate : Candidates)
    {
        if (!IsValid(Candidate.Actor)) continue;

        // Atualiza interações (objeto pode mudar dinamicamente)
        Candidate.Interactions = IZfInteractionInterface::Execute_GetInteractionDataArray(Candidate.Actor);
        if (Candidate.Interactions.IsEmpty()) continue;

        // FIX: Usa o MAIOR InteractionRadius entre as interações do objeto para
        // o filtro de distância de foco. O comportamento anterior usava o menor,
        // o que impedia o foco quando havia interações com raios diferentes —
        // o objeto só entrava em foco quando o player estivesse dentro do menor
        // raio, ignorando interações com raio maior que ainda estariam válidas.
        float LargestRadius = 0.f;
        for (const FInteractionData& Data : Candidate.Interactions)
            LargestRadius = FMath::Max(LargestRadius, Data.InteractionRadius);

        // Filtro 1: Distância (pelo maior raio disponível)
        const float Distance = FVector::Dist(OwnerLocation, Candidate.Actor->GetActorLocation());
        if (Distance > LargestRadius) continue;

        // Filtro 2: Ângulo (camera forward — FIX aplicado acima)
        const FVector Dir = (Candidate.Actor->GetActorLocation() - OwnerLocation).GetSafeNormal();
        if (FVector::DotProduct(CamForward, Dir) < MinFocusDot) continue;

        // Filtro 3: LOS Câmera
        if (!PassesCameraLOS(Candidate.Actor)) continue;

        // Filtro 4: LOS Player
        if (!PassesPlayerLOS(Candidate.Actor)) continue;

        // Score
        const float Score = ComputeScore(Candidate);
        if (Score > BestScore)
        {
            BestScore        = Score;
            BestActor        = Candidate.Actor;
            BestInteractions = Candidate.Interactions;
        }
    }

    if (BestActor != CurrentFocus)
    {
        if (IsValid(CurrentFocus)) ClearFocus(true);
        if (IsValid(BestActor))    SetFocus(BestActor, BestInteractions);
    }
}

float UZfInteractionComponent::ComputeScore(const FZfCandidateInfo& Candidate) const
{
    if (!IsValid(Candidate.Actor)) return 0.f;

    const FVector OwnerLocation     = GetOwner()->GetActorLocation();
    const FVector CandidateLocation = Candidate.Actor->GetActorLocation();

    float LargestRadius = 0.f;
    for (const FInteractionData& Data : Candidate.Interactions)
        LargestRadius = FMath::Max(LargestRadius, Data.InteractionRadius);

    const float Distance = FVector::Dist(OwnerLocation, CandidateLocation);
    const float NormDist = FMath::Clamp(1.f - (Distance / FMath::Max(LargestRadius, 1.f)), 0.f, 1.f);

    float Centrality = 0.f;
    if (APlayerController* PC = GetOwningController())
    {
        FVector CamLoc; FRotator CamRot;
        PC->GetPlayerViewPoint(CamLoc, CamRot);
        const FVector CamForward       = CamRot.Vector();
        const FVector DirectionToActor = (CandidateLocation - CamLoc).GetSafeNormal();
        Centrality = FMath::Clamp(FVector::DotProduct(CamForward, DirectionToActor), 0.f, 1.f);
    }

    return (NormDist * DistanceScoreWeight) + (Centrality * CentralityScoreWeight);
}

bool UZfInteractionComponent::PassesCameraLOS(AActor* Target) const
{
    APlayerController* PC = GetOwningController();
    if (!IsValid(PC) || !IsValid(Target)) return false;

    FVector CamLoc; FRotator CamRot;
    PC->GetPlayerViewPoint(CamLoc, CamRot);

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());
    Params.AddIgnoredActor(Target);

    // FIX: Usa LOSTraceChannel (UPROPERTY configurável) em vez de
    // ECC_GameTraceChannel1 hardcoded.
    return !GetWorld()->LineTraceSingleByChannel(
        Hit, CamLoc, Target->GetActorLocation(), LOSTraceChannel, Params);
}

bool UZfInteractionComponent::PassesPlayerLOS(AActor* Target) const
{
    if (!IsValid(GetOwner()) || !IsValid(Target)) return false;

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());
    Params.AddIgnoredActor(Target);

    // FIX: Usa LOSTraceChannel (UPROPERTY configurável) em vez de
    // ECC_GameTraceChannel1 hardcoded.
    return !GetWorld()->LineTraceSingleByChannel(
        Hit, GetOwner()->GetActorLocation(), Target->GetActorLocation(),
        LOSTraceChannel, Params);
}

// ─────────────────────────────────────────────────────────────────────────
// Gerenciamento de Foco
// ─────────────────────────────────────────────────────────────────────────

void UZfInteractionComponent::SetFocus(AActor* NewFocus, const TArray<FInteractionData>& Interactions)
{
    if (!IsValid(NewFocus)) return;

    CurrentFocus = NewFocus;

    if (ActiveHold.bActive) CancelHold();

    BindInputForFocus(Interactions);
    IZfInteractionInterface::Execute_OnFocusGained(NewFocus, GetOwningController());
    ShowInteractionWidget(NewFocus, Interactions);
}

void UZfInteractionComponent::ClearFocus(bool bNotifyObject)
{
    if (!IsValid(CurrentFocus)) return;

    if (ActiveHold.bActive) CancelHold();

    UnbindCurrentInput();

    if (bNotifyObject)
        IZfInteractionInterface::Execute_OnFocusLost(CurrentFocus, GetOwningController());

    HideInteractionWidget();
    CurrentFocus = nullptr;
}

// ─────────────────────────────────────────────────────────────────────────
// Input
// ─────────────────────────────────────────────────────────────────────────

void UZfInteractionComponent::BindInputForFocus(const TArray<FInteractionData>& Interactions)
{
    UnbindCurrentInput();

    UEnhancedInputComponent* EIC = GetEnhancedInputComponent();
    if (!IsValid(EIC)) return;

    APlayerController* PC = GetOwningController();
    UEnhancedInputLocalPlayerSubsystem* Sub = nullptr;
    if (IsValid(PC))
        Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());

    for (const FInteractionData& Data : Interactions)
    {
        if (!IsValid(Data.InputAction)) continue;

        if (IsValid(Data.MappingContext) && Sub &&
            !ActiveMappingContexts.Contains(Data.MappingContext))
        {
            Sub->AddMappingContext(Data.MappingContext, Data.MappingContextPriority);
            ActiveMappingContexts.Add(Data.MappingContext);
        }

        FName ID               = Data.InteractionID;
        EInteractionInputMode Mode = Data.InputMode;
        float HoldDur          = Data.HoldDuration;

        TArray<uint32> Handles;

        if (Data.InputMode == EInteractionInputMode::Press)
        {
            Handles.Add(EIC->BindAction(Data.InputAction, ETriggerEvent::Started,
                this, &UZfInteractionComponent::HandleInputStarted, ID, Mode, HoldDur).GetHandle());
        }
        else
        {
            Handles.Add(EIC->BindAction(Data.InputAction, ETriggerEvent::Started,
                this, &UZfInteractionComponent::HandleInputStarted, ID, Mode, HoldDur).GetHandle());
            Handles.Add(EIC->BindAction(Data.InputAction, ETriggerEvent::Completed,
                this, &UZfInteractionComponent::HandleInputCompleted, ID).GetHandle());
            Handles.Add(EIC->BindAction(Data.InputAction, ETriggerEvent::Canceled,
                this, &UZfInteractionComponent::HandleInputCompleted, ID).GetHandle());
        }

        ActiveInputHandles.Add(Data.InputAction, Handles);
        InputActionToInteractionID.Add(Data.InputAction, ID);
    }
}

void UZfInteractionComponent::UnbindCurrentInput()
{
    if (UEnhancedInputComponent* EIC = GetEnhancedInputComponent())
    {
        for (auto& Pair : ActiveInputHandles)
            for (uint32 Handle : Pair.Value)
                EIC->RemoveBindingByHandle(Handle);
    }
    ActiveInputHandles.Empty();
    InputActionToInteractionID.Empty();

    if (APlayerController* PC = GetOwningController())
    {
        if (auto* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
            PC->GetLocalPlayer()))
        {
            for (UInputMappingContext* MC : ActiveMappingContexts)
                if (IsValid(MC)) Sub->RemoveMappingContext(MC);
        }
    }
    ActiveMappingContexts.Empty();
}

void UZfInteractionComponent::HandleInputStarted(FName InteractionID, EInteractionInputMode Mode, float HoldDuration)
{
    if (!IsValid(CurrentFocus)) return;

    if (Mode == EInteractionInputMode::Press)
    {
        Server_Interact(CurrentFocus, InteractionID);
    }
    else
    {
        ActiveHold.InteractionID = InteractionID;
        ActiveHold.Elapsed       = 0.f;
        ActiveHold.Duration      = HoldDuration;
        ActiveHold.bActive       = true;

        // FIX: Cache do ator em foco no momento do início do hold.
        // Garante que CancelHold possa enviar o RPC mesmo se CurrentFocus
        // se tornar inválido antes do hold ser concluído ou cancelado.
        ActiveHold.HoldTarget = CurrentFocus;

        // FIX: OnInteractBegin agora vai via Server RPC, consistente com
        // OnInteract / OnInteractComplete / OnInteractCanceled.
        // A chamada local foi removida — a lógica roda no servidor.
        Server_InteractBegin(CurrentFocus, InteractionID);
    }
}

void UZfInteractionComponent::HandleInputCompleted(FName InteractionID)
{
    if (ActiveHold.bActive && ActiveHold.InteractionID == InteractionID)
        CancelHold();
}

// ─────────────────────────────────────────────────────────────────────────
// Hold
// ─────────────────────────────────────────────────────────────────────────

void UZfInteractionComponent::UpdateHold(float DeltaTime)
{
    if (!IsValid(CurrentFocus))
    {
        // FIX: Mesmo com CurrentFocus inválido, CancelHold agora consegue
        // enviar o RPC usando ActiveHold.HoldTarget (cacheado no início do hold).
        CancelHold();
        return;
    }

    ActiveHold.Elapsed += DeltaTime;
    const float Progress = FMath::Clamp(ActiveHold.Elapsed / ActiveHold.Duration, 0.f, 1.f);

    // OnInteractProgress permanece local — é exclusivamente cosmético
    // (feedback de UI/som para o jogador local), sem necessidade de RPC.
    IZfInteractionInterface::Execute_OnInteractProgress(CurrentFocus, GetOwningController(), ActiveHold.InteractionID, Progress);
    UpdateInteractionWidgetProgress(ActiveHold.InteractionID, Progress);

    if (ActiveHold.Elapsed >= ActiveHold.Duration)
    {
        const FName ID = ActiveHold.InteractionID;
        ActiveHold.bActive = false;
        Server_InteractComplete(CurrentFocus, ID);
    }
}

void UZfInteractionComponent::CancelHold()
{
    if (!ActiveHold.bActive) return;

    const FName  ID         = ActiveHold.InteractionID;
    AActor*      HoldTarget = ActiveHold.HoldTarget;

    ActiveHold = FZfActiveHoldInfo();

    // FIX: Usa HoldTarget (cacheado) em vez de CurrentFocus.
    // Antes: se o foco fosse perdido durante o hold (objeto saiu do range,
    // LOS quebrou), CurrentFocus já era nullptr/inválido aqui e o RPC
    // Server_InteractCanceled nunca era enviado — o objeto ficava num estado
    // de hold indefinido no servidor.
    // Agora: HoldTarget preserva a referência ao ator correto independente
    // do estado atual de CurrentFocus.
    if (IsValid(HoldTarget))
    {
        Server_InteractCanceled(HoldTarget, ID);
    }

    UpdateInteractionWidgetProgress(ID, 0.f);
}

// ─────────────────────────────────────────────────────────────────────────
// Server RPCs
// ─────────────────────────────────────────────────────────────────────────

void UZfInteractionComponent::Server_Interact_Implementation(AActor* TargetActor, FName InteractionID)
{
    if (!IsValid(TargetActor) || !TargetActor->Implements<UZfInteractionInterface>()) return;

    const TArray<FInteractionData> Interactions =
        IZfInteractionInterface::Execute_GetInteractionDataArray(TargetActor);

    const FInteractionData* Data = Interactions.FindByPredicate(
        [InteractionID](const FInteractionData& D) { return D.InteractionID == InteractionID; });
    if (!Data) return;

    const float Distance = FVector::Dist(GetOwner()->GetActorLocation(), TargetActor->GetActorLocation());
    
    if (Distance > Data->InteractionRadius * 1.1f) return;

    IZfInteractionInterface::Execute_OnInteract(TargetActor, GetOwningController(), InteractionID);
}

// FIX: Implementação do novo Server_InteractBegin.
// Mesma estrutura de validação dos outros RPCs: verifica ator, interface,
// InteractionID válido e distância dentro do raio (com margem de 10%).
void UZfInteractionComponent::Server_InteractBegin_Implementation(AActor* TargetActor, FName InteractionID)
{
    if (!IsValid(TargetActor) || !TargetActor->Implements<UZfInteractionInterface>()) return;

    const TArray<FInteractionData> Interactions =
        IZfInteractionInterface::Execute_GetInteractionDataArray(TargetActor);

    const FInteractionData* Data = Interactions.FindByPredicate(
        [InteractionID](const FInteractionData& D) { return D.InteractionID == InteractionID; });
    if (!Data) return;

    const float Distance = FVector::Dist(GetOwner()->GetActorLocation(), TargetActor->GetActorLocation());
    if (Distance > Data->InteractionRadius * 1.1f) return;

    IZfInteractionInterface::Execute_OnInteractBegin(TargetActor, GetOwningController(), InteractionID);
}

void UZfInteractionComponent::Server_InteractComplete_Implementation(AActor* TargetActor, FName InteractionID)
{
    if (!IsValid(TargetActor) || !TargetActor->Implements<UZfInteractionInterface>()) return;

    const TArray<FInteractionData> Interactions =
        IZfInteractionInterface::Execute_GetInteractionDataArray(TargetActor);

    const FInteractionData* Data = Interactions.FindByPredicate(
        [InteractionID](const FInteractionData& D) { return D.InteractionID == InteractionID; });
    if (!Data) return;

    const float Distance = FVector::Dist(GetOwner()->GetActorLocation(), TargetActor->GetActorLocation());
    if (Distance > Data->InteractionRadius * 1.1f) return;

    IZfInteractionInterface::Execute_OnInteractComplete(TargetActor, GetOwningController(), InteractionID);
}

void UZfInteractionComponent::Server_InteractCanceled_Implementation(AActor* TargetActor, FName InteractionID)
{
    if (!IsValid(TargetActor) || !TargetActor->Implements<UZfInteractionInterface>()) return;
    IZfInteractionInterface::Execute_OnInteractCanceled(TargetActor, GetOwningController(), InteractionID);
}

// ─────────────────────────────────────────────────────────────────────────
// Widgets
// ─────────────────────────────────────────────────────────────────────────

TSubclassOf<UUserWidget> UZfInteractionComponent::ResolveInteractionWidgetClass(AActor* Actor) const
{
    if (IsValid(Actor) && Actor->Implements<UZfInteractionInterface>())
    {
        TSubclassOf<UUserWidget> ActorClass =
            IZfInteractionInterface::Execute_GetInteractionWidgetClass(Actor);
        if (ActorClass) return ActorClass;
    }
    return DefaultInteractionWidgetClass;
}

TSubclassOf<UUserWidget> UZfInteractionComponent::ResolveIndicatorWidgetClass(AActor* Actor) const
{
    if (IsValid(Actor) && Actor->Implements<UZfInteractionInterface>())
    {
        TSubclassOf<UUserWidget> ActorClass =
            IZfInteractionInterface::Execute_GetIndicatorWidgetClass(Actor);
        if (ActorClass) return ActorClass;
    }
    return DefaultIndicatorWidgetClass;
}

void UZfInteractionComponent::CreateIndicatorFor(AActor* Actor)
{
    if (!IsValid(Actor) || IndicatorWidgets.Contains(Actor)) return;

    APlayerController* PC = GetOwningController();
    if (!IsValid(PC)) return;

    TSubclassOf<UUserWidget> WidgetClass = ResolveIndicatorWidgetClass(Actor);
    if (!WidgetClass) return;

    if (UZfIndicatorWidget* Widget = CreateWidget<UZfIndicatorWidget>(PC, WidgetClass))
    {
        Widget->TrackedActor = Actor;
        Widget->AddToViewport();
        Widget->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
        IndicatorWidgets.Add(Actor, Widget);
    }
}

void UZfInteractionComponent::DestroyIndicatorFor(AActor* Actor)
{
    if (UUserWidget** WidgetPtr = IndicatorWidgets.Find(Actor))
        if (IsValid(*WidgetPtr)) (*WidgetPtr)->RemoveFromParent();
    IndicatorWidgets.Remove(Actor);
}

void UZfInteractionComponent::UpdateWidgetsPositions()
{
    APlayerController* PC = GetOwningController();
    if (!IsValid(PC)) return;

    for (auto& Pair : IndicatorWidgets)
    {
        AActor*      Actor  = Pair.Key;
        UUserWidget* Widget = Pair.Value;
        if (!IsValid(Actor) || !IsValid(Widget)) continue;

        FVector2D ScreenPos;
        FVector2D IndicatorOffset = IZfInteractionInterface::Execute_GetIndicatorWidgetOffset(Actor);
        const bool bOnScreen = PC->ProjectWorldLocationToScreen(Actor->GetActorLocation(), ScreenPos, false);

        Widget->SetVisibility(bOnScreen ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);

        if (bOnScreen)
            Widget->SetPositionInViewport(ScreenPos + IndicatorOffset, true);
    }

    // ── InteractionWidget segue o foco ────────────────────────────────
    if (IsValid(InteractionWidget) && IsValid(CurrentFocus) &&
        InteractionWidget->GetVisibility() != ESlateVisibility::Hidden)
    {
        FVector2D ScreenPos;
        FVector2D InteractionOffset = IZfInteractionInterface::Execute_GetInteractionWidgetOffset(CurrentFocus);
        const bool bOnScreen = PC->ProjectWorldLocationToScreen(CurrentFocus->GetActorLocation(), ScreenPos, false);

        if (bOnScreen)
            InteractionWidget->SetPositionInViewport(ScreenPos + InteractionOffset, true);
        else
            InteractionWidget->SetVisibility(ESlateVisibility::Hidden);
    }
}

void UZfInteractionComponent::ShowInteractionWidget(AActor* FocusActor,
    const TArray<FInteractionData>& Interactions)
{
    APlayerController* PC = GetOwningController();
    if (!IsValid(PC)) return;

    TSubclassOf<UUserWidget> WidgetClass = ResolveInteractionWidgetClass(FocusActor);
    if (!WidgetClass) return;

    if (IsValid(InteractionWidget) && InteractionWidget->GetClass() != WidgetClass)
    {
        InteractionWidget->RemoveFromParent();
        InteractionWidget = nullptr;
    }

    if (!IsValid(InteractionWidget))
    {
        InteractionWidget = CreateWidget<UUserWidget>(PC, WidgetClass);
        if (IsValid(InteractionWidget))
        {
            InteractionWidget->AddToViewport();
            InteractionWidget->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
        }
    }

    if (UZfInteractionWidget* IW = Cast<UZfInteractionWidget>(InteractionWidget))
    {
        IW->SetVisibility(ESlateVisibility::HitTestInvisible);
        IW->SetInteractions(Interactions);
    }
}

void UZfInteractionComponent::HideInteractionWidget()
{
    if (IsValid(InteractionWidget))
        InteractionWidget->SetVisibility(ESlateVisibility::Hidden);
}

void UZfInteractionComponent::UpdateInteractionWidgetProgress(FName InteractionID, float Progress)
{
    if (UZfInteractionWidget* IW = Cast<UZfInteractionWidget>(InteractionWidget))
        IW->SetHoldProgress(InteractionID, Progress);
}

// ─────────────────────────────────────────────────────────────────────────
// Utils
// ─────────────────────────────────────────────────────────────────────────

bool UZfInteractionComponent::IsLocallyControlled() const
{
    if (const APawn* Pawn = Cast<APawn>(GetOwner()))
        return Pawn->IsLocallyControlled();
    return false;
}

APlayerController* UZfInteractionComponent::GetOwningController() const
{
    if (const APawn* Pawn = Cast<APawn>(GetOwner()))
        return Cast<APlayerController>(Pawn->GetController());
    return nullptr;
}

UEnhancedInputComponent* UZfInteractionComponent::GetEnhancedInputComponent() const
{
    if (const AActor* Owner = GetOwner())
        return Cast<UEnhancedInputComponent>(Owner->InputComponent);
    return nullptr;
}