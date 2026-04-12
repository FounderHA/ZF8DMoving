#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "ZfInteractionTypes.generated.h"

UENUM(BlueprintType)
enum class EInteractionInputMode : uint8
{
    Press   UMETA(DisplayName = "Press"),
    Hold    UMETA(DisplayName = "Hold")
};

USTRUCT(BlueprintType)
struct ZF8DMOVING_API FInteractionData
{
    GENERATED_BODY()

    /** ID único desta interação — usado nos callbacks para identificar qual foi executada */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FName InteractionID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FText InteractionName = FText::FromString("Interact");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    TObjectPtr<UTexture2D> Icon = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    TObjectPtr<UInputAction> InputAction = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    TObjectPtr<UInputMappingContext> MappingContext = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    int32 MappingContextPriority = 1;

    /** Tecla exibida na widget */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FKey DisplayKey;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    EInteractionInputMode InputMode = EInteractionInputMode::Press;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction",
        meta = (EditCondition = "InputMode == EInteractionInputMode::Hold", ClampMin = "0.1"))
    float HoldDuration = 1.5f;

    /** Distância máxima para a widget de interação aparecer. Default: 150cm */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction",
        meta = (ClampMin = "50.0"))
    float InteractionRadius = 150.f;
};