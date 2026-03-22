// Copyright ZfGame Studio. All Rights Reserved.
// ZfModifierDataTypes.h
// Define as estruturas de linha de DataTable para o sistema de Modifiers.
// Cada linha representa um modifier único que pode ser rolado em um item.
// Alimenta o sistema Data-Driven integrado com o GAS em runtime.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"               // FTableRowBase — base para linhas de DataTable
#include "GameplayTagContainer.h"           // FGameplayTagContainer — compatibilidade por tag
#include "GameplayEffect.h"                 // UGameplayEffect — efeito base do GAS
#include "ZfInventoryTypes.h"               // EZfModifierClass, EZfModifierOperationType, etc.
#include "ZfInventoryTags.h"                // Tags nativas do inventário
#include "ZfModifierDataTypes.generated.h"

// ============================================================
// FZfModifierDataTableRow
// ============================================================
// Representa uma linha completa na DataTable de modifiers.
// Cada linha é um modifier único (ex: "MoveSpeed", "MaxHealth").
//
// COMO USAR NO EDITOR:
// 1. Crie uma DataTable com esse struct como tipo de linha
// 2. Cada item terá seu próprio DataTable (DT_WeaponModifiers,
//    DT_BootsModifiers, etc.) ou um DataTable global filtrado por tag
// 3. O sistema lê essa tabela em runtime para rolar e aplicar modifiers
// ============================================================

USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfModifierDataTableRow : public FTableRowBase
{
    GENERATED_BODY()

    // ----------------------------------------------------------
    // IDENTIFICAÇÃO
    // ----------------------------------------------------------

    // Nome legível exibido na UI do inventário
    // Ex: "Aumento de Velocidade de Movimento"
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Identity")
    FText DisplayName;

    // Descrição exibida no tooltip do modifier na UI
    // Ex: "Aumenta a velocidade de movimento do personagem"
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Identity")
    FText Description;

    // ----------------------------------------------------------
    // COMPATIBILIDADE — usa o sistema de Tags (substitui EZfItemType)
    // ----------------------------------------------------------

    // Tags dos tipos de item onde este modifier pode aparecer.
    // Ex: MoveSpeed → adiciona apenas "Inventory.Item.Feet"
    // Ex: MaxHealth → adiciona "Inventory.Item.Chest", "Inventory.Item.Helmet", etc.
    // Um modifier só será rolado se o item tiver ao menos uma dessas tags.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Compatibility")
    FGameplayTagContainer CompatibleItemTags;

    // ----------------------------------------------------------
    // CLASSIFICAÇÃO
    // ----------------------------------------------------------

    // Classe do modifier — usada para limitar por tipo no item
    // Ex: Offensive, Defensive, Utility, Attribute, Resource
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Classification")
    EZfModifierClass ModifierClass = EZfModifierClass::None;

    // Tag do atributo GAS que este modifier afeta.
    // Ex: "Attribute.Movement.Speed", "Attribute.Combat.PhysicalDamage"
    // O GameplayEffect base usará esta tag para saber qual atributo modificar
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Classification")
    FGameplayTag AffectedAttributeTag;

    // Como o valor é aplicado no atributo via GAS
    // Additive = flat, MultiplyBase = percentual, Override = substitui
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Classification")
    EZfModifierOperationType OperationType = EZfModifierOperationType::Additive;

    // ----------------------------------------------------------
    // CORRUPÇÃO
    // ----------------------------------------------------------

    // Se verdadeiro, este modifier é um debuff e só pode aparecer
    // em itens corrompidos via mecânica de Corruption
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Corruption")
    bool bIsDebuffModifier = false;

    // Se verdadeiro, este modifier só pode ser extraído e inserido
    // em itens que já estejam corrompidos (mecânica de Extraction)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Corruption")
    bool bRequiresCorruption = false;

    // ----------------------------------------------------------
    // RANKS — valores por nível de rank (1 a 6 por padrão)
    // ----------------------------------------------------------

    // Array de ranks deste modifier em ordem crescente de poder.
    // Índice 0 = Rank 1, Índice 1 = Rank 2, etc.
    // Facilmente expansível adicionando mais entradas no editor.
    //
    // Exemplo para MoveSpeed:
    // Rank 1: Min=3,  Max=7
    // Rank 2: Min=7,  Max=12
    // Rank 3: Min=11, Max=17
    // Rank 4: Min=15, Max=22
    // Rank 5: Min=19, Max=28
    // Rank 6: Min=25, Max=35
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Tiers")
    TArray<FZfTierData> Tiers;

    // ----------------------------------------------------------
    // MARKET VALUE
    // ----------------------------------------------------------

    // Peso deste modifier no cálculo do valor de mercado do item.
    // Modifiers mais poderosos devem ter peso maior.
    // Ex: MoveSpeed = 1.0, MaxHealth = 1.5, PhysicalDamage = 2.0
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Ranks")
    TArray<FZfModifierRankData> Ranks;

    // ----------------------------------------------------------
    // TIERS — probabilidade de cada rank por tier do item
    // ----------------------------------------------------------

    // Array de tiers definindo quais ranks podem aparecer
    // e com qual probabilidade em cada tier do item.
    // Índice 0 = Tier 0, Índice 1 = Tier 1, etc.
    //
    // Exemplo para MoveSpeed:
    // Tier 0: Rank 1 = 100%
    // Tier 1: Rank 1 = 60%, Rank 2 = 40%
    // Tier 2: Rank 2 = 50%, Rank 3 = 50%  (ou qualquer config)
    // Tier 3: Rank 2 = 30%, Rank 3 = 50%, Rank 4 = 20%
    // Tier 4: Rank 3 = 40%, Rank 4 = 40%, Rank 5 = 20%
    // Tier 5: Rank 4 = 30%, Rank 5 = 40%, Rank 6 = 30%
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Market")
    float MarketValueWeight = 1.0f;

    // ----------------------------------------------------------
    // VALIDAÇÃO — helpers chamados em runtime e no editor
    // ----------------------------------------------------------

    // Retorna true se este modifier é compatível com a tag do item fornecido.
    // Chamado antes de rolar modifiers para filtrar incompatíveis.
    bool IsCompatibleWithItemTag(const FGameplayTag& ItemTag) const
    {
        return CompatibleItemTags.HasTag(ItemTag);
    }

    // Retorna true se este modifier é compatível com qualquer
    // tag de um container (útil para itens com múltiplas tags).
    bool IsCompatibleWithAnyItemTag(const FGameplayTagContainer& ItemTags) const
    {
        return CompatibleItemTags.HasAny(ItemTags);
    }

    // Retorna os dados do rank pelo nível (1-based).
    // Retorna nullptr se o rank não existir.
    const FZfModifierRankData* GetRankData(int32 RankLevel) const
    {
        // Ranks são 1-based, array é 0-based
        const int32 RankIndex = RankLevel - 1;
        if (Ranks.IsValidIndex(RankIndex))
        {
            return &Ranks[RankIndex];
        }
        UE_LOG(LogZfInventory, Warning,
            TEXT("FZfModifierDataTableRow::GetRankData — Rank %d não existe neste modifier."),
            RankLevel);
        return nullptr;
    }

    // Retorna os dados do tier pelo nível (0-based).
    // Retorna nullptr se o tier não existir.
    const FZfTierData* GetTierData(int32 TierLevel) const
    {
        for (const FZfTierData& TierData : Tiers)
        {
            if (TierData.TierLevel == TierLevel)
            {
                return &TierData;
            }
        }
        UE_LOG(LogZfInventory, Warning,
            TEXT("FZfModifierDataTableRow::GetTierData — Tier %d não existe neste modifier."),
            TierLevel);
        return nullptr;
    }

    // Retorna o número máximo de ranks disponíveis neste modifier
    int32 GetMaxRankCount() const
    {
        return Ranks.Num();
    }

    // Retorna o número máximo de tiers configurados neste modifier
    int32 GetMaxTierCount() const
    {
        return Tiers.Num();
    }
};

// ============================================================
// FZfModifierRollResult
// ============================================================
// Resultado de um roll de modifier — preenchido pelo
// UZfModifierRollSubsystem (ou função utilitária equivalente)
// e usado para construir o FZfAppliedModifier final.
// ============================================================

USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfModifierRollResult
{
    GENERATED_BODY()

    // Nome da linha na DataTable (identificador único do modifier)
    UPROPERTY(BlueprintReadOnly, Category = "Modifier|Roll")
    FName ModifierRowName = NAME_None;

    // Rank que foi sorteado para este roll
    UPROPERTY(BlueprintReadOnly, Category = "Modifier|Roll")
    int32 RolledRank = 1;

    // Valor final calculado dentro do range do rank
    UPROPERTY(BlueprintReadOnly, Category = "Modifier|Roll")
    float RolledValue = 0.0f;

    // Percentual dentro do range (0.0 = mínimo, 1.0 = máximo)
    // Salvo para uso futuro do Modifier UP
    UPROPERTY(BlueprintReadOnly, Category = "Modifier|Roll")
    float RollPercentage = 0.0f;

    // Se o roll foi bem sucedido (false = falha na validação)
    UPROPERTY(BlueprintReadOnly, Category = "Modifier|Roll")
    bool bSuccess = false;

    // Motivo da falha se bSuccess = false (para debug)
    UPROPERTY(BlueprintReadOnly, Category = "Modifier|Roll")
    FString FailureReason = TEXT("");
};

// ============================================================
// FZfItemModifierConfig
// ============================================================
// Configuração de modifiers de um item específico.
// Definida dentro do ItemDefinition (PDA) para cada item,
// controlando quantos e quais tipos de modifier ele pode ter.
// ============================================================

USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfItemModifierConfig
{
    GENERATED_BODY()

    // Quantidade total máxima de modifiers que este item pode ter.
    // Ex: um capacete épico pode ter até 4 modifiers no total.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Config",
        meta = (ClampMin = "0", ClampMax = "20"))
    int32 MaxTotalModifiers = 3;

    // Limites por classe de modifier para este item específico.
    // Ex: [{Offensive, 2}, {Utility, 1}] = máx 2 ofensivos e 1 utilitário.
    // Se uma classe não está na lista, considera-se limite = MaxTotalModifiers.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Config")
    TArray<FZfModifierClassLimit> ModifierClassLimits;

    // DataTable de modifiers compatíveis com este item.
    // Soft reference para carregamento assíncrono via AssetManager.
    // Cada tipo de item pode ter sua própria DataTable de modifiers.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Config")
    TSoftObjectPtr<UDataTable> ModifierDataTable;

    // Retorna o limite máximo de uma classe específica de modifier.
    // Se a classe não estiver configurada, retorna MaxTotalModifiers.
    int32 GetClassLimit(EZfModifierClass ModifierClass) const
    {
        for (const FZfModifierClassLimit& Limit : ModifierClassLimits)
        {
            if (Limit.ModifierClass == ModifierClass)
            {
                return Limit.MaxCount;
            }
        }
        // Classe não configurada = usa o limite total como fallback
        return MaxTotalModifiers;
    }
};