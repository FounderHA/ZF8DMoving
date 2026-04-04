// Copyright ZfGame Studio. All Rights Reserved.

#include "InteractionSystem/ZFInteractionComponent.h"
#include "Components/SphereComponent.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Engine/UserInterfaceSettings.h"
#include "InteractionSystem/ZfInteractionWidget.h"

UZfInteractionComponent::UZfInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.f;

    DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
    DetectionSphere->SetSphereRadius(InteractionRadius);
    DetectionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    DetectionSphere->SetGenerateOverlapEvents(true);
    DetectionSphere->SetHiddenInGame(true);
}

// ============================================================
// CICLO DE VIDA
// ============================================================

void UZfInteractionComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AActor* Owner = GetOwner())
    {
        DetectionSphere->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        DetectionSphere->SetSphereRadius(InteractionRadius);
    }

    DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &UZfInteractionComponent::OnOverlapBegin);
    DetectionSphere->OnComponentEndOverlap.AddDynamic(this, &UZfInteractionComponent::OnOverlapEnd);

    Internal_PollInteractables();
}

void UZfInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Polling periódico
    PollingTimer += DeltaTime;
    if (PollingTimer >= PollingInterval)
    {
        PollingTimer = 0.f;
        Internal_PollInteractables();
    }

    // Atualiza status e score de cada entry
    for (FZfInteractableEntry& Entry : NearbyInteractables)
        Internal_UpdateEntryStatus(Entry);

    // Atualiza foco
    Internal_UpdateFocus();

    // Atualiza widgets
    Internal_UpdateWidgets();

    // Atualiza cooldowns e hold
    Internal_UpdateCooldowns(DeltaTime);
    Internal_UpdateHold(DeltaTime);

    if (bShowDebugSphere && GetOwner())
    {
        DrawDebugSphere(GetWorld(), GetOwner()->GetActorLocation(),
            InteractionRadius, 32, FColor::Green, false, -1.f, 0, 2.f);

        DrawDebugSphere(GetWorld(), GetOwner()->GetActorLocation(),
            InteractionDistance, 32, FColor::Yellow, false, -1.f, 0, 2.f);
    }
}

// ============================================================
// FUNÇÕES PÚBLICAS
// ============================================================

void UZfInteractionComponent::TryInteract(int32 ActionIndex)
{
    // Só pode interagir se houver foco com status CanInteract
    if (!CurrentFocus) return;
    if (!Internal_ImplementsInterface(CurrentFocus)) return;

    if (Internal_IsActionOnCooldown(CurrentFocus, ActionIndex))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ZfInteractionComponent: Ação %d em cooldown."), ActionIndex);
        return;
    }

    TArray<FZfInteractAction> Actions = IZfInteractionInterface::Execute_GetInteractActions(CurrentFocus);

    if (!Actions.IsValidIndex(ActionIndex)) return;

    const FZfInteractAction& Action = Actions[ActionIndex];

    if (!Internal_MeetsRequirements(Action.Requirements)) return;

    if (Action.InteractionMethod == EZfInteractionMethod::Hold)
    {
        StartHold(ActionIndex);
        return;
    }

    ServerTryInteract(CurrentFocus, ActionIndex);

    if (Action.Cooldown > 0.f)
        Internal_StartActionCooldown(CurrentFocus, ActionIndex, Action.Cooldown);
}

void UZfInteractionComponent::StartHold(int32 ActionIndex)
{
    if (!CurrentFocus || bIsHolding) return;

    bIsHolding      = true;
    HoldActionIndex = ActionIndex;
    HoldTimer       = 0.f;
}

void UZfInteractionComponent::CancelHold()
{
    if (!bIsHolding) return;

    if (CurrentFocus && Internal_ImplementsInterface(CurrentFocus))
        IZfInteractionInterface::Execute_OnInteractionHoldCancelled(CurrentFocus, GetOwner(), HoldActionIndex);

    bIsHolding      = false;
    HoldTimer       = 0.f;
    HoldActionIndex = 0;
    OnHoldProgress.Broadcast(0.f, HoldActionIndex);
}

TArray<AActor*> UZfInteractionComponent::GetNearbyInteractables() const
{
    TArray<AActor*> Result;
    for (const FZfInteractableEntry& Entry : NearbyInteractables)
        if (Entry.Actor) Result.Add(Entry.Actor);
    return Result;
}

float UZfInteractionComponent::GetHoldProgress() const
{
    if (!bIsHolding || !CurrentFocus) return 0.f;

    TArray<FZfInteractAction> Actions = IZfInteractionInterface::Execute_GetInteractActions(CurrentFocus);

    if (!Actions.IsValidIndex(HoldActionIndex)) return 0.f;

    const float Duration = Actions[HoldActionIndex].HoldDuration;
    return Duration > 0.f ? FMath::Clamp(HoldTimer / Duration, 0.f, 1.f) : 0.f;
}

// ============================================================
// CALLBACKS DE OVERLAP
// ============================================================

void UZfInteractionComponent::OnOverlapBegin(
    UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == GetOwner()) return;
    if (!Internal_ImplementsInterface(OtherActor)) return;
    Internal_AddInteractable(OtherActor);
}

void UZfInteractionComponent::OnOverlapEnd(
    UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!OtherActor || OtherActor == GetOwner()) return;
    Internal_RemoveInteractable(OtherActor);
}

// ============================================================
// FUNÇÕES INTERNAS
// ============================================================

void UZfInteractionComponent::Internal_AddInteractable(AActor* Actor)
{
    if (!Actor) return;

    for (const FZfInteractableEntry& Entry : NearbyInteractables)
        if (Entry.Actor == Actor) return;

    FZfInteractableEntry NewEntry;
    NewEntry.Actor = Actor;
    NearbyInteractables.Add(NewEntry);

    // Calcula status inicial
    Internal_UpdateEntryStatus(NearbyInteractables.Last());

    // Cria widget de indicação se já passar o ângulo
    if (NearbyInteractables.Last().Status >= EZfInteractableStatus::ShowIndicator)
        Internal_CreateIndicatorWidget(NearbyInteractables.Last());

    IZfInteractionInterface::Execute_OnInteractorEnterRange(Actor, GetOwner());
}

void UZfInteractionComponent::Internal_RemoveInteractable(AActor* Actor)
{
    if (!Actor) return;

    for (int32 i = NearbyInteractables.Num() - 1; i >= 0; i--)
    {
        if (NearbyInteractables[i].Actor == Actor)
        {
            Internal_RemoveIndicatorWidget(NearbyInteractables[i]);

            if (Internal_ImplementsInterface(Actor))
                IZfInteractionInterface::Execute_OnInteractorExitRange(Actor, GetOwner());

            if (CurrentFocus == Actor)
            {
                Internal_SetFocus(nullptr);
                CancelHold();
            }

            NearbyInteractables.RemoveAt(i);
            return;
        }
    }
}

void UZfInteractionComponent::Internal_PollInteractables()
{
    if (!DetectionSphere) return;

    TArray<AActor*> OverlappingActors;
    DetectionSphere->GetOverlappingActors(OverlappingActors);

    for (AActor* Actor : OverlappingActors)
    {
        if (!Actor || Actor == GetOwner()) continue;
        if (!Internal_ImplementsInterface(Actor)) continue;
        Internal_AddInteractable(Actor);
    }

    for (int32 i = NearbyInteractables.Num() - 1; i >= 0; i--)
    {
        AActor* Actor = NearbyInteractables[i].Actor;
        if (!Actor || !OverlappingActors.Contains(Actor))
            Internal_RemoveInteractable(Actor);
    }
}

void UZfInteractionComponent::Internal_UpdateEntryStatus(FZfInteractableEntry& Entry)
{
    if (!Entry.Actor)
    {
        Entry.Status = EZfInteractableStatus::OutOfRange;
        return;
    }

    // --- Verifica ângulo do personagem para indicador ---
    if (!Internal_IsInCharacterAngle(Entry.Actor, IndicatorAngle))
    {
        Entry.Status     = EZfInteractableStatus::OutOfIndicatorAngle;
        Entry.CameraScore = 0.f;
        return;
    }

    // Passou o ângulo de indicador
    Entry.Status = EZfInteractableStatus::ShowIndicator;

    // --- Verifica ângulo do personagem para interação ---
    if (!Internal_IsInCharacterAngle(Entry.Actor, InteractionAngle))
    {
        Entry.Status     = EZfInteractableStatus::OutOfInteractionAngle;
        Entry.CameraScore = 0.f;
        return;
    }

    // --- Verifica distância de interação ---
    const float Distance = FVector::Dist(GetOwner()->GetActorLocation(), Entry.Actor->GetActorLocation());

    if (Distance > InteractionDistance)
    {
        Entry.Status     = EZfInteractableStatus::OutOfInteractionDistance;
        Entry.CameraScore = 0.f;
        return;
    }

    // --- Calcula score da câmera ---
    Entry.CameraScore = Internal_CalculateCameraScore(Entry.Actor);

    // --- Verifica score mínimo ---
    if (Entry.CameraScore < MinInteractionScore)
    {
        Entry.Status = EZfInteractableStatus::BelowMinScore;
        return;
    }

    // Passou tudo — pode interagir
    Entry.Status = EZfInteractableStatus::CanInteract;
}

float UZfInteractionComponent::Internal_CalculateCameraScore(AActor* Actor) const
{
    if (!Actor || !GetOwner()) return 0.f;

    APlayerCameraManager* CamManager = Internal_GetCameraManager();
    if (!CamManager) return 0.f;

    const FVector CamLocation = CamManager->GetCameraLocation();
    const FVector CamForward  = CamManager->GetCameraRotation().Vector();

    // Centro real do bounding box — altura correta do objeto
    FVector Origin, Extent;
    Actor->GetActorBounds(false, Origin, Extent);

    const FVector ToActor = (Origin - CamLocation).GetSafeNormal();
    const float DotProduct = FVector::DotProduct(CamForward, ToActor);

    return FMath::Clamp((DotProduct - MinInteractionScore) / (1.f - MinInteractionScore), 0.f, 1.f);
}


bool UZfInteractionComponent::Internal_IsInCharacterAngle(AActor* Actor, float AngleDegrees) const
{
    if (!Actor || !GetOwner()) return false;

    // Direção para frente do personagem
    const FVector CharacterForward = GetOwner()->GetActorForwardVector();

    // Direção do personagem para o objeto (ignorando Z)
    FVector ToActor = Actor->GetActorLocation() - GetOwner()->GetActorLocation();
    ToActor.Z = 0.f;
    ToActor.Normalize();

    const float DotProduct = FVector::DotProduct(CharacterForward, ToActor);
    const float AngleRad   = FMath::Acos(FMath::Clamp(DotProduct, -1.f, 1.f));
    const float AngleDeg   = FMath::RadiansToDegrees(AngleRad);

    return AngleDeg <= (AngleDegrees / 2.f);
}

void UZfInteractionComponent::Internal_UpdateFocus()
{
    AActor* BestActor = nullptr;
    float   BestScore = -1.f;

    for (const FZfInteractableEntry& Entry : NearbyInteractables)
    {
        if (!Entry.Actor) continue;
        if (Entry.Status != EZfInteractableStatus::CanInteract) continue;

        if (Entry.CameraScore > BestScore)
        {
            BestScore = Entry.CameraScore;
            BestActor = Entry.Actor;
        }
    }

    if (BestActor != CurrentFocus)
        Internal_SetFocus(BestActor);
}

void UZfInteractionComponent::Internal_SetFocus(AActor* NewFocus)
{
    AActor* OldFocus = CurrentFocus;

    if (OldFocus && Internal_ImplementsInterface(OldFocus))
        IZfInteractionInterface::Execute_OnInteractionUnfocused(OldFocus, GetOwner());

    Internal_RemoveFocusWidget();
    CurrentFocus = NewFocus;

    if (NewFocus && Internal_ImplementsInterface(NewFocus))
    {
        IZfInteractionInterface::Execute_OnInteractionFocused(NewFocus, GetOwner());
        Internal_CreateFocusWidget(NewFocus);

        // Esconde a widget de indicação do objeto que virou foco
        for (FZfInteractableEntry& Entry : NearbyInteractables)
        {
            if (Entry.Actor == NewFocus && Entry.IndicatorWidget)
                Entry.IndicatorWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    // Quando perde foco, mostra a widget de indicação novamente
    if (OldFocus)
    {
        for (FZfInteractableEntry& Entry : NearbyInteractables)
        {
            if (Entry.Actor == OldFocus && Entry.IndicatorWidget)
                Entry.IndicatorWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    }

    if (!NewFocus && bIsHolding)
        CancelHold();

    OnFocusChanged.Broadcast(NewFocus, OldFocus);
}

void UZfInteractionComponent::Internal_UpdateCooldowns(float DeltaTime)
{
    for (FZfInteractableEntry& Entry : NearbyInteractables)
    {
        TArray<int32> ExpiredKeys;
        for (auto& Pair : Entry.ActionCooldowns)
        {
            Pair.Value -= DeltaTime;
            if (Pair.Value <= 0.f)
                ExpiredKeys.Add(Pair.Key);
        }
        for (int32 Key : ExpiredKeys)
            Entry.ActionCooldowns.Remove(Key);
    }
}

void UZfInteractionComponent::Internal_UpdateHold(float DeltaTime)
{
    if (!bIsHolding || !CurrentFocus) return;

    if (!Internal_ImplementsInterface(CurrentFocus))
    {
        CancelHold();
        return;
    }

    TArray<FZfInteractAction> Actions = IZfInteractionInterface::Execute_GetInteractActions(CurrentFocus);

    if (!Actions.IsValidIndex(HoldActionIndex))
    {
        CancelHold();
        return;
    }

    const FZfInteractAction& Action = Actions[HoldActionIndex];
    HoldTimer += DeltaTime;

    const float Progress = FMath::Clamp(HoldTimer / Action.HoldDuration, 0.f, 1.f);
    OnHoldProgress.Broadcast(Progress, HoldActionIndex);

    if (HoldTimer >= Action.HoldDuration)
    {
        bIsHolding = false;
        HoldTimer  = 0.f;

        ServerTryInteract(CurrentFocus, HoldActionIndex);

        if (Action.Cooldown > 0.f)
            Internal_StartActionCooldown(CurrentFocus, HoldActionIndex, Action.Cooldown);

        OnHoldProgress.Broadcast(0.f, HoldActionIndex);
    }
}

bool UZfInteractionComponent::Internal_IsActionOnCooldown(AActor* Actor, int32 ActionIndex) const
{
    for (const FZfInteractableEntry& Entry : NearbyInteractables)
        if (Entry.Actor == Actor)
            return Entry.ActionCooldowns.Contains(ActionIndex);
    return false;
}

void UZfInteractionComponent::Internal_StartActionCooldown(AActor* Actor, int32 ActionIndex, float Duration)
{
    for (FZfInteractableEntry& Entry : NearbyInteractables)
    {
        if (Entry.Actor == Actor)
        {
            Entry.ActionCooldowns.Add(ActionIndex, Duration);
            return;
        }
    }
}

void UZfInteractionComponent::Internal_CreateIndicatorWidget(FZfInteractableEntry& Entry)
{
    if (!Entry.Actor) return;
    if (!Internal_ImplementsInterface(Entry.Actor)) return;
    if (Entry.IndicatorWidget) return;

    TSubclassOf<UUserWidget> WidgetClass =
        IZfInteractionInterface::Execute_GetIndicatorWidgetClass(Entry.Actor);

    if (!WidgetClass) return;

    APlayerController* PC = GetOwner()
        ? GetOwner()->GetInstigatorController<APlayerController>()
        : nullptr;

    if (!PC || !PC->IsLocalController()) return;

    UZfInteractionWidget* Widget =
        CreateWidget<UZfInteractionWidget>(PC, WidgetClass);
    if (Widget)
    {
        Widget->AddToViewport(1);
        Widget->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
        Widget->InitializeWidget(Entry.Actor, this);
        Entry.IndicatorWidget = Widget;
        Entry.WorldOffset =
            IZfInteractionInterface::Execute_GetIndicatorOffset(Entry.Actor);
    }
}

void UZfInteractionComponent::Internal_RemoveIndicatorWidget(FZfInteractableEntry& Entry)
{
    if (Entry.IndicatorWidget)
    {
        Entry.IndicatorWidget->OnWidgetHide();
    }
}

void UZfInteractionComponent::Internal_UpdateWidgets()
{
    APlayerController* PC = GetOwner() ? GetOwner()->GetInstigatorController<APlayerController>() : nullptr;

    if (!PC || !PC->IsLocalController()) return;

    for (FZfInteractableEntry& Entry : NearbyInteractables)
    {
        if (!Entry.Actor) continue;

        const bool bShouldShowIndicator = Entry.Status >= EZfInteractableStatus::ShowIndicator;

        // Cria ou remove widget de indicação conforme status
        if (bShouldShowIndicator && !Entry.IndicatorWidget)
            Internal_CreateIndicatorWidget(Entry);
        else if (!bShouldShowIndicator && Entry.IndicatorWidget)
            Internal_RemoveIndicatorWidget(Entry);

        // Posiciona widget de indicação na tela
        if (Entry.IndicatorWidget)
        {

            if (Entry.Actor == CurrentFocus)
            {
                Entry.IndicatorWidget->SetVisibility(ESlateVisibility::Collapsed);
                continue;
            }
            
            const FVector WorldPos = Entry.Actor->GetActorLocation() + Entry.WorldOffset;
            FVector2D ScreenPos;

            if (PC->ProjectWorldLocationToScreen(WorldPos, ScreenPos, false))
            {
                Entry.IndicatorWidget->SetPositionInViewport(ScreenPos, true);
                Entry.IndicatorWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
            }
            else
            {
                Entry.IndicatorWidget->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }

    // Posiciona widget de foco na tela
    if (CurrentFocusWidget && CurrentFocus)
    {
        const FVector PromptOffset = IZfInteractionInterface::Execute_GetPromptOffset(CurrentFocus);

        const FVector WorldPos = CurrentFocus->GetActorLocation() + PromptOffset;
        FVector2D ScreenPos;

        if (PC->ProjectWorldLocationToScreen(WorldPos, ScreenPos, true))
        {
            CurrentFocusWidget->SetPositionInViewport(ScreenPos, true);
            CurrentFocusWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else
        {
            CurrentFocusWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UZfInteractionComponent::Internal_CreateFocusWidget(AActor* FocusActor)
{
    if (!FocusActor) return;
    if (!Internal_ImplementsInterface(FocusActor)) return;

    TSubclassOf<UUserWidget> WidgetClass =
        IZfInteractionInterface::Execute_GetInteractionWidgetClass(FocusActor);

    if (!WidgetClass) return;

    APlayerController* PC = GetOwner()
        ? GetOwner()->GetInstigatorController<APlayerController>()
        : nullptr;

    if (!PC || !PC->IsLocalController()) return;

    UZfInteractionWidget* Widget =
        CreateWidget<UZfInteractionWidget>(PC, WidgetClass);
    if (Widget)
    {
        Widget->AddToViewport(2);
        Widget->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
        Widget->InitializeWidget(FocusActor, this);
        CurrentFocusWidget = Widget;
    }
}

void UZfInteractionComponent::Internal_RemoveFocusWidget()
{
    if (CurrentFocusWidget)
    {
        CurrentFocusWidget->OnWidgetHide();
    }
}

bool UZfInteractionComponent::Internal_ImplementsInterface(AActor* Actor) const
{
    return Actor && Actor->GetClass()->ImplementsInterface(
        UZfInteractionInterface::StaticClass());
}

bool UZfInteractionComponent::Internal_MeetsRequirements(
    const FZfInteractionRequirements& Requirements) const
{
    if (Requirements.RequiredTags.IsEmpty() &&
        !Requirements.RequiredItemTag.IsValid() &&
        Requirements.MinLevel == 0)
        return true;

    return true;
}

void UZfInteractionComponent::ServerTryInteract_Implementation(AActor* TargetActor, int32 ActionIndex)
{
    if (!TargetActor) return;
    if (!Internal_ImplementsInterface(TargetActor)) return;

    if (GetOwner())
    {
        const float Distance = FVector::Dist(GetOwner()->GetActorLocation(), TargetActor->GetActorLocation());
        if (Distance > InteractionRadius * 1.5f)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("ServerTryInteract: distância inválida."));
            return;
        }
    }

    IZfInteractionInterface::Execute_OnInteract(TargetActor, GetOwner(), ActionIndex);
    OnInteractionCompleted.Broadcast(TargetActor, ActionIndex);
}

APlayerCameraManager* UZfInteractionComponent::Internal_GetCameraManager() const
{
    if (!GetOwner()) return nullptr;
    APlayerController* PC = GetOwner()->GetInstigatorController<APlayerController>();
    return PC ? PC->PlayerCameraManager : nullptr;
}