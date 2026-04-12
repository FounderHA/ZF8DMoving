#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZfInteractionTypes.h"
#include "ZFInteractionComponent.generated.h"

class USphereComponent;
class UEnhancedInputComponent;
class UUserWidget;
class UZfInteractionWidget;
class UZfIndicatorWidget;

// ── Struct de Hold ativo ──────────────────────────────────────────────────

USTRUCT()
struct FZfActiveHoldInfo
{
    GENERATED_BODY()

    FName InteractionID = NAME_None;
    float Elapsed       = 0.f;
    float Duration      = 1.5f;
    bool  bActive       = false;

    /**
     * FIX: Cache do ator em foco quando o hold começou.
     * Necessário porque CurrentFocus pode se tornar inválido durante
     * o hold (objeto sai do range, LOS quebra), e CancelHold precisa
     * enviar o RPC Server_InteractCanceled mesmo nesse cenário.
     */
    UPROPERTY()
    TObjectPtr<AActor> HoldTarget = nullptr;
};

// ── Struct por candidato ──────────────────────────────────────────────────

USTRUCT()
struct FZfCandidateInfo
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<AActor> Actor = nullptr;

    TArray<FInteractionData> Interactions;
    float LastScore = 0.f;
};

// ─────────────────────────────────────────────────────────────────────────

UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class ZF8DMOVING_API UZfInteractionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UZfInteractionComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // ── Configuração ──────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Detection")
    float AwarenessRadius = 300.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Detection",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinFocusDot = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Score",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DistanceScoreWeight = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Score",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CentralityScoreWeight = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Detection",
        meta = (ClampMin = "0.05"))
    float ReconciliationInterval = 0.2f;

    /** Widget de interação padrão — usado se o objeto não definir o seu próprio */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Widgets")
    TSubclassOf<UUserWidget> DefaultInteractionWidgetClass;

    /** Widget indicador padrão — usado se o objeto não definir o seu próprio */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Widgets")
    TSubclassOf<UUserWidget> DefaultIndicatorWidgetClass;

    /**
     * FIX: Canal de trace para LOS (câmera e player) agora configurável via
     * UPROPERTY, evitando o hardcode de ECC_GameTraceChannel1.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Detection")
    TEnumAsByte<ECollisionChannel> LOSTraceChannel = ECC_GameTraceChannel1;

private:
    // ── Componentes ───────────────────────────────────────────────────

    UPROPERTY()
    TObjectPtr<USphereComponent> AwarenessSphere;

    // ── Candidatos ────────────────────────────────────────────────────

    UPROPERTY()
    TArray<FZfCandidateInfo> Candidates;

    UPROPERTY()
    TObjectPtr<AActor> CurrentFocus;

    // ── Hold ativo ────────────────────────────────────────────────────

    FZfActiveHoldInfo ActiveHold;

    // ── Input ─────────────────────────────────────────────────────────

    TMap<TObjectPtr<UInputAction>, TArray<uint32>> ActiveInputHandles;
    TMap<TObjectPtr<UInputAction>, FName> InputActionToInteractionID;
    TArray<TObjectPtr<UInputMappingContext>> ActiveMappingContexts;

    // ── Widgets ───────────────────────────────────────────────────────

    UPROPERTY()
    TObjectPtr<UUserWidget> InteractionWidget;

    UPROPERTY()
    TMap<AActor*, UUserWidget*> IndicatorWidgets;

    // ── Timers ────────────────────────────────────────────────────────

    float ReconciliationTimer = 0.f;

    // ── Setup ─────────────────────────────────────────────────────────

    void SetupAwarenessSphere();
    void ReconcileCandidates();

    // ── Candidatos ────────────────────────────────────────────────────

    void AddCandidate(AActor* Actor);
    void RemoveCandidate(AActor* Actor);
    bool HasCandidate(AActor* Actor) const;

    // ── Foco / Score ──────────────────────────────────────────────────

    void UpdateFocus(float DeltaTime);
    float ComputeScore(const FZfCandidateInfo& Candidate) const;
    bool PassesCameraLOS(AActor* Target) const;
    bool PassesPlayerLOS(AActor* Target) const;
    void SetFocus(AActor* NewFocus, const TArray<FInteractionData>& Interactions);
    void ClearFocus(bool bNotifyObject = true);

    // ── Input ─────────────────────────────────────────────────────────

    void BindInputForFocus(const TArray<FInteractionData>& Interactions);
    void UnbindCurrentInput();

    void HandleInputStarted(FName InteractionID, EInteractionInputMode Mode, float HoldDuration);
    void HandleInputCompleted(FName InteractionID);

    // ── Hold ──────────────────────────────────────────────────────────

    void UpdateHold(float DeltaTime);
    void CancelHold();

    // ── Widgets ───────────────────────────────────────────────────────

    TSubclassOf<UUserWidget> ResolveInteractionWidgetClass(AActor* Actor) const;
    TSubclassOf<UUserWidget> ResolveIndicatorWidgetClass(AActor* Actor) const;

    void CreateIndicatorFor(AActor* Actor);
    void DestroyIndicatorFor(AActor* Actor);
    void UpdateWidgetsPositions();
    void ShowInteractionWidget(AActor* FocusActor, const TArray<FInteractionData>& Interactions);
    void HideInteractionWidget();
    void UpdateInteractionWidgetProgress(FName InteractionID, float Progress);

    // ── Utils ─────────────────────────────────────────────────────────

    bool IsLocallyControlled() const;
    APlayerController* GetOwningController() const;
    UEnhancedInputComponent* GetEnhancedInputComponent() const;

    // ── Overlap ───────────────────────────────────────────────────────

    UFUNCTION()
    void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    // ── Server RPCs ───────────────────────────────────────────────────

    UFUNCTION(Server, Reliable)
    void Server_Interact(AActor* TargetActor, FName InteractionID);

    /**
     * FIX: Adicionado Server_InteractBegin para que o servidor também
     * receba a notificação de início de hold, consistente com os demais
     * RPCs (Server_Interact, Server_InteractComplete, Server_InteractCanceled).
     */
    UFUNCTION(Server, Reliable)
    void Server_InteractBegin(AActor* TargetActor, FName InteractionID);

    UFUNCTION(Server, Reliable)
    void Server_InteractComplete(AActor* TargetActor, FName InteractionID);

    UFUNCTION(Server, Reliable)
    void Server_InteractCanceled(AActor* TargetActor, FName InteractionID);
};