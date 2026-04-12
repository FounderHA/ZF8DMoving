#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZfInteractionTypes.h"
#include "ZfInteractionWidget.generated.h"

UCLASS(Abstract)
class ZF8DMOVING_API UZfInteractionWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetInteractions(const TArray<FInteractionData>& Interactions);
    void SetHoldProgress(FName InteractionID, float Progress);

protected:
    /** Chamado quando um novo objeto entra em foco — atualize nome, teclas, ícones aqui */
    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
    void OnSetInteractions(const TArray<FInteractionData>& Interactions);

    /** Chamado a cada frame durante o hold */
    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
    void OnSetHoldProgress(FName InteractionID, float Progress);
};