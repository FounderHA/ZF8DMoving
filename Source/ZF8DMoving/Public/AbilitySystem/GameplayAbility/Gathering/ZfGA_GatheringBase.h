// Copyright ZfGame Studio. All Rights Reserved.
// ZfGA_GatherBase.h
// Gameplay Ability que executa o loop completo de coleta de recursos.
//
// SISTEMA DE DANO:
// O loop não tem número fixo de golpes — continua até o HP do recurso
// chegar a zero. Cada golpe causa:
// BaseDamage(ferramenta) * DamageMultiplier(recurso) * DamageMultiplier(QTE)
//
// SISTEMA DE SCORE:
// Corre em paralelo — cada golpe acumula um valor de score.
// Ao fim do loop, o score médio define a qualidade dos drops.
//
// FLUXO:
// 1. Valida ferramenta e recurso
// 2. Loop enquanto ResourceHP > 0:
//    a. K2_OnQTEStarted → Blueprint exibe o círculo com as duas zonas
//    b. Aguarda GameplayEvent com resultado do QTE
//    c. Calcula dano e score do golpe
//    d. K2_OnHitImpact → Blueprint feedback visual
//    e. Subtrai dano do HP restante
// 3. Score final → avalia loot table → spawna pickups
// 4. K2_OnDropsResolved → Blueprint feedback visual
// 5. Deplete no GatherableComponent

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GatheringSystem/ZfGatheringTypes.h"
#include "ZfGA_GatheringBase.generated.h"

// Forward declarations
class UZfGatheringComponent;
class UZfGatheringResourceData;
class UZfEquipmentComponent;
class UAbilityTask_WaitGameplayEvent;

// ============================================================
// FZfGatherDropResult
// ============================================================

USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfGatherDropResult
{
    GENERATED_BODY()

    // ItemDefinition do item dropado (já carregado)
    UPROPERTY(BlueprintReadOnly, Category = "Gather|Drop")
    TObjectPtr<class UZfItemDefinition> ItemDefinition = nullptr;

    // Quantidade sorteada já com DropMultiplier aplicado
    UPROPERTY(BlueprintReadOnly, Category = "Gather|Drop")
    int32 Quantity = 0;

    // Tier do item criado
    UPROPERTY(BlueprintReadOnly, Category = "Gather|Drop")
    int32 ItemTier = 0;

    // Raridade do item criado
    UPROPERTY(BlueprintReadOnly, Category = "Gather|Drop")
    EZfItemRarity ItemRarity = EZfItemRarity::Common;

    // Raio de scatter para posição dos pickups
    UPROPERTY(BlueprintReadOnly, Category = "Gather|Drop")
    float SpawnScatterRadius = 80.0f;
};

// ============================================================
// UZfGA_GatherBase
// ============================================================

UCLASS(Abstract, BlueprintType, Blueprintable)
class ZF8DMOVING_API UZfGA_GatheringBase : public UGameplayAbility
{
    GENERATED_BODY()

public:

    UZfGA_GatheringBase();

    // ----------------------------------------------------------
    // CONFIGURAÇÃO
    // ----------------------------------------------------------

    // Slot onde a ferramenta deve estar equipada.
    // Ex: "EquipmentSlot.MainHand"
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Config",
        meta = (GameplayTagFilter = "EquipmentSlot"))
    FGameplayTag ToolSlotTag;

    // Tag do GameplayEvent que ativa esta ability.
    // O sistema de interação dispara este evento com o ator
    // do recurso em EventData.OptionalObject.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Config")
    FGameplayTag GatherActivationEventTag;

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

    // Chamado no início de cada golpe — exibe o círculo do QTE com as duas zonas.
    // GoodSize e PerfectSize são normalizados (0.0 a 1.0) e vêm do
    // FZfGatherToolEntry do recurso para a ferramenta atual.
    // O WBP_SkillCheck deve converter para graus: Degrees = Size * 360.
    // PerfectZone fica centralizada dentro da GoodZone:
    //   Margin = (GoodDegrees - PerfectDegrees) / 2
    //   PerfectZoneStart = GoodZoneStart + Margin
    //
    // @param GoodSize      — tamanho da zona externa/verde   (0.0 a 1.0)
    // @param PerfectSize   — tamanho da zona interna/amarela (0.0 a 1.0, < GoodSize)
    // @param WindowSeconds — tempo disponível para reagir
    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|GatherAbility")
    void K2_OnQTEStarted(float GoodSize, float PerfectSize, float NeedleRotationTime);

    // Chamado após cada golpe — feedback visual e sonoro.
    // @param HitResult    — resultado do QTE
    // @param DamageDealt  — dano causado neste golpe
    // @param RemainingHP  — HP restante do recurso após o golpe
    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|GatherAbility")
    void K2_OnHitImpact(EZfGatherHitResult HitResult, float DamageDealt, float RemainingHP);

    // Chamado ao fim de todos os golpes — feedback visual dos drops.
    // Pickups já foram spawnados no mundo antes desta chamada.
    // @param Drops      — itens que foram spawnados
    // @param ScoreFinal — score final (0.0 a 1.0)
    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|GatherAbility")
    void K2_OnDropsResolved(const TArray<FZfGatherDropResult>& Drops, float ScoreFinal);

    // Chamado quando a ability é cancelada antes de terminar.
    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|GatherAbility")
    void K2_OnGatherCancelled();

private:

    // ----------------------------------------------------------
    // ESTADO INTERNO DO LOOP
    // ----------------------------------------------------------

    UPROPERTY()
    TObjectPtr<UZfGatheringComponent> TargetGatherableComponent;

    UPROPERTY()
    TObjectPtr<UZfGatheringResourceData> TargetResourceData;

    // Stats resolvidos da ferramenta (base + modifiers).
    // GoodSize e PerfectSize são populados em Internal_ValidateAndSetup
    // a partir do FZfGatherToolEntry do recurso — não vêm do fragment.
    FZfResolvedGatherStats ResolvedToolStats;

    // DamageMultiplier do recurso para a ferramenta atual.
    // Cacheado na setup para evitar busca a cada golpe.
    float CachedResourceDamageMultiplier = 1.0f;

    // Registro de todos os golpes para calcular score final
    TArray<FZfGatherHitRecord> HitRecords;

    // Task ativa aguardando resultado do QTE
    UPROPERTY()
    TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveQTEWaitTask;

    // ----------------------------------------------------------
    // FUNÇÕES INTERNAS
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