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
    // Configure na GA Blueprint com o mesmo IA_Interact do seu projeto.
    // A GA registra o binding ao ativar e remove ao terminar.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Config")
    TObjectPtr<UInputAction> GatherHitInputAction;

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

    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|GatherAbility")
    void K2_OnQTEStarted(float GoodSize, float PerfectSize, float NeedleRotationTime);

    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|GatherAbility")
    void K2_OnHitImpact(EZfGatherHitResult HitResult, float DamageDealt, float RemainingHP);

    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|GatherAbility")
    void K2_OnDropsResolved(const TArray<FZfGatherDropResult>& Drops, float ScoreFinal);

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

    // Handle do binding de input — usado para remover no cleanup
    uint32 HitInputHandle = 0;

    bool Internal_ValidateAndSetup(const FGameplayEventData* TriggerEventData);
    void Internal_ExecuteNextHit();
    void Internal_BindHitInput();
    void Internal_UnbindHitInput();
    void Internal_OnHitInputPressed();

    UFUNCTION()
    void Internal_OnQTEResultReceived(FGameplayEventData EventData);

    void Internal_ResolveAndFinish();
    float Internal_CalculateFinalScore() const;
    TArray<FZfGatherDropResult> Internal_ResolveLootTable(float FinalScore) const;
    void Internal_SpawnDrops(const TArray<FZfGatherDropResult>& Drops);
    EZfGatherHitResult Internal_TagToHitResult(const FGameplayTag& EventTag) const;
    void Internal_Cleanup();
};