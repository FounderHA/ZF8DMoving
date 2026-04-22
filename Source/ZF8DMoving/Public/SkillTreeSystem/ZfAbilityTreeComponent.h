// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayEffect.h"
#include "SkillTreeSystem/ZfAbilityTreeTypes.h"
#include "SkillTreeSystem/ZfAbilityNodeData.h"
#include "ZfAbilityTreeComponent.generated.h"

class UZfAbilityTreeData;
class UAbilitySystemComponent;
class APlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTreeStateChanged);

// =============================================================================
// FAbilitySlotLoadout
// =============================================================================

/**
 * Estado replicado dos slots ativos de habilidade do personagem.
 *
 * Armazena NodeIDs em vez de FGameplayAbilitySpecHandle porque:
 *   - Handles são internos ao ASC e sem significado para clientes remotos
 *   - NodeID é suficiente para a UI consultar o Data Asset e renderizar o slot
 *   - TArray<FName> replica nativamente sem problemas
 *
 * Tamanho de Slots é definido por MaxActiveAbilitySlots da classe do personagem.
 * Slots vazios são representados por NAME_None.
 *
 * BasicAttack e WeaponSpecial são gerenciados pelo UZfEquipmentComponent.
 */
USTRUCT(BlueprintType)
struct FAbilitySlotLoadout
{
	GENERATED_BODY()

	/**
	 * NodeIDs das abilities equipadas nos slots ativos.
	 * Tamanho = UZfPrimaryDataAssetClass::MaxActiveAbilitySlots.
	 * NAME_None = slot vazio.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "SkillTree|Loadout")
	TArray<FName> Slots;
};

// =============================================================================
// UZfAbilityTreeComponent
// =============================================================================

/**
 * Componente central da Skill Tree do projeto Zf.
 *
 * Responsabilidades:
 *   - Validar pré-requisitos de desbloqueio (tags + atributos + SkillPoints)
 *   - Conceder/revogar Gameplay Abilities ao ASC ao desbloquear/respecar
 *   - Conceder/remover GameplayTags ao desbloquear nós e sub-efeitos
 *   - Gerenciar o loadout de slots ativos
 *   - Serializar e restaurar o estado da tree via FCharacterTreeSaveData
 *
 * Onde vive:
 *   AZfPlayerState — mesmo padrão de UZfInventoryComponent e UZfEquipmentComponent.
 *
 * Replicação:
 *   O componente replica apenas o FAbilitySlotLoadout (slots ativos visíveis a todos).
 *   O estado completo da tree (nós desbloqueados, ranks, sub-efeitos) é derivável
 *   pelas GameplayTags do ASC, que já replicam automaticamente via Mixed mode.
 *   Operações de escrita (unlock, upgrade, respec) ocorrem exclusivamente no servidor,
 *   disparadas pelos Server RPCs do AZfPlayerState.
 *
 * Separação UnlockNode vs RestoreNode:
 *   UnlockNode    → valida pré-requisitos e SkillPoints antes de conceder (jogador)
 *   RestoreNode   → concede diretamente sem validação (load do save)
 *   Nunca misture os dois caminhos.
 */
UCLASS(ClassGroup = "Zf", meta = (BlueprintSpawnableComponent))
class ZF8DMOVING_API UZfAbilityTreeComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UZfAbilityTreeComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ── Configuração ──────────────────────────────────────────────────────

	/**
	 * Número máximo de slots de ability possíveis no jogo.
	 * Define o tamanho fixo visual da widget de slots — sempre exibe
	 * este número de slots, independente da classe do personagem.
	 * Slots além do MaxActiveAbilitySlots da classe aparecem como "bloqueados".
	 *
	 * Deve corresponder à classe com mais slots do jogo (ex: Arcmage = 8).
	 */
	static constexpr int32 MaxPossibleAbilitySlots = 8;

	/** Retorna o número máximo de slots possíveis — usado pela widget para
	 * construir o layout fixo de slots independente da classe. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree")
	static int32 GetMaxPossibleAbilitySlots() { return MaxPossibleAbilitySlots; }

	/**
	 * Data Asset único da skill tree universal.
	 * Configure no CDO do PlayerState apontando para DA_AbilityTree.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree")
	TObjectPtr<UZfAbilityTreeData> AbilityTreeData;

	/**
	 * GE Instant com SetByCaller usado para gastar e devolver SkillPoints.
	 * Negativo = gastar. Positivo = devolver (respec).
	 * Configure no PlayerState Blueprint apontando para GE_SpendSkillPoints.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree")
	TSubclassOf<UGameplayEffect> SpendSkillPointsEffectClass;

	// ── Validação — acessível no servidor e no cliente ────────────────────

	/**
	 * Verifica se o personagem pode desbloquear um nó.
	 * Checa: tags do ASC + AttributeRequirements + SkillPoints disponíveis.
	 *
	 * Chamado em dois contextos:
	 *   Servidor → antes de conceder a ability (UnlockNode)
	 *   Cliente  → pela widget para derivar EAbilityNodeState de cada nó
	 *
	 * @param PlayerState  PlayerState do personagem a verificar
	 * @param Node         Nó a ser verificado
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	static bool CanUnlockNode(APlayerState* PlayerState, const UZfAbilityNodeData* Node);

	/**
	 * Verifica se o personagem pode evoluir um nó já desbloqueado.
	 * Checa: nó já desbloqueado + rank atual < MaxAbilityRank + SkillPoints disponíveis.
	 *
	 * @param PlayerState  PlayerState do personagem a verificar
	 * @param Node         Nó a ser verificado
	 * @param CurrentRank  Rank atual do nó
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	static bool CanUpgradeNode(APlayerState* PlayerState, const UZfAbilityNodeData* Node, int32 CurrentRank);

	/**
	 * Verifica se o personagem pode desbloquear um sub-efeito específico.
	 * Checa: nó pai desbloqueado + sub-efeito não desbloqueado ainda
	 *        + limite MaxUnlockableSubEffects não atingido + SkillPoints disponíveis.
	 *
	 * @param PlayerState      PlayerState do personagem
	 * @param Node             Nó pai do sub-efeito
	 * @param SubEffectIndex   Índice em FAbilityTreeNode::SubEffects
	 * @param UnlockedIndices  Sub-efeitos já desbloqueados neste nó
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	static bool CanUnlockSubEffect(APlayerState* PlayerState, const UZfAbilityNodeData* Node,
		int32 SubEffectIndex, const TArray<int32>& UnlockedIndices);

	/**
	 * Deriva o estado visual de um nó para a UI.
	 * Nunca armazenado — calculado em runtime com base no estado atual do personagem.
	 *
	 * @param PlayerState   PlayerState do personagem
	 * @param Node          Nó a derivar o estado
	 * @param CurrentRank   Rank atual (0 = não desbloqueado)
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	static EAbilityNodeState DeriveNodeState(APlayerState* PlayerState,
		const UZfAbilityNodeData* Node, int32 CurrentRank);

	// ── Operações de escrita — servidor only ──────────────────────────────

	/**
	 * Desbloqueia um nó: valida, consome SkillPoints, concede ability ao ASC
	 * e adiciona GrantedTag ao personagem.
	 * Chamado pelas GAs, nunca diretamente pela widget.
	 *
	 * @param ASC     ASC do personagem
	 * @param NodeID  Identificador do nó a desbloquear
	 */
	bool UnlockNode(UAbilitySystemComponent* ASC, FName NodeID);

	/**
	 * Evolui um nó já desbloqueado: valida, consome SkillPoints e incrementa
	 * o Level no FGameplayAbilitySpec correspondente.
	 *
	 * @param ASC     ASC do personagem
	 * @param NodeID  Identificador do nó a evoluir
	 */
	bool UpgradeNode(UAbilitySystemComponent* ASC, FName NodeID);

	/**
	 * Desbloqueia um sub-efeito de um nó: valida, consome SkillPoints
	 * e adiciona a GrantedTag do sub-efeito ao personagem.
	 *
	 * @param ASC            ASC do personagem
	 * @param NodeID         Nó pai do sub-efeito
	 * @param SubEffectIndex Índice em FAbilityTreeNode::SubEffects
	 */
	bool UnlockSubEffect(UAbilitySystemComponent* ASC, FName NodeID, int32 SubEffectIndex);

	/**
	 * Reseta toda a tree: revoga abilities, remove tags concedidas pela tree,
	 * zera NodeRanks e UnlockedSubEffects, e devolve todos os SkillPoints gastos.
	 * Respeita o custo de respec (grátis até o nível threshold, custo depois).
	 *
	 * Tags de classe e tags de missão NÃO são removidas.
	 *
	 * @param ASC ASC do personagem
	 */
	bool RespecTree(UAbilitySystemComponent* ASC);

	// ── Loadout de slots ──────────────────────────────────────────────────

	/**
	 * Equipa uma ability desbloqueada num slot ativo.
	 * Substitui o slot sem revogar a ability do ASC — ability continua concedida.
	 *
	 * @param ASC       ASC do personagem
	 * @param NodeID    Nó a equipar (deve estar desbloqueado)
	 * @param SlotIndex Índice do slot (0-based, < MaxActiveAbilitySlots)
	 */
	bool EquipAbilityInSlot(UAbilitySystemComponent* ASC, FName NodeID, int32 SlotIndex);

	/**
	 * Remove a ability de um slot ativo.
	 * A ability continua concedida no ASC — apenas sai do slot visível.
	 *
	 * @param SlotIndex Índice do slot a esvaziar
	 */
	bool UnequipAbilityFromSlot(int32 SlotIndex);

	// ── Save e Restore ────────────────────────────────────────────────────

	/**
	 * Restaura o estado da tree a partir do save do personagem.
	 * Não valida pré-requisitos — o save já representa um estado válido anterior.
	 * Chamado no load do personagem, antes de qualquer interação do jogador.
	 *
	 * @param ASC      ASC do personagem
	 * @param SaveData Dados serializados do save
	 */
	void RestoreFromSaveData(UAbilitySystemComponent* ASC, const FCharacterTreeSaveData& SaveData);

	/**
	 * Serializa o estado atual da tree para salvar em disco.
	 * Chamado pelo sistema de save do jogo.
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Save")
	FCharacterTreeSaveData BuildSaveData() const;

	// ── Consulta de estado runtime ────────────────────────────────────────

	/** Retorna true se o nó está desbloqueado pelo personagem. */
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	bool IsNodeUnlocked(FName NodeID) const;

	/** Retorna o rank atual de um nó (0 = não desbloqueado). */
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	int32 GetNodeRank(FName NodeID) const;

	/** Retorna os índices de sub-efeitos desbloqueados de um nó. */
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	TArray<int32> GetUnlockedSubEffects(FName NodeID) const;
	
	/**
	* Retorna o rank atual de um nó consultando o AbilityLevel do ASC.
	* Funciona tanto no servidor quanto no cliente owner.
	* Preferível a GetNodeRank() para a UI pois não depende do TMap server-only.
	*
	* @param ASC     ASC do personagem (owner client ou servidor)
	* @param NodeID  Identificador do nó
	*/
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	int32 GetNodeRankFromASC(UAbilitySystemComponent* ASC, FName NodeID) const;

	/** Retorna o loadout atual de slots (replicado). */
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	const FAbilitySlotLoadout& GetLoadout() const { return Loadout; }

	/**
	 * Disparado no cliente quando qualquer estado da tree muda —
	 * nó desbloqueado, rank evoluído, sub-efeito desbloqueado ou loadout alterado.
	 *
	 * A WBP_SkillTree_Region ouve este delegate e dispara a animação
	 * e recriação do tooltip no nó afetado.
	 *
	 * Bind no Blueprint:
	 *   AbilityTreeComponent → Assign On Tree State Changed → RefreshNodes()
	 */
	UPROPERTY(BlueprintAssignable, Category = "SkillTree")
	FOnTreeStateChanged OnTreeStateChanged;

	/**
	 * Registra o listener de tags da skill tree no ASC.
	 * Deve ser chamado após InitAbilityActorInfo no PlayerState.
	 * Sem HasAuthority — deve rodar no servidor e no owning client.
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	void InitializeTagListeners(UAbilitySystemComponent* ASC);

private:

	// ── Estado runtime — servidor only ────────────────────────────────────

	/** NodeIDs de todos os nós desbloqueados. */
	TSet<FName> UnlockedNodes;

	/** Rank atual por NodeID. Ausente = Rank 1 (estado inicial do desbloqueio). */
	TMap<FName, int32> NodeRanks;

	/** Sub-efeitos desbloqueados por NodeID. */
	TMap<FName, FSubEffectIndexList> UnlockedSubEffects;

	// ── Handles internos — servidor only ──────────────────────────────────

	/**
	 * Handles das abilities concedidas ao ASC pela tree.
	 * Necessário para revogar corretamente no Respec.
	 * Nunca replicado — handle não tem significado para outros clientes.
	 */
	TMap<FName, FGameplayAbilitySpecHandle> GrantedAbilityHandles;

	// ── Loadout replicado ─────────────────────────────────────────────────

	/**
	 * Slots ativos do personagem — replicado para todos os clientes.
	 * Clientes remotos usam os NodeIDs para exibir as abilities equipadas.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_Loadout)
	FAbilitySlotLoadout Loadout;

	UFUNCTION()
	void OnRep_Loadout();

	// ── Helpers privados ──────────────────────────────────────────────────

	/** Consome SkillPoints via GE. Retorna false se insuficientes. */
	bool SpendSkillPoints(UAbilitySystemComponent* ASC, int32 Amount);

	/** Devolve SkillPoints via GE (respec). */
	void RefundSkillPoints(UAbilitySystemComponent* ASC, int32 Amount);

	/** Concede a ability do nó ao ASC e armazena o handle. */
	void GrantNodeAbility(UAbilitySystemComponent* ASC, const UZfAbilityNodeData* Node);

	/** Revoga a ability do nó do ASC e remove o handle. */
	void RevokeNodeAbility(UAbilitySystemComponent* ASC, FName NodeID);

	/** Retorna a quantidade de SkillPoints disponíveis do personagem. */
	float GetAvailableSkillPoints(UAbilitySystemComponent* ASC) const;

	/**
	 * Inicializa o tamanho do array Slots com base no MaxActiveAbilitySlots da classe.
	 * Chamado no RestoreFromSaveData e na primeira configuração do personagem.
	 */
	void InitializeSlots(APlayerState* PlayerState);

	/**
	 * Callback disparado quando qualquer tag SkillTree.Node ou SkillTree.SubEffect
	 * é adicionada ou removida do ASC.
	 * Propaga o evento via OnTreeStateChanged para as widgets.
	 */
	void OnSkillTreeTagChanged(const FGameplayTag Tag, int32 NewCount);
};