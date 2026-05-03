// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"
#include "GameplayTagContainer.h"
#include "AttributeSet.h"
#include "Abilities/GameplayAbility.h"
#include "ZfSkillTreeNodeData.generated.h"

// =============================================================================
// EResourceType
// =============================================================================

/**
 * Tipo de recurso consumido por uma Skill.
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
// FSkillCostData
// =============================================================================

/**
 * Dado de custo de uma Skill — vive no NodeData como fonte única de verdade.
 * Usado tanto pela lógica de gameplay (ApplyCost) quanto pela UI (display do slot).
 *
 * CostByRank segue o mesmo padrão de RankUpCosts:
 *   Índice 0 = custo no Rank 1, Índice 1 = custo no Rank 2, etc.
 *   Se o array tiver menos entradas que o rank atual, usa o último valor.
 */
USTRUCT(BlueprintType)
struct FSkillCostData
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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree|Requirement", meta = (GameplayTagFilter = "SkillTree.Node"))
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
 * A Gameplay Skill dona do sub-efeito é responsável por interpretar
 * essa tag e ajustar sua execução (dano, área, comportamento, etc.).
 *
 * Convenção de tag: SkillTree.SubEffect.<NomeDaSkill>.<NomeDoEfeito>
 */
USTRUCT(BlueprintType)
struct FSubEffectData
{
	GENERATED_BODY()

	/**
	 * Tag concedida ao personagem ao desbloquear este sub-efeito.
	 * A Skill correspondente consulta esta tag durante a execução.
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
// UZfSkillTreeNodeData
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
 *   → selecione UZfSkillTreeNodeData → nomeie como DA_Node_<NomeDaSkill>
 *   Ex: DA_Node_Fireball, DA_Node_Sprint, DA_Node_MoveSpeedBoost
 *
 * Pré-requisitos de dois tipos:
 *   RequiredTags           → tags que o personagem precisa ter
 *   AttributeRequirements  → atributos com valor mínimo
 *
 * Escalamento por rank:
 *   MaxSkillRank define quantas vezes o nó pode ser evoluído.
 *   Custo de evolução = RankUpCost * rank_atual.
 */
UCLASS(BlueprintType)
class ZF8DMOVING_API UZfSkillTreeNodeData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	// ── Identidade ────────────────────────────────────────────────────────

	/**
	 * Identificador único do nó na tree.
	 * Usado como chave em TMaps de save e lookup em runtime.
	 * Convenção: <Região>.<NomeDaSkill>  Ex: "Mage.Fireball", "Novice.Sprint"
	 * Deve ser único em toda a tree.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|Identity")
	FName NodeID;

	/** Gameplay Ability concedida ao ASC quando este nó é desbloqueado. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|Identity")
	TSubclassOf<UGameplayAbility> SkillClass;

	/**
	 * Tag concedida ao personagem ao desbloquear este nó.
	 * Usada como pré-requisito de outros nós que dependem deste.
	 * Convenção: SkillTree.Node.<Região>.<NomeDaSkill>
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|Identity",
		meta = (Categories = "SkillTree.Node"))
	FGameplayTag GrantedTag;

	// ── Progressão ────────────────────────────────────────────────────────

	/** Rank máximo que esta Skill pode atingir. Mínimo: 1. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|Progression",
		meta = (ClampMin = "1"))
	int32 MaxSkillRank = 1;

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
	 * O array deve ter exatamente MaxSkillRank - 1 entradas.
	 * Ex: MaxSkillRank=3, RankUpCosts=[2, 4] → Rank1→2 custa 2, Rank2→3 custa 4.
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
		const int32 Index = CurrentRank - 1;
		if (RankUpCosts.IsValidIndex(Index))
		{
			return FMath::Max(1, RankUpCosts[Index]);
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
	 * Custos de recurso desta Skill — fonte única de verdade.
	 * Lido por ZfSkill_Active::ApplyCost e pela UI do slot.
	 * Suporta múltiplos recursos simultaneamente (ex: Mana + Stamina).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|Cost")
	TArray<FSkillCostData> Costs;
	
	// ── Cooldown ──────────────────────────────────────────────────────────
 
	/**
	 * Tag do GE de cooldown desta skill.
	 * Usada pela widget do slot para detectar quando o cooldown está ativo
	 * e iniciar a animação de preenchimento.
	 *
	 * Convenção: Cooldown.Node.<Região>.<NomeDaSkill>
	 * Ex: Cooldown.Node.Novice.MoveSpeedBoost
	 *
	 * Configure também no GE de cooldown em GrantedTags com esta mesma tag.
	 * Deixe vazio para skills passivas (sem cooldown).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|Cooldown", meta = (GameplayTagFilter = "Cooldown.Node"))
	FGameplayTag CooldownTag;
	 
	/**
	 * Duração do cooldown por rank em segundos.
	 * Índice 0 = Rank 1, Índice 1 = Rank 2, etc.
	 * Se vazio ou índice não existir, usa o último valor disponível.
	 * Deixe vazio para skills passivas (sem cooldown).
	 * Ex: [5.0, 4.0, 3.0, 2.0] → Rank1=5s, Rank2=4s, Rank3=3s, Rank4=2s
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|Cooldown",
		meta = (ClampMin = "0"))
	TArray<float> CooldownDurations;
 
	/**
	 * Retorna a duração do cooldown para o rank informado.
	 * Rank começa em 1. Índice 0 = Rank 1.
	 * Clampado ao tamanho do array.
	 * Retorna 0 se vazio (sem cooldown).
	 */
	float GetCooldownDuration(int32 CurrentRank) const
	{
		if (CooldownDurations.IsEmpty()) return 0.f;
		const int32 Index = FMath::Clamp(CurrentRank - 1, 0, CooldownDurations.Num() - 1);
		return FMath::Max(0.f, CooldownDurations[Index]);
	}
	
	// ── Buff ──────────────────────────────────────────────────────────────
 
	/**
	 * Tag do GE de buff desta skill.
	 * Usada pela widget do slot para detectar quando o buff está ativo
	 * e iniciar a animação inversa ao cooldown (vai esvaziando).
	 *
	 * Convenção: Buff.Node.<Região>.<NomeDaSkill>
	 * Ex: Buff.Node.Novice.MoveSpeedBoost
	 *
	 * Configure também no GE de buff em GrantedTags com esta mesma tag.
	 * Deixe vazio para skills sem buff (dano puro, etc.).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|Buff",
		meta = (Categories = "Buff.Node"))
	FGameplayTag BuffTag;
 
	/**
	 * Duração do buff por rank em segundos.
	 * Índice 0 = Rank 1, Índice 1 = Rank 2, etc.
	 * Se vazio ou índice não existir, usa o último valor disponível.
	 * Deixe vazio para skills sem buff.
	 * Ex: [30.0, 35.0, 40.0, 45.0] → aumenta com o rank
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|Buff",
		meta = (ClampMin = "0"))
	TArray<float> BuffDurations;
 
	/**
	 * Retorna a duração do buff para o rank informado.
	 * Rank começa em 1. Índice 0 = Rank 1.
	 * Clampado ao tamanho do array.
	 * Retorna 0 se vazio (sem buff).
	 */
	float GetBuffDuration(int32 CurrentRank) const
	{
		if (BuffDurations.IsEmpty()) return 0.f;
		const int32 Index = FMath::Clamp(CurrentRank - 1, 0, BuffDurations.Num() - 1);
		return FMath::Max(0.f, BuffDurations[Index]);
	}

	// ── UI ────────────────────────────────────────────────────────────────

	/** Ícone exibido no nó da skill tree */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|UI")
	TObjectPtr<UTexture2D> Icon;
	
	/** Ícone exibido no Slot da Hud */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|UI")
	TObjectPtr<UTexture2D> IconSlot;

	/** Nome exibido na UI. Ex: "Bola de Fogo" */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|UI")
	FText DisplayName;

	/** Descrição exibida no tooltip do nó. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|UI")
	FText Description;

	// ── Cast e Targeting ──────────────────────────────────────────────────

	/**
	 * Raio da área de efeito por rank em unidades do mundo.
	 * Índice 0 = Rank 1, Índice 1 = Rank 2, etc.
	 * Se vazio ou índice não existir, usa o último valor disponível.
	 * Relevante apenas para skills de AoE (GroundCircle).
	 *
	 * Valor base — a GA pode aplicar modificadores adicionais via
	 * sub-efeitos em runtime sobrescrevendo GetEffectiveAimRadius().
	 * Ex: [300.0, 380.0, 480.0] → Rank1=300, Rank2=380, Rank3=480
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|Cast",
		meta = (ClampMin = "50"))
	TArray<float> AimRadiusByRank;

	/**
	 * Retorna o raio de aim base para o rank informado.
	 * Rank começa em 1. Índice 0 = Rank 1.
	 * Retorna 300 se vazio (fallback razoável).
	 */
	float GetAimRadiusForRank(int32 CurrentRank) const
	{
		if (AimRadiusByRank.IsEmpty()) return 300.f;
		const int32 Index = FMath::Clamp(CurrentRank - 1, 0, AimRadiusByRank.Num() - 1);
		return FMath::Max(50.f, AimRadiusByRank[Index]);
	}

	/**
	 * Alcance máximo da skill por rank em unidades do mundo.
	 * Índice 0 = Rank 1, Índice 1 = Rank 2, etc.
	 * Se vazio ou índice não existir, usa o último valor disponível.
	 * 0 = sem limite de alcance para aquele rank.
	 *
	 * Valor base — a GA pode aplicar modificadores adicionais via
	 * sub-efeitos em runtime sobrescrevendo GetEffectiveMaxRange().
	 * Ex: [1500.0, 1800.0, 2200.0] → aumenta com o rank
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node|Cast",
		meta = (ClampMin = "0"))
	TArray<float> MaxRangeByRank;

	/**
	 * Retorna o alcance máximo base para o rank informado.
	 * Rank começa em 1. Índice 0 = Rank 1.
	 * Retorna 1500 se vazio (fallback padrão).
	 */
	float GetMaxRangeForRank(int32 CurrentRank) const
	{
		if (MaxRangeByRank.IsEmpty()) return 1500.f;
		const int32 Index = FMath::Clamp(CurrentRank - 1, 0, MaxRangeByRank.Num() - 1);
		return FMath::Max(0.f, MaxRangeByRank[Index]);
	}
};