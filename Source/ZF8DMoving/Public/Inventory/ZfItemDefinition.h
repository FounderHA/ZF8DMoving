// Copyright ZfGame Studio. All Rights Reserved.
// ZfItemDefinition.h
// Primary Data Asset que define todas as informações ESTÁTICAS de um item.
//
// CONCEITO:
// O ItemDefinition é um asset criado no editor (PDA) que representa
// o "tipo" do item — não uma instância dele. É como a "classe" do item.
// Ex: "Espada de Ferro" é um ItemDefinition.
//     "A espada de ferro do jogador João com 73 de durabilidade" é um ItemInstance.
//
// GERENCIAMENTO VIA ASSETMANAGER:
// Por ser um UPrimaryDataAsset, o ItemDefinition é gerenciado pelo
// AssetManager do Unreal. Isso permite:
// - Carregamento assíncrono (não trava o jogo)
// - Referência por FPrimaryAssetId (leve, sem carregar o asset inteiro)
// - Gerenciamento automático de memória
//
// REPLICAÇÃO:
// O ItemDefinition NÃO é replicado diretamente.
// O ItemInstance replica apenas o FPrimaryAssetId do seu ItemDefinition.
// Cada cliente carrega o asset localmente via AssetManager.
//
// COMO CRIAR UM NOVO ITEM NO EDITOR:
// 1. Content Browser → Add → Miscellaneous → Data Asset
// 2. Selecione UZfItemDefinition como classe
// 3. Configure todas as propriedades
// 4. Adicione os Fragments necessários no array Fragments
// 5. Configure o ModifierConfig se o item tiver modifiers

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ZfInventoryTypes.h"
#include "ZfInventoryTags.h"
#include "ZfModifierDataTypes.h"
#include "Inventory/Fragments/ZfItemFragment.h"
#include "Inventory/Fragments/ZfFragment_Quality.h"
#include "ZfItemDefinition.generated.h"

// ============================================================
// UZfItemDefinition
// ============================================================

UCLASS(BlueprintType, Blueprintable, Const)
class ZF8DMOVING_API UZfItemDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:

    UZfItemDefinition();

    // ----------------------------------------------------------
    // IDENTIFICAÇÃO
    // ----------------------------------------------------------

    // Nome do item exibido na UI do inventário e tooltips.
    // Ex: "Espada Longa de Ferro", "Poção de Vida Maior"
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Identity")
    FText ItemName = FText::FromString(TEXT("None"));

    // Descrição do item exibida no tooltip do inventário.
    // Ex: "Uma espada forjada nas minas de ferro do norte."
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Identity", meta = (MultiLine = true))
    FText ItemDescription;

    // Ícone 2D exibido no slot do inventário e no tooltip.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Identity")
    TSoftObjectPtr<UTexture2D> ItemIcon;

    // ----------------------------------------------------------
    // CLASSIFICAÇÃO POR TAGS
    // Tags que identificam o tipo deste item.
    // Substituem o EZfItemType — usadas para:
    // - Filtrar modifiers compatíveis
    // - Validar slots de equipamento
    // - Lógica de UI (o que mostrar/esconder)
    // - Filtros e ordenação no inventário
    // Ex: Uma espada teria: "Inventory.Item.Weapon" +
    //                       "Inventory.Item.Weapon.Sword"
    // ----------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Identity", meta =(GameplayTagFilter = "ItemType"))
    FGameplayTagContainer ItemTags;

    // ----------------------------------------------------------
    // Actor Spawn - seta o ActorSpawn do item
    // ----------------------------------------------------------
    
    // Actor de pickup específico deste item.
    // Cada item pode ter seu próprio actor de pickup com visual,
    // colisão e comportamento únicos.
    // Spawned pelo sistema de drop quando o item é jogado no mundo.
    // Soft reference — carregado assincronamente via AssetManager
    // apenas quando necessário (ao dropar o item).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Identity")
    TSoftClassPtr<class AZfItemPickup> ItemPickupActorClass;

    // ----------------------------------------------------------
    // FRAGMENTS
    // Array de fragments que definem as capacidades do item.
    // Cada fragment adiciona um comportamento específico.
    // Ex: [Equippable, Durability, Quality, SetPiece]
    //
    // EditInlineNew nos fragments permite criá-los diretamente
    // aqui no editor sem precisar de assets separados.
    // ----------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Fragments",
        Instanced)
    TArray<TObjectPtr<UZfItemFragment>> Fragments;

    // ----------------------------------------------------------
    // MODIFIERS
    // Configuração de modifiers para este item específico.
    // Define quantos modifiers, quais classes e qual DataTable usar.
    // Deixe vazio para itens sem modifiers (consumíveis, quest items, etc.)
    // ----------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Modifiers")
    FZfItemModifierConfig ModifierConfig;
    

    // ----------------------------------------------------------
    // MARKET VALUE
    // Valor base usado pelo sistema de Market Value para
    // calcular o preço de venda para NPCs.
    // O valor final é calculado em runtime pelo ItemInstance
    // baseado neste valor + raridade + tier + modifiers.
    // ----------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Economy", meta = (ClampMin = "0.0"))
    float BaseMarketValue = 10.0f;

    // ----------------------------------------------------------
    // FUNÇÕES DE ACESSO AOS FRAGMENTS
    // Helpers para buscar fragments por tipo sem precisar
    // iterar o array manualmente em outros sistemas.
    // ----------------------------------------------------------

    // Busca o primeiro fragment do tipo T neste ItemDefinition.
    // Retorna nullptr se não encontrar.
    // Uso: const UZfFragment_Equippable* F = Def->FindFragment<UZfFragment_Equippable>();
    template<typename T>
    const T* FindFragment() const
    {
        static_assert(TIsDerivedFrom<T, UZfItemFragment>::IsDerived,
            "T deve ser uma subclasse de UZfItemFragment.");

        for (const TObjectPtr<UZfItemFragment>& Fragment : Fragments)
        {
            if (const T* TypedFragment = Cast<T>(Fragment))
            {
                return TypedFragment;
            }
        }
        return nullptr;
    }

    // Verifica se este item possui um fragment do tipo T.
    // Uso: if (Def->HasFragment<UZfFragment_Equippable>()) { ... }
    template<typename T>
    bool HasFragment() const
    {
        return FindFragment<T>() != nullptr;
    }

    // Verifica se este item possui uma tag específica.
    // Uso: if (Def->HasItemTag(ZfInventoryTags::WeaponTypes::Weapon)) { ... }
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemDefinition")
    bool HasItemTag(const FGameplayTag& Tag) const
    {
        return ItemTags.HasTag(Tag);
    }

    // Verifica se este item possui alguma das tags fornecidas.
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemDefinition")
    bool HasAnyItemTag(const FGameplayTagContainer& Tags) const
    {
        return ItemTags.HasAny(Tags);
    }

    // Verifica se este item possui todas as tags fornecidas.
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemDefinition")
    bool HasAllItemTags(const FGameplayTagContainer& Tags) const
    {
        return ItemTags.HasAll(Tags);
    }

    // ----------------------------------------------------------
    // ASSETMANAGER — UPrimaryDataAsset overrides
    // ----------------------------------------------------------

    // Tipo primário deste asset — registrado no AssetManager via DefaultGame.ini
    // Usado para carregar assets por categoria:
    // AssetManager->LoadPrimaryAsset(FPrimaryAssetId("ZfItemDefinition", "NomeDoAsset"))
    static const FPrimaryAssetType PrimaryAssetType;

#if WITH_EDITOR
    // Validação no editor — garante consistência dos dados configurados.
    // Chamado ao salvar o asset no editor.
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;

#endif

    // ----------------------------------------------------------
    // DEBUG
    // ----------------------------------------------------------

    // Retorna string com todas as informações do ItemDefinition para debug.
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemDefinition|Debug")
    FString GetDebugString() const;
};