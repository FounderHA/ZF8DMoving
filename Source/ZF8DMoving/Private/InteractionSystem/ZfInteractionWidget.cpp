#include "InteractionSystem/ZfInteractionWidget.h"
#include "InteractionSystem/ZFInteractionComponent.h"
#include "InteractionSystem/ZfInteractionInterface.h"

void UZfInteractionWidget::InitializeWidget(AActor* InInteractableActor, UZfInteractionComponent* InInteractionComponent)
{
    InteractableActor     = InInteractableActor;
    InteractionComponent  = InInteractionComponent;
    OnWidgetInitialized();
}

float UZfInteractionWidget::GetHoldProgress() const
{
    if (!InteractionComponent) return 0.f;
    return InteractionComponent->GetHoldProgress();
}

bool UZfInteractionWidget::IsHolding() const
{
    if (!InteractionComponent) return false;
    return InteractionComponent->IsHolding();
}

float UZfInteractionWidget::GetHoldTimeRemaining() const
{
    if (!InteractionComponent) return 0.f;
    const float Duration = GetHoldDuration();
    const float Progress = GetHoldProgress();
    return Duration * (1.f - Progress);
}

float UZfInteractionWidget::GetHoldDuration() const
{
    if (!InteractableActor || !InteractableActor->GetClass()->ImplementsInterface(UZfInteractionInterface::StaticClass()))
        return 0.f;

    TArray<FZfInteractAction> Actions =
        IZfInteractionInterface::Execute_GetInteractActions(InteractableActor);

    if (!Actions.IsValidIndex(0)) return 0.f;
    return Actions[0].HoldDuration;
}

TArray<FZfInteractAction> UZfInteractionWidget::GetInteractActions() const
{
    if (!InteractableActor || !InteractableActor->GetClass()->ImplementsInterface(UZfInteractionInterface::StaticClass()))
        return {};

    return IZfInteractionInterface::Execute_GetInteractActions(InteractableActor);
}

EZfInteractionState UZfInteractionWidget::GetInteractionState() const
{
    if (!InteractableActor || !InteractableActor->GetClass()->ImplementsInterface(UZfInteractionInterface::StaticClass()))
        return EZfInteractionState::Unavailable;

    return IZfInteractionInterface::Execute_GetInteractionState(InteractableActor, nullptr);
}

void UZfInteractionWidget::HideComplete()
{
    RemoveFromParent();
}