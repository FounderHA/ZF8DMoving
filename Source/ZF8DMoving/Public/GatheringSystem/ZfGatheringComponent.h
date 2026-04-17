// Copyright ZfGame Studio. All Rights Reserved.
// ZfGatherableComponent.h
//
// RESPONSABILIDADES:
// - Estado do recurso: HP, depleção, respawn
// - Todo o QTE: ângulo atual (tick), zonas, avaliação, resultado
//
// A widget é puramente visual — lê delegates, não sabe de nada.
// O sistema de interação chama RegisterHit() direto via GA.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "GatheringSystem/ZfGatheringTypes.h"
#include "ZfGatheringComponent.generated.h"

class UZfGatheringResourceData;

// ============================================================
// DELEGATES — recurso
// ============================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnResourceDepleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnResourceRespawned);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnResourceHPChanged,
    float, CurrentHP, float, MaxHP);

// ============================================================
// DELEGATES — Skill Check (widget faz bind aqui, só lê)
// ============================================================

// Novo round iniciado — widget posiciona as zonas no material.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnSkillCheckRoundBegun,
    float, GoodStart,
    float, GoodSize,
    float, PerfectStart,
    float, PerfectSize);

// Ângulo atualizado a cada frame — widget gira o ponteiro.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillCheckAngleUpdated,
    float, NormalizedAngle);

// Resultado avaliado — widget exibe o feedback visual.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillCheckHitEvaluated,
    EZfGatherHitResult, Result);

// ============================================================
// UZfGatheringComponent
// ============================================================

UCLASS(ClassGroup = "Zf|Gathering", BlueprintType, Blueprintable,
    meta = (BlueprintSpawnableComponent))
class ZF8DMOVING_API UZfGatheringComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UZfGatheringComponent();

    // ----------------------------------------------------------
    // CONFIGURAÇÃO
    // ----------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Config")
    TObjectPtr<UZfGatheringResourceData> GatherResourceData;

    // ----------------------------------------------------------
    // DELEGATES — recurso
    // ----------------------------------------------------------

    UPROPERTY(BlueprintAssignable, Category = "Gather|Events")
    FOnResourceDepleted OnResourceDepleted;

    UPROPERTY(BlueprintAssignable, Category = "Gather|Events")
    FOnResourceRespawned OnResourceRespawned;

    UPROPERTY(BlueprintAssignable, Category = "Gather|Events")
    FOnResourceHPChanged OnResourceHPChanged;

    // ----------------------------------------------------------
    // DELEGATES — Skill Check
    // ----------------------------------------------------------

    UPROPERTY(BlueprintAssignable, Category = "Gather|SkillCheck|Events")
    FOnSkillCheckRoundBegun OnSkillCheckRoundBegun;

    UPROPERTY(BlueprintAssignable, Category = "Gather|SkillCheck|Events")
    FOnSkillCheckAngleUpdated OnSkillCheckAngleUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Gather|SkillCheck|Events")
    FOnSkillCheckHitEvaluated OnSkillCheckHitEvaluated;

    // ----------------------------------------------------------
    // CICLO DE VIDA
    // ----------------------------------------------------------

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ----------------------------------------------------------
    // ESTADO DO RECURSO — leitura
    // ----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    bool IsAvailable() const { return !bIsDepleted; }

    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    bool IsDepleted() const { return bIsDepleted; }

    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    float GetCurrentHP() const { return CurrentHP; }

    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    float GetMaxHP() const;

    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    float GetHPPercent() const;

    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    float GetRespawnTimeRemaining() const;

    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    UZfGatheringResourceData* GetGatherResourceData() const { return GatherResourceData; }

    UFUNCTION(BlueprintPure, Category = "Zf|Gatherable|SkillCheck")
    bool IsRoundActive() const { return bRoundActive; }
    
    // ----------------------------------------------------------
    // ESTADO DO RECURSO — escrita
    // ----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    void ApplyDamage(float DamageAmount);

    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    void Deplete();

    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    void ForceRespawn();

    // ----------------------------------------------------------
    // SKILL CHECK — chamados pela GA
    // ----------------------------------------------------------

    // Inicia um novo round: gera zonas, liga o Tick, guarda o Instigator.
    // Chamado pela GA em Internal_ExecuteNextHit, após K2_OnQTEStarted.
    // @param InGoodSize         — tamanho da zona externa (0.0–1.0)
    // @param InPerfectSize      — tamanho da zona interna (0.0–1.0, < GoodSize)
    // @param InNeedleRotTime    — duração de uma volta em segundos
    // @param InInstigator       — ator do jogador (dono do ASC)
    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable|SkillCheck")
    void BeginSkillCheckRound(
        float InGoodSize,
        float InPerfectSize,
        float InNeedleRotTime,
        AActor* InInstigator);

    // Para o Tick do QTE. Chamado pela GA no cleanup.
    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable|SkillCheck")
    void EndSkillCheck();

    // Registra o hit no ângulo atual — chamado via GA::RegisterGatherHit().
    // Avalia o ângulo, dispara OnSkillCheckHitEvaluated e envia GameplayEvent.
    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable|SkillCheck")
    void RegisterHit();

    // Avalia um ângulo sem disparar eventos — útil para debug.
    UFUNCTION(BlueprintPure, Category = "Zf|Gatherable|SkillCheck")
    EZfGatherHitResult EvaluateAngle(float Angle) const;

    // Chamado pelo OnInteract do objeto quando o jogador pressiona
    // o botão durante o QTE. Verifica se o round está ativo e se o
    // InstigatorPawn é o jogador correto antes de registrar.
    // Seguro chamar a qualquer momento — ignora se não for o momento certo.
    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable|SkillCheck")
    void TryRegisterHit(AActor* InstigatorPawn);

private:

    // ----------------------------------------------------------
    // ESTADO — recurso (replicado)
    // ----------------------------------------------------------

    UPROPERTY(ReplicatedUsing = OnRep_IsDepleted, VisibleInstanceOnly, Category = "Gather|State")
    bool bIsDepleted = false;

    UPROPERTY(ReplicatedUsing = OnRep_CurrentHP, VisibleInstanceOnly, Category = "Gather|State")
    float CurrentHP = 0.0f;

    FTimerHandle RespawnTimerHandle;

    // ----------------------------------------------------------
    // ESTADO — Skill Check (não replicado, cliente local)
    // ----------------------------------------------------------

    // true enquanto o ponteiro estiver girando
    bool bRoundActive = false;

    // Ângulo atual normalizado (0.0 a 1.0)
    float CurrentAngle = 0.0f;

    // Voltas por segundo (1.0 / NeedleRotationTime)
    float AngularSpeed = 0.0f;

    // Zonas do round atual
    float RoundGoodStart    = 0.0f;
    float RoundGoodSize     = 0.0f;
    float RoundPerfectStart = 0.0f;
    float RoundPerfectSize  = 0.0f;

    // Ator do jogador — usado para enviar o GameplayEvent
    UPROPERTY()
    TObjectPtr<AActor> CachedInstigator;

    // ----------------------------------------------------------
    // REP NOTIFIES
    // ----------------------------------------------------------

    UFUNCTION()
    void OnRep_IsDepleted();

    UFUNCTION()
    void OnRep_CurrentHP();

    // ----------------------------------------------------------
    // HELPERS
    // ----------------------------------------------------------

    bool IsAngleInRange(float Angle, float Start, float Size) const;
    void Internal_SendHitEvent(EZfGatherHitResult Result);
    void Internal_OnRespawnTimerExpired();
};