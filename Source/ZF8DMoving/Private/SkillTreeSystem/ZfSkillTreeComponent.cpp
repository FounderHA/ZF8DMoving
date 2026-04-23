// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillTreeSystem/ZfSkillTreeComponent.h"
#include "SkillTreeSystem/ZfSkillTreeNodeData.h"

#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "SkillTreeSystem/ZfSkillTreeData.h"
#include "AbilitySystem/Attributes/ZfProgressionAttributeSet.h"
#include "Player/ZfPlayerState.h"
#include "Player/Class/ZfClassBaseSettings.h"
#include "Tags/ZfGameplayTags.h"

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
	if (!PlayerState) return false;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PlayerState);
	if (!ASI) return false;

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC) return false;

	// Nó já desbloqueado — tag já presente no ASC
	if (Node->GrantedTag.IsValid() && ASC->HasMatchingGameplayTag(Node->GrantedTag))
	{
		return false;
	}

	// Verifica RequiredTags — todas precisam estar presentes (AND)
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
	if (!PlayerState) return false;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PlayerState);
	if (!ASI) return false;

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC) return false;

	// Nó precisa estar desbloqueado
	if (!Node->GrantedTag.IsValid() || !ASC->HasMatchingGameplayTag(Node->GrantedTag))
	{
		return false;
	}

	// Já está no rank máximo
	if (CurrentRank >= Node->MaxAbilityRank)
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
	if (!PlayerState) return false;
	if (!Node->SubEffects.IsValidIndex(SubEffectIndex)) return false;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PlayerState);
	if (!ASI) return false;

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC) return false;

	// Nó pai precisa estar desbloqueado
	if (!Node->GrantedTag.IsValid() || !ASC->HasMatchingGameplayTag(Node->GrantedTag))
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

EAbilityNodeState UZfSkillTreeComponent::DeriveNodeState(APlayerState* PlayerState,
	const UZfSkillTreeNodeData* Node, int32 CurrentRank)
{
	if (!PlayerState) return EAbilityNodeState::Locked;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PlayerState);
	if (!ASI) return EAbilityNodeState::Locked;

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC) return EAbilityNodeState::Locked;

	const bool bIsUnlocked = Node->GrantedTag.IsValid() && ASC->HasMatchingGameplayTag(Node->GrantedTag);

	// ── Nó já desbloqueado ────────────────────────────────────────────────
	if (bIsUnlocked)
	{
		if (CurrentRank >= Node->MaxAbilityRank)
		{
			return EAbilityNodeState::Maxed;
		}

		if (CanUpgradeNode(PlayerState, Node, CurrentRank))
		{
			return EAbilityNodeState::AffordableUpgrade;
		}

		return EAbilityNodeState::Unlocked;
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
					return EAbilityNodeState::Excluded;
				}
			}
		}
	}

	// ── Disponível ou bloqueado ───────────────────────────────────────────
	if (CanUnlockNode(PlayerState, Node))
	{
		return EAbilityNodeState::Available;
	}

	return EAbilityNodeState::Locked;
}

// =============================================================================
// Operações de escrita — servidor only
// =============================================================================

bool UZfSkillTreeComponent::UnlockNode(UAbilitySystemComponent* ASC, FName NodeID)
{
	if (!ASC || NodeID.IsNone() || !AbilityTreeData) return false;

	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("UZfSkillTreeComponent::UnlockNode: chamado fora do servidor."));
		return false;
	}

	UZfSkillTreeNodeData* Node = AbilityTreeData->FindNode(NodeID);
	if (!Node)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UZfSkillTreeComponent::UnlockNode: NodeID '%s' não encontrado."), *NodeID.ToString());
		return false;
	}

	APlayerState* PS = Cast<APlayerState>(GetOwner());
	if (!CanUnlockNode(PS, Node))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UZfSkillTreeComponent::UnlockNode: pré-requisitos não atendidos para '%s'."),
			*NodeID.ToString());
		return false;
	}

	if (!SpendSkillPoints(ASC, Node->UnlockCost)) return false;

	// Concede ability ao ASC
	GrantNodeAbility(ASC, Node);

	// Adiciona GrantedTag ao personagem
	if (Node->GrantedTag.IsValid())
	{
		ASC->AddLooseGameplayTag(Node->GrantedTag);
	}

	// Atualiza estado runtime
	UnlockedNodes.Add(NodeID);
	NodeRanks.Add(NodeID, 0);

	return true;
}

bool UZfSkillTreeComponent::UpgradeNode(UAbilitySystemComponent* ASC, FName NodeID)
{
	if (!ASC || NodeID.IsNone() || !AbilityTreeData) return false;

	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("UZfSkillTreeComponent::UpgradeNode: chamado fora do servidor."));
		return false;
	}

	UZfSkillTreeNodeData* Node = AbilityTreeData->FindNode(NodeID);
	if (!Node) return false;

	const int32 CurrentRank = GetNodeRank(NodeID);

	APlayerState* PS = Cast<APlayerState>(GetOwner());
	if (!CanUpgradeNode(PS, Node, CurrentRank)) return false;

	const int32 Cost = Node->GetRankUpCost(CurrentRank);
	if (!SpendSkillPoints(ASC, Cost)) return false;

	// Incrementa Level no FGameplayAbilitySpec
	if (FGameplayAbilitySpecHandle* Handle = GrantedAbilityHandles.Find(NodeID))
	{
		FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(*Handle);
		if (Spec)
		{
			Spec->Level += 1;
			ASC->MarkAbilitySpecDirty(*Spec);
		}
	}

	NodeRanks[NodeID] = CurrentRank + 1;

	return true;
}

bool UZfSkillTreeComponent::UnlockSubEffect(UAbilitySystemComponent* ASC, FName NodeID, int32 SubEffectIndex)
{
	if (!ASC || NodeID.IsNone() || !AbilityTreeData) return false;

	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("UZfSkillTreeComponent::UnlockSubEffect: chamado fora do servidor."));
		return false;
	}

	UZfSkillTreeNodeData* Node = AbilityTreeData->FindNode(NodeID);
	if (!Node) return false;

	const TArray<int32> CurrentUnlocked = GetUnlockedSubEffects(NodeID);

	APlayerState* PS = Cast<APlayerState>(GetOwner());
	if (!CanUnlockSubEffect(PS, Node, SubEffectIndex, CurrentUnlocked)) return false;

	const FSubEffectData& SubEffect = Node->SubEffects[SubEffectIndex];
	if (!SpendSkillPoints(ASC, SubEffect.UnlockCost)) return false;

	// Adiciona tag do sub-efeito ao personagem
	if (SubEffect.GrantedTag.IsValid())
	{
		ASC->AddLooseGameplayTag(SubEffect.GrantedTag);
	}

	// Atualiza estado runtime
	UnlockedSubEffects.FindOrAdd(NodeID).Indices.AddUnique(SubEffectIndex);

	return true;
}

bool UZfSkillTreeComponent::RespecTree(UAbilitySystemComponent* ASC)
{
	if (!ASC || !AbilityTreeData) return false;

	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("UZfSkillTreeComponent::RespecTree: chamado fora do servidor."));
		return false;
	}

	int32 TotalSPToRefund = 0;

	for (const FName& NodeID : UnlockedNodes)
	{
		UZfSkillTreeNodeData* Node = nullptr;
		Node = AbilityTreeData->FindNode(NodeID);
	if (!Node) continue;

		// Calcula SP a devolver pelo desbloqueio
		TotalSPToRefund += Node->UnlockCost;

		// Calcula SP a devolver pelas evoluções (Rank 0 é o estado inicial sem custo)
		const int32 CurrentRank = GetNodeRank(NodeID);
		for (int32 Rank = 0; Rank < CurrentRank; ++Rank)
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
					ASC->RemoveLooseGameplayTag(Node->SubEffects[Index].GrantedTag);
				}
			}
		}

		// Revoga ability do ASC
		RevokeNodeAbility(ASC, NodeID);

		// Remove GrantedTag do nó
		if (Node->GrantedTag.IsValid())
		{
			ASC->RemoveLooseGameplayTag(Node->GrantedTag);
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

bool UZfSkillTreeComponent::EquipAbilityInSlot(UAbilitySystemComponent* ASC, FName NodeID, int32 SlotIndex)
{
	if (!ASC || NodeID.IsNone()) return false;

	if (!GetOwner()->HasAuthority()) return false;

	if (!Loadout.Slots.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UZfSkillTreeComponent::EquipAbilityInSlot: SlotIndex %d inválido (total: %d)."),
			SlotIndex, Loadout.Slots.Num());
		return false;
	}

	if (!UnlockedNodes.Contains(NodeID))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UZfSkillTreeComponent::EquipAbilityInSlot: NodeID '%s' não está desbloqueado."),
			*NodeID.ToString());
		return false;
	}

	Loadout.Slots[SlotIndex] = NodeID;
	return true;
}

bool UZfSkillTreeComponent::UnequipAbilityFromSlot(int32 SlotIndex)
{
	if (!GetOwner()->HasAuthority()) return false;

	if (!Loadout.Slots.IsValidIndex(SlotIndex)) return false;

	Loadout.Slots[SlotIndex] = NAME_None;
	return true;
}

// =============================================================================
// Save e Restore
// =============================================================================

void UZfSkillTreeComponent::RestoreFromSaveData(UAbilitySystemComponent* ASC, const FCharacterTreeSaveData& SaveData)
{
	if (!ASC || !AbilityTreeData) return;

	if (!GetOwner()->HasAuthority()) return;

	// Inicializa slots com base na classe do personagem
	APlayerState* PS = Cast<APlayerState>(GetOwner());
	InitializeSlots(PS);

	for (const FName& NodeID : SaveData.UnlockedNodes)
	{
		UZfSkillTreeNodeData* Node = nullptr;
		Node = AbilityTreeData->FindNode(NodeID);
	if (!Node)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("UZfSkillTreeComponent::RestoreFromSaveData: NodeID '%s' não encontrado no Data Asset — ignorado."),
				*NodeID.ToString());
			continue;
		}

		// Concede ability sem validação de pré-requisitos
		GrantNodeAbility(ASC, Node);

		// Restaura GrantedTag do nó
		if (Node->GrantedTag.IsValid())
		{
			ASC->AddLooseGameplayTag(Node->GrantedTag);
		}

		// Restaura rank e aplica no AbilitySpec
		const int32* SavedRank = SaveData.NodeRanks.Find(NodeID);
		const int32 Rank = SavedRank ? *SavedRank : 0;
		NodeRanks.Add(NodeID, Rank);

		if (Rank > 1)
		{
			if (FGameplayAbilitySpecHandle* Handle = GrantedAbilityHandles.Find(NodeID))
			{
				FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(*Handle);
				if (Spec)
				{
					Spec->Level = Rank;
					ASC->MarkAbilitySpecDirty(*Spec);
				}
			}
		}

		// Restaura sub-efeitos
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

		UnlockedNodes.Add(NodeID);
	}
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
	if (!ASC || NodeID.IsNone() || !AbilityTreeData) return 0;

	UZfSkillTreeNodeData* Node = AbilityTreeData->FindNode(NodeID);
	if (!Node || !Node->AbilityClass) return 0;

	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(Node->AbilityClass);
	if (!Spec) return 0;

	return Spec->Level;
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

	// Escuta qualquer tag filha de SkillTree.Node — cobre desbloqueio de nós
	ASC->RegisterGameplayTagEvent(
		FGameplayTag::RequestGameplayTag(FName("SkillTree.Node")),
		EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UZfSkillTreeComponent::OnSkillTreeTagChanged);

	// Escuta qualquer tag filha de SkillTree.SubEffect — cobre desbloqueio de sub-efeitos
	ASC->RegisterGameplayTagEvent(
		FGameplayTag::RequestGameplayTag(FName("SkillTree.SubEffect")),
		EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UZfSkillTreeComponent::OnSkillTreeTagChanged);
}

void UZfSkillTreeComponent::OnSkillTreeTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// Propaga para as widgets independente de servidor ou cliente
	OnTreeStateChanged.Broadcast();
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
	if (!ASC || !Node->AbilityClass) return;

	// Guard contra dupla concessão
	if (GrantedAbilityHandles.Contains(Node->NodeID)) return;

	FGameplayAbilitySpec Spec(Node->AbilityClass, 1);
	const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);

	if (Handle.IsValid())
	{
		GrantedAbilityHandles.Add(Node->NodeID, Handle);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UZfSkillTreeComponent::GrantNodeAbility: falha ao conceder ability do nó '%s'."),
			*Node->NodeID.ToString());
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
	if (!ZfPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("UZfSkillTreeComponent::InitializeSlots: PlayerState inválido."));
		return;
	}

	const UZfPrimaryDataAssetClass* ClassData = ZfPS->GetCharacterClassData();
	if (!ClassData)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UZfSkillTreeComponent::InitializeSlots: CharacterClassData não configurado."));
		return;
	}

	const int32 SlotCount = FMath::Max(1, ClassData->MaxActiveAbilitySlots);
	Loadout.Slots.SetNum(SlotCount);

	for (FName& Slot : Loadout.Slots)
	{
		Slot = NAME_None;
	}
}