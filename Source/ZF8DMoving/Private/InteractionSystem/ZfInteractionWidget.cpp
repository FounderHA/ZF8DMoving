#include "InteractionSystem/ZfInteractionWidget.h"

void UZfInteractionWidget::SetInteractions(const TArray<FInteractionData>& Interactions)
{
    OnSetInteractions(Interactions);
}

void UZfInteractionWidget::SetHoldProgress(FName InteractionID, float Progress)
{
    OnSetHoldProgress(InteractionID, Progress);
}