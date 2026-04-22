// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"
#include "GameplayTagContainer.h"
#include "AttributeSet.h"
#include "Abilities/GameplayAbility.h"
#include "ZfAbilityNodeData.generated.h"

// =============================================================================
// EResourceType
// =============================================================================

/**
 * Tipo de recurso consumido por uma ability.
 * Usado pela UI para exibir a cor correta do custo no slot.
 *   Mana    → azul
 *   Stamina → amarelo
 *   Health  → vermelho
 */
UENUM(BlueprintType)
enum class EResourceType : uint8
{
	Mana    UMETA(DisplayName = "Mana"),
	Stamina UMETA(DisplayName = "Stamina"),
	Health  UMETA(DisplayName = "Health"),
};

// =============================================================================
// FAbilityCostData
// =============================================================================

/**
 * Dado de custo de uma ability — vive no NodeData como fonte única de verdade.
 * Usado tanto pela lógica de gameplay (ApplyCost) quanto pela UI (display do slot).
 *
 * CostByRank segue o mesmo padrão de RankUpCosts:
 *   Índice 0 = custo no Rank 1, Índice 1 = custo no Rank 2, etc.
 *   Se o array tiver menos entradas que o rank atual, usa o último valor.
 */
USTRUCT(BlueprintType)
struct FAbilityCostData
{
	GENERATED_BODY()

	/** GE que aplica o custo. Ex: GE_ConsumeMana, GE_ConsumeStamina, GE_ConsumeHealth */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cost")
	TSubclassOf<UGameplayEffect> CostEffectClass;

	/** Tipo de recurso — usado pela UI para cor e ícone corretos. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cost")
	EResourceType ResourceType = EResourceType::Mana;

	/**
	 * Custo base por rank.
	 * Índice 0 = Rank 1, Índice 1 = Rank 2, etc.
	 * Se vazio, custo = 0 (gratuito para este recurso).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cost")
	TArray<float> CostByRank;

	/** Retorna o custo base para o rank informado.
	 * Rank começa em 0. Índice 0 = Rank 0, Índice 1 = Rank 1, etc.
	 * Clampado ao tamanho do array.
	 */
	float GetCostForRank(int32 Rank) const
	{
		if (CostByRank.IsEmpty()) return 0.f;
		const int32 Index = FMath::Clamp(Rank, 0, CostByRank.Num() - 1);
		return FMath::Max(0.f, CostByRank[Index]);
	}
};

// =============================================================================
// FTagRequirement
// =============================================================================

/**
 * Requisito de tag com nome e ícone amigáveis para exibição na UI.
 * Substitui o uso direto de FGameplayTagContainer em RequiredTags.
 */
USTRUCT(BlueprintType)
struct FTagRequirement
{
	GENERATED_BODY()

	/** Tag necessária para desbloquear o nó. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree|Requirement")
	FGameplayTag RequiredTag;

	/**
	 * Nome de exibição na UI.
	 * Ex: "Classe: Mago", "Quest: Matar o Dragão", "Skill: Bola de Fogo"
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree|Requirement")
	FText DisplayName;

	/** Ícone opcional do requisito exibido no WBP_RequirementEntry. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree|Requirement")
	TObjectPtr<UTexture2D> Icon;
};

// =============================================================================
// FAttributeRequirement
// =============================================================================

/**
 * Requisito numérico de atributo para desbloquear um nó.
 *
 * Separado de RequiredTags por design:
 *   RequiredTags          → estado do personagem (classe, missões, skills anteriores)
 *   AttributeRequirements → valor atual de um atributo (ex: Intelligence >= 30)
 */
USTRUCT(BlueprintType)
struct FAttributeRequirement
{
	GENERATED_BODY()

	/** Atributo a ser verificado. Ex: UZfMainAttributeSet::GetIntelligenceAttribute() */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree|Requirement")
	FGameplayAttribute Attribute;

	/** Valor mínimo necessário para o desbloqueio. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree|Requirement", meta = (ClampMin = "0"))
	float MinValue = 0.f;

	/**
	 * Nome de exibição do atributo na UI.
	 * Ex: "Inteligência", "Força", "Constituição"
	 * Evita expor nomes técnicos como "ZfMainAttributeSet.Intelligence".
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree|Requirement")
	FText AttributeDisplayName;

	/**
	 * Ícone do atributo exibido no WBP_RequirementEntry.
	 * Opcional — deixe nulo para exibir apenas o texto.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree|Requirement")
	TObjectPtr<UTexture2D> AttributeIcon;
};

// =============================================================================
// FSubEffectData
// =============================================================================

/**
 * Sub-efeito de uma habilidade específica.
 *
 * Cada sub-efeito pertence exclusivamente a um nó da tree e concede
 * uma FGameplayTag ao personagem quando desbloqueado.
 * A Gameplay Ability dona do sub-efeito é responsável por interpretar
 * essa tag e ajustar sua execução (dano, área, comportamento, etc.).
 *
 * Convenção de tag: SkillTree.SubEffect.<NomeDaAbility>.<NomeDoEfeito>
 */
USTRUCT(BlueprintType)
struct FSubEffectData
{
	GENERATED_BODY()

	/**
	 * Tag concedida ao personagem ao desbloquear este sub-efeito.
	 * A ability correspondente consulta esta tag durante a execução.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree|SubEffect")
	FGameplayTag GrantedTag;

	/** Ícone exibido no sub-efeito na skill tree. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree|SubEffect")
	TObjectPtr<UTexture2D> Icon;

	/** Custo em SkillPoints para desbloquear este sub-efeito. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree|SubEffect", meta = (ClampMin = "1"))
	int32 UnlockCost = 1;

	/**
	 * Tags que impedem o desbloqueio deste sub-efeito.
	 * Se o personagem possuir QUALQUER uma dessas tags, este sub-efeito
	 * fica permanentemente inacessível (lógica OR).
	 *
	 * Use para criar grupos mutuamente exclusivos entre sub-efeitos:
	 *   SubEffect A → ExclusiveTags = [GrantedTag do SubEffect B]
	 *   SubEffect B → ExclusiveTags = [GrantedTag do SubEffect A]
	 *
	 * Sub-efeitos sem ExclusiveTags são livres — podem ser combinados com qualquer outro.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree|SubEffect")
	FGameplayTagContainer ExclusiveTags;

	/** Nome exibido na UI. Ex: "Queimadura" */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree|SubEffect")
	FText DisplayName;

	/** Descrição do efeito exibida na UI. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree|SubEffect")
	FText Description;
};

// =============================================================================
// UZfAbilityNodeData
// =============================================================================

/**
 * Data Asset individual que representa um nó da Skill Tree.
 *
 * Cada habilidade desbloqueável na tree tem seu próprio asset deste tipo.
 * Isso permite editar cada nó em sua própria janela no editor, e o array
 * de nós na região exibe o nome do asset automaticamente.
 *
 * Como criar no editor:
 *   Content Browser → botão direito → Miscellaneous → Data Asset
 *   → selecione UZfAbilityNodeData → nomeie como DA_Node_<NomeDaAbility>
 *   Ex: DA_Node_Fireball, DA_Node_Sprint, DA_Node_MoveSpeedBoost
 *
 * Pré-requisitos de dois tipos:
 *   RequiredTags           → tags que o personagem precisa ter
 *   AttributeRequirements  → atributos com valor mínimo
 *
 * Escalamento por rank:
 *   MaxAbilityRank define quantas vezes o nó pode ser evoluído.
 *   Custo de evolução = RankUpCost * rank_atual.
 */
UCLASS(BlueprintType)
class ZF8DMOVING_API UZfAbilityNodeData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	// ── Identidade ────────────────────────────────────────────────────────

	/**
	 * Identificador único do nó na tree.
	 * Usado como chave em TMaps de save e lookup em runtime.
	 * Convenção: <Região>.<NomeDaAbility>  Ex: "Mage.Fireball", "Novice.Sprint"
	 * Deve ser único em toda a tree.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|Identity")
	FName NodeID;

	/** Gameplay Ability concedida ao ASC quando este nó é desbloqueado. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|Identity")
	TSubclassOf<UGameplayAbility> AbilityClass;

	/**
	 * Tag concedida ao personagem ao desbloquear este nó.
	 * Usada como pré-requisito de outros nós que dependem deste.
	 * Convenção: SkillTree.Node.<Região>.<NomeDaAbility>
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|Identity",
		meta = (Categories = "SkillTree.Node"))
	FGameplayTag GrantedTag;

	// ── Progressão ────────────────────────────────────────────────────────

	/** Rank máximo que esta ability pode atingir. Mínimo: 1. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|Progression",
		meta = (ClampMin = "1"))
	int32 MaxAbilityRank = 1;

	/** Custo em SkillPoints para desbloquear (Rank 0 → Rank 1). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|Progression",
		meta = (ClampMin = "1"))
	int32 UnlockCost = 1;

	/**
	 * Custo em SkillPoints por evolução de rank — customizado por nível.
	 * Índice 0 = custo para Rank 1 → Rank 2
	 * Índice 1 = custo para Rank 2 → Rank 3
	 * E assim por diante.
	 *
	 * O array deve ter exatamente MaxAbilityRank - 1 entradas.
	 * Ex: MaxAbilityRank=3, RankUpCosts=[2, 4] → Rank1→2 custa 2, Rank2→3 custa 4.
	 *
	 * Se o array estiver vazio ou o índice não existir, o custo padrão é 1.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|Progression")
	TArray<int32> RankUpCosts;

	/**
	 * Retorna o custo para evoluir do rank atual para o próximo.
	 * Rank começa em 0. Índice 0 = custo para Rank 0 → Rank 1.
	 * @param CurrentRank rank atual (começa em 0)
	 */
	int32 GetRankUpCost(int32 CurrentRank) const
	{
		if (RankUpCosts.IsValidIndex(CurrentRank))
		{
			return FMath::Max(1, RankUpCosts[CurrentRank]);
		}
		return 1;
	}

	// ── Pré-requisitos ────────────────────────────────────────────────────

	/**
	 * Tags necessárias para desbloquear este nó — com nome e ícone para a UI.
	 * Todas precisam ser satisfeitas (lógica AND).
	 * Inclui: tag da classe, tags de missão, tags de skills anteriores.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|Requirements")
	TArray<FTagRequirement> RequiredTags;

	/**
	 * Requisitos numéricos de atributo.
	 * Cada entrada é verificada individualmente (AND).
	 * Ex: Intelligence >= 30, Constitution >= 15
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|Requirements")
	TArray<FAttributeRequirement> AttributeRequirements;

	// ── Sub-efeitos ───────────────────────────────────────────────────────

	/** Todos os sub-efeitos possíveis deste nó. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|SubEffects")
	TArray<FSubEffectData> SubEffects;

	/**
	 * Quantos sub-efeitos podem ser desbloqueados simultaneamente.
	 * -1 = sem limite (todos podem ser desbloqueados).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|SubEffects",
		meta = (ClampMin = "-1"))
	int32 MaxUnlockableSubEffects = -1;

	// ── Custo ─────────────────────────────────────────────────────────────

	/**
	 * Custos de recurso desta ability — fonte única de verdade.
	 * Lido por ZfAbility_Active::ApplyCost e pela UI do slot.
	 * Suporta múltiplos recursos simultaneamente (ex: Mana + Stamina).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|Cost")
	TArray<FAbilityCostData> Costs;

	// ── UI ────────────────────────────────────────────────────────────────

	/**
	 * Ícone exibido no nó da skill tree e no slot ativo da HUD.
	 * O mesmo asset é reutilizado nos dois contextos.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|UI")
	TObjectPtr<UTexture2D> Icon;

	/** Nome exibido na UI. Ex: "Bola de Fogo" */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|UI")
	FText DisplayName;

	/** Descrição exibida no tooltip do nó. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|UI")
	FText Description;
};