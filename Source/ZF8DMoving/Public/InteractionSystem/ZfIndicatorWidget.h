// IndicatorWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZfIndicatorWidget.generated.h"

/**
 * Widget indicador (bolinha).
 * Um por candidato na área de awareness.
 * Posicionamento via SetPositionInViewport feito pelo InteractionComponent.
 */
UCLASS(Abstract)
class ZF8DMOVING_API UZfIndicatorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** O actor que este indicador representa — setado ao criar */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<AActor> TrackedActor;
};