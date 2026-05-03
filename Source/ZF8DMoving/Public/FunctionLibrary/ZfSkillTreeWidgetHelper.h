// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SkillTreeSystem/ZfSkillTreeTypes.h"
#include "SkillTreeSystem/ZfSkillTreeNodeData.h"
#include "ZfSkillTreeWidgetHelper.generated.h"

class UZfSkillTreeComponent;
class APlayerState;

// =============================================================================
// FNodeTooltipData
// =============================================================================

/**
 * Dados prontos para exibição no WBP_Tooltip_Node.
 * Montado por GetNodeTooltipData — a widget só exibe, sem lógica.
 */
USTRUCT(BlueprintType)
struct FNodeTooltipData
{
	GENERATED_BODY()

	/** Ícone do nó. */
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip")
	TObjectPtr<UTexture2D> Icon;

	/** Nome da Skill. */
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip")
	FText DisplayName;

	/** Descrição da Skill. */
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip")
	FText Description;

	/** Rank atual do nó (0 = não desbloqueado). */
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip")
	int32 CurrentRank = 0;

	/**
	 * Próximo rank — usado para calcular o custo de ativação a exibir.
	 * = CurrentRank + 1 se não estiver no máximo.
	 * = CurrentRank se já estiver no máximo (Maxed).
	 * A widget usa GetCostForRank(NextRankForDisplay) em cada FSkillCostData.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip")
	int32 NextRankForDisplay = 1;

	/** Estado atual do nó — define qual custo exibir. */
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip")
	ESkillNodeState NodeState = ESkillNodeState::Locked;

	/**
	 * Custo a exibir em SP.
	 * Se Available/Locked    → custo de desbloqueio
	 * Se Unlocked/Affordable → custo de evolução do rank atual
	 * Se Maxed               → 0 (não exibe)
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip")
	int32 SkillPointCost = 0;

	/**
	 * Custos de ativação do próximo rank.
	 * Use GetCostForRank(NextRankForDisplay) em cada entry para obter o valor.
	 * Vazio se a Skill for passiva.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip")
	TArray<FSkillCostData> ActivationCosts;

	/** Requisitos de atributo não atendidos — só exibidos se Locked. */
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip")
	TArray<FAttributeRequirement> UnmetAttributeRequirements;

	/** Requisitos de tag não atendidos — só exibidos se Locked. */
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip")
	TArray<FTagRequirement> UnmetRequiredTags;
};

// =============================================================================
// FSubEffectExclusiveEntry
// =============================================================================

/**
 * Entrada de sub-efeito exclusivo para exibição no WBP_Tooltip_SubEffect.
 * Dados lidos diretamente do FSubEffectData do DA_Node — não precisa
 * ser configurado separadamente.
 */
USTRUCT(BlueprintType)
struct FSubEffectExclusiveEntry
{
	GENERATED_BODY()

	/** Nome do sub-efeito que bloqueia este. Ex: "Tiro Duplo" */
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip")
	FText DisplayName;

	/** Ícone do sub-efeito que bloqueia este. */
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip")
	TObjectPtr<UTexture2D> Icon;
};

// =============================================================================
// FSubEffectTooltipData
// =============================================================================

/**
 * Dados prontos para exibição no WBP_Tooltip_SubEffect.
 */
USTRUCT(BlueprintType)
struct FSubEffectTooltipData
{
	GENERATED_BODY()

	/** Ícone do sub-efeito. */
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip")
	TObjectPtr<UTexture2D> Icon;

	/** Nome do sub-efeito. */
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip")
	FText DisplayName;

	/** Descrição do sub-efeito. */
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip")
	FText Description;

	/** Custo em SkillPoints para desbloquear. */
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip")
	int32 UnlockCost = 0;

	/** Se true, o sub-efeito está bloqueado por exclusividade. */
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip")
	bool bIsExcluded = false;

	/**
	 * Sub-efeitos que bloqueiam este — com nome e ícone para exibição.
	 * Preenchido apenas se bIsExcluded = true.
	 * Dados lidos do FSubEffectData do DA_Node automaticamente.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip")
	TArray<FSubEffectExclusiveEntry> ExclusiveWith;
};

// =============================================================================
// ESkillSlotState
// =============================================================================

/**
 * Estado visual de uma skill equipada no slot.
 * Nunca armazenado — sempre derivado em runtime.
 *
 * Normal      → skill disponível para uso
 * NoResource  → sem mana/stamina/vida suficiente
 * OnCooldown  → em cooldown
 * Executing   → sendo executada agora
 * Active      → buff ativo (animação inversa ao cooldown)
 */
UENUM(BlueprintType)
enum class ESkillSlotState : uint8
{
	Normal      UMETA(DisplayName = "Normal"),
	NoResource  UMETA(DisplayName = "No Resource"),
	OnCooldown  UMETA(DisplayName = "On Cooldown"),
	Executing   UMETA(DisplayName = "Executing"),
	Active      UMETA(DisplayName = "Active"),
};

// =============================================================================
// UZfSkillTreeWidgetHelper
// =============================================================================

/**
 * Biblioteca de funções para as widgets da Skill Tree.
 *
 * Centraliza a lógica de consulta ao sistema de skill tree para que
 * os Blueprints das widgets não precisem conhecer detalhes de implementação.
 *
 * Todas as funções são BlueprintCallable e recebem o PlayerState como contexto.
 */
UCLASS()
class ZF8DMOVING_API UZfSkillTreeWidgetHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// ── Tooltip ───────────────────────────────────────────────────────────

	/**
	 * Monta os dados completos para o WBP_Tooltip_Node.
	 * Determina qual custo exibir baseado no estado atual do nó.
	 *
	 * @param PlayerState  PlayerState do personagem local
	 * @param NodeData     Data Asset do nó
	 * @param OutData      Dados prontos para a widget exibir
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Widget")
	static void GetNodeTooltipData(
		APlayerState* PlayerState,
		UZfSkillTreeNodeData* NodeData,
		FNodeTooltipData& OutData);

	/**
	 * Monta os dados completos para o WBP_Tooltip_SubEffect.
	 *
	 * @param PlayerState     PlayerState do personagem local
	 * @param NodeData        Nó pai do sub-efeito
	 * @param SubEffectIndex  Índice em NodeData->SubEffects
	 * @param OutData         Dados prontos para a widget exibir
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Widget")
	static void GetSubEffectTooltipData(
		APlayerState* PlayerState,
		UZfSkillTreeNodeData* NodeData,
		int32 SubEffectIndex,
		FSubEffectTooltipData& OutData);

	// ── Estado dos nós ────────────────────────────────────────────────────

	/**
	 * Retorna o estado visual de um nó para a widget.
	 * Wrapper de UZfSkillTreeComponent::DeriveNodeState acessível no Blueprint.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree|Widget")
	static ESkillNodeState GetNodeState(
		APlayerState* PlayerState,
		UZfSkillTreeNodeData* NodeData);
	
	/**
	* Retorna o estado visual de uma região para a WBP_RegionOrb.
	*
	* Available → região livre (Novice/Special) ou personagem tem a tag da classe
	* Locked    → região de classe que o personagem ainda não atingiu
	* Excluded  → região de classe diferente da escolhida permanentemente
	*
	* @param PlayerState  PlayerState do personagem local
	* @param Region       Dados da região a verificar
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree|Widget")
	static ESkillNodeState GetRegionState(
		APlayerState* PlayerState,
		const FSkillTreeRegion& Region);

	/**
	 * Retorna o rank atual de um nó.
	 * Wrapper de UZfSkillTreeComponent::GetNodeRankFromASC.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree|Widget")
	static int32 GetNodeRank(
		APlayerState* PlayerState,
		UZfSkillTreeNodeData* NodeData);

	/**
	 * Retorna true se um sub-efeito específico está desbloqueado.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree|Widget")
	static bool IsSubEffectUnlocked(
		APlayerState* PlayerState,
		UZfSkillTreeNodeData* NodeData,
		int32 SubEffectIndex);

	/**
	 * Retorna true se um sub-efeito pode ser desbloqueado agora.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree|Widget")
	static bool CanUnlockSubEffect(
		APlayerState* PlayerState,
		UZfSkillTreeNodeData* NodeData,
		int32 SubEffectIndex);

	// ── Slots ─────────────────────────────────────────────────────────────

	/**
	 * Retorna o NodeData da Skill equipada num slot.
	 * Retorna nullptr se o slot estiver vazio ou bloqueado.
	 *
	 * @param PlayerState  PlayerState do personagem local
	 * @param SlotIndex    Índice do slot (0-based)
	 * @param SkillTree  Data Asset da tree (para lookup do NodeID)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree|Widget")
	static UZfSkillTreeNodeData* GetNodeDataForSlot(
		APlayerState* PlayerState,
		int32 SlotIndex,
		UZfSkillTreeData* SkillTree);

	/**
	 * Retorna o NodeData da skill equipada num slot de arma.
	 * Slot 0 = botão primário, Slot 1 = botão secundário.
	 * Retorna nullptr se o slot estiver vazio.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree|Widget")
	static UZfSkillTreeNodeData* GetNodeDataForWeaponSlot(
		APlayerState* PlayerState,
		int32 SlotIndex,
		UZfSkillTreeData* SkillTree);

	/**
	 * Retorna true se o slot está bloqueado para a classe atual.
	 * SlotIndex >= MaxActiveSkillSlots da classe → bloqueado.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree|Widget")
	static bool IsSlotLocked(APlayerState* PlayerState, int32 SlotIndex);

	/**
	 * Retorna o custo de ativação de um recurso para o rank informado.
	 * Wrapper de FSkillCostData::GetCostForRank acessível no Blueprint.
	 *
	 * @param CostData  Entry de custo do NodeData->Costs
	 * @param Rank      Rank a consultar (use NextRankForDisplay do TooltipData)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree|Widget")
	static float GetCostForRank(const FSkillCostData& CostData, int32 Rank);

	/**
	 * Retorna o nome do tipo de recurso para exibição na UI.
	 * Ex: Mana → "Mana", Stamina → "Stamina", Health → "Health"
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree|Widget")
	static FText GetResourceTypeName(EResourceType ResourceType);

	/**
	 * Retorna a cor associada ao tipo de recurso.
	 * Mana → azul | Stamina → amarelo | Health → vermelho
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree|Widget")
	static FLinearColor GetResourceTypeColor(EResourceType ResourceType);

	/**
	* Retorna o valor float para o parâmetro escalar "RegionState" do material da orb.
	* Configure o material para interpretar:
	*   0.0 = Locked    → dessaturado + cadeado (classe futura)
	*   1.0 = Available → cor normal (região acessível)
	*   2.0 = Excluded  → muito escuro + rachaduras (classe diferente)
	*
	* Uso no Blueprint:
	*   GetRegionState → GetRegionStateMaterialValue → Set Scalar Parameter Value (RegionState)
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree|Widget")
	static float GetRegionStateMaterialValue(ESkillNodeState RegionState);
	
	/**
	* Retorna o valor float para o parâmetro escalar "NodeState" do material do nó.
	* Configure o material para interpretar:
	*   0.0 = Locked
	*   1.0 = Available
	*   2.0 = Unlocked
	*   3.0 = Disabled
	*   4.0 = AffordableUpgrade
	*   5.0 = Maxed
	*   6.0 = Excluded
	*
	* Uso no Blueprint:
	*   GetNodeState → GetNodeStateMaterialValue → Set Scalar Parameter Value (NodeState)
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree|Widget")
	static float GetNodeStateMaterialValue(ESkillNodeState NodeState);

	/**
	 * Retorna o estado visual de uma skill equipada no slot.
	 * Análogo ao GetNodeState mas para o slot — derivado em runtime.
	 * Usado pelo WBP_SkillSlot para material e para verificar se pode arrastar.
	 *
	 * @param PlayerState  PlayerState do personagem local
	 * @param NodeData     NodeData da skill equipada no slot
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree|Widget")
	static ESkillSlotState GetSlotState(
		APlayerState* PlayerState,
		UZfSkillTreeNodeData* NodeData);

	/**
	 * Retorna o valor float para o parâmetro escalar "SlotState" do M_SkillSlot_Icon.
	 * Configure o material para interpretar:
	 *   0.0 = Normal
	 *   1.0 = NoResource
	 *   2.0 = OnCooldown
	 *   3.0 = Executing
	 *   4.0 = Active
	 *
	 * Uso no Blueprint:
	 *   GetSlotState → GetSlotStateMaterialValue → Set Scalar Parameter Value (SlotState)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree|Widget")
	static float GetSlotStateMaterialValue(ESkillSlotState SlotState);
	
	/**
	* Retorna o valor float para o parâmetro escalar "SubEffectState" do material do sub-efeito.
	* Configure o material para interpretar:
	*   0.0 = Locked    → não desbloqueado ainda
	*   1.0 = Available → pode desbloquear agora
	*   2.0 = Unlocked  → desbloqueado
	*   3.0 = Excluded  → bloqueado por exclusividade
	*
	* @param PlayerState     PlayerState do personagem
	* @param NodeData        Nó pai do sub-efeito
	* @param SubEffectIndex  Índice em NodeData->SubEffects
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree|Widget")
	static float GetSubEffectStateMaterialValue(APlayerState* PlayerState, UZfSkillTreeNodeData* NodeData, int32 SubEffectIndex);
	
	/**
	 * Retorna o tempo restante e a duração total do cooldown de uma Skill num slot.
	 * Retorna false se a Skill não estiver em cooldown.
	 *
	 * @param PlayerState      PlayerState do personagem local
	 * @param SlotIndex        Índice do slot
	 * @param SkillTree      Data Asset da tree
	 * @param OutTimeRemaining Tempo restante em segundos
	 * @param OutDuration      Duração total do cooldown
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Widget")
	static bool GetSlotCooldown(
		APlayerState* PlayerState,
		int32 SlotIndex,
		UZfSkillTreeData* SkillTree,
		float& OutTimeRemaining,
		float& OutDuration);

private:

	/** Recupera o SkillTreeComponent do PlayerState. Retorna nullptr se inválido. */
	static UZfSkillTreeComponent* GetTreeComponent(APlayerState* PlayerState);

	/** Recupera o ASC do PlayerState. Retorna nullptr se inválido. */
	static UAbilitySystemComponent* GetASC(APlayerState* PlayerState);
};