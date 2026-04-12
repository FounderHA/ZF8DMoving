#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ZfInteractionTypes.h"
#include "ZfInteractionInterface.generated.h"

class UZfInteractionWidget;
class UZfIndicatorWidget;

UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class UZfInteractionInterface : public UInterface
{
    GENERATED_BODY()
};

class ZF8DMOVING_API IZfInteractionInterface
{
    GENERATED_BODY()

public:
    /** Retorna TODAS as interações disponíveis neste objeto */
    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Information")
    TArray<FInteractionData> GetInteractionDataArray() const;

    /**
     * Widget de interação customizada para este objeto.
     * Retorne nullptr para usar o default do InteractionComponent.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Information")
    TSubclassOf<UUserWidget> GetInteractionWidgetClass() const;

    /**
     * Widget indicador customizado para este objeto.
     * Retorne nullptr para usar o default do InteractionComponent.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Information")
    TSubclassOf<UUserWidget> GetIndicatorWidgetClass() const;

    /** Offset em screen space da InteractionWidget relativo ao objeto */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    FVector2D GetInteractionWidgetOffset() const;

    /** Offset em screen space do IndicatorWidget relativo ao objeto */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    FVector2D GetIndicatorWidgetOffset() const;

    // ── Área ──────────────────────────────────────────────────────────

    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
    void OnPlayerEnterRange(APlayerController* InstigatorController);

    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
    void OnPlayerExitRange(APlayerController* InstigatorController);

    // ── Foco ──────────────────────────────────────────────────────────

    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
    void OnFocusGained(APlayerController* InstigatorController);

    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
    void OnFocusLost(APlayerController* InstigatorController);

    // ── Press ─────────────────────────────────────────────────────────

    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
    void OnInteract(APlayerController* InstigatorController, FName InteractionID);

    // ── Hold ──────────────────────────────────────────────────────────

    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
    void OnInteractBegin(APlayerController* InstigatorController, FName InteractionID);

    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
    void OnInteractProgress(APlayerController* InstigatorController, FName InteractionID, float Progress);

    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
    void OnInteractComplete(APlayerController* InstigatorController, FName InteractionID);

    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
    void OnInteractCanceled(APlayerController* InstigatorController, FName InteractionID);
};