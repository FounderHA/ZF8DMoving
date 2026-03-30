// Copyright ZfGame Studio. All Rights Reserved.
// ZfModifierRollSystem.h
// Sistema de roll de modifiers para itens.
//
// CONCEITO:
// Classe utilitária com funções estáticas responsável por:
// - Determinar quantos modifiers um item recebe baseado na raridade
// - Filtrar modifiers compatíveis com o tipo do item via tags
// - Respeitar limites por classe de modifier
// - Garantir que não haja modifiers duplicados
// - Determinar o rank do modifier baseado no tier do item
// - Rolar o valor dentro do range do rank
//
// COMO USAR:
// UZfModifierRollSystem::RollModifiersForItem(ItemInstance);
//
// QUANDO CHAMAR:
// Após InitializeItemInstance no CreateAndAddItemToInventory,
// apenas no servidor.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Inventory/ZfInventoryTypes.h"
#include "Inventory/ZfModifierDataTypes.h"
#include "ZfModifierRollSystem.generated.h"

// Forward declarations
class UZfItemInstance;
class UZfFragment_Modifiers;
class UDataTable;

// ============================================================
// FZfModifierRollContext
// ============================================================
// Contexto passado para o roll — contém todas as informações
// necessárias para rolar os modifiers de um item.
// ============================================================
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfModifierRollContext
{
    GENERATED_BODY()

    // Item que receberá os modifiers
    UPROPERTY(BlueprintReadWrite, Category = "Roll|Context")
    TObjectPtr<UZfItemInstance> ItemInstance = nullptr;

    // DataTable carregado com os modifiers disponíveis
    UPROPERTY(BlueprintReadWrite, Category = "Roll|Context")
    TObjectPtr<UDataTable> ModifierDataTable = nullptr;

    // Tags do item para filtrar modifiers compatíveis
    UPROPERTY(BlueprintReadWrite, Category = "Roll|Context")
    FGameplayTagContainer ItemTags;

    // Tier do item para determinar o rank do modifier
    UPROPERTY(BlueprintReadWrite, Category = "Roll|Context")
    int32 ItemTier = 0;

    // Raridade do item para determinar quantidade de modifiers
    UPROPERTY(BlueprintReadWrite, Category = "Roll|Context")
    EZfItemRarity ItemRarity = EZfItemRarity::Common;

    // Limites por classe de modifier configurados no Fragment
    UPROPERTY(BlueprintReadWrite, Category = "Roll|Context")
    TArray<FZfModifierClassLimit> ClassLimits;
};

// ============================================================
// UZfModifierRollSystem
// ============================================================

UCLASS()
class ZF8DMOVING_API UZfModifierRollSystem : public UObject
{
    GENERATED_BODY()

public:

    // ----------------------------------------------------------
    // FUNÇÃO PRINCIPAL
    // ----------------------------------------------------------

    // Rola todos os modifiers para um item.
    // Ponto de entrada principal — chama todas as funções internas.
    // Deve ser chamado apenas no servidor após InitializeItemInstance.
    // @param ItemInstance — item que receberá os modifiers
    // @return true se ao menos um modifier foi aplicado
    UFUNCTION(BlueprintCallable, Category = "Zf|ModifierRoll")
    static bool RollModifiersForItem(UZfItemInstance* ItemInstance);

    // ----------------------------------------------------------
    // FUNÇÕES DE CONSULTA
    // ----------------------------------------------------------

    // Retorna a quantidade de modifiers a rolar para uma raridade.
    // Usa o ZfModifierRangeByRarity definido em ZfInventoryTypes.
    // @param Rarity — raridade do item
    // @return quantidade sorteada entre Min e Max da raridade
    UFUNCTION(BlueprintCallable, Category = "Zf|ModifierRoll")
    static int32 RollModifierCountForRarity(EZfItemRarity Rarity);

    // Retorna o rank a aplicar baseado no tier do item.
    // Usa as probabilidades configuradas no TierData do modifier.
    // @param ModifierRow — linha do DataTable do modifier
    // @param ItemTier — tier do item (0 a 5)
    // @return rank sorteado
    UFUNCTION(BlueprintCallable, Category = "Zf|ModifierRoll")
    static int32 RollRankForTier(const FZfModifierDataTypes& ModifierRow, int32 ItemTier);

    // Rola o valor dentro do range do rank.
    // @param RankData — dados do rank com Min e Max
    // @return valor sorteado entre Min e Max
    UFUNCTION(BlueprintCallable, Category = "Zf|ModifierRoll")
    static float RollValueForRank(const FZfModifierRankData& RankData);

    // Retorna todas as linhas do DataTable compatíveis com o item.
    // Filtra por:
    // - CompatibleItemTags (deve ter ao menos uma tag do item)
    // - Modifiers já aplicados (sem duplicatas)
    // - Limite por classe (respeita ModifierClassLimits)
    // @param Context — contexto do roll com todas as informações
    // @param ModifierClass — classe a filtrar (None = todas as classes)
    // @return array de nomes de linha compatíveis
    static TArray<FName> GetCompatibleModifierRows(const FZfModifierRollContext& Context,EZfModifierClass ModifierClass = EZfModifierClass::None);

private:

    // ----------------------------------------------------------
    // FUNÇÕES INTERNAS
    // ----------------------------------------------------------

    // Monta o contexto de roll a partir do ItemInstance.
    // Carrega o DataTable sincronamente e preenche o contexto.
    // @param ItemInstance — item para montar o contexto
    // @param OutContext — contexto preenchido
    // @return true se o contexto foi montado com sucesso
    static bool Internal_BuildRollContext(UZfItemInstance* ItemInstance, FZfModifierRollContext& OutContext);

    // Rola um modifier específico e adiciona ao ItemInstance.
    // @param Context — contexto do roll
    // @param RowName — nome da linha no DataTable
    // @return true se o modifier foi aplicado com sucesso
    static bool Internal_RollAndApplyModifier(FZfModifierRollContext& Context, const FName& RowName);

    // Verifica se um modifier já foi aplicado ao item.
    // @param Context — contexto do roll
    // @param RowName — nome da linha a verificar
    static bool Internal_IsModifierAlreadyApplied(const FZfModifierRollContext& Context, const FName& RowName);

    // Verifica se o limite de uma classe foi atingido.
    // @param Context — contexto do roll
    // @param ModifierClass — classe a verificar
    static bool Internal_IsClassLimitReached(const FZfModifierRollContext& Context, EZfModifierClass ModifierClass);

    // Conta quantos modifiers de uma classe já foram aplicados.
    // @param Context — contexto do roll
    // @param ModifierClass — classe a contar
    static int32 Internal_CountAppliedModifiersOfClass(const FZfModifierRollContext& Context, EZfModifierClass ModifierClass);

    // Seleciona uma linha aleatória de um array de nomes.
    // @param RowNames — array de nomes válidos
    // @return nome sorteado ou NAME_None se vazio
    static FName Internal_SelectRandomRow(const TArray<FName>& RowNames);

    // Normaliza os pesos do TierData para que somem 1.0.
    // Usado para garantir que as probabilidades estejam corretas.
    // @param TierData — dados do tier com os pesos
    // @return array de pesos normalizados
    static TArray<float> Internal_NormalizeTierWeights(const FZfTierData& TierData);
};