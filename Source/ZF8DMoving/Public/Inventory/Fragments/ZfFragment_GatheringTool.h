// Copyright ZfGame Studio. All Rights Reserved.
// ZfFragment_GatherTool.h
// Fragment que transforma um item em uma ferramenta de coleta.
//
// CONCEITO:
// Itens sem este fragment não podem iniciar coletas.
// A ZfGA_GatherBase busca este fragment no item equipado como
// primeira validação — se não encontrar, cancela imediatamente.
//
// RESPONSABILIDADE:
// Este fragment é a ponte entre o sistema de items e o sistema
// de coleta. Ele:
// 1. Define a identidade e stats base da ferramenta
// 2. Resolve os stats finais somando modifiers ativos do ItemInstance
// 3. Expõe o resultado via FZfResolvedGatherStats para a GA
//
// MODIFIERS SUPORTADOS (via Item.Property.Gather.*):
// - Item.Property.Gather.BonusDamage      → aumenta o dano por golpe
// - Item.Property.Gather.DropMultiplier   → aumenta quantidade dos drops
// - Item.Property.Gather.ScoreBonus       → facilita atingir drops raros
//
// COMO USAR NO EDITOR:
// 1. Abra o ItemDefinition da ferramenta (picareta, machado, etc.)
// 2. Adicione ZfFragment_GatherTool no array Fragments
// 3. Configure ToolTag, BaseDamage, BaseDropMultiplier e BaseScoreBonus

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Fragments/ZfItemFragment.h"
#include "GatheringSystem/ZfGatheringTypes.h"
#include "ZfFragment_GatheringTool.generated.h"

// Forward declarations
class UZfItemInstance;

// ============================================================
// UZfFragment_GatherTool
// ============================================================

UCLASS(DisplayName = "Fragment: Gather Tool")
class ZF8DMOVING_API UZfFragment_GatheringTool : public UZfItemFragment
{
    GENERATED_BODY()

public:

    // ----------------------------------------------------------
    // IDENTIDADE
    // ----------------------------------------------------------

    // Tag que identifica esta ferramenta no sistema de coleta.
    // Deve bater com as tags listadas no AllowedTools[] dos recursos.
    // Ex: "Tool.Pickaxe.Tier1", "Tool.Axe.Tier2"
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Tool",
        meta = (GameplayTagFilter = "Tool"))
    FGameplayTag ToolTag;

    // ----------------------------------------------------------
    // STATS BASE
    // Valores antes de aplicar modifiers.
    // ResolveGatherStats() soma os modifiers ativos por cima.
    // ----------------------------------------------------------

    // Dano base por golpe desta ferramenta.
    // O dano final de cada golpe é calculado como:
    // BaseDamage * DamageMultiplier(recurso) * DamageMultiplier(QTE)
    // Modifiers Item.Property.Gather.BonusDamage somam aqui.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Tool",
        meta = (ClampMin = "1.0"))
    float BaseDamage = 10.0f;

    // Multiplicador base sobre as quantidades da loot table.
    // 1.0 = sem bônus. Modifiers Gather.DropMultiplier somam aqui.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Tool",
        meta = (ClampMin = "1.0"))
    float BaseDropMultiplier = 1.0f;

    // Bônus base somado ao score final antes de avaliar a loot table.
    // 0.0 = sem bônus. Modifiers Gather.ScoreBonus somam aqui.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Tool",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BaseScoreBonus = 0.0f;

    // ----------------------------------------------------------
    // RESOLUÇÃO DE STATS
    // ----------------------------------------------------------

    // Resolve os stats finais desta ferramenta para uso pela GA.
    // Lê os valores base acima e soma os modifiers ativos do
    // ItemInstance que tenham TargetType == ItemProperty e
    // ItemPropertyTag dentro do namespace Item.Property.Gather.*
    //
    // @param ItemInstance — instância do item que possui este fragment
    UFUNCTION(BlueprintCallable, Category = "Zf|Fragment|GatherTool")
    FZfResolvedGatherStats ResolveGatherStats(const UZfItemInstance* ItemInstance) const;

    // ----------------------------------------------------------
    // VALIDAÇÃO NO EDITOR
    // ----------------------------------------------------------

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

    // ----------------------------------------------------------
    // DEBUG
    // ----------------------------------------------------------

    virtual FString GetDebugString() const override;
};