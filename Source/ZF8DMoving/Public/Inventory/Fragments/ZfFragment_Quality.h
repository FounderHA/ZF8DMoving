// Copyright ZfGame Studio. All Rights Reserved.
// ZfFragment_Quality.h
// Fragment que define o sistema de qualidade do item via DataTable.
//
// CONCEITO:
// Cada tipo de item tem seu próprio DataTable de qualidade
// (ex: DT_WeaponQuality, DT_ArmorQuality).
// Cada linha do DataTable representa um nível de qualidade (0 a 9)
// e define os valores EXATOS dos stats base para aquele nível.
//
// COMPORTAMENTO AO UPAR DE QUALIDADE:
// Os stats base do ItemInstance são SUBSTITUÍDOS pelos valores
// da linha correspondente ao novo nível no DataTable.
// Não é cumulativo — Quality 3 sempre tem os mesmos valores,
// independente de ter passado por Quality 1 e 2 antes.
//
// O QUE ESCALA COM QUALIDADE:
// Apenas os stats base (PhysicalDamage, Resistance, etc.)
// Os modifiers NÃO escalam com qualidade.
//
// INDEPENDÊNCIA DE TIER:
// Os valores no DataTable são fixos por nível de qualidade.
// Quality 3 sempre dá os mesmos stats, seja Tier 0 ou Tier 5.
// O Tier influencia apenas os modifiers e os stats base iniciais
// ao dropar o item — não interfere na progressão de qualidade.
//
// ONDE FICAM OS DADOS DINÂMICOS:
// CurrentQuality fica no UZfItemInstance e é replicado
// via FFastArraySerializer pelo UZfInventoryComponent.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Inventory/Fragments/ZfItemFragment.h"
#include "ZfFragment_Quality.generated.h"

// ============================================================
// FZfQualityLevelRow
// ============================================================
// Linha do DataTable de qualidade.
// Cada linha representa um nível de qualidade (0 a 9) e define
// os valores EXATOS dos stats base para aquele nível.
//
// COMO CONFIGURAR NO EDITOR:
// - Crie um DataTable com esse struct como tipo de linha
// - Nomeie cada linha como "Quality_0", "Quality_1", ... "Quality_9"
// - Preencha apenas os stats relevantes para o tipo de item
//   (ex: DT_WeaponQuality preenche dano, DT_ArmorQuality preenche resistência)
// - Deixe 0.0 nos stats que não se aplicam ao tipo de item
// ============================================================

USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfQualityLevelRow : public FTableRowBase
{
    GENERATED_BODY()

    // ----------------------------------------------------------
    // IDENTIFICAÇÃO DO NÍVEL
    // ----------------------------------------------------------

    // Nível de qualidade que esta linha representa (0 a 9).
    // Deve ser único por DataTable.
    // Usado para busca e validação em runtime.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality|Level")
    int32 QualityLevel = 0;

    // Descrição do nível para exibição na UI.
    // Ex: "Qualidade Bruta", "Qualidade Refinada", "Obra-Prima"
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality|Level")
    FText LevelDisplayName;

    // ----------------------------------------------------------
    // STATS DE ARMA
    // Preenchidos em DT_WeaponQuality — deixe 0 nos demais tipos
    // ----------------------------------------------------------

    // Dano físico base para este nível de qualidade
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality|Stats|Weapon")
    float PhysicalDamage = 0.0f;

    // Dano mágico base para este nível de qualidade
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality|Stats|Weapon")
    float MagicalDamage = 0.0f;

    // Velocidade de ataque base para este nível de qualidade
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality|Stats|Weapon")
    float AttackSpeed = 0.0f;

    // Chance de crítico base para este nível de qualidade
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality|Stats|Weapon")
    float CriticalHitChance = 0.0f;

    // ----------------------------------------------------------
    // STATS DE ARMADURA
    // Preenchidos em DT_ArmorQuality (Helmet, Chest, Legs, Feet, Hands)
    // ----------------------------------------------------------

    // Resistência física base para este nível de qualidade
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality|Stats|Armor")
    float PhysicalResistance = 0.0f;

    // Resistência mágica base para este nível de qualidade
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality|Stats|Armor")
    float MagicalResistance = 0.0f;

    // ----------------------------------------------------------
    // CUSTO DE UPGRADE
    // Custo para ir DO nível atual PARA o próximo nível.
    // A linha Quality_9 pode deixar este campo em 0
    // pois é o nível máximo — não há próximo upgrade.
    // ----------------------------------------------------------

    // Custo em moeda para realizar o upgrade para o próximo nível
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality|Upgrade")
    int32 UpgradeCostCurrency = 0;

    // Material necessário para o upgrade (tag identifica o material)
    // Ex: "Item.CraftMaterial.IronOre"
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality|Upgrade")
    FGameplayTag UpgradeMaterialTag;

    // Quantidade do material necessário para o upgrade
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality|Upgrade",
        meta = (ClampMin = "0"))
    int32 UpgradeMaterialAmount = 0;
};

// ============================================================
// UZfFragment_Quality
// ============================================================

// Qualidade máxima global — igual para todos os itens do jogo.
// Centralizado aqui para ajuste em um único lugar.
static constexpr int32 MAX_ITEM_QUALITY = 9;

// Prefixo padrão do nome das linhas no DataTable.
// Ex: "Quality_0", "Quality_3", "Quality_9"
static const FString QUALITY_ROW_PREFIX = TEXT("Quality_");

UCLASS(DisplayName = "Fragment: Quality")
class ZF8DMOVING_API UZfFragment_Quality : public UZfItemFragment
{
    GENERATED_BODY()

public:

    // DataTable de qualidade para este tipo de item.
    // Soft reference para carregamento assíncrono via AssetManager.
    // Ex: DT_WeaponQuality para armas, DT_ArmorQuality para armaduras.
    // Configurado no ItemDefinition (PDA) de cada item.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|Quality")
    TSoftObjectPtr<UDataTable> QualityDataTable;

    // ----------------------------------------------------------
    // FUNÇÕES DE ACESSO AO DATATABLE
    // ----------------------------------------------------------

    // Busca a linha do DataTable para um nível de qualidade específico.
    // Retorna nullptr se o nível não existir ou o DataTable não estiver carregado.
    // ATENÇÃO: O DataTable deve estar carregado antes de chamar esta função.
    // Use o AssetManager para garantir o carregamento assíncrono.
    // @param QualityLevel — nível desejado (0 a MAX_ITEM_QUALITY)
    const FZfQualityLevelRow* GetQualityRowForLevel(int32 QualityLevel) const
    {
        // Valida se o DataTable está carregado
        UDataTable* LoadedTable = QualityDataTable.Get();
        if (!LoadedTable)
        {
            UE_LOG(LogZfInventory, Error,
                TEXT("UZfFragment_Quality::GetQualityRowForLevel — "
                     "DataTable não está carregado. Use o AssetManager para carregá-lo."));
            return nullptr;
        }

        // Valida o range do nível
        if (!FMath::IsWithinInclusive(QualityLevel, 0, MAX_ITEM_QUALITY))
        {
            UE_LOG(LogZfInventory, Warning,
                TEXT("UZfFragment_Quality::GetQualityRowForLevel — "
                     "QualityLevel %d fora do range válido (0 a %d)."),
                QualityLevel, MAX_ITEM_QUALITY);
            return nullptr;
        }

        // Monta o nome da linha: "Quality_0", "Quality_1", etc.
        const FName RowName = FName(*(QUALITY_ROW_PREFIX + FString::FromInt(QualityLevel)));
        const FZfQualityLevelRow* Row = LoadedTable->FindRow<FZfQualityLevelRow>(
            RowName,
            TEXT("UZfFragment_Quality::GetQualityRowForLevel"));

        if (!Row)
        {
            UE_LOG(LogZfInventory, Warning,
                TEXT("UZfFragment_Quality::GetQualityRowForLevel — "
                     "Linha '%s' não encontrada no DataTable '%s'."),
                *RowName.ToString(),
                *LoadedTable->GetName());
        }

        return Row;
    }

    // Retorna a qualidade máxima global.
    // Útil para UI e validações sem referenciar a constante diretamente.
    UFUNCTION(BlueprintCallable, Category = "Zf|Fragment|Quality")
    static int32 GetMaxItemQuality()
    {
        return MAX_ITEM_QUALITY;
    }

    // Verifica se um nível de qualidade é válido.
    // Usado para validações antes de aplicar upgrades.
    UFUNCTION(BlueprintCallable, Category = "Zf|Fragment|Quality")
    static bool IsValidQualityLevel(int32 QualityLevel)
    {
        return FMath::IsWithinInclusive(QualityLevel, 0, MAX_ITEM_QUALITY);
    }

    // Verifica se o item pode ainda ser melhorado.
    // Retorna false se já estiver no nível máximo.
    UFUNCTION(BlueprintCallable, Category = "Zf|Fragment|Quality")
    static bool CanUpgradeQuality(int32 CurrentQuality)
    {
        return CurrentQuality < MAX_ITEM_QUALITY;
    }

// ----------------------------------------------------------
// DEBUG
// ----------------------------------------------------------	
    
    virtual FString GetDebugString() const override
    {
        const FString TableName = QualityDataTable.IsValid()
            ? QualityDataTable.GetAssetName()
            : TEXT("Not Loaded");

        return FString::Printf(
            TEXT("[Fragment_Quality] MaxQuality: %d (Global) | DataTable: %s"),
            MAX_ITEM_QUALITY,
            *TableName);
    }
};