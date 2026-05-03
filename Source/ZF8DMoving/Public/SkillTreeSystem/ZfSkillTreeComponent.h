// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "SkillTreeSystem/ZfSkillTreeTypes.h"
#include "SkillTreeSystem/ZfSkillTreeNodeData.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "ZfSkillTreeComponent.generated.h"

class AZfSkillAimIndicator;

class UZfSkillTreeData;
class UAbilitySystemComponent;
class APlayerState;
class UZfEquipmentComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTreeStateChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillCooldownChanged, FGameplayTag, CooldownTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillBuffChanged, FGameplayTag, BuffTag);

// =============================================================================
// FSkillSlotLoadout
// =============================================================================

/**
 * Estado replicado dos slots ativos de habilidade do personagem.
 *
 * Armazena NodeIDs em vez de FGameplayAbilitySpecHandle porque:
 *   - Handles são internos ao ASC e sem significado para clientes remotos
 *   - NodeID é suficiente para a UI consultar o Data Asset e renderizar o slot
 *   - TArray<FName> replica nativamente sem problemas
 *
 * Tamanho de Slots é definido por MaxActiveSkillSlots da classe do personagem.
 * Slots vazios são representados por NAME_None.
 *
 * BasicAttack e WeaponSpecial são gerenciados pelo UZfEquipmentComponent.
 */
USTRUCT(BlueprintType)
struct FSkillSlotLoadout
{
	GENERATED_BODY()

	/**
	 * NodeIDs das abilities equipadas nos slots ativos.
	 * Tamanho = UZfPrimaryDataAssetClass::MaxActiveSkillSlots.
	 * NAME_None = slot vazio.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "SkillTree|Loadout")
	TArray<FName> Slots;

	/**
	 * NodeIDs das abilities equipadas nos slots de arma.
	 * Índice 0 = botão primário (esquerdo), Índice 1 = botão secundário (direito).
	 * Gerenciado pelo UZfEquipmentComponent ao equipar/desequipar armas.
	 * NAME_None = slot de arma vazio (usa skill padrão da classe).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "SkillTree|Loadout")
	TArray<FName> WeaponSlots;
};

// =============================================================================
// UZfSkillTreeComponent
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
 *   O componente replica apenas o FSkillSlotLoadout (slots ativos visíveis a todos).
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
class ZF8DMOVING_API UZfSkillTreeComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UZfSkillTreeComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ── Configuração ──────────────────────────────────────────────────────

	/**
	 * Número máximo de slots de Skill possíveis no jogo.
	 * Define o tamanho fixo visual da widget de slots — sempre exibe
	 * este número de slots, independente da classe do personagem.
	 * Slots além do MaxActiveSkillSlots da classe aparecem como "bloqueados".
	 *
	 * Deve corresponder à classe com mais slots do jogo (ex: Arcmage = 8).
	 */
	static constexpr int32 MaxPossibleSkillSlots = 8;

	/** Retorna o número máximo de slots possíveis — usado pela widget para
	 * construir o layout fixo de slots independente da classe. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree")
	static int32 GetMaxPossibleSkillSlots() { return MaxPossibleSkillSlots; }

	/**
	 * Data Asset único da skill tree universal.
	 * Configure no CDO do PlayerState apontando para DA_SkillTree.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree")
	TObjectPtr<UZfSkillTreeData> SkillTreeData;

	/** Retorna o Data Asset da skill tree. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree")
	UZfSkillTreeData* GetSkillTreeData() const { return SkillTreeData; }

	/**
	 * Retorna o handle da ability concedida para o NodeID informado.
	 * Retorna nullptr se o NodeID não estiver em GrantedAbilityHandles.
	 * Usado pelo Server_ConfirmSkillCast para validar o rank atual da skill.
	 */
	const FGameplayAbilitySpecHandle* GetGrantedAbilityHandle(const FName& NodeID) const
	{
		return GrantedAbilityHandles.Find(NodeID);
	}

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
	 *   Servidor → antes de conceder a Skill (UnlockNode)
	 *   Cliente  → pela widget para derivar ESkillNodeState de cada nó
	 *
	 * @param PlayerState  PlayerState do personagem a verificar
	 * @param Node         Nó a ser verificado
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	static bool CanUnlockNode(APlayerState* PlayerState, const UZfSkillTreeNodeData* Node);

	/**
	 * Verifica se o personagem pode evoluir um nó já desbloqueado.
	 * Checa: nó já desbloqueado + rank atual < MaxSkillRank + SkillPoints disponíveis.
	 *
	 * @param PlayerState  PlayerState do personagem a verificar
	 * @param Node         Nó a ser verificado
	 * @param CurrentRank  Rank atual do nó
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	static bool CanUpgradeNode(APlayerState* PlayerState, const UZfSkillTreeNodeData* Node, int32 CurrentRank);

	/**
	 * Verifica se o personagem pode desbloquear um sub-efeito específico.
	 * Checa: nó pai desbloqueado + sub-efeito não desbloqueado ainda
	 *        + limite MaxUnlockableSubEffects não atingido + SkillPoints disponíveis.
	 *
	 * @param PlayerState      PlayerState do personagem
	 * @param Node             Nó pai do sub-efeito
	 * @param SubEffectIndex   Índice em FSkillTreeNode::SubEffects
	 * @param UnlockedIndices  Sub-efeitos já desbloqueados neste nó
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	static bool CanUnlockSubEffect(APlayerState* PlayerState, const UZfSkillTreeNodeData* Node,
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
	static ESkillNodeState DeriveNodeState(APlayerState* PlayerState,
		const UZfSkillTreeNodeData* Node, int32 CurrentRank);

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
	 * @param SubEffectIndex Índice em FSkillTreeNode::SubEffects
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
	 * Equipa uma Skill desbloqueada num slot ativo.
	 * Substitui o slot sem revogar a ability do ASC — ability continua concedida.
	 *
	 * @param ASC       ASC do personagem
	 * @param NodeID    Nó a equipar (deve estar desbloqueado)
	 * @param SlotIndex Índice do slot (0-based, < MaxActiveSkillSlots)
	 */
	bool EquipSkillInSlot(UAbilitySystemComponent* ASC, FName NodeID, int32 SlotIndex);

	/**
	 * Remove a Skill de um slot ativo.
	 * A ability continua concedida no ASC — apenas sai do slot visível.
	 *
	 * @param SlotIndex Índice do slot a esvaziar
	 */
	bool UnequipSkillFromSlot(int32 SlotIndex);

	// ── Weapon Slots ──────────────────────────────────────────────────────

	/**
	 * Equipa a skill de uma arma num slot de arma.
	 * Chamado pelo UZfEquipmentComponent ao equipar uma arma.
	 * Slot 0 = botão primário, Slot 1 = botão secundário.
	 *
	 * @param ASC       ASC do personagem
	 * @param NodeID    Nó da skill da arma
	 * @param SlotIndex 0 = primário, 1 = secundário
	 */
	bool EquipWeaponSkill(UAbilitySystemComponent* ASC, FName NodeID, int32 SlotIndex);

	/**
	 * Remove a skill de arma de um slot de arma.
	 * Chamado pelo UZfEquipmentComponent ao desequipar uma arma.
	 *
	 * @param SlotIndex 0 = primário, 1 = secundário
	 */
	bool UnequipWeaponSkill(int32 SlotIndex);

	/**
	 * Registra handles de abilities de arma concedidas diretamente pelo
	 * UZfFragment_WeaponSkill — sem NodeData na skill tree.
	 *
	 * Armas têm suas abilities concedidas pelo Fragment no equip, não pela
	 * skill tree. Este método armazena os handles para que UnequipWeaponSkill
	 * possa revogar corretamente, e atualiza o Loadout.WeaponSlots para
	 * que o BP_PlayerController saiba quais handles usar ao disparar.
	 *
	 * Deve ser chamado apenas no servidor após GiveAbility.
	 *
	 * @param WeaponNodeID    ID sintético gerado pelo Fragment (GUID-based)
	 * @param SlotIndex       0 = primário, 1 = secundário
	 * @param PrimaryHandle   Handle da ability primária (pode ser inválido)
	 * @param SecondaryHandle Handle da ability secundária (pode ser inválido) — 
	 *                        use NAME_None no parâmetro NodeID da segunda chamada
	 *                        para indicar que é um complemento do mesmo slot
	 */
	/**
	 * Recalcula WeaponSlots[0] e WeaponSlots[1] com base nas armas
	 * atualmente equipadas no EquipmentComponent.
	 *
	 * Chamado pelo UZfFragment_WeaponSkill sempre que uma arma é
	 * equipada ou desequipada — nunca chamado manualmente.
	 *
	 * Regras de distribuição:
	 *   WeaponSlots[0] → Primary da arma no MainHand
	 *                    Primary da arma no OffHand se MainHand não tiver
	 *   WeaponSlots[1] → Secondary da arma no OffHand (prioridade)
	 *                    Secondary da arma no MainHand se OffHand não tiver
	 *   Conflito no slot 1 (ambas têm Secondary) → OffHand vence,
	 *     NodeID do MainHand Secondary fica em WeaponSlotConflict
	 *     para o popup da skill tree exibir as opções ao jogador.
	 *
	 * @param EquipmentComponent  Componente com as armas equipadas atuais
	 */
	void RecalculateWeaponSlots(UZfEquipmentComponent* EquipmentComponent);

	/**
	 * Retorna o NodeID da ability em conflito no slot 1.
	 * NAME_None se não houver conflito.
	 * Consultado pela widget da skill tree para exibir o popup de escolha.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree")
	FName GetWeaponSlotConflict() const { return WeaponSlotConflict; }

	/**
	 * Chamado pela widget quando o jogador resolve o conflito escolhendo
	 * qual ability quer no WeaponSlots[1].
	 * @param NodeID  NodeID da ability escolhida para o slot 1
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	void ResolveWeaponSlotConflict(FName NodeID);

	/** Retorna o loadout de weapon slots. */
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	const TArray<FName>& GetWeaponLoadout() const { return Loadout.WeaponSlots; }

	// ── Ativação de abilities — chamados pelo BP_PlayerController ─────────

	/**
	 * Tenta ativar a ability equipada no slot de skill informado.
	 *
	 * Resolve internamente: SlotIndex → NodeID → Handle → TryActivateAbility.
	 * O BP_PlayerController chama este método diretamente ao receber
	 * IA_SkillSlot_1..8 sem precisar gerenciar handles.
	 *
	 * Retorna false se:
	 *   - SlotIndex inválido ou slot vazio (NAME_None)
	 *   - Ability não concedida ao ASC (não está em GrantedAbilityHandles)
	 *   - TryActivateAbility falhou (cooldown, custo, etc.)
	 *
	 * @param SlotIndex  Índice do slot (0-7, corresponde a IA_SkillSlot_1..8)
	 * @param ASC        ASC do personagem
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	bool TryActivateSkillInSlot(int32 SlotIndex, UAbilitySystemComponent* ASC) const;

	/**
	 * Tenta ativar a ability equipada no slot de arma informado.
	 *
	 * Resolve internamente: SlotIndex → NodeID → Handle → TryActivateAbility.
	 * O BP_PlayerController chama este método ao receber
	 * IA_WeaponPrimary (SlotIndex=0) ou IA_WeaponSecondary (SlotIndex=1),
	 * mas apenas quando a tag SkillTree.AimMode.Active NÃO está ativa —
	 * quando está ativa, o click primário confirma o aim da skill.
	 *
	 * Retorna false se:
	 *   - SlotIndex inválido ou slot vazio (NAME_None)
	 *   - Ability não concedida ao ASC
	 *   - TryActivateAbility falhou
	 *
	 * @param SlotIndex  0 = botão primário, 1 = botão secundário
	 * @param ASC        ASC do personagem
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	bool TryActivateWeaponSlot(int32 SlotIndex, UAbilitySystemComponent* ASC) const;

	// ── AimMode — ponte entre GA e BP_PlayerController ────────────────────

	/**
	 * Registra o indicador ativo e o NodeID da skill em AimMode.
	 * Chamado por UZfAbility_Active::EnterAimMode após spawnar o indicador.
	 * Permite que o BP_PlayerController acesse HitLocation e NodeID
	 * sem referência direta à GA.
	 */
	void SetActiveAimIndicator(AZfSkillAimIndicator* Indicator, FName InNodeID);

	/**
	 * Limpa a referência ao indicador ativo e o NodeID ao sair do AimMode.
	 * Chamado por UZfAbility_Active::ExitAimMode antes de destruir o indicador.
	 */
	void ClearActiveAimIndicator();

	/**
	 * Retorna a última posição de mira calculada pelo indicador ativo.
	 * Consultado pelo BP_PlayerController ao confirmar o cast.
	 * Retorna ZeroVector se nenhum indicador estiver ativo.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree")
	FVector GetActiveAimHitLocation() const;

	/**
	 * Retorna a última normal de superfície calculada pelo indicador ativo.
	 * Consultado pelo BP_PlayerController ao confirmar o cast.
	 * Retorna UpVector se nenhum indicador estiver ativo.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree")
	FVector GetActiveAimHitNormal() const;

	/**
	 * Retorna o NodeID da skill atualmente em AimMode.
	 * Consultado pelo BP_PlayerController para passar ao Server_ConfirmSkillCast.
	 * Retorna NAME_None se nenhum AimMode estiver ativo.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree")
	FName GetActiveAimNodeID() const { return ActiveAimNodeID; }

	/**
	 * Retorna true se há um indicador de mira ativo no momento.
	 * Consultado pelo BP_PlayerController para confirmar que o AimMode
	 * está realmente ativo antes de chamar Server_ConfirmSkillCast.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SkillTree")
	bool HasActiveAimIndicator() const { return ActiveAimIndicator.Get() != nullptr; }

	/**
	 * Tenta fechar o AimMode ativo.
	 * Chamado pelo BP_PlayerController ao pressionar ESC.
	 *
	 * Se havia AimMode ativo:
	 *   1. Cancela a GA que está em AimMode via CancelAbilityHandle
	 *      → GA recebe EndAbility(bWasCancelled=true)
	 *      → Blueprint filho chama ExitAimMode que limpa tag e indicador
	 *   2. Retorna true — ESC foi consumido pelo AimMode
	 *
	 * Se não havia AimMode ativo:
	 *   → Retorna false — ESC segue fluxo normal (abre menu)
	 *
	 * @param ASC  ASC do personagem
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	bool TryCancelAimMode(UAbilitySystemComponent* ASC);

	/**
	 * RPC: cliente confirma o cast de uma skill após o AimMode.
	 *
	 * Chamado pelo BP_PlayerController quando o jogador confirma o
	 * targeting (WeaponPrimary com tag AimMode.Active ativa).
	 *
	 * No servidor:
	 *   1. Valida distância — rejeita se HitLocation além do MaxRange
	 *      do NodeData para o rank atual (margem de 10% para latência)
	 *   2. Monta FGameplayAbilityTargetDataHandle com o HitResult
	 *   3. Dispara SendGameplayEventToActor com tag AimMode.Confirm
	 *      A GA recebe via WaitGameplayEvent e executa o efeito
	 *
	 * @param NodeID         NodeID da skill sendo confirmada
	 * @param HitLocation    Posição no mundo do ponto de mira (cliente)
	 * @param HitNormal      Normal da superfície no ponto de impacto
	 * @param CastDirection  Direção forward da câmera no momento da confirmação
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "SkillTree")
	void Server_ConfirmSkillCast(
		const FName& NodeID,
		FVector_NetQuantize HitLocation,
		FVector_NetQuantize10 HitNormal,
		FVector_NetQuantize10 CastDirection);

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
	const FSkillSlotLoadout& GetLoadout() const { return Loadout; }

	/**
	 * Disparado no cliente quando qualquer estado da tree muda —
	 * nó desbloqueado, rank evoluído, sub-efeito desbloqueado ou loadout alterado.
	 *
	 * A WBP_SkillTree_Region ouve este delegate e dispara a animação
	 * e recriação do tooltip no nó afetado.
	 *
	 * Bind no Blueprint:
	 *   SkillTreeComponent → Assign On Tree State Changed → RefreshNodes()
	 */
	UPROPERTY(BlueprintAssignable, Category = "SkillTree")
	FOnTreeStateChanged OnTreeStateChanged;

	/**
	 * Disparado quando o cooldown de qualquer skill inicia ou termina.
	 * O WBP_SkillSlot faz bind aqui e filtra pela CooldownTag do seu NodeData.
	 *
	 * Bind no Blueprint:
	 *   SkillTreeComponent → Assign On Skill Cooldown Changed → filtrar por CooldownTag
	 */
	UPROPERTY(BlueprintAssignable, Category = "SkillTree")
	FOnSkillCooldownChanged OnSkillCooldownChanged;

	/**
	* Disparado quando o buff de qualquer skill inicia ou termina.
	* O WBP_SkillSlot faz bind aqui e filtra pela BuffTag do seu NodeData.
	* Inicia a animação inversa ao cooldown (vai esvaziando).
	*/
	UPROPERTY(BlueprintAssignable, Category = "SkillTree")
	FOnSkillBuffChanged OnSkillBuffChanged;
	
	/**
	 * Registra o listener de tags da skill tree no ASC.
	 * Deve ser chamado após InitSkillActorInfo no PlayerState.
	 * Sem HasAuthority — deve rodar no servidor e no owning client.
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	void InitializeTagListeners(UAbilitySystemComponent* ASC);

	/** Atualizado pelo Client RPC — sincroniza rank sem depender de AbilitySpec. */
	void SetNodeRankOnClient(FName NodeID, int32 Rank);
	
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

	/**
	 * NodeID da ability de arma em conflito no WeaponSlots[1].
	 * Preenchido por RecalculateWeaponSlots quando ambas as armas equipadas
	 * têm Secondary — a arma do OffHand vence automaticamente e a do
	 * MainHand fica aqui disponível para o jogador escolher via popup.
	 * NAME_None quando não há conflito.
	 */
	FName WeaponSlotConflict = NAME_None;

	/**
	 * Referencia ao indicador de mira ativo durante o AimMode.
	 * Preenchido por SetActiveAimIndicator (chamado pela GA em EnterAimMode).
	 * Limpo por ClearActiveAimIndicator (chamado pela GA em ExitAimMode).
	 * Existe apenas no cliente local - nao replicado.
	 */
	UPROPERTY()
	TObjectPtr<AZfSkillAimIndicator> ActiveAimIndicator;

	/** NodeID da skill atualmente em AimMode. NAME_None quando inativo. */
	FName ActiveAimNodeID = NAME_None;

	// ── Loadout replicado ─────────────────────────────────────────────────

	/**
	 * Slots ativos do personagem — replicado para todos os clientes.
	 * Clientes remotos usam os NodeIDs para exibir as abilities equipadas.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_Loadout)
	FSkillSlotLoadout Loadout;

	UFUNCTION()
	void OnRep_Loadout();

	// ── Helpers privados ──────────────────────────────────────────────────

	/** Consome SkillPoints via GE. Retorna false se insuficientes. */
	bool SpendSkillPoints(UAbilitySystemComponent* ASC, int32 Amount);

	/** Devolve SkillPoints via GE (respec). */
	void RefundSkillPoints(UAbilitySystemComponent* ASC, int32 Amount);

	/** Concede a ability do nó ao ASC e armazena o handle. */
	void GrantNodeAbility(UAbilitySystemComponent* ASC, const UZfSkillTreeNodeData* Node);

	/** Revoga a ability do nó do ASC e remove o handle. */
	void RevokeNodeAbility(UAbilitySystemComponent* ASC, FName NodeID);

	/** Retorna a quantidade de SkillPoints disponíveis do personagem. */
	float GetAvailableSkillPoints(UAbilitySystemComponent* ASC) const;

	/**
	 * Inicializa o tamanho do array Slots com base no MaxActiveSkillSlots da classe.
	 * Chamado no RestoreFromSaveData e na primeira configuração do personagem.
	 */
	void InitializeSlots(APlayerState* PlayerState);

	/**
	 * Callback disparado quando qualquer tag SkillTree.Node ou SkillTree.SubEffect
	 * é adicionada ou removida do ASC.
	 * Propaga o evento via OnTreeStateChanged para as widgets.
	 */
	void OnSkillTreeTagChanged(const FGameplayTag Tag, int32 NewCount);

	/**
	 * Callback disparado quando um atributo usado como requisito de nó muda.
	 * Propaga via OnTreeStateChanged para as widgets recalcularem o estado
	 * dos nós — pode ter ficado Disabled ou voltado ao estado normal.
	 * Registrado em InitializeTagListeners para cada atributo relevante.
	 */
	void OnRequiredAttributeChanged(const FOnAttributeChangeData& Data);

	/**
	 * Callback disparado quando a CooldownTag de qualquer nó é adicionada
	 * ou removida do ASC — indica início ou fim de cooldown.
	 * Propaga via OnSkillCooldownChanged para o WBP_SkillSlot correspondente.
	 */
	void OnCooldownTagChanged(const FGameplayTag Tag, int32 NewCount);
	
	/**
	* Callback disparado quando a BuffTag de qualquer nó é adicionada
	* ou removida do ASC — indica início ou fim de buff.
	 * Propaga via OnSkillBuffChanged para o WBP_SkillSlot correspondente.
	 */
	void OnBuffTagChanged(const FGameplayTag Tag, int32 NewCount);

};