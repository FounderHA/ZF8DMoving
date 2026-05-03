// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillTreeSystem/ZfSkillTreeComponent.h"
#include "SkillTreeSystem/ZfSkillTreeNodeData.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Net/UnrealNetwork.h"
#include "SkillTreeSystem/ZfSkillTreeData.h"
#include "AbilitySystem/Attributes/ZfProgressionAttributeSet.h"
#include "Player/ZfPlayerState.h"
#include "Player/Class/ZfClassBaseSettings.h"
#include "Tags/ZfGameplayTags.h"
#include "Inventory/ZfEquipmentComponent.h"
#include "Inventory/ZfItemInstance.h"
#include "Inventory/Fragments/ZfFragment_WeaponSkill.h"
#include "Tags/ZfEquipmentTags.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "SkillTreeSystem/ZfSkillAimIndicator.h"
#include "AbilitySystem/GameplayAbility/SkillTreeSystem/ZfAbility_Active.h"

// =============================================================================
// Construtor e replicação
// =============================================================================

UZfSkillTreeComponent::UZfSkillTreeComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UZfSkillTreeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Apenas o loadout replica — é o único dado que clientes remotos precisam
	// para exibir as abilities equipadas do personagem.
	// O estado completo da tree (nós, ranks, sub-efeitos) é derivável pelas
	// GameplayTags do ASC, que já replicam via Mixed mode do PlayerState.
	DOREPLIFETIME(UZfSkillTreeComponent, Loadout);
}

// =============================================================================
// Validação — acessível no servidor e no cliente
// =============================================================================

bool UZfSkillTreeComponent::CanUnlockNode(APlayerState* PlayerState, const UZfSkillTreeNodeData* Node)
{
	if (!PlayerState || !Node) return false;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PlayerState);
	if (!ASI) return false;

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC) return false;

	// Nó já desbloqueado — usa NodeRanks como fonte de verdade.
	// Funciona imediatamente no cliente após SetNodeRankOnClient,
	// sem depender de replicação assíncrona de tag.
	const AZfPlayerState* ZfPS = Cast<AZfPlayerState>(PlayerState);
	if (ZfPS)
	{
		const UZfSkillTreeComponent* TreeComp = ZfPS->GetSkillTreeComponent();
		if (TreeComp && TreeComp->GetNodeRank(Node->NodeID) > 0)
		{
			return false;
		}
	}

	// Verifica RequiredTags — todas precisam estar presentes (AND)
	// Tags de requisito de outros nós ainda usam o ASC — correto,
	// pois replicam via MinimalReplicationTags antes do clique chegar.
	FGameplayTagContainer TagsToCheck;
	for (const FTagRequirement& Req : Node->RequiredTags)
	{
		TagsToCheck.AddTag(Req.RequiredTag);
	}
	if (TagsToCheck.IsValid() && !ASC->HasAllMatchingGameplayTags(TagsToCheck))
	{
		return false;
	}

	// Verifica AttributeRequirements — cada entrada verificada individualmente
	for (const FAttributeRequirement& Req : Node->AttributeRequirements)
	{
		if (!Req.Attribute.IsValid()) continue;

		const float CurrentValue = ASC->GetNumericAttribute(Req.Attribute);
		if (CurrentValue < Req.MinValue)
		{
			return false;
		}
	}

	// Verifica SkillPoints disponíveis
	const UZfProgressionAttributeSet* ProgSet = ASC->GetSet<UZfProgressionAttributeSet>();
	if (!ProgSet) return false;

	if (ProgSet->GetSkillPoints() < static_cast<float>(Node->UnlockCost))
	{
		return false;
	}

	return true;
}

bool UZfSkillTreeComponent::CanUpgradeNode(APlayerState* PlayerState, const UZfSkillTreeNodeData* Node, int32 CurrentRank)
{
	if (!PlayerState || !Node) return false;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PlayerState);
	if (!ASI) return false;

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC) return false;

	// Nó precisa estar desbloqueado — usa NodeRanks como fonte de verdade.
	// CurrentRank vem de GetNodeRankFromASC que já usa NodeRanks.
	if (CurrentRank <= 0)
	{
		return false;
	}

	// Já está no rank máximo
	if (CurrentRank >= Node->MaxSkillRank)
	{
		return false;
	}

	const int32 Cost = Node->GetRankUpCost(CurrentRank);

	const UZfProgressionAttributeSet* ProgSet = ASC->GetSet<UZfProgressionAttributeSet>();
	if (!ProgSet) return false;

	return ProgSet->GetSkillPoints() >= static_cast<float>(Cost);
}

bool UZfSkillTreeComponent::CanUnlockSubEffect(APlayerState* PlayerState, const UZfSkillTreeNodeData* Node,
	int32 SubEffectIndex, const TArray<int32>& UnlockedIndices)
{
	if (!PlayerState || !Node) return false;
	if (!Node->SubEffects.IsValidIndex(SubEffectIndex)) return false;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PlayerState);
	if (!ASI) return false;

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC) return false;

	// Nó pai precisa estar desbloqueado — usa NodeRanks como fonte de verdade.
	const AZfPlayerState* ZfPS = Cast<AZfPlayerState>(PlayerState);
	if (!ZfPS) return false;

	const UZfSkillTreeComponent* TreeComp = ZfPS->GetSkillTreeComponent();
	if (!TreeComp || TreeComp->GetNodeRank(Node->NodeID) <= 0)
	{
		return false;
	}

	// Sub-efeito já desbloqueado
	if (UnlockedIndices.Contains(SubEffectIndex))
	{
		return false;
	}

	// Limite de sub-efeitos atingido (-1 = sem limite)
	if (Node->MaxUnlockableSubEffects >= 0 && UnlockedIndices.Num() >= Node->MaxUnlockableSubEffects)
	{
		return false;
	}

	const FSubEffectData& SubEffect = Node->SubEffects[SubEffectIndex];

	// Verifica exclusividade — bloqueado se o personagem tiver qualquer tag exclusiva (OR)
	if (SubEffect.ExclusiveTags.IsValid())
	{
		FGameplayTagContainer PlayerTags;
		ASC->GetOwnedGameplayTags(PlayerTags);

		if (PlayerTags.HasAny(SubEffect.ExclusiveTags))
		{
			return false;
		}
	}

	// Verifica SkillPoints
	const UZfProgressionAttributeSet* ProgSet = ASC->GetSet<UZfProgressionAttributeSet>();
	if (!ProgSet) return false;

	return ProgSet->GetSkillPoints() >= static_cast<float>(SubEffect.UnlockCost);
}

ESkillNodeState UZfSkillTreeComponent::DeriveNodeState(APlayerState* PlayerState,
	const UZfSkillTreeNodeData* Node, int32 CurrentRank)
{
	if (!PlayerState || !Node) return ESkillNodeState::Locked;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PlayerState);
	if (!ASI) return ESkillNodeState::Locked;

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC) return ESkillNodeState::Locked;

	// Nó desbloqueado quando NodeRanks >= 1.
	// NodeRanks é fonte de verdade — funciona imediatamente no cliente
	// após SetNodeRankOnClient, sem depender de replicação assíncrona de tag.
	const bool bIsUnlocked = (CurrentRank > 0);

	// ── Nó já desbloqueado ────────────────────────────────────────────────
	if (bIsUnlocked)
	{
		// Verifica se AttributeRequirements ainda são atendidos.
		// Atributos são flexíveis — podem cair abaixo do mínimo após
		// desequipar itens. Nesse caso o nó fica Disabled mas permanece
		// desbloqueado e no slot.
		for (const FAttributeRequirement& Req : Node->AttributeRequirements)
		{
			if (!Req.Attribute.IsValid()) continue;

			const float CurrentValue = ASC->GetNumericAttribute(Req.Attribute);
			if (CurrentValue < Req.MinValue)
			{
				return ESkillNodeState::Disabled;
			}
		}

		if (CurrentRank >= Node->MaxSkillRank)
		{
			return ESkillNodeState::Maxed;
		}

		if (CanUpgradeNode(PlayerState, Node, CurrentRank))
		{
			return ESkillNodeState::AffordableUpgrade;
		}

		return ESkillNodeState::Unlocked;
	}

	// ── Verifica Excluded antes de Locked ─────────────────────────────────
	// Um nó é Excluded quando exige uma tag de classe específica E o personagem
	// já escolheu uma classe diferente — tornando o requisito permanentemente
	// inacessível.
	if (Node->RequiredTags.Num() > 0)
	{
		FGameplayTagContainer PlayerTags;
		ASC->GetOwnedGameplayTags(PlayerTags);

		const FGameplayTag ClassParentTag = FGameplayTag::RequestGameplayTag(FName("SkillTree.Class"));

		FGameplayTagContainer PlayerClassTags = PlayerTags.Filter(FGameplayTagContainer(ClassParentTag));
		PlayerClassTags.RemoveTag(ZfAbilityTreeTags::SkillTree_Class_Novice);

		if (PlayerClassTags.Num() > 0)
		{
			for (const FTagRequirement& Req : Node->RequiredTags)
			{
				const FGameplayTag& RequiredTag = Req.RequiredTag;

				if (RequiredTag.MatchesTag(ClassParentTag) &&
					!RequiredTag.MatchesTagExact(ZfAbilityTreeTags::SkillTree_Class_Novice) &&
					!PlayerTags.HasTagExact(RequiredTag))
				{
					return ESkillNodeState::Excluded;
				}
			}
		}
	}

	// ── Disponível ou bloqueado ───────────────────────────────────────────
	if (CanUnlockNode(PlayerState, Node))
	{
		return ESkillNodeState::Available;
	}

	return ESkillNodeState::Locked;
}

// =============================================================================
// Operações de escrita — servidor only
// =============================================================================

bool UZfSkillTreeComponent::UnlockNode(UAbilitySystemComponent* ASC, FName NodeID)
{
	if (!ASC || NodeID.IsNone() || !SkillTreeData) return false;

	if (!GetOwner()->HasAuthority())
	{
		return false;
	}

	UZfSkillTreeNodeData* Node = SkillTreeData->FindNode(NodeID);
	if (!Node)
	{
		return false;
	}

	APlayerState* PS = Cast<APlayerState>(GetOwner());
	if (!CanUnlockNode(PS, Node))
	{
		return false;
	}

	if (!SpendSkillPoints(ASC, Node->UnlockCost)) return false;

	// Ability NÃO é concedida aqui — apenas quando equipada no slot.
	// Isso garante que o GE passivo só é aplicado quando a skill está ativa.

	// Adiciona GrantedTag ao personagem
	if (Node->GrantedTag.IsValid())
	{
		UAbilitySystemBlueprintLibrary::AddLooseGameplayTags(
			ASC->GetAvatarActor(),
			FGameplayTagContainer(Node->GrantedTag),
			true); // bShouldReplicate = true
	}

	// Atualiza estado runtime
	UnlockedNodes.Add(NodeID);
	NodeRanks.Add(NodeID, 1);

	return true;
}

bool UZfSkillTreeComponent::UpgradeNode(UAbilitySystemComponent* ASC, FName NodeID)
{
	if (!ASC || NodeID.IsNone() || !SkillTreeData) return false;

	if (!GetOwner()->HasAuthority())
	{
		return false;
	}

	UZfSkillTreeNodeData* Node = SkillTreeData->FindNode(NodeID);
	if (!Node) return false;

	const int32 CurrentRank = GetNodeRank(NodeID);

	APlayerState* PS = Cast<APlayerState>(GetOwner());
	if (!CanUpgradeNode(PS, Node, CurrentRank)) return false;

	const int32 Cost = Node->GetRankUpCost(CurrentRank);
	if (!SpendSkillPoints(ASC, Cost)) return false;

	// Incrementa Level no FGameplayAbilitySpec — apenas se a skill estiver equipada.
	// Se não estiver equipada, o rank é atualizado em NodeRanks e quando
	// for equipada o GrantNodeAbility passa o rank correto via Spec.Level.
	if (GrantedAbilityHandles.Contains(NodeID))
	{
		if (FGameplayAbilitySpecHandle* Handle = GrantedAbilityHandles.Find(NodeID))
		{
			FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(*Handle);
			if (Spec)
			{
				Spec->Level += 1;
				ASC->MarkAbilitySpecDirty(*Spec);
			}
		}
	}

	NodeRanks[NodeID] = CurrentRank + 1;

	return true;
}

bool UZfSkillTreeComponent::UnlockSubEffect(UAbilitySystemComponent* ASC, FName NodeID, int32 SubEffectIndex)
{
	if (!ASC || NodeID.IsNone() || !SkillTreeData) return false;

	if (!GetOwner()->HasAuthority())
	{
		return false;
	}

	UZfSkillTreeNodeData* Node = SkillTreeData->FindNode(NodeID);
	if (!Node) return false;

	const TArray<int32> CurrentUnlocked = GetUnlockedSubEffects(NodeID);

	APlayerState* PS = Cast<APlayerState>(GetOwner());
	if (!CanUnlockSubEffect(PS, Node, SubEffectIndex, CurrentUnlocked)) return false;

	const FSubEffectData& SubEffect = Node->SubEffects[SubEffectIndex];
	if (!SpendSkillPoints(ASC, SubEffect.UnlockCost)) return false;

	// Adiciona tag do sub-efeito ao personagem
	if (SubEffect.GrantedTag.IsValid())
	{
		UAbilitySystemBlueprintLibrary::AddLooseGameplayTags(
			ASC->GetAvatarActor(),
			FGameplayTagContainer(SubEffect.GrantedTag),
			true); // bShouldReplicate = true
	}

	// Atualiza estado runtime
	UnlockedSubEffects.FindOrAdd(NodeID).Indices.AddUnique(SubEffectIndex);

	return true;
}

bool UZfSkillTreeComponent::RespecTree(UAbilitySystemComponent* ASC)
{
	if (!ASC || !SkillTreeData) return false;

	if (!GetOwner()->HasAuthority())
	{
		return false;
	}

	int32 TotalSPToRefund = 0;

	for (const FName& NodeID : UnlockedNodes)
	{
		UZfSkillTreeNodeData* Node = nullptr;
		Node = SkillTreeData->FindNode(NodeID);
	if (!Node) continue;

		// Calcula SP a devolver pelo desbloqueio
		TotalSPToRefund += Node->UnlockCost;

		// Calcula SP a devolver pelas evoluções.
		// GetRankUpCost(R) = custo para ir do Rank R para R+1.
		// Um nó com CurrentRank=3 teve evoluções: 1→2 e 2→3.
		// Loop de Rank=1 a CurrentRank-1 cobre exatamente essas evoluções.
		const int32 CurrentRank = GetNodeRank(NodeID);
		for (int32 Rank = 1; Rank < CurrentRank; ++Rank)
		{
			TotalSPToRefund += Node->GetRankUpCost(Rank);
		}

		// Calcula SP a devolver pelos sub-efeitos
		if (const FSubEffectIndexList* SubEffectList = UnlockedSubEffects.Find(NodeID))
		{
			for (const int32 Index : SubEffectList->Indices)
			{
				if (Node->SubEffects.IsValidIndex(Index))
				{
					TotalSPToRefund += Node->SubEffects[Index].UnlockCost;
				}
			}

			// Remove tags dos sub-efeitos do personagem
			for (const int32 Index : SubEffectList->Indices)
			{
				if (Node->SubEffects.IsValidIndex(Index) && Node->SubEffects[Index].GrantedTag.IsValid())
				{
					UAbilitySystemBlueprintLibrary::RemoveLooseGameplayTags(
						ASC->GetAvatarActor(),
						FGameplayTagContainer(Node->SubEffects[Index].GrantedTag),
						true);
				}
			}
		}

		// Revoga ability do ASC
		RevokeNodeAbility(ASC, NodeID);

		// Remove GrantedTag do nó
		if (Node->GrantedTag.IsValid())
		{
			UAbilitySystemBlueprintLibrary::RemoveLooseGameplayTags(
				ASC->GetAvatarActor(),
				FGameplayTagContainer(Node->GrantedTag),
				true);
		}
	}

	// Limpa estado runtime
	UnlockedNodes.Empty();
	NodeRanks.Empty();
	UnlockedSubEffects.Empty();
	GrantedAbilityHandles.Empty();

	// Esvazia slots do loadout sem alterar o tamanho do array
	for (FName& Slot : Loadout.Slots)
	{
		Slot = NAME_None;
	}

	// Devolve os SkillPoints gastos
	if (TotalSPToRefund > 0)
	{
		RefundSkillPoints(ASC, TotalSPToRefund);
	}

	return true;
}

// =============================================================================
// Loadout de slots
// =============================================================================

bool UZfSkillTreeComponent::EquipSkillInSlot(UAbilitySystemComponent* ASC, FName NodeID, int32 SlotIndex)
{
	if (!ASC || NodeID.IsNone()) return false;
	if (!GetOwner()->HasAuthority()) return false;

	if (!Loadout.Slots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	if (!UnlockedNodes.Contains(NodeID))
	{
		return false;
	}

	Loadout.Slots[SlotIndex] = NodeID;

	// Se a skill já estava equipada em outro slot, limpar o slot antigo.
	// Não chama RevokeNodeAbility pois a ability continua concedida no novo slot.
	for (int32 i = 0; i < Loadout.Slots.Num(); i++)
	{
		if (i != SlotIndex && Loadout.Slots[i] == NodeID)
		{
			Loadout.Slots[i] = NAME_None;
			break;
		}
	}

	// Concede a ability ao ASC agora que está equipada.
	// Se já estava concedida (estava em outro slot), GrantNodeAbility
	// retorna sem fazer nada pelo guard de dupla concessão.
	UZfSkillTreeNodeData* Node = SkillTreeData ? SkillTreeData->FindNode(NodeID) : nullptr;

	if (Node)
	{
		GrantNodeAbility(ASC, Node);
	}

	return true;
}

bool UZfSkillTreeComponent::UnequipSkillFromSlot(int32 SlotIndex)
{
	if (!GetOwner()->HasAuthority()) return false;
	if (!Loadout.Slots.IsValidIndex(SlotIndex)) return false;

	const FName NodeID = Loadout.Slots[SlotIndex];
	if (NodeID.IsNone()) return false;

	// Revoga a ability do ASC — passiva tem GE removido automaticamente.
	// Busca o ASC pelo PlayerState dono do componente.
	APlayerState* PS = Cast<APlayerState>(GetOwner());
	if (PS)
	{
		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PS);
		if (ASI)
		{
			UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
			if (ASC)
			{
				RevokeNodeAbility(ASC, NodeID);
			}
		}
	}

	Loadout.Slots[SlotIndex] = NAME_None;
	return true;
}

// =============================================================================
// Save e Restore
// =============================================================================

void UZfSkillTreeComponent::RestoreFromSaveData(UAbilitySystemComponent* ASC, const FCharacterTreeSaveData& SaveData)
{
	if (!ASC || !SkillTreeData) return;

	if (!GetOwner()->HasAuthority()) return;

	// Inicializa slots com base na classe do personagem
	APlayerState* PS = Cast<APlayerState>(GetOwner());
	InitializeSlots(PS);

	// ── Passo 1: restaura estado de todos os nós desbloqueados ───────────
	// Registra nós, ranks, tags e sub-efeitos — SEM conceder abilities.
	// A ability só é concedida no Passo 2, apenas para nós no loadout.
	// Isso garante que passivas não equipadas não ganhem seus GEs Infinite.

	for (const FName& NodeID : SaveData.UnlockedNodes)
	{
		UZfSkillTreeNodeData* Node = SkillTreeData->FindNode(NodeID);
		if (!Node)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("UZfSkillTreeComponent::RestoreFromSaveData: NodeID '%s' não encontrado no SkillTreeData. "
					 "Node removido do jogo? Ignorando."), *NodeID.ToString());
			continue;
		}

		// Restaura rank — fallback para 1 (estado mínimo de um nó desbloqueado)
		const int32* SavedRank = SaveData.NodeRanks.Find(NodeID);
		const int32 Rank = (SavedRank && *SavedRank >= 1) ? *SavedRank : 1;
		NodeRanks.Add(NodeID, Rank);
		UnlockedNodes.Add(NodeID);

		// Restaura GrantedTag do nó — tag de progressão, independente de equip
		if (Node->GrantedTag.IsValid())
		{
			ASC->AddLooseGameplayTag(Node->GrantedTag);
		}

		// Restaura tags dos sub-efeitos desbloqueados
		if (const FSubEffectIndexList* SubEffectList = SaveData.UnlockedSubEffects.Find(NodeID))
		{
			for (const int32 Index : SubEffectList->Indices)
			{
				if (!Node->SubEffects.IsValidIndex(Index)) continue;

				const FSubEffectData& SubEffect = Node->SubEffects[Index];
				if (SubEffect.GrantedTag.IsValid())
				{
					ASC->AddLooseGameplayTag(SubEffect.GrantedTag);
				}

				UnlockedSubEffects.FindOrAdd(NodeID).Indices.AddUnique(Index);
			}
		}
	}

	// Replica todas as tags adicionadas de uma vez — mais eficiente que
	// ForceReplication() por tag individual
	ASC->ForceReplication();

	// ── Passo 2: concede abilities apenas dos nós no loadout ─────────────
	// Percorre todos os slots salvos e concede a ability de cada um.
	// O save do loadout ainda não existe — este passo é preparatório para
	// quando o BuildSaveData incluir o Loadout. Por ora restaura o estado
	// de GrantedAbilityHandles para nós que já estavam equipados.
	//
	// Iteramos Loadout.Slots (recém inicializado como NAME_None por InitializeSlots).
	// O sistema de save precisará salvar e restaurar o Loadout separadamente —
	// isso é feito via Server RPCs de EquipSkillInSlot após RestoreFromSaveData.
	// A função abaixo garante que se o save chamar EquipSkillInSlot para restaurar
	// o loadout, GrantNodeAbility funcionará corretamente com os ranks já no map.

	// Concede abilities para nós nos WeaponSlots (vindos do EquipmentComponent ao equipar)
	// e slots ativos que o sistema de save restaurar via EquipSkillInSlot.
	// Não há nada para fazer aqui além de garantir que GrantedAbilityHandles está limpo
	// — GrantNodeAbility será chamado por EquipSkillInSlot e EquipWeaponSkill.
}

FCharacterTreeSaveData UZfSkillTreeComponent::BuildSaveData() const
{
	FCharacterTreeSaveData SaveData;
	SaveData.UnlockedNodes      = UnlockedNodes;
	SaveData.NodeRanks          = NodeRanks;
	SaveData.UnlockedSubEffects = UnlockedSubEffects;
	return SaveData;
}

// =============================================================================
// Consulta de estado runtime
// =============================================================================

bool UZfSkillTreeComponent::IsNodeUnlocked(FName NodeID) const
{
	return UnlockedNodes.Contains(NodeID);
}

int32 UZfSkillTreeComponent::GetNodeRank(FName NodeID) const
{
	if (const int32* Rank = NodeRanks.Find(NodeID))
	{
		return *Rank;
	}
	return 0;
}

TArray<int32> UZfSkillTreeComponent::GetUnlockedSubEffects(FName NodeID) const
{
	if (const FSubEffectIndexList* List = UnlockedSubEffects.Find(NodeID))
	{
		return List->Indices;
	}
	return TArray<int32>();
}

int32 UZfSkillTreeComponent::GetNodeRankFromASC(UAbilitySystemComponent* ASC, FName NodeID) const
{
	// NodeRanks é a fonte única de verdade para rank — servidor e cliente.
	// Servidor: atualizado em UnlockNode e UpgradeNode.
	// Cliente:  sincronizado via Client_NotifySkillUpgraded → SetNodeRankOnClient.
	// Rank 0 = não desbloqueado. Rank >= 1 = desbloqueado.
	if (const int32* Rank = NodeRanks.Find(NodeID))
	{
		return *Rank;
	}
	return 0;
}

// =============================================================================
// RepNotify
// =============================================================================

void UZfSkillTreeComponent::OnRep_Loadout()
{
	// Notifica as widgets que o loadout mudou
	OnTreeStateChanged.Broadcast();
}

// =============================================================================
// Tag listeners — notificação de mudanças da tree para as widgets
// =============================================================================

void UZfSkillTreeComponent::InitializeTagListeners(UAbilitySystemComponent* ASC)
{
	if (!ASC) return;

	// Inicializa slots no servidor se ainda não foram inicializados.
	// Cobre o caso de personagem novo (sem save) onde RestoreFromSaveData
	// nunca é chamado e Loadout.Slots fica vazio.
	if (GetOwner()->HasAuthority() && Loadout.Slots.Num() == 0)
	{
		APlayerState* PS = Cast<APlayerState>(GetOwner());
		InitializeSlots(PS);
	}

	// AnyCountChange dispara toda vez que o count muda (0→1, 1→2, etc.)
	// necessário para detectar cada desbloqueio de nó independente de quantos
	// já estão desbloqueados.
	ASC->RegisterGameplayTagEvent(
		FGameplayTag::RequestGameplayTag(FName("SkillTree.Node")),
		EGameplayTagEventType::AnyCountChange)
		.AddUObject(this, &UZfSkillTreeComponent::OnSkillTreeTagChanged);

	ASC->RegisterGameplayTagEvent(
		FGameplayTag::RequestGameplayTag(FName("SkillTree.SubEffect")),
		EGameplayTagEventType::AnyCountChange)
		.AddUObject(this, &UZfSkillTreeComponent::OnSkillTreeTagChanged);

	// Registra delegates para atributos usados como requisito nos nós da tree.
	// Quando um atributo muda, verifica se alguma skill desbloqueada ficou
	// Disabled (atributo caiu abaixo do mínimo) ou voltou a ficar disponível.
	// Funciona no servidor e no owning client — replicação via Mixed mode.
	if (!SkillTreeData) return;

	TSet<FGameplayAttribute> RequiredAttributes;
	for (UZfSkillTreeNodeData* Node : SkillTreeData->GetAllNodes())
	{
		for (const FAttributeRequirement& Req : Node->AttributeRequirements)
		{
			if (Req.Attribute.IsValid())
			{
				RequiredAttributes.Add(Req.Attribute);
			}
		}
	}

	for (const FGameplayAttribute& Attr : RequiredAttributes)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(Attr)
			.AddUObject(this, &UZfSkillTreeComponent::OnRequiredAttributeChanged);
	}

	// Registra listener para CooldownTag de cada nó da tree.
	// NewOrRemoved dispara quando cooldown inicia (tag adicionada) ou termina (tag removida).
	// O WBP_SkillSlot filtra pelo CooldownTag do seu NodeData.
	for (UZfSkillTreeNodeData* Node : SkillTreeData->GetAllNodes())
	{
		if (Node && Node->CooldownTag.IsValid())
		{
			ASC->RegisterGameplayTagEvent(
				Node->CooldownTag,
				EGameplayTagEventType::NewOrRemoved)
				.AddUObject(this, &UZfSkillTreeComponent::OnCooldownTagChanged);
		}

		// Registra listener para BuffTag
		if (Node && Node->BuffTag.IsValid())
		{
			ASC->RegisterGameplayTagEvent(
				Node->BuffTag,
				EGameplayTagEventType::NewOrRemoved)
				.AddUObject(this, &UZfSkillTreeComponent::OnBuffTagChanged);
		}
	}
}

void UZfSkillTreeComponent::OnSkillTreeTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// Propaga para as widgets independente de servidor ou cliente
	OnTreeStateChanged.Broadcast();
}

void UZfSkillTreeComponent::OnCooldownTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// Propaga para o WBP_SkillSlot que tem essa CooldownTag.
	// NewCount > 0 = cooldown iniciou, NewCount == 0 = cooldown terminou.
	OnSkillCooldownChanged.Broadcast(Tag);
}

void UZfSkillTreeComponent::OnBuffTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// Propaga para o WBP_SkillSlot que tem essa BuffTag.
	// NewCount > 0 = buff iniciou, NewCount == 0 = buff terminou.
	OnSkillBuffChanged.Broadcast(Tag);
}

void UZfSkillTreeComponent::OnRequiredAttributeChanged(const FOnAttributeChangeData& Data)
{
	// Verifica todos os slots e desequipa skills cujos
	// AttributeRequirements não são mais atendidos.
	// Apenas no servidor — desequipe replica via OnRep_Loadout.
	if (!GetOwner()->HasAuthority()) return;

	APlayerState* PS = Cast<APlayerState>(GetOwner());
	if (!PS) return;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PS);
	if (!ASI) return;

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC || !SkillTreeData) return;

	bool bChanged = false;

	for (int32 i = 0; i < Loadout.Slots.Num(); i++)
	{
		const FName NodeID = Loadout.Slots[i];
		if (NodeID.IsNone()) continue;

		UZfSkillTreeNodeData* Node = SkillTreeData->FindNode(NodeID);
		if (!Node) continue;

		for (const FAttributeRequirement& Req : Node->AttributeRequirements)
		{
			if (!Req.Attribute.IsValid()) continue;

			const float CurrentValue = ASC->GetNumericAttribute(Req.Attribute);
			if (CurrentValue < Req.MinValue)
			{
				UnequipSkillFromSlot(i);
				bChanged = true;
				break;
			}
		}
	}

	// Propaga para as widgets — UI mostra Disabled nos nós afetados
	// e atualiza os slots. OnRep_Loadout cuida do cliente via replicação.
	if (bChanged)
	{
		OnTreeStateChanged.Broadcast();
	}
}

void UZfSkillTreeComponent::SetNodeRankOnClient(FName NodeID, int32 Rank)
{
	if (GetOwner()->HasAuthority()) return;

	if (Rank <= 0)
	{
		NodeRanks.Remove(NodeID);
		UnlockedNodes.Remove(NodeID);

		// Remove a tag localmente no cliente
		if (SkillTreeData)
		{
			if (UZfSkillTreeNodeData* Node = SkillTreeData->FindNode(NodeID))
			{
				if (Node->GrantedTag.IsValid())
				{
					APlayerState* PS = Cast<APlayerState>(GetOwner());
					if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PS))
					{
						if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
						{
							ASC->RemoveLooseGameplayTag(Node->GrantedTag);
						}
					}
				}
			}
		}
	}
	else
	{
		NodeRanks.FindOrAdd(NodeID) = Rank;
		UnlockedNodes.Add(NodeID);

		// Adiciona a tag localmente no cliente para que RequiredTags
		// de outros nós seja satisfeita imediatamente sem depender
		// de replicação assíncrona do servidor.
		if (SkillTreeData)
		{
			if (UZfSkillTreeNodeData* Node = SkillTreeData->FindNode(NodeID))
			{
				if (Node->GrantedTag.IsValid())
				{
					APlayerState* PS = Cast<APlayerState>(GetOwner());
					if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PS))
					{
						if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
						{
							ASC->AddLooseGameplayTag(Node->GrantedTag);
						}
					}
				}
			}
		}
	}
}

// =============================================================================
// Helpers privados
// =============================================================================

bool UZfSkillTreeComponent::SpendSkillPoints(UAbilitySystemComponent* ASC, int32 Amount)
{
	if (!ASC || Amount <= 0) return false;

	if (!SpendSkillPointsEffectClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("UZfSkillTreeComponent: SpendSkillPointsEffectClass não configurado. "
				 "Configure no PlayerState Blueprint."));
		return false;
	}

	// Guarda verificada — CanUnlock já validou, mas defensivo aqui também
	const UZfProgressionAttributeSet* ProgSet = ASC->GetSet<UZfProgressionAttributeSet>();
	if (!ProgSet || ProgSet->GetSkillPoints() < static_cast<float>(Amount))
	{
		return false;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(SpendSkillPointsEffectClass, 1.f, Context);
	if (!Spec.IsValid()) return false;

	// Negativo = gastar pontos
	Spec.Data->SetSetByCallerMagnitude(
		ZfAbilityTreeTags::SkillTree_Data_SkillPoints,
		static_cast<float>(-Amount));

	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	return true;
}

void UZfSkillTreeComponent::RefundSkillPoints(UAbilitySystemComponent* ASC, int32 Amount)
{
	if (!ASC || Amount <= 0) return;

	if (!SpendSkillPointsEffectClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("UZfSkillTreeComponent: SpendSkillPointsEffectClass não configurado."));
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(SpendSkillPointsEffectClass, 1.f, Context);
	if (!Spec.IsValid()) return;

	// Positivo = devolver pontos
	Spec.Data->SetSetByCallerMagnitude(
		ZfAbilityTreeTags::SkillTree_Data_SkillPoints,
		static_cast<float>(Amount));

	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void UZfSkillTreeComponent::GrantNodeAbility(UAbilitySystemComponent* ASC, const UZfSkillTreeNodeData* Node)
{
	if (!ASC || !Node || !Node->SkillClass) return;

	// Guard contra dupla concessão
	if (GrantedAbilityHandles.Contains(Node->NodeID)) return;

	// Usa o rank atual do nó como Level inicial do spec.
	// GetNodeRank retorna 0 se não encontrado — fallback para 1.
	const int32 CurrentRank = FMath::Max(1, GetNodeRank(Node->NodeID));

	FGameplayAbilitySpec Spec(Node->SkillClass, CurrentRank);
	const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);

	if (Handle.IsValid())
	{
		GrantedAbilityHandles.Add(Node->NodeID, Handle);
	}
}

void UZfSkillTreeComponent::RevokeNodeAbility(UAbilitySystemComponent* ASC, FName NodeID)
{
	if (!ASC) return;

	FGameplayAbilitySpecHandle* Handle = GrantedAbilityHandles.Find(NodeID);
	if (!Handle) return;

	ASC->ClearAbility(*Handle);
	GrantedAbilityHandles.Remove(NodeID);
}

float UZfSkillTreeComponent::GetAvailableSkillPoints(UAbilitySystemComponent* ASC) const
{
	if (!ASC) return 0.f;

	const UZfProgressionAttributeSet* ProgSet = ASC->GetSet<UZfProgressionAttributeSet>();
	return ProgSet ? ProgSet->GetSkillPoints() : 0.f;
}

void UZfSkillTreeComponent::InitializeSlots(APlayerState* PlayerState)
{
	AZfPlayerState* ZfPS = Cast<AZfPlayerState>(PlayerState);
	if (!ZfPS) return;

	const UZfPrimaryDataAssetClass* ClassData = ZfPS->GetCharacterClassData();
	if (!ClassData) return;

	// Slots normais
	const int32 SlotCount = FMath::Max(1, ClassData->MaxActiveSkillSlots);
	Loadout.Slots.SetNum(SlotCount);
	for (FName& Slot : Loadout.Slots)
	{
		Slot = NAME_None;
	}

	// Weapon slots — sempre 2 (primário e secundário)
	Loadout.WeaponSlots.SetNum(2);
	for (FName& Slot : Loadout.WeaponSlots)
	{
		Slot = NAME_None;
	}
}

// =============================================================================
// Weapon Slots
// =============================================================================

bool UZfSkillTreeComponent::EquipWeaponSkill(UAbilitySystemComponent* ASC, FName NodeID, int32 SlotIndex)
{
	if (!ASC || NodeID.IsNone()) return false;
	if (!GetOwner()->HasAuthority()) return false;
	if (!Loadout.WeaponSlots.IsValidIndex(SlotIndex)) return false;

	// Remove skill anterior do slot de arma se existir
	const FName OldNodeID = Loadout.WeaponSlots[SlotIndex];
	if (!OldNodeID.IsNone())
	{
		RevokeNodeAbility(ASC, OldNodeID);
	}

	Loadout.WeaponSlots[SlotIndex] = NodeID;

	// Concede a ability da arma
	UZfSkillTreeNodeData* Node = SkillTreeData ? SkillTreeData->FindNode(NodeID) : nullptr;
	if (Node)
	{
		GrantNodeAbility(ASC, Node);
	}

	return true;
}

bool UZfSkillTreeComponent::UnequipWeaponSkill(int32 SlotIndex)
{
	if (!GetOwner()->HasAuthority()) return false;
	if (!Loadout.WeaponSlots.IsValidIndex(SlotIndex)) return false;

	// As abilities são revogadas pelo ZfFragment_WeaponSkill::OnItemUnequipped
	// antes deste método ser chamado — aqui apenas limpa o slot do loadout.
	// RecalculateWeaponSlots será chamado pelo Fragment após a revogação.
	Loadout.WeaponSlots[SlotIndex] = NAME_None;
	return true;
}
// =============================================================================
// RecalculateWeaponSlots
// =============================================================================

void UZfSkillTreeComponent::RecalculateWeaponSlots(UZfEquipmentComponent* EquipmentComponent)
{
	if (!GetOwner()->HasAuthority()) return;
	if (!EquipmentComponent) return;

	// Limpa estado anterior
	Loadout.WeaponSlots[0] = NAME_None;
	Loadout.WeaponSlots[1] = NAME_None;
	WeaponSlotConflict     = NAME_None;

	// ── Busca armas equipadas ────────────────────────────────────────────
	// MainHand → arma principal (espada, arco, cajado, etc.)
	// OffHand  → arma secundária (escudo, adaga offhand, etc.)
	//
	// NOTA: ZfEquipmentTags::EquipmentSlots::Slot_MainHand e Slot_OffHand
	// devem estar declarados em ZfEquipmentTags.h

	const UZfItemInstance* MainHandItem =
		EquipmentComponent->GetItemAtEquipmentSlot(
			ZfEquipmentTags::EquipmentSlots::Slot_MainHand, 0);

	const UZfItemInstance* OffHandItem =
		EquipmentComponent->GetItemAtEquipmentSlot(
			ZfEquipmentTags::EquipmentSlots::Slot_OffHand, 0);

	const UZfFragment_WeaponSkill* MainHandFragment =
		MainHandItem ? MainHandItem->GetFragment<UZfFragment_WeaponSkill>() : nullptr;

	const UZfFragment_WeaponSkill* OffHandFragment =
		OffHandItem ? OffHandItem->GetFragment<UZfFragment_WeaponSkill>() : nullptr;

	// ── Distribui WeaponSlots[0] — botão primário ────────────────────────
	// Prioridade: Primary do MainHand → Primary do OffHand → vazio
	if (MainHandFragment && MainHandFragment->HasPrimaryAbility())
	{
		Loadout.WeaponSlots[0] = FName(*MainHandFragment->PrimaryAbilityClass->GetName());
	}
	else if (OffHandFragment && OffHandFragment->HasPrimaryAbility())
	{
		Loadout.WeaponSlots[0] = FName(*OffHandFragment->PrimaryAbilityClass->GetName());
	}

	// ── Distribui WeaponSlots[1] — botão secundário ──────────────────────
	// Prioridade: Secondary do OffHand (prioridade) → Secondary do MainHand → vazio
	// Conflito: ambas têm Secondary → OffHand vence, MainHand vai para WeaponSlotConflict

	const bool bOffHandHasSecondary  = OffHandFragment  && OffHandFragment->HasSecondaryAbility();
	const bool bMainHandHasSecondary = MainHandFragment  && MainHandFragment->HasSecondaryAbility();

	if (bOffHandHasSecondary)
	{
		Loadout.WeaponSlots[1] = FName(*OffHandFragment->SecondaryAbilityClass->GetName());

		// Registra conflito se MainHand também tem Secondary
		// O popup da skill tree exibe as duas opções ao jogador
		if (bMainHandHasSecondary)
		{
			WeaponSlotConflict = FName(*MainHandFragment->SecondaryAbilityClass->GetName());
		}
	}
	else if (bMainHandHasSecondary)
	{
		Loadout.WeaponSlots[1] = FName(*MainHandFragment->SecondaryAbilityClass->GetName());
	}
}

void UZfSkillTreeComponent::ResolveWeaponSlotConflict(FName NodeID)
{
	if (!GetOwner()->HasAuthority()) return;
	if (NodeID.IsNone() || WeaponSlotConflict.IsNone()) return;

	// Troca o slot 1 atual com o conflito — jogador escolheu o outro
	const FName Previous   = Loadout.WeaponSlots[1];
	Loadout.WeaponSlots[1] = NodeID;
	WeaponSlotConflict     = Previous;
}

// =============================================================================
// TryActivateSkillInSlot / TryActivateWeaponSlot
// =============================================================================

bool UZfSkillTreeComponent::TryActivateSkillInSlot(
	int32 SlotIndex, UAbilitySystemComponent* ASC) const
{
	if (!ASC) return false;

	if (!Loadout.Slots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const FName NodeID = Loadout.Slots[SlotIndex];
	if (NodeID.IsNone())
	{
		return false;
	}

	// Resolve NodeID -> Handle
	const FGameplayAbilitySpecHandle* Handle = GrantedAbilityHandles.Find(NodeID);
	if (!Handle || !Handle->IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UZfSkillTreeComponent::TryActivateSkillInSlot: "
				 "NodeID '%s' no slot %d nao esta em GrantedAbilityHandles."),
			*NodeID.ToString(), SlotIndex);
		return false;
	}

	return ASC->TryActivateAbility(*Handle);
}

bool UZfSkillTreeComponent::TryActivateWeaponSlot(
	int32 SlotIndex, UAbilitySystemComponent* ASC) const
{
	if (!ASC) return false;

	if (!Loadout.WeaponSlots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const FName NodeID = Loadout.WeaponSlots[SlotIndex];
	if (NodeID.IsNone())
	{
		return false;
	}

	// Weapon slots usam o nome da classe como NodeID sintetico.
	// Busca o AbilitySpec no ASC pela classe diretamente.
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (!Spec.Ability) continue;

		if (FName(*Spec.Ability->GetClass()->GetName()) == NodeID)
		{
			return ASC->TryActivateAbility(Spec.Handle);
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("UZfSkillTreeComponent::TryActivateWeaponSlot: "
			 "NodeID '%s' no weapon slot %d nao encontrado no ASC."),
		*NodeID.ToString(), SlotIndex);

	return false;
}

// =============================================================================
// Server_ConfirmSkillCast
// =============================================================================

void UZfSkillTreeComponent::Server_ConfirmSkillCast_Implementation(
	const FName& NodeID,
	FVector_NetQuantize HitLocation,
	FVector_NetQuantize10 HitNormal,
	FVector_NetQuantize10 CastDirection)
{
	if (!SkillTreeData) return;

	// Busca o pawn do dono para calcular distancia e disparar o evento
	AActor* AvatarActor = nullptr;
	if (const APlayerState* PS = Cast<APlayerState>(GetOwner()))
	{
		if (const APlayerController* PC = PS->GetPlayerController())
		{
			AvatarActor = PC->GetPawn();
		}
	}

	if (!AvatarActor) return;

	// ── Validacao de distancia ────────────────────────────────────────
	const UZfSkillTreeNodeData* NodeData = SkillTreeData->FindNode(NodeID);
	if (NodeData)
	{
		const int32 CurrentRank = GetNodeRank(NodeID);
		const float MaxRange    = NodeData->GetMaxRangeForRank(CurrentRank);

		if (MaxRange > 0.f)
		{
			const float DistSq           = FVector::DistSquared(AvatarActor->GetActorLocation(), HitLocation);
			const float MaxRangeWithMargin = MaxRange * 1.1f;

			if (DistSq > FMath::Square(MaxRangeWithMargin))
			{
				UE_LOG(LogTemp, Warning,
					TEXT("Server_ConfirmSkillCast: NodeID '%s' rejeitado — "
						 "distancia %.0f excede MaxRange %.0f."),
					*NodeID.ToString(),
					FMath::Sqrt(DistSq), MaxRangeWithMargin);
				return;
			}
		}
	}

	// ── Monta TargetData com os dados recebidos do cliente ────────────
	FHitResult HitResult;
	HitResult.Location     = HitLocation;
	HitResult.ImpactPoint  = HitLocation;
	HitResult.ImpactNormal = HitNormal;

	FGameplayAbilityTargetData_SingleTargetHit* TargetData =
		new FGameplayAbilityTargetData_SingleTargetHit(HitResult);

	FGameplayAbilityTargetDataHandle TargetDataHandle;
	TargetDataHandle.Add(TargetData);

	// ── Dispara evento de confirmacao para a GA ───────────────────────
	FGameplayEventData Payload;
	Payload.TargetData     = TargetDataHandle;
	Payload.Instigator     = AvatarActor;
	Payload.EventMagnitude = 1.f;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		AvatarActor,
		ZfAbilityTreeTags::SkillTree_AimMode_Confirm,
		Payload);
}

// =============================================================================
// AimIndicator registration
// =============================================================================

void UZfSkillTreeComponent::SetActiveAimIndicator(AZfSkillAimIndicator* Indicator, FName InNodeID)
{
	ActiveAimIndicator = Indicator;
	ActiveAimNodeID    = InNodeID;
}

void UZfSkillTreeComponent::ClearActiveAimIndicator()
{
	ActiveAimIndicator = nullptr;
	ActiveAimNodeID    = NAME_None;
}

bool UZfSkillTreeComponent::TryCancelAimMode(UAbilitySystemComponent* ASC)
{
	// Sem indicador ativo — nao ha AimMode para fechar
	if (!HasActiveAimIndicator()) return false;

	if (!ASC) return false;

	// Itera specs ativas buscando a UZfAbility_Active em AimMode
	// FScopedAbilityListLock protege contra modificacao da lista durante iteracao
	FScopedAbilityListLock ActiveScopeLock(*ASC);

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (!Spec.IsActive()) continue;

		UZfAbility_Active* ActiveAbility =
			Cast<UZfAbility_Active>(Spec.GetPrimaryInstance());

		if (ActiveAbility && ActiveAbility->IsInAimMode())
		{
			// Cancela a GA — o Blueprint filho recebe EndAbility(bWasCancelled=true)
			// e chama ExitAimMode, que limpa a tag e destroi o indicador
			ASC->CancelAbilityHandle(Spec.Handle);
			return true;
		}
	}

	// Indicador existia mas GA nao foi encontrada (edge case) —
	// limpa o estado manualmente para nao travar
	ClearActiveAimIndicator();
	return true;
}

FVector UZfSkillTreeComponent::GetActiveAimHitLocation() const
{
	if (!IsValid(ActiveAimIndicator.Get())) return FVector::ZeroVector;
	return ActiveAimIndicator->GetCurrentHitLocation();
}

FVector UZfSkillTreeComponent::GetActiveAimHitNormal() const
{
	if (!IsValid(ActiveAimIndicator.Get())) return FVector::UpVector;
	return ActiveAimIndicator->GetCurrentHitNormal();
}