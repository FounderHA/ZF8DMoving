// Copyright ZfGame Studio. All Rights Reserved.
// ZfGatherableComponent.h
//
// RESPONSABILIDADES:
// - Estado do recurso: HP, depleção, respawn
// - Lock de coleta: apenas um jogador por vez (CurrentGatherer)
// - QTE server-authoritative: ângulo e avaliação no servidor
//   Dados replicados ao cliente via RepNotify para feedback visual
//
// FLUXO MULTIPLAYER:
// 1. GA chama StartGatheringLock(Instigator) → trava o recurso no servidor
// 2. GA chama BeginSkillCheckRound()  → servidor gera zonas, seta RoundData
// 3. OnRep_RoundData dispara no cliente → widget inicia tick visual local
// 4. Jogador clica → GA::Server_RegisterHit() → Component::RegisterHit()
// 5. Servidor avalia ângulo → seta LastHitData → envia GameplayEvent para GA
// 6. OnRep_LastHitData dispara no cliente → widget exibe feedback
// 7. GA processa dano → loop ou finaliza
// 8. GA chama EndSkillCheck() → para Tick, libera lock
//
// SEGURANÇA:
// - Tick do ângulo roda APENAS no servidor (HasAuthority check)
// - RegisterHit() avalia o ângulo do SERVIDOR — cliente não envia dados de ângulo
// - Lock impede dois jogadores coletando simultaneamente
//
// WIDGET:
// - Não recebe mais ângulo via delegate frame-a-frame (removido OnSkillCheckAngleUpdated)
// - Inicia seu próprio tick visual local ao receber OnSkillCheckRoundBegun
// - NeedleRotTime é replicado junto com as zonas para sincronizar a velocidade visual

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

// Novo round iniciado — disparado pelo OnRep_RoundData no cliente.
// Inclui NeedleRotTime para que a widget inicie o tick visual na velocidade correta.
// ATENÇÃO: Assinatura alterada em relação à versão anterior — agora tem 5 parâmetros.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnSkillCheckRoundBegun,
    float, GoodStart,
    float, GoodSize,
    float, PerfectStart,
    float, PerfectSize,
    float, NeedleRotTime);

// Resultado do hit avaliado pelo servidor — disparado pelo OnRep_LastHitData no cliente.
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

    // Disparado no cliente quando OnRep_RoundData é recebido.
    // A widget faz bind aqui para posicionar zonas e iniciar tick visual.
    UPROPERTY(BlueprintAssignable, Category = "Gather|SkillCheck|Events")
    FOnSkillCheckRoundBegun OnSkillCheckRoundBegun;

    // Disparado no cliente quando OnRep_LastHitData é recebido.
    // A widget faz bind aqui para exibir o feedback do resultado.
    UPROPERTY(BlueprintAssignable, Category = "Gather|SkillCheck|Events")
    FOnSkillCheckHitEvaluated OnSkillCheckHitEvaluated;

    // ----------------------------------------------------------
    // CICLO DE VIDA
    // ----------------------------------------------------------

    virtual void BeginPlay() override;

    // Tick roda APENAS no servidor (HasAuthority check interno).
    // Avança CurrentAngle e detecta volta completa (Missed).
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ----------------------------------------------------------
    // ESTADO DO RECURSO — leitura
    // ----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    bool IsAvailable() const { return !bIsDepleted && !IsBeingGathered(); }

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

    // Retorna os dados do round atual.
    // Usado em C++ por InitSkillCheck para inicialização tardia da widget.
    // Não exposto ao Blueprint — FZfSkillCheckRoundData é uma struct interna.
    const FZfSkillCheckRoundData& GetCurrentRoundData() const { return RoundData; }

    // ----------------------------------------------------------
    // LOCK DE COLETA
    // Garante que apenas um jogador colete o recurso por vez.
    // ----------------------------------------------------------

    // True se algum jogador está coletando este recurso no momento.
    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    bool IsBeingGathered() const { return CurrentGatherer != nullptr; }

    // Retorna o pawn que está coletando. Nullptr se livre.
    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    AActor* GetCurrentGatherer() const { return CurrentGatherer; }

    // Trava o recurso para o Instigator.
    // Deve ser chamado pela GA ANTES do primeiro BeginSkillCheckRound.
    // Só executa com autoridade — ignorado em clientes.
    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    void StartGatheringLock(AActor* Instigator);

    // ----------------------------------------------------------
    // ESTADO DO RECURSO — escrita (servidor)
    // ----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    void ApplyDamage(float DamageAmount);

    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    void Deplete();

    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    void ForceRespawn();

    // ----------------------------------------------------------
    // SKILL CHECK — chamados pela GA no servidor
    // ----------------------------------------------------------

    // Inicia um novo round: gera zonas aleatórias, liga o Tick no servidor,
    // seta RoundData que replica ao cliente via RepNotify.
    // NOTA: StartGatheringLock deve ser chamado antes do primeiro round.
    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable|SkillCheck")
    void BeginSkillCheckRound(
        float InGoodSize,
        float InPerfectSize,
        float InNeedleRotTime);

    // Para o Tick do QTE e libera o lock de coleta.
    // Chamado pela GA no cleanup (cancelamento ou conclusão).
    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable|SkillCheck")
    void EndSkillCheck();

    // Registra o hit no ângulo ATUAL DO SERVIDOR.
    // Chamado pela GA via Server_RegisterHit RPC — sem dados de ângulo do cliente.
    // Avalia, seta LastHitData (replica ao cliente) e envia GameplayEvent para a GA.
    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable|SkillCheck")
    void RegisterHit();

    // Avalia um ângulo sem disparar eventos — útil para debug.
    UFUNCTION(BlueprintPure, Category = "Zf|Gatherable|SkillCheck")
    EZfGatherHitResult EvaluateAngle(float Angle) const;

private:

    // ----------------------------------------------------------
    // ESTADO — recurso (replicado)
    // ----------------------------------------------------------

    UPROPERTY(ReplicatedUsing = OnRep_IsDepleted, VisibleInstanceOnly, Category = "Gather|State")
    bool bIsDepleted = false;

    UPROPERTY(ReplicatedUsing = OnRep_CurrentHP, VisibleInstanceOnly, Category = "Gather|State")
    float CurrentHP = 0.0f;

    // Pawn que está coletando o recurso atualmente.
    // Replicado para que todos os clientes saibam que o recurso está ocupado.
    // Reservado para uso futuro na UI (ex: ocultar widget de interação para outros players).
    UPROPERTY(ReplicatedUsing = OnRep_CurrentGatherer, VisibleInstanceOnly, Category = "Gather|State")
    TObjectPtr<AActor> CurrentGatherer;

    FTimerHandle RespawnTimerHandle;

    // ----------------------------------------------------------
    // ESTADO — Skill Check (replicado via structs)
    // ----------------------------------------------------------

    // Dados do round atual — replicados ao cliente para iniciar o visual.
    // OnRep_RoundData dispara OnSkillCheckRoundBegun → widget posiciona zonas.
    UPROPERTY(ReplicatedUsing = OnRep_RoundData, VisibleInstanceOnly, Category = "Gather|SkillCheck")
    FZfSkillCheckRoundData RoundData;

    // Resultado do último hit — replicado ao cliente para exibir o feedback.
    // OnRep_LastHitData dispara OnSkillCheckHitEvaluated → widget anima resultado.
    UPROPERTY(ReplicatedUsing = OnRep_LastHitData, VisibleInstanceOnly, Category = "Gather|SkillCheck")
    FZfSkillCheckHitData LastHitData;

    // ----------------------------------------------------------
    // ESTADO — Skill Check (servidor apenas, NÃO replicado)
    // Estes valores são a fonte da verdade para a avaliação de hits.
    // ----------------------------------------------------------

    bool  bRoundActive      = false;
    float CurrentAngle      = 0.0f;  // 0.0–1.0, fonte da verdade
    float AngularSpeed      = 0.0f;  // voltas/segundo

    // Zonas do round — usadas por EvaluateAngle()
    float RoundGoodStart    = 0.0f;
    float RoundGoodSize     = 0.0f;
    float RoundPerfectStart = 0.0f;
    float RoundPerfectSize  = 0.0f;

    // ----------------------------------------------------------
    // REP NOTIFIES
    // ----------------------------------------------------------

    UFUNCTION()
    void OnRep_IsDepleted();

    UFUNCTION()
    void OnRep_CurrentHP();

    // Reservado para uso futuro (ex: bloquear indicadores em outros clientes).
    UFUNCTION()
    void OnRep_CurrentGatherer();

    // Notifica o cliente de novo round → dispara OnSkillCheckRoundBegun.
    UFUNCTION()
    void OnRep_RoundData();

    // Notifica o cliente do resultado → dispara OnSkillCheckHitEvaluated.
    UFUNCTION()
    void OnRep_LastHitData();

    // ----------------------------------------------------------
    // HELPERS
    // ----------------------------------------------------------

    bool IsAngleInRange(float Angle, float Start, float Size) const;
    void Internal_SendHitEvent(EZfGatherHitResult Result);
    void Internal_OnRespawnTimerExpired();
    void Internal_ReleaseGatheringLock();
};