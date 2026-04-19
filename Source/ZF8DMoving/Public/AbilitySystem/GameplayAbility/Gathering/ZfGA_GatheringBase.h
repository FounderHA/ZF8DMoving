// Copyright ZfGame Studio. All Rights Reserved.
// ZfGA_GatherBase.h

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GatheringSystem/ZfGatheringTypes.h"
#include "ZfGA_GatheringBase.generated.h"

class UZfGatheringComponent;
class UZfGatheringResourceData;
class UZfEquipmentComponent;
class UAbilityTask_WaitGameplayEvent;
class UInputAction;

// ============================================================
// FZfGatherDropResult
// ============================================================

USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfGatherDropResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Gather|Drop")
    TObjectPtr<class UZfItemDefinition> ItemDefinition = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Gather|Drop")
    int32 Quantity = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Gather|Drop")
    int32 ItemTier = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Gather|Drop")
    EZfItemRarity ItemRarity = EZfItemRarity::Common;

    UPROPERTY(BlueprintReadOnly, Category = "Gather|Drop")
    float SpawnScatterRadius = 80.0f;
};

// ============================================================
// UZfGA_GatherBase
// ============================================================
//
// FLUXO MULTIPLAYER:
// - ActivateAbility roda nos dois lados (servidor e cliente dono)
// - Servidor: valida, faz commit, trava recurso, inicia QTE, bind de cancelamento
// - Cliente dono: bind de input para registrar clique (Server RPC)
//
// CANCELAMENTO (servidor):
// - Movimento:         Timer verifica velocidade do Avatar a cada 0.1s
// - Outras abilities:  Delegate OnAbilityActivated do ASC
// - Status tags:       RegisterGameplayTagEvent para cada tag em CancellationStatusTags
//
// REGISTRO DE HIT:
// - Cliente pressiona botão → Enhanced Input → Server_RegisterHit()
// - Servidor recebe RPC → RegisterHit() no componente → avalia ângulo interno
// - Cliente NUNCA envia dados de ângulo — servidor é a fonte da verdade

UCLASS(Abstract, BlueprintType, Blueprintable)
class ZF8DMOVING_API UZfGA_GatheringBase : public UGameplayAbility
{
    GENERATED_BODY()

public:

    UZfGA_GatheringBase();

    // ----------------------------------------------------------
    // CONFIGURAÇÃO
    // ----------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Config",
        meta = (GameplayTagFilter = "EquipmentSlot"))
    FGameplayTag ToolSlotTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Config")
    FGameplayTag GatherActivationEventTag;

    // Input Action do botão de hit durante o QTE.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Config")
    TObjectPtr<UInputAction> GatherHitInputAction;

    // Input Action de movimento (W/A/S/D ou equivalente).
    // Ao ser pressionado pelo cliente, cancela imediatamente a coleta localmente.
    // O servidor também detecta movimento via timer — os dois são complementares.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Config")
    TObjectPtr<UInputAction> MoveInputAction;

    // Tags de status no ASC do jogador que cancelam a coleta quando aplicadas.
    // Ex: "Status.HitReact", "Status.Knockback", "Status.Stunned"
    // Estas tags devem ser adicionadas pelos GameplayEffects de combate/CC.
    // A GA observa no servidor — não precisa tocar nesta GA ao adicionar novos status.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Config")
    TArray<FGameplayTag> CancellationStatusTags;

    // ----------------------------------------------------------
    // GETTERS — Blueprint
    // ----------------------------------------------------------

    UFUNCTION(BlueprintPure, Category = "Zf|GatherAbility")
    UZfGatheringResourceData* GetTargetResourceData() const { return TargetResourceData; }

    UFUNCTION(BlueprintPure, Category = "Zf|GatherAbility")
    UZfGatheringComponent* GetTargetGatherableComponent() const { return TargetGatherableComponent; }

    UFUNCTION(BlueprintPure, Category = "Zf|GatherAbility")
    FZfResolvedGatherStats GetResolvedToolStats() const { return ResolvedToolStats; }

    // ----------------------------------------------------------
    // OVERRIDES DO GAS
    // ----------------------------------------------------------

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    virtual void CancelAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateCancelAbility) override;

    // ----------------------------------------------------------
    // HOOKS PARA BLUEPRINT
    // ----------------------------------------------------------

    // Chamado no servidor ao iniciar cada round do QTE.
    // Use para tocar montagens de animação, sons, etc. (lógica de servidor).
    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|GatherAbility")
    void K2_OnQTEStarted(float GoodSize, float PerfectSize, float NeedleRotationTime);

    // Chamado no CLIENTE ao iniciar cada round (via delegate do componente).
    // Use para criar e mostrar a SkillCheck widget aqui.
    // NOTA: Ao criar a widget, chame InitSkillCheck() passando o componente
    //       retornado por GetTargetGatherableComponent().
    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|GatherAbility")
    void K2_OnClientRoundBegun(float GoodSize, float PerfectSize, float NeedleRotTime);

    // Chamado no servidor após cada hit ser processado.
    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|GatherAbility")
    void K2_OnHitImpact(EZfGatherHitResult HitResult, float DamageDealt, float RemainingHP);

    // Chamado no servidor ao finalizar a coleta com sucesso.
    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|GatherAbility")
    void K2_OnDropsResolved(const TArray<FZfGatherDropResult>& Drops, float ScoreFinal);

    // Chamado no servidor ao cancelar a coleta.
    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|GatherAbility")
    void K2_OnGatherCancelled();

private:

    UPROPERTY()
    TObjectPtr<UZfGatheringComponent> TargetGatherableComponent;

    UPROPERTY()
    TObjectPtr<UZfGatheringResourceData> TargetResourceData;

    FZfResolvedGatherStats ResolvedToolStats;
    float CachedResourceDamageMultiplier = 1.0f;
    TArray<FZfGatherHitRecord> HitRecords;

    UPROPERTY()
    TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveQTEWaitTask;


    // ----------------------------------------------------------
    // Tool Modifiers Bonus
    // ----------------------------------------------------------
    UPROPERTY()
    float CachedScoreBonus = 0.0f;

    UPROPERTY()
    float CachedDamageBonus = 0.0f;

    UPROPERTY()
    float CachedGoodSizeBonus = 0.0f;

    UPROPERTY()
    float CachedPerfectSizeBonus = 0.0f;

    UPROPERTY()
    float CachedNeedleTimeBonus = 0.0f;

    // ----------------------------------------------------------
    // INPUT — Hit (cliente)
    // ----------------------------------------------------------

    uint32 HitInputHandle  = 0;
    uint32 MoveInputHandle = 0;

    void Internal_BindHitInput();
    void Internal_UnbindHitInput();
    void Internal_OnHitInputPressed();
    void Internal_OnMovementInputPressed();

    // RPC: cliente pressiona o botão → servidor registra o hit no componente.
    // O servidor usa o ângulo INTERNO do componente — cliente não envia ângulo.
    UFUNCTION(Server, Reliable)
    void Server_RegisterHit();

    // ----------------------------------------------------------
    // CANCELAMENTO (servidor)
    // ----------------------------------------------------------

    // Timer que verifica se o Avatar se moveu (dedicated server não tem input)
    FTimerHandle MovementCheckHandle;
    FVector      GatherStartLocation;

    // Handle do delegate OnAbilityActivated do ASC
    FDelegateHandle AbilityActivatedDelegateHandle;

    // Handles dos eventos de tag de status
    TArray<FDelegateHandle> StatusTagEventHandles;

    void Internal_BindCancellationListeners();
    void Internal_UnbindCancellationListeners();

    void Internal_CheckMovement();

    // AbilityActivatedCallbacks tem assinatura void(UGameplayAbility*) — apenas um parâmetro.
    void Internal_OnAnyAbilityActivated(UGameplayAbility* ActivatedAbility);

    void Internal_OnStatusTagChanged(const FGameplayTag Tag, int32 NewCount);

    // ----------------------------------------------------------
    // CLIENTE — notificações visuais
    // ----------------------------------------------------------

    // Chamado quando o cliente recebe OnSkillCheckRoundBegun do componente.
    // Repassa para K2_OnClientRoundBegun (hook Blueprint para criar a widget).
    UFUNCTION()
    void Internal_OnClientRoundBegun(
        float GoodStart, float GoodSize,
        float PerfectStart, float PerfectSize,
        float NeedleRotTime);

    void Internal_BindClientDelegates();
    void Internal_UnbindClientDelegates();

    // ----------------------------------------------------------
    // LÓGICA PRINCIPAL (servidor)
    // ----------------------------------------------------------

    bool Internal_ValidateAndSetup(const FGameplayEventData* TriggerEventData);
    void Internal_ExecuteNextHit();

    UFUNCTION()
    void Internal_OnQTEResultReceived(FGameplayEventData EventData);

    void Internal_ResolveAndFinish();
    float Internal_CalculateFinalScore() const;
    TArray<FZfGatherDropResult> Internal_ResolveLootTable(float FinalScore) const;
    void Internal_SpawnDrops(const TArray<FZfGatherDropResult>& Drops);
    EZfGatherHitResult Internal_TagToHitResult(const FGameplayTag& EventTag) const;
    void Internal_Cleanup();
};