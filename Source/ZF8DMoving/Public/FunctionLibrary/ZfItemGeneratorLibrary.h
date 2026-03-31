// Copyright ZfGame Studio. All Rights Reserved.
// ZfItemGeneratorLibrary.h
// Biblioteca estática para geração procedural de ItemInstances.
// Acessível de qualquer lugar — C++ e Blueprint.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Inventory/ZfInventoryTypes.h"
#include "Inventory/ZfModifierDataTypes.h"
#include "ZfItemGeneratorLibrary.generated.h"

class UZfItemInstance;

// -----------------------------------------------------------
// UZfItemGeneratorLibrary
// -----------------------------------------------------------
UCLASS()
class ZF8DMOVING_API UZfItemGeneratorLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    
	// Formata o texto do modifier com todos os dados disponíveis como placeholders.
	// O designer escolhe quais usar no TooltipFormat do DataTable.
	// Placeholders disponíveis:
	// {value}      — valor atual do modifier
	// {min}        — valor mínimo do rank atual
	// {max}        — valor máximo do rank atual
	// {rank}       — rank atual
	// {maxrank}    — rank máximo disponível
	// {percentage} — percentual dentro do range (0-100%)
	// @param AppliedModifier — modifier já aplicado no item
	// @param ModifierData    — dados do DataTable do modifier
	// @return texto formatado pronto para exibir na UI
	UFUNCTION(BlueprintCallable, Category = "Zf|ItemGenerator")
	static FText FormatModifierTooltip(const FZfAppliedModifier& AppliedModifier, UDataTable* ModifierDataTable);
	
	
    // Sorteia uma raridade baseada em pesos de probabilidade.
    // Deixe RarityWeights vazio para usar os pesos padrão do sistema.
    // @param RarityWeights — pesos de cada raridade (vazio = defaults)
    // @return raridade sorteada
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemGenerator")
    static EZfItemRarity RollRarity(const TArray<FZfRarityWeight>& RarityWeights);

    // Sorteia um tier baseado em pesos de probabilidade.
    // Deixe TierWeights vazio para usar os pesos padrão do sistema.
    // @param TierWeights — pesos de cada tier (vazio = defaults)
    // @return tier sorteado
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemGenerator")
    static int32 RollTier(const TArray<FZfTierWeight>& TierWeights);

	// Sorteia a quantidade de modifiers usando cascata de chance.
	// Começa no mínimo e tenta aumentar 1 por vez com chance decrescente.
	// @param Rarity — raridade do item já sorteada
	// @return quantidade de modifiers a serem gerados
	UFUNCTION(BlueprintCallable, Category = "Zf|ItemGenerator")
	static int32 RollModifierCount(EZfItemRarity Rarity);

    // Sorteia o rank de um modifier baseado nas probabilidades configuradas no tier.
    // Os pesos são definidos no FZfTierData do DataTable de modifiers.
    // @param TierData — dados do tier contendo os ranks possíveis e seus pesos
    // @return rank sorteado (1-based)
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemGenerator")
    static int32 RollRankForTier(const FZfTierData& TierData);

    // Rola o rank e o valor de um modifier já selecionado do DataTable.
    // @param ModifierData — dados completos da linha do DataTable já sorteada
    // @param ItemTier     — tier do item para determinar quais ranks podem aparecer
    // @return modifier com rank e valor já sorteados, pronto para aplicar no item
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemGenerator")
    static FZfAppliedModifier RollSingleModifier(const FZfModifierDataTypes& ModifierData, int32 ItemTier);

    // Sorteia quais modifiers um item terá a partir do DataTable.
    // Filtra por tags compatíveis, respeita limites por classe e evita duplicatas.
    // @param ModifierDataTable — DataTable com todos os modifiers disponíveis
    // @param ItemTags          — tags do item para filtrar modifiers compatíveis
    // @param ItemTier          — tier do item para determinar ranks disponíveis
    // @param ModifierCount     — quantidade de modifiers a sortear
    // @return array de modifiers prontos para aplicar no item
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemGenerator")
    static TArray<FZfAppliedModifier> RollModifiers(UDataTable* ModifierDataTable,
    	const FGameplayTagContainer& ItemTags, int32 ItemTier, int32 ModifierCount,
    	const FZfItemModifierConfig& ModifierConfig);
    // Aplica a raridade, tier e modifiers gerados em um ItemInstance existente.
    // Deve ser chamado após todos os rolls estarem concluídos.
    // @param ItemInstance    — instância do item a ser preenchida
    // @param Rarity          — raridade sorteada
    // @param Tier            — tier sorteado
    // @param Modifiers       — modifiers sorteados
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemGenerator")
    static void ApplyGenerationToInstance(UZfItemInstance* ItemInstance, EZfItemRarity Rarity, int32 Tier, const TArray<FZfAppliedModifier>& Modifiers);
    
    // Gera um ItemInstance completo com raridade, tier e modifiers sorteados.
    // @param Outer            — dono do ItemInstance (geralmente o próprio ator)
    // @param InItemDefinition — definição do item a ser gerado
    // @param RarityWeights    — pesos de raridade (vazio = defaults)
    // @param TierWeights      — pesos de tier (vazio = defaults)
    // @return ItemInstance gerado e pronto para uso
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemGenerator")
    static UZfItemInstance* GenerateItem(
        UObject* Outer,
        UZfItemDefinition* InItemDefinition,
        const TArray<FZfRarityWeight>& RarityWeights,
        const TArray<FZfTierWeight>& TierWeights);
	
	/** Retorna o range de modifiers (Min/Max) para uma raridade específica. */
	UFUNCTION(BlueprintPure, Category = "Zf|Inventory|Modifiers", meta = (DisplayName = "Get Modifier Range By Rarity"))
	static bool GetModifierRangeByRarity(EZfItemRarity Rarity, FZfModifierRange& OutRange);
};