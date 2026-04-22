// Copyright ZfGame Studio. All Rights Reserved.
// ZfGatherTypes.h
// Define todos os tipos base do sistema de coleta de recursos.
//
// SISTEMA DE DANO:
// O recurso tem HP e cada golpe causa dano baseado no BaseDamage
// da ferramenta * multiplicadores. O loop termina quando HP chega a 0.
//
// SISTEMA DE SCORE:
// Corre em paralelo — acumula valores por golpe e define a qualidade
// dos drops ao fim.
//
// MULTIPLAYER / SERVER-AUTHORITATIVE:
// FZfSkillCheckRoundData — replicada do servidor ao cliente via RepNotify.
//   O cliente usa os dados para posicionar zonas na widget e iniciar
//   o tick visual local da agulha na velocidade correta.
//
// FZfSkillCheckHitData — replicada do servidor ao cliente via RepNotify.
//   O cliente usa o resultado para exibir o feedback visual correto.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Inventory/ZfInventoryTypes.h"
#include "ZfGatheringTypes.generated.h"

ZF8DMOVING_API DECLARE_LOG_CATEGORY_EXTERN(LogZfGathering, Log, All);


// ============================================================
// EZfGatherHitResult
// ============================================================

UENUM(BlueprintType)
enum class EZfGatherHitResult : uint8
{
    Perfect UMETA(DisplayName = "Perfect"),  // Zona interna  — 1.4x dano | score 1.0
    Good    UMETA(DisplayName = "Good"),     // Zona externa  — 1.1x dano | score 0.6
    Bad     UMETA(DisplayName = "Bad"),      // Fora das zonas — 1.0x dano | score 0.1
    Missed  UMETA(DisplayName = "Missed")   // Sem clique    — 1.0x dano | score 0.0
};

// ============================================================
// FZfSkillCheckRoundData
// ============================================================
// Dados de um round do Skill Check — replicados do servidor para o
// cliente via RepNotify no GatheringComponent.
//
// O cliente usa esses dados para:
//   1. Posicionar as zonas (Good/Perfect) na widget
//   2. Iniciar o tick visual local da agulha em NeedleRotTime
//
// RoundIndex é incrementado a cada round pelo servidor para garantir
// que o RepNotify dispare mesmo que os valores gerados sejam idênticos
// ao round anterior (valores normalizados 0.0–1.0 podem se repetir).
// ============================================================

USTRUCT()
struct ZF8DMOVING_API FZfSkillCheckRoundData
{
    GENERATED_BODY()

    UPROPERTY()
    float GoodStart = 0.f;

    UPROPERTY()
    float GoodSize = 0.f;

    UPROPERTY()
    float PerfectStart = 0.f;

    UPROPERTY()
    float PerfectSize = 0.f;

    // Tempo em segundos para uma volta completa da agulha.
    // Replicado para que o cliente inicie o tick visual na mesma velocidade.
    UPROPERTY()
    float NeedleRotTime = 2.f;

    // Contador incremental — garante RepNotify a cada novo round.
    UPROPERTY()
    uint8 RoundIndex = 0;
};

// ============================================================
// FZfSkillCheckHitData
// ============================================================
// Resultado do último hit avaliado pelo servidor — replicado ao
// cliente via RepNotify para que a widget exiba o feedback correto.
//
// HitIndex é incrementado a cada hit para garantir que o RepNotify
// dispare mesmo para resultados consecutivos iguais (ex: dois Bad).
// ============================================================

USTRUCT()
struct ZF8DMOVING_API FZfSkillCheckHitData
{
    GENERATED_BODY()

    UPROPERTY()
    EZfGatherHitResult Result = EZfGatherHitResult::Missed;

    // Contador incremental — garante RepNotify a cada novo hit.
    UPROPERTY()
    uint8 HitIndex = 0;
};

// ============================================================
// FZfGatherToolEntry
// ============================================================

USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfGatherToolEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Tool",
        meta = (GameplayTagFilter = "Gathering.Tool"))
    FGameplayTag ToolTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Tool",
        meta = (ClampMin = "0.1"))
    float DamageMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Tool",
        meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float GoodSize = 0.20f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Tool",
        meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float PerfectSize = 0.10f;
};

// ============================================================
// FZfGatherLootEntry
// ============================================================

USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfGatherLootEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Loot")
    TSoftObjectPtr<class UZfItemDefinition> ItemDefinition;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Loot",
        meta = (ClampMin = "1"))
    int32 QuantityMin = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Loot",
        meta = (ClampMin = "1"))
    int32 QuantityMax = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Loot",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DropChance = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Loot",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ScoreMinimum = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Loot",
        meta = (ClampMin = "0"))
    int32 ItemTier = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Loot")
    EZfItemRarity ItemRarity = EZfItemRarity::Common;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Loot",
        meta = (ClampMin = "0.0"))
    float SpawnScatterRadius = 80.0f;
};

// ============================================================
// FZfGatherHitRecord
// ============================================================

USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfGatherHitRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Gather|Record")
    EZfGatherHitResult HitResult = EZfGatherHitResult::Missed;

    UPROPERTY(BlueprintReadOnly, Category = "Gather|Record")
    float DamageDealt = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Gather|Record")
    float ScoreValue = 0.0f;

    static float GetDamageMultiplierForResult(EZfGatherHitResult Result)
    {
        switch (Result)
        {
            case EZfGatherHitResult::Perfect:   return 1.4f;
            case EZfGatherHitResult::Good:      return 1.1f;
            case EZfGatherHitResult::Bad:       return 1.0f;
            case EZfGatherHitResult::Missed:    return 1.0f;
            default:                            return 1.0f;
        }
    }

    static float GetScoreForResult(EZfGatherHitResult Result)
    {
        switch (Result)
        {
            case EZfGatherHitResult::Perfect:   return 1.0f;
            case EZfGatherHitResult::Good:      return 0.6f;
            case EZfGatherHitResult::Bad:       return 0.1f;
            case EZfGatherHitResult::Missed:    return 0.0f;
            default:                            return 0.0f;
        }
    }
};

// ============================================================
// FZfResolvedGatherStats
// ============================================================

USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfResolvedGatherStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Gather|Resolved")
    FGameplayTag ToolTag;

    UPROPERTY(BlueprintReadOnly, Category = "Gather|Resolved")
    float BaseDamage = 10.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Gather|Resolved")
    float DropMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Gather|Resolved")
    float ScoreBonus = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Gather|Resolved")
    float GoodSize = 0.20f;

    UPROPERTY(BlueprintReadOnly, Category = "Gather|Resolved")
    float PerfectSize = 0.10f;

    UPROPERTY(BlueprintReadOnly, Category = "Gather|Resolved")
    bool bIsValid = false;
};