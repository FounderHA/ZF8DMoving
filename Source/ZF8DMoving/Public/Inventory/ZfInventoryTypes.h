// Copyright ZfGame Studio. All Rights Reserved.
// ZfInventoryTypes.h
// Arquivo central de tipos do sistema de inventário.
// Contém todos os Enums, Structs e tipos de dados compartilhados
// entre os componentes do inventário, equipamento, itens e modifiers.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayTagContainer.h"   // Para uso do sistema de Tags do Unreal (TAGs para identificar tipos, slots, classes de modifier, etc.)
#include "Engine/DataTable.h"       // Para herança de FTableRowBase nas structs de DataTable
#include "Net/Serialization/FastArraySerializer.h"
#include "ZfInventoryTypes.generated.h"

// ============================================================
// ENUMS
// ============================================================




// -----------------------------------------------------------
// EZfItemType
// Define a categoria principal do item.
// Usado para filtrar modifiers compatíveis, lógica de UI,
// regras de stack e comportamento de equipamento.
// -----------------------------------------------------------
UENUM(BlueprintType)
enum class EZfItemType: uint8

{   
    None            UMETA(DisplayName = "None"),

    // Equipamentos
    Weapon          UMETA(DisplayName = "Weapon"),          // Armas corpo a corpo ou à distância
    Shield          UMETA(DisplayName = "Shield"),          // Escudos (OffHand defensivo)
    Helmet          UMETA(DisplayName = "Helmet"),          // Capacete
    Chest           UMETA(DisplayName = "Chest Armor"),     // Armadura de peito
    Legs            UMETA(DisplayName = "Leg Armor"),       // Armadura de pernas
    Feet            UMETA(DisplayName = "Boots"),           // Botas
    Hands           UMETA(DisplayName = "Gloves"),          // Luvas
    Cape            UMETA(DisplayName = "Cape"),            // Capa
    Ring            UMETA(DisplayName = "Ring"),            // Anel
    Necklace        UMETA(DisplayName = "Necklace"),        // Colar/Amuleto
    Backpack        UMETA(DisplayName = "Backpack"),        // Mochila (expande inventário)

    // Ferramentas
    Pickaxe         UMETA(DisplayName = "Pickaxe"),         // Picareta
    Axe             UMETA(DisplayName = "Axe"),             // Machado
    Shovel          UMETA(DisplayName = "Shovel"),          // Pá
    FishingRod      UMETA(DisplayName = "Fishing Rod"),     // Vara de pesca

    // Consumíveis
    Consumable      UMETA(DisplayName = "Consumable"),      // Poções, comidas, pergaminhos de uso único

    // Munição
    Throwable       UMETA(DisplayName = "Throwable"),  // bombas, granadas

    // Materiais de Craft
    CraftMaterial   UMETA(DisplayName = "Craft Material"),  // Minérios, madeira, couro, reagentes mágicos

    // Receitas
    Recipe          UMETA(DisplayName = "Recipe"),          // Receitas/esquemas/plantas de craft

    // Quest Items
    QuestItem       UMETA(DisplayName = "Quest Item"),      // Chaves, documentos, artefatos de missão

    // Progressão
    ProgressionItem UMETA(DisplayName = "Progression Item"), // Livros, runas, gemas, fragmentos de poder

    // Econômicos
    Currency        UMETA(DisplayName = "Currency"),        // Moedas, gemas/joias, itens de troca

    // Colecionáveis
    Collectible     UMETA(DisplayName = "Collectible"),     // Cartas, pinturas, troféus, diários/lore books
};

// -----------------------------------------------------------
// EZfEquipmentSlot
// Define os slots disponíveis no ZfEquipmentComponent.
// Cada slot pode ter múltiplas instâncias (ex: Ring_1, Ring_2)
// configurado pelo sistema de slots flexíveis.
// -----------------------------------------------------------
UENUM(BlueprintType)
enum class EZfEquipmentSlot : uint8
{
    None        UMETA(DisplayName = "None"),
    MainHand    UMETA(DisplayName = "Main Hand"),
    OffHand     UMETA(DisplayName = "Off Hand"),
    Head        UMETA(DisplayName = "Head"),
    Chest       UMETA(DisplayName = "Chest"),
    Legs        UMETA(DisplayName = "Legs"),
    Feet        UMETA(DisplayName = "Feet"),
    Hands       UMETA(DisplayName = "Hands"),
    Cape        UMETA(DisplayName = "Cape"),
    Backpack    UMETA(DisplayName = "Backpack"),
    Ring        UMETA(DisplayName = "Ring"),
    Necklace    UMETA(DisplayName = "Necklace"),
    Pickaxe     UMETA(DisplayName = "Pickaxe"),
    Axe         UMETA(DisplayName = "Axe"),
    Shovel      UMETA(DisplayName = "Shovel"),
    FishingRod  UMETA(DisplayName = "Fishing Rod"),
};

// -----------------------------------------------------------
// EZfItemRarity
// Define a raridade do item, que influencia:
// - Cor na UI
// - Quantidade máxima de modifiers possíveis
// - Valor de mercado
// -----------------------------------------------------------
UENUM(BlueprintType)
enum class EZfItemRarity : uint8
{
    Common      UMETA(DisplayName = "Common"),      // 0 - Cinza
    Uncommon    UMETA(DisplayName = "Uncommon"),    // 1 - Verde
    Rare        UMETA(DisplayName = "Rare"),        // 2 - Azul
    Epic        UMETA(DisplayName = "Epic"),        // 3 - Roxo
    Legendary   UMETA(DisplayName = "Legendary"),   // 4 - Laranja
};

// -----------------------------------------------------------
// EZfModifierClass
// Classe/categoria do modifier.
// Usada para limitar quantos modifiers de cada classe
// podem aparecer em um mesmo item.
// -----------------------------------------------------------
UENUM(BlueprintType)
enum class EZfModifierClass : uint8
{
    None        UMETA(DisplayName = "None"),
    Offensive   UMETA(DisplayName = "Offensive"),     // Dano, velocidade de ataque, crítico
    Defensive   UMETA(DisplayName = "Defensive"),     // Resistências, armadura
    Utility     UMETA(DisplayName = "Utility"),       // Velocidade de movimento, cooldown
    Attribute   UMETA(DisplayName = "Attribute"),     // Força, destreza, inteligência
    Resource    UMETA(DisplayName = "Resource"),      // Vida, mana, stamina máxima
};

// -----------------------------------------------------------
// EZfModifierOperationType
// Define como o valor do modifier é aplicado no atributo.
// -----------------------------------------------------------
UENUM(BlueprintType)
enum class EZfModifierOperationType : uint8
{
    Additive        UMETA(DisplayName = "Additive"),        // Adiciona valor flat: +10 de vida
    MultiplyBase    UMETA(DisplayName = "Multiply Base"),   // Multiplica a base: +10% de dano
    Override        UMETA(DisplayName = "Override"),        // Substitui o valor completamente
};

// -----------------------------------------------------------
// EZfCorruptionState
// Estado de corrupção do item.
// -----------------------------------------------------------
UENUM(BlueprintType)
enum class EZfCorruptionState : uint8
{
    None        UMETA(DisplayName = "Not Corrupted"),   // Item normal
    Corrupted   UMETA(DisplayName = "Corrupted"),       // Corrompido: tem modifier de debuff permanente
};

// -----------------------------------------------------------
// EZfItemMechanicResult
// Resultado genérico retornado pelas mecânicas do inventário
// (adicionar, remover, equipar, etc.) para facilitar debug e UI.
// -----------------------------------------------------------
UENUM(BlueprintType)
enum class EZfItemMechanicResult : uint8
{
    Success                 UMETA(DisplayName = "Success"),
    Failed_InventoryFull    UMETA(DisplayName = "Failed: Inventory Full"),
    Failed_InvalidSlot      UMETA(DisplayName = "Failed: Invalid Slot"),
    Failed_ItemNotFound     UMETA(DisplayName = "Failed: Item Not Found"),
    Failed_CannotEquip      UMETA(DisplayName = "Failed: Cannot Equip"),
    Failed_ItemBroken       UMETA(DisplayName = "Failed: Item Broken"),
    Failed_NotCorrupted     UMETA(DisplayName = "Failed: Item Not Corrupted"),
    Failed_NotEnoughCost    UMETA(DisplayName = "Failed: Not Enough Cost"),
    Failed_InvalidOperation UMETA(DisplayName = "Failed: Invalid Operation"),
    Failed_StackFull        UMETA(DisplayName = "Failed: Stack Full"),
    Failed_IncompatibleItem UMETA(DisplayName = "Failed: Incompatible Item Tag"),
    Failed_SlotBlocked      UMETA(DisplayName = "Failed: Slot Blocked By Two Handed Weapon")
    
};

// ============================================================
// STRUCTS DE MODIFIER
// ============================================================

// -----------------------------------------------------------
// FZfModifierRankRange
// Representa o range de valores (mínimo e máximo) para um
// rank específico de um modifier.
// Exemplo: Rank 1 de MoveSpeed = Min: 3%, Max: 7%
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfModifierRankRange
{
    GENERATED_BODY()
    
    // Valor mínimo que este rank pode rolar
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Rank")
    float MinValue = 0.0f;
    
    // Valor máximo que este rank pode rolar
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Rank")
    float MaxValue = 0.0f;
};

// -----------------------------------------------------------
// FZfModifierRankData
// Dados completos de um rank de modifier.
// Contém o range de valores e o multiplicador de Awakening.
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfModifierRankData
{
    GENERATED_BODY()
    
    // Nível deste rank (1 a 6, ou mais se expandido)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Rank")
    int32 RankLevel = 1;
    
    // Range de valores para este rank
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Rank")
    FZfModifierRankRange RankRange;
    
    // Percentual máximo base do valor dentro do range (0.0 = MinValue, 1.0 = MaxValue)
    // Awakening aumenta esse teto além de 1.0
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Rank")
    float CurrentMaxPercentage = 1.0f;
    
    // Quantas vezes este rank foi despertado (Awakening)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Rank")
    int32 AwakeningCount = 0;
    
    // Percentual adicional por stack de Awakening
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Rank")
    float AwakeningBonusPerStack = 0.1f;
};

// -----------------------------------------------------------
// FZfTierRankWeight
// Define a chance de um rank aparecer em um tier específico.
// Exemplo: No Tier 2, Rank 1 tem 60% e Rank 2 tem 40%.
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfTierRankWeight
{
    GENERATED_BODY()
    
    // Qual rank pode aparecer neste tier
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Tier")
    int32 RankToAppear = 1;

    // Probabilidade de aparecer (0.0 a 1.0, onde 1.0 = 100%)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Tier")
    float ProbabilityToAppear = 1.0f;
};

// -----------------------------------------------------------
// FZfTierData
// Dados de um tier específico, incluindo quais ranks podem
// aparecer e com qual probabilidade.
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfTierData
{
    GENERATED_BODY()
    
    // Nível do tier (0 a 5, ou mais se expandido)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Tier")
    int32 TierLevel = 0;
    
    // Lista de ranks possíveis e suas probabilidades neste tier
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Tier")
    TArray<FZfTierRankWeight> RankWeights;
};

// -----------------------------------------------------------
// FZfModifierClassLimit
// Define quantos modifiers de uma classe específica
// podem aparecer em um item.
// Exemplo: máx 2 Ofensivos, máx 1 Utilitário neste item.
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API  FZfModifierClassLimit
{
    GENERATED_BODY()

    // Classe do modifier que será limitada
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Limit")
    EZfModifierClass ModifierClass = EZfModifierClass::None;

    // Quantidade máxima de modifiers desta classe no item
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Limit")
    int32 MaxCount = 1;
};

// ============================================================
// STRUCTS DE ITEM INSTANCE
// ============================================================

// -----------------------------------------------------------
// FZfAppliedModifier
// Modifier já rolado e aplicado a um item específico.
// Armazena referência à DataTable, rank sorteado,
// valor atual e estado de Awakening.
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfAppliedModifier
{
    GENERATED_BODY()
    
    // Nome da linha na DataTable que define este modifier
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Applied")
    FName ModifierRowName = NAME_None;
    
    // classe do modifier cacheada para evitar lookup no DataTable
    // Preenchida ao rolar o modifier e usada para validar limites por classe
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Applied")
    EZfModifierClass ModifierClass = EZfModifierClass::None;
    
    // Rank atual deste modifier (1 a 6)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Applied")
    int32 CurrentRank = 0.0f;

    // Valor atual do modifier (interpolado entre Min e Max do rank * MaxRollPercentage)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Applied")
    float CurrentValue = 0.0f;

    // Percentual atual dentro do range (0.0 = mínimo, 1.0 = máximo base)
    // Modifier UP aumenta este valor em 10% por uso
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Applied")
    float CurrentRollPercentage = 0.0f;

    // Teto atual do percentual (1.0 base, aumenta com cada Awakening)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Applied")
    float MaxRollPercentage = 1.0f;

    // Quantas vezes este modifier foi despertado via Awakening
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Applied")
    int32 AwakeningCount = 0;

    // Se verdadeiro, este modifier é de debuff (gerado pela Corrupção)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Applied")
    bool bIsDebuffModifier = false;

    // Handle do GameplayEffect ativo em runtime (não replicado diretamente)
    // Gerenciado pelo EquipmentComponent ao equipar/desequipar
    FActiveGameplayEffectHandle ActiveEffectHandle;
};


// ============================================================
// STRUCTS DE COMBO SET
// ============================================================

// -----------------------------------------------------------
// FZfSetBonusEntry
// Um bônus de combo set ativado ao equipar N peças do set.
// Pode haver múltiplos bônus por set (ex: 2 peças, 4 peças, 6 peças).
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfSetBonusEntry
{
    GENERATED_BODY()

    // Quantidade mínima de peças do set para ativar este bônus
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set|Bonus")
    int32 RequiredPieceCount = 2;

    // GameplayEffect aplicado ao equipar as peças necessárias
    // Soft reference para não carregar em memória desnecessariamente
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set|Bonus")
    TSoftClassPtr<class UGameplayEffect> BonusGameplayEffect;

    // Descrição exibida na UI do inventário
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set|Bonus")
    FText BonusDescription;

    // Handle do efeito ativo em runtime
    // Não replicado — gerenciado pelo servidor via EquipmentComponent
    FActiveGameplayEffectHandle ActiveEffectHandle;
};

// ============================================================
// STRUCTS DE MARKET VALUE
// ============================================================

// -----------------------------------------------------------
// FZfMarketValueConfig
// Configuração do cálculo automático de valor de venda.
// O valor final é calculado por:
// BaseValue * RarityMultiplier[Rarity] * TierMultiplier[Tier]
// + (ModifierRankValueMultiplier * média dos ranks dos modifiers)
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfMarketValueConfig
{
    GENERATED_BODY()

    // Valor base do item (definido no ItemDefinition)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market|Value")
    float BaseValue = 10.0f;

    // Multiplicador indexado pela raridade (índice = EZfItemRarity)
    // Normal=1x, Uncommon=1.5x, Rare=2.5x, Epic=4x, Legendary=8x, Unique=20x
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market|Value")
    TArray<float> RarityMultipliers = { 1.0f, 1.5f, 2.5f, 4.0f, 8.0f, 20.0f };

    // Multiplicador indexado pelo tier (índice = ItemTier)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market|Value")
    TArray<float> TierMultipliers = { 1.0f, 1.5f, 2.2f, 3.0f, 4.5f, 6.0f };

    // Peso do rank médio dos modifiers no valor final
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market|Value")
    float ModifierRankValueMultiplier = 0.5f;
};

// ============================================================
// LOG CATEGORY
// ============================================================

// Categoria de log central usada em todo o sistema de inventário.
// Uso: UE_LOG(LogZfInventory, Warning, TEXT("Mensagem: %s"), *Info);
DECLARE_LOG_CATEGORY_EXTERN(LogZfInventory, Log, All);

