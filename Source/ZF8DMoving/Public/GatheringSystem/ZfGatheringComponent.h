// Copyright ZfGame Studio. All Rights Reserved.
// ZfGatherableComponent.h
// Component que vive nos atores de recurso do mundo e gerencia
// o estado de coleta: HP atual, disponibilidade, depleção e respawn.
//
// CONCEITO:
// Qualquer ator que queira ser coletável adiciona este component.
// Ele é a interface entre o mundo (o ator do recurso) e o sistema
// de coleta (a GA_GatherBase).
//
// RESPONSABILIDADE:
// - Guardar a referência ao ZfGatherResourceData
// - Manter o HP atual do recurso entre sessões de coleta
// - Controlar se o recurso está disponível ou esgotado
// - Gerenciar o timer de respawn
// - Disparar delegates para o Blueprint reagir visualmente
//
// HP PERSISTENTE:
// O CurrentHP vive aqui — não na GA. Se o jogador para no meio
// da coleta e volta, o recurso continua com o HP danificado.
// O HP reseta apenas quando o recurso é esgotado e respawna.
//
// REPLICAÇÃO:
// bIsDepleted e CurrentHP são replicados para que todos os
// clientes vejam o estado correto do recurso.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "ZfGatheringComponent.generated.h"

// Forward declarations
class UZfGatheringResourceData;

// ============================================================
// DELEGATES
// ============================================================

// Disparado quando o recurso é esgotado.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnResourceDepleted);

// Disparado quando o recurso reaparece após respawn.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnResourceRespawned);

// Disparado quando o HP do recurso muda — útil para barra de vida no Blueprint.
// @param CurrentHP — HP atual após o dano
// @param MaxHP     — HP máximo do recurso
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnResourceHPChanged, float, CurrentHP, float, MaxHP);

// ============================================================
// UZfGatherableComponent
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

    // DataAsset que define este recurso.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Config")
    TObjectPtr<UZfGatheringResourceData> GatherResourceData;

    // ----------------------------------------------------------
    // DELEGATES
    // ----------------------------------------------------------

    UPROPERTY(BlueprintAssignable, Category = "Gather|Events")
    FOnResourceDepleted OnResourceDepleted;

    UPROPERTY(BlueprintAssignable, Category = "Gather|Events")
    FOnResourceRespawned OnResourceRespawned;

    // Disparado a cada golpe — Blueprint pode exibir barra de HP diminuindo.
    UPROPERTY(BlueprintAssignable, Category = "Gather|Events")
    FOnResourceHPChanged OnResourceHPChanged;

    // ----------------------------------------------------------
    // CICLO DE VIDA
    // ----------------------------------------------------------

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ----------------------------------------------------------
    // ESTADO — leitura
    // ----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    bool IsAvailable() const { return !bIsDepleted; }

    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    bool IsDepleted() const { return bIsDepleted; }

    // Retorna o HP atual do recurso.
    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    float GetCurrentHP() const { return CurrentHP; }

    // Retorna o HP máximo definido no DataAsset.
    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    float GetMaxHP() const;

    // Retorna o HP como percentual (0.0 a 1.0) — útil para barra de vida.
    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    float GetHPPercent() const;

    // Retorna o tempo restante até o respawn.
    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    float GetRespawnTimeRemaining() const;

    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    UZfGatheringResourceData* GetGatherResourceData() const { return GatherResourceData; }

    // ----------------------------------------------------------
    // ESTADO — escrita
    // Chamadas pela GA_GatherBase durante e após a coleta.
    // ----------------------------------------------------------

    // Aplica dano ao HP atual do recurso.
    // Se HP chegar a 0, chama Deplete automaticamente.
    // Chamado pela GA_GatherBase a cada golpe.
    // @param DamageAmount — dano a aplicar
    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    void ApplyDamage(float DamageAmount);

    // Esgota o recurso e inicia o timer de respawn.
    // Normalmente chamado internamente por ApplyDamage quando HP <= 0.
    // Pode ser chamado externamente para forçar depleção.
    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    void Deplete();

    // Força o respawn imediato ignorando o timer.
    UFUNCTION(BlueprintCallable, Category = "Zf|Gatherable")
    void ForceRespawn();

private:

    // ----------------------------------------------------------
    // ESTADO INTERNO REPLICADO
    // ----------------------------------------------------------

    // True quando o recurso foi esgotado e está aguardando respawn.
    // Replicado — clientes veem e reagem via OnRep.
    UPROPERTY(ReplicatedUsing = OnRep_IsDepleted, VisibleInstanceOnly, Category = "Gather|State")
    bool bIsDepleted = false;

    // HP atual do recurso — persiste entre sessões de coleta.
    // Replicado para que clientes exibam barra de HP correta.
    UPROPERTY(ReplicatedUsing = OnRep_CurrentHP, VisibleInstanceOnly, Category = "Gather|State")
    float CurrentHP = 0.0f;

    // Handle do timer de respawn.
    FTimerHandle RespawnTimerHandle;

    // ----------------------------------------------------------
    // REP NOTIFIES
    // ----------------------------------------------------------

    // Clientes reagem à mudança de bIsDepleted.
    UFUNCTION()
    void OnRep_IsDepleted();

    // Clientes disparam OnResourceHPChanged quando HP muda.
    UFUNCTION()
    void OnRep_CurrentHP();

    // ----------------------------------------------------------
    // FUNÇÕES INTERNAS
    // ----------------------------------------------------------

    void Internal_OnRespawnTimerExpired();
};