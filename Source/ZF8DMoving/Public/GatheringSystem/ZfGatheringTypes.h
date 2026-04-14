// Copyright ZfGame Studio. All Rights Reserved.
// ZfGatherTypes.h
// Define todos os tipos base do sistema de coleta de recursos:
// enums, structs de configuração e structs de resultado.
//
// SISTEMA DE DANO:
// O recurso tem HP e cada golpe causa dano baseado no BaseDamage
// da ferramenta * multiplicadores. O loop termina quando HP chega a 0.
// O número de golpes é dinâmico — varia por ferramenta e habilidade.
//
// SISTEMA DE SCORE:
// Corre em paralelo — acumula valores por golpe e define a qualidade
// dos drops ao fim. Score alto = drops melhores e mais raros.
// Os dois sistemas são independentes mas complementares:
// Acertar Perfeitos causa mais dano (termina mais rápido)
// E acumula score mais alto (drops melhores).

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Inventory/ZfInventoryTypes.h"
#include "ZfGatheringTypes.generated.h"

// ============================================================
// EZfGatherHitResult
// ============================================================
// Resultado de um golpe individual no QTE.
//
// Cada resultado tem dois efeitos simultâneos:
//
// DANO (multiplicador sobre BaseDamage da ferramenta):
// Perfect → 1.4x  |  Good → 1.1x  |  Bad → 1.0x  |  Missed → 1.0x
//
// SCORE (acumulado para avaliar loot table):
// Perfect → 1.0  |  Good → 0.6  |  Bad → 0.1  |  Missed → 0.0
//
// Bad e Missed causam o mesmo dano mas score diferente —
// quem tentou e errou ainda contribui levemente para os drops.
//
// ZONAS DO QTE:
// PerfectZone fica centralizada dentro da GoodZone.
// A agulha é testada primeiro contra PerfectZone, depois GoodZone.
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
// FZfGatherToolEntry
// ============================================================
// Define como uma ferramenta específica interage com este recurso.
// Armazenada no array AllowedTools do ZfGatherResourceData.
//
// O dano final por golpe é:
// BaseDamage(ferramenta) * DamageMultiplier(entrada) * DamageMultiplier(QTE)
//
// DamageMultiplier permite que a mesma ferramenta seja mais ou menos
// eficiente dependendo do recurso — picareta de ferro é eficiente
// em minério de ferro mas ineficiente em cristal.
//
// As duas zonas do QTE (Good e Perfect) são definidas por ferramenta:
// GoodSize  — zona externa (verde), mais fácil de acertar.
// PerfectSize — zona interna (amarela), centralizada dentro da Good.
//              Deve ser menor que GoodSize.
// ============================================================

USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfGatherToolEntry
{
    GENERATED_BODY()

    // Tag que identifica a ferramenta aceita.
    // Deve bater com a ToolTag definida no ZfFragment_GatherTool.
    // Ex: "Tool.Pickaxe.Tier2", "Tool.Axe.Tier1"
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Tool",
        meta = (GameplayTagFilter = "Tool"))
    FGameplayTag ToolTag;

    // Multiplicador de dano desta ferramenta neste recurso.
    // Aplicado sobre o BaseDamage da ferramenta antes do multiplicador do QTE.
    // 1.0 = eficiência normal | 0.5 = ferramenta pouco adequada
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Tool",
        meta = (ClampMin = "0.1"))
    float DamageMultiplier = 1.0f;

    // Tamanho da zona externa (Good) do QTE para esta ferramenta.
    // Valor normalizado de 0.0 a 1.0 representando a proporção do círculo.
    // Ferramentas melhores têm zona maior (mais fácil de acertar).
    // Ex: 0.12 = zona pequena (difícil) | 0.35 = zona grande (fácil)
    // Deve ser maior que PerfectSize.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Tool",
        meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float GoodSize = 0.20f;

    // Tamanho da zona interna (Perfect) do QTE para esta ferramenta.
    // Fica centralizada dentro da GoodZone — acertar aqui dá bônus máximo.
    // Valor normalizado de 0.0 a 1.0 representando a proporção do círculo.
    // Deve ser menor que GoodSize.
    // Ex: 0.06 = zona muito pequena (difícil) | 0.12 = zona média
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Tool",
        meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float PerfectSize = 0.10f;
};

// ============================================================
// FZfGatherLootEntry
// ============================================================
// Define um item que pode ser obtido ao coletar este recurso.
// Cada entrada é avaliada individualmente pelo score final:
//   1. ScoreFinal >= ScoreMinimum → entrada elegível
//   2. Rola DropChance → decide se dropa
//   3. Sorteia Quantity entre Min e Max
// ============================================================

USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfGatherLootEntry
{
    GENERATED_BODY()

    // ItemDefinition do item que pode ser dropado.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Loot")
    TSoftObjectPtr<class UZfItemDefinition> ItemDefinition;

    // Quantidade mínima dropada (inclusive).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Loot",
        meta = (ClampMin = "1"))
    int32 QuantityMin = 1;

    // Quantidade máxima dropada (inclusive). Deve ser >= QuantityMin.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Loot",
        meta = (ClampMin = "1"))
    int32 QuantityMax = 1;

    // Chance de dropar quando elegível (0.0 a 1.0).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Loot",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DropChance = 1.0f;

    // Score mínimo para esta entrada ser avaliada.
    // 0.0 = sempre elegível | 1.0 = requer score perfeito.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Loot",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ScoreMinimum = 0.0f;

    // Tier do ItemInstance criado ao dropar.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Loot",
        meta = (ClampMin = "0"))
    int32 ItemTier = 0;

    // Raridade do ItemInstance criado ao dropar.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Loot")
    EZfItemRarity ItemRarity = EZfItemRarity::Common;

    // Raio em cm para posição aleatória do pickup ao redor do recurso.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Loot",
        meta = (ClampMin = "0.0"))
    float SpawnScatterRadius = 80.0f;
};

// ============================================================
// FZfGatherHitRecord
// ============================================================
// Registro de um golpe individual durante a coleta.
// Armazenado internamente pela ZfGA_GatherBase durante o loop.
// Não exposto ao designer — é dado de runtime da ability.
// ============================================================

USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfGatherHitRecord
{
    GENERATED_BODY()

    // Resultado do QTE deste golpe.
    UPROPERTY(BlueprintReadOnly, Category = "Gather|Record")
    EZfGatherHitResult HitResult = EZfGatherHitResult::Missed;

    // Dano causado neste golpe — já com todos os multiplicadores.
    // BaseDamage * DamageMultiplier(recurso) * DamageMultiplier(QTE)
    UPROPERTY(BlueprintReadOnly, Category = "Gather|Record")
    float DamageDealt = 0.0f;

    // Contribuição de score deste golpe.
    // Acumulado para calcular o score final ao fim do loop.
    UPROPERTY(BlueprintReadOnly, Category = "Gather|Record")
    float ScoreValue = 0.0f;

    // Multiplicador de dano por resultado do QTE.
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

    // Valor de score por resultado do QTE.
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
// Stats finais da ferramenta após aplicar todos os modifiers.
// Produzida pelo ZfFragment_GatherTool::ResolveGatherStats()
// e consumida pela ZfGA_GatherBase. A GA nunca acessa o
// fragment diretamente após o setup — usa sempre esta struct.
//
// GoodSize e PerfectSize são copiados do FZfGatherToolEntry
// correspondente e passados diretamente ao WBP_SkillCheck via
// StartSkillCheck(GoodSize, PerfectSize) para configurar as
// duas zonas visuais do QTE.
// ============================================================

USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfResolvedGatherStats
{
    GENERATED_BODY()

    // Tag da ferramenta — busca a entrada no AllowedTools[] do recurso.
    UPROPERTY(BlueprintReadOnly, Category = "Gather|Resolved")
    FGameplayTag ToolTag;

    // Dano base final após modifiers de BonusDamage.
    // DanoFinal = BaseDamage * DamageMultiplier(recurso) * DamageMultiplier(QTE)
    UPROPERTY(BlueprintReadOnly, Category = "Gather|Resolved")
    float BaseDamage = 10.0f;

    // Multiplicador sobre as quantidades da loot table.
    UPROPERTY(BlueprintReadOnly, Category = "Gather|Resolved")
    float DropMultiplier = 1.0f;

    // Bônus somado ao score bruto antes de avaliar a loot table.
    UPROPERTY(BlueprintReadOnly, Category = "Gather|Resolved")
    float ScoreBonus = 0.0f;

    // Tamanho normalizado (0.0–1.0) da zona externa (Good) do QTE.
    // Copiado de FZfGatherToolEntry::GoodSize pelo ResolveGatherStats().
    // Passado ao WBP_SkillCheck para desenhar o arco verde.
    // Deve ser maior que PerfectSize.
    UPROPERTY(BlueprintReadOnly, Category = "Gather|Resolved")
    float GoodSize = 0.20f;

    // Tamanho normalizado (0.0–1.0) da zona interna (Perfect) do QTE.
    // Copiado de FZfGatherToolEntry::PerfectSize pelo ResolveGatherStats().
    // Passado ao WBP_SkillCheck para desenhar o arco amarelo centralizado
    // dentro da GoodZone. Deve ser menor que GoodSize.
    UPROPERTY(BlueprintReadOnly, Category = "Gather|Resolved")
    float PerfectSize = 0.10f;

    // False se o fragment não estiver configurado — GA cancela a ability.
    UPROPERTY(BlueprintReadOnly, Category = "Gather|Resolved")
    bool bIsValid = false;
};