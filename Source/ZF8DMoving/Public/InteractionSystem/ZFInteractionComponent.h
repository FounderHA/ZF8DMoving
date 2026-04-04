// Copyright ZfGame Studio. All Rights Reserved.
// ZfInteractionComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZfInteractionTypes.h"
#include "ZfInteractionInterface.h"
#include "ZfInteractionWidget.h"
#include "ZfInteractionComponent.generated.h"

class USphereComponent;
class UUserWidget;

// -----------------------------------------------------------
// EZfInteractableStatus
// Estado calculado de cada interagível no range
// -----------------------------------------------------------
UENUM(BlueprintType)
enum class EZfInteractableStatus : uint8
{
    OutOfRange               UMETA(DisplayName = "Out Of Range"),
    OutOfIndicatorAngle      UMETA(DisplayName = "Out Of Indicator Angle"),
    ShowIndicator            UMETA(DisplayName = "Show Indicator"),
    OutOfInteractionAngle    UMETA(DisplayName = "Out Of Interaction Angle"),
    OutOfInteractionDistance UMETA(DisplayName = "Out Of Interaction Distance"),
    BelowMinScore            UMETA(DisplayName = "Below Min Score"),
    CanInteract              UMETA(DisplayName = "Can Interact"),
};

// -----------------------------------------------------------
// FZfInteractableEntry
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfInteractableEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Interaction")
    TObjectPtr<AActor> Actor = nullptr;

    // Score baseado somente na câmera (0 a 1)
    UPROPERTY(BlueprintReadOnly, Category = "Interaction")
    float CameraScore = 0.f;

    // Status atual deste interagível
    UPROPERTY(BlueprintReadOnly, Category = "Interaction")
    EZfInteractableStatus Status = EZfInteractableStatus::OutOfRange;

    // Widget de indicação ativa
    UPROPERTY(BlueprintReadOnly, Category = "Interaction")
    TObjectPtr<UZfInteractionWidget> IndicatorWidget = nullptr;

    // Offset de posição da widget no mundo
    FVector WorldOffset = FVector(0.f, 0.f, 0.f);

    // Cooldowns ativos por índice de ação
    TMap<int32, float> ActionCooldowns;

    bool operator==(const FZfInteractableEntry& Other) const
    {
        return Actor == Other.Actor;
    }
};

// -----------------------------------------------------------
// DELEGATES
// -----------------------------------------------------------
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractionFocusChanged, AActor*, NewFocus, AActor*, OldFocus);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractionCompleted, AActor*, InteractedActor, int32, ActionIndex);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractionHoldProgress, float, Progress, int32, ActionIndex);

// -----------------------------------------------------------
// UZfInteractionComponent
// -----------------------------------------------------------
UCLASS(ClassGroup = (Zf), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent, DisplayName = "Zf Interaction Component"))
class ZF8DMOVING_API UZfInteractionComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UZfInteractionComponent();

    // ----------------------------------------------------------
    // CONFIGURAÇÃO — Range
    // ----------------------------------------------------------

    // Raio de detecção — objetos fora disso são ignorados completamente
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Range", meta = (ClampMin = "10.0", UIMin = "10.0"))
    float InteractionRadius = 350.f;

    // ----------------------------------------------------------
    // CONFIGURAÇÃO — Indicador (bolinha)
    // ----------------------------------------------------------

    // Ângulo máximo em graus na frente do PERSONAGEM para mostrar a bolinha
    // Ex: 180 = mostra em qualquer direção, 90 = só na frente
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Indicator", meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "360.0", UIMax = "360.0"))
    float IndicatorAngle = 150.f;

    // ----------------------------------------------------------
    // CONFIGURAÇÃO — Interação
    // ----------------------------------------------------------

    // Distância máxima para poder interagir — independente do score
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Interaction", meta = (ClampMin = "10.0", UIMin = "10.0"))
    float InteractionDistance = 200.f;
    
    // Ângulo máximo em graus na frente do PERSONAGEM para poder interagir
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Interaction", meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "180.0", UIMax = "180.0"))
    float InteractionAngle = 90.f;

    // Score mínimo da câmera para poder interagir (0 a 1)
    // Score = dot product normalizado da câmera para o objeto
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Interaction", meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
    float MinInteractionScore = 0.95f;

    // ----------------------------------------------------------
    // CONFIGURAÇÃO — Polling
    // ----------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Config", meta = (ClampMin = "0.05"))
    float PollingInterval = 0.2f;

    // ----------------------------------------------------------
    // CONFIGURAÇÃO — Debug
    // ----------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Debug")
    bool bShowDebugSphere = false;

    // ----------------------------------------------------------
    // DELEGATES
    // ----------------------------------------------------------

    UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
    FOnInteractionFocusChanged OnFocusChanged;

    UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
    FOnInteractionCompleted OnInteractionCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
    FOnInteractionHoldProgress OnHoldProgress;

    // ----------------------------------------------------------
    // FUNÇÕES PÚBLICAS
    // ----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Zf|Interaction")
    void TryInteract(int32 ActionIndex = 0);

    UFUNCTION(BlueprintCallable, Category = "Zf|Interaction")
    void StartHold(int32 ActionIndex = 0);

    UFUNCTION(BlueprintCallable, Category = "Zf|Interaction")
    void CancelHold();

    UFUNCTION(BlueprintCallable, Category = "Zf|Interaction|Query")
    AActor* GetCurrentFocus() const { return CurrentFocus; }

    UFUNCTION(BlueprintCallable, Category = "Zf|Interaction|Query")
    TArray<AActor*> GetNearbyInteractables() const;

    UFUNCTION(BlueprintCallable, Category = "Zf|Interaction|Query")
    float GetHoldProgress() const;

    UFUNCTION(BlueprintCallable, Category = "Zf|Interaction|Query")
    bool IsHolding() const { return bIsHolding; }

    // ----------------------------------------------------------
    // CICLO DE VIDA
    // ----------------------------------------------------------

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction|Components")
    TObjectPtr<USphereComponent> DetectionSphere;

    UPROPERTY(BlueprintReadOnly, Category = "Interaction")
    TArray<FZfInteractableEntry> NearbyInteractables;

    UPROPERTY(BlueprintReadOnly, Category = "Interaction")
    TObjectPtr<AActor> CurrentFocus = nullptr;

    UPROPERTY()
    TObjectPtr<UZfInteractionWidget> CurrentFocusWidget = nullptr;

    bool bIsHolding      = false;
    int32 HoldActionIndex = 0;
    float HoldTimer      = 0.f;
    float PollingTimer   = 0.f;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor, UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex, bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor, UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex);

private:

    void Internal_AddInteractable(AActor* Actor);
    void Internal_RemoveInteractable(AActor* Actor);
    void Internal_PollInteractables();

    // Calcula o status e score de câmera de um entry
    void Internal_UpdateEntryStatus(FZfInteractableEntry& Entry);

    // Calcula dot product normalizado da câmera para o ator (0 a 1)
    float Internal_CalculateCameraScore(AActor* Actor) const;

    // Verifica se o ator está dentro do ângulo na frente do personagem
    bool Internal_IsInCharacterAngle(AActor* Actor, float AngleDegrees) const;

    // Atualiza o foco — somente o melhor CanInteract
    void Internal_UpdateFocus();

    void Internal_SetFocus(AActor* NewFocus);
    void Internal_UpdateCooldowns(float DeltaTime);
    void Internal_UpdateHold(float DeltaTime);
    bool Internal_IsActionOnCooldown(AActor* Actor, int32 ActionIndex) const;
    void Internal_StartActionCooldown(AActor* Actor, int32 ActionIndex, float Duration);
    void Internal_CreateIndicatorWidget(FZfInteractableEntry& Entry);
    void Internal_RemoveIndicatorWidget(FZfInteractableEntry& Entry);
    void Internal_UpdateWidgets();
    void Internal_CreateFocusWidget(AActor* FocusActor);
    void Internal_RemoveFocusWidget();
    bool Internal_ImplementsInterface(AActor* Actor) const;
    bool Internal_MeetsRequirements(const FZfInteractionRequirements& Requirements) const;

    UFUNCTION(Server, Reliable)
    void ServerTryInteract(AActor* TargetActor, int32 ActionIndex);

    APlayerCameraManager* Internal_GetCameraManager() const;
};