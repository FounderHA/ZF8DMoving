// Fill out your copyright notice in the Description page of Project Settings.

#include "FunctionLibrary/ZfSkillTreeWidgetHelper.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "SkillTreeSystem/ZfSkillTreeComponent.h"
#include "SkillTreeSystem/ZfSkillTreeData.h"
#include "AbilitySystem/GameplayAbility/SkillTreeSystem/ZfGameplayAbilitySkill.h"
#include "Player/ZfPlayerState.h"
#include "Player/Class/ZfClassBaseSettings.h"

// =============================================================================
// Privados
// =============================================================================

UZfSkillTreeComponent* UZfSkillTreeWidgetHelper::GetTreeComponent(APlayerState* PlayerState)
{
	if (!PlayerState) return nullptr;
	AZfPlayerState* ZfPS = Cast<AZfPlayerState>(PlayerState);
	return ZfPS ? ZfPS->GetSkillTreeComponent() : nullptr;
}

UAbilitySystemComponent* UZfSkillTreeWidgetHelper::GetASC(APlayerState* PlayerState)
{
	if (!PlayerState) return nullptr;
	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PlayerState);
	return ASI ? ASI->GetAbilitySystemComponent() : nullptr;
}

// =============================================================================
// Tooltip — Nó
// =============================================================================

void UZfSkillTreeWidgetHelper::GetNodeTooltipData(
	APlayerState* PlayerState,
	UZfSkillTreeNodeData* NodeData,
	FNodeTooltipData& OutData)
{
	OutData = FNodeTooltipData();

	if (!PlayerState || !NodeData) return;

	UZfSkillTreeComponent* TreeComp = GetTreeComponent(PlayerState);
	UAbilitySystemComponent* ASC = GetASC(PlayerState);
	if (!TreeComp || !ASC) return;

	// ── Dados básicos ─────────────────────────────────────────────────────
	OutData.Icon        = NodeData->Icon;
	OutData.DisplayName = NodeData->DisplayName;
	OutData.Description = NodeData->Description;
	OutData.CurrentRank = TreeComp->GetNodeRankFromASC(ASC, NodeData->NodeID);
	OutData.NodeState   = UZfSkillTreeComponent::DeriveNodeState(PlayerState, NodeData, OutData.CurrentRank);

	// ── Próximo rank para exibição do custo de ativação ───────────────────
	// Se Maxed → mostra custo do rank atual (não tem próximo)
	// Caso contrário → mostra custo do próximo rank
	if (OutData.NodeState == ESkillNodeState::Maxed)
	{
		OutData.NextRankForDisplay = OutData.CurrentRank;
	}
	else
	{
		// CurrentRank 0 = recém desbloqueado, próximo = 1
		OutData.NextRankForDisplay = OutData.CurrentRank + 1;
	}

	// ── Custo em SP — depende do estado ──────────────────────────────────
	switch (OutData.NodeState)
	{
		case ESkillNodeState::Locked:
		case ESkillNodeState::Available:
			OutData.SkillPointCost = NodeData->UnlockCost;
			break;

		case ESkillNodeState::Unlocked:
		case ESkillNodeState::AffordableUpgrade:
			OutData.SkillPointCost = NodeData->GetRankUpCost(OutData.CurrentRank);
			break;

		case ESkillNodeState::Maxed:
		case ESkillNodeState::Excluded:
			OutData.SkillPointCost = 0;
			break;
	}

	// ── Custos de ativação — apenas se a Skill for ativa ────────────────
	OutData.ActivationCosts = NodeData->Costs;

	// ── Requisitos não atendidos — apenas se Locked ───────────────────────
	if (OutData.NodeState == ESkillNodeState::Locked)
	{
		FGameplayTagContainer PlayerTags;
		ASC->GetOwnedGameplayTags(PlayerTags);

		// Tags não atendidas — preserva DisplayName e Icon para a UI
		for (const FTagRequirement& Req : NodeData->RequiredTags)
		{
			if (!PlayerTags.HasTagExact(Req.RequiredTag))
			{
				OutData.UnmetRequiredTags.Add(Req);
			}
		}

		// Atributos não atendidos
		for (const FAttributeRequirement& Req : NodeData->AttributeRequirements)
		{
			if (!Req.Attribute.IsValid()) continue;
			const float Current = ASC->GetNumericAttribute(Req.Attribute);
			if (Current < Req.MinValue)
			{
				OutData.UnmetAttributeRequirements.Add(Req);
			}
		}
	}
}

// =============================================================================
// Tooltip — Sub-efeito
// =============================================================================

void UZfSkillTreeWidgetHelper::GetSubEffectTooltipData(
	APlayerState* PlayerState,
	UZfSkillTreeNodeData* NodeData,
	int32 SubEffectIndex,
	FSubEffectTooltipData& OutData)
{
	OutData = FSubEffectTooltipData();

	if (!PlayerState || !NodeData) return;
	if (!NodeData->SubEffects.IsValidIndex(SubEffectIndex)) return;

	UAbilitySystemComponent* ASC = GetASC(PlayerState);
	if (!ASC) return;

	const FSubEffectData& SubEffect = NodeData->SubEffects[SubEffectIndex];

	// ── Dados básicos ─────────────────────────────────────────────────────
	OutData.Icon        = SubEffect.Icon;
	OutData.DisplayName = SubEffect.DisplayName;
	OutData.Description = SubEffect.Description;
	OutData.UnlockCost  = SubEffect.UnlockCost;

	// ── Exclusividade ─────────────────────────────────────────────────────
	if (SubEffect.ExclusiveTags.IsValid())
	{
		FGameplayTagContainer PlayerTags;
		ASC->GetOwnedGameplayTags(PlayerTags);

		if (PlayerTags.HasAny(SubEffect.ExclusiveTags))
		{
			OutData.bIsExcluded = true;

			for (const FSubEffectData& OtherSubEffect : NodeData->SubEffects)
			{
				if (!OtherSubEffect.GrantedTag.IsValid()) continue;

				if (SubEffect.ExclusiveTags.HasTagExact(OtherSubEffect.GrantedTag)
					&& PlayerTags.HasTagExact(OtherSubEffect.GrantedTag))
				{
					FSubEffectExclusiveEntry Entry;
					Entry.DisplayName = OtherSubEffect.DisplayName;
					Entry.Icon        = OtherSubEffect.Icon;
					OutData.ExclusiveWith.Add(Entry);
				}
			}
		}
	}
}

// =============================================================================
// Estado dos nós
// =============================================================================

ESkillNodeState UZfSkillTreeWidgetHelper::GetNodeState(
	APlayerState* PlayerState,
	UZfSkillTreeNodeData* NodeData)
{
	if (!PlayerState || !NodeData) return ESkillNodeState::Locked;

	UZfSkillTreeComponent* TreeComp = GetTreeComponent(PlayerState);
	UAbilitySystemComponent* ASC = GetASC(PlayerState);
	if (!TreeComp || !ASC) return ESkillNodeState::Locked;

	const int32 CurrentRank = TreeComp->GetNodeRankFromASC(ASC, NodeData->NodeID);
	return UZfSkillTreeComponent::DeriveNodeState(PlayerState, NodeData, CurrentRank);
}

ESkillNodeState UZfSkillTreeWidgetHelper::GetRegionState(
	APlayerState* PlayerState,
	const FSkillTreeRegion& Region)
{
	if (!PlayerState) return ESkillNodeState::Locked;

	// Região livre — sem requisito de classe
	if (!Region.RequiredClassTag.IsValid())
	{
		return ESkillNodeState::Available;
	}

	UAbilitySystemComponent* ASC = GetASC(PlayerState);
	if (!ASC) return ESkillNodeState::Locked;

	// Personagem tem a tag da classe desta região → Available
	if (ASC->HasMatchingGameplayTag(Region.RequiredClassTag))
	{
		return ESkillNodeState::Available;
	}

	// Verifica se o personagem já escolheu uma classe diferente → Excluded
	const FGameplayTag ClassParentTag = FGameplayTag::RequestGameplayTag(FName("SkillTree.Class"));
	const FGameplayTag NoviceTag = FGameplayTag::RequestGameplayTag(FName("SkillTree.Class.Novice"));

	FGameplayTagContainer PlayerTags;
	ASC->GetOwnedGameplayTags(PlayerTags);

	FGameplayTagContainer PlayerClassTags = PlayerTags.Filter(FGameplayTagContainer(ClassParentTag));
	PlayerClassTags.RemoveTag(NoviceTag);

	// Tem tag de classe diferente → Excluded permanentemente
	if (PlayerClassTags.Num() > 0)
	{
		return ESkillNodeState::Excluded;
	}

	// Ainda é Novice, não escolheu classe → Locked (pode chegar lá)
	return ESkillNodeState::Locked;
}

int32 UZfSkillTreeWidgetHelper::GetNodeRank(
	APlayerState* PlayerState,
	UZfSkillTreeNodeData* NodeData)
{
	if (!PlayerState || !NodeData) return 0;

	UZfSkillTreeComponent* TreeComp = GetTreeComponent(PlayerState);
	UAbilitySystemComponent* ASC = GetASC(PlayerState);
	if (!TreeComp || !ASC) return 0;

	return TreeComp->GetNodeRankFromASC(ASC, NodeData->NodeID);
}

bool UZfSkillTreeWidgetHelper::IsSubEffectUnlocked(
	APlayerState* PlayerState,
	UZfSkillTreeNodeData* NodeData,
	int32 SubEffectIndex)
{
	if (!PlayerState || !NodeData) return false;
	if (!NodeData->SubEffects.IsValidIndex(SubEffectIndex)) return false;

	UAbilitySystemComponent* ASC = GetASC(PlayerState);
	if (!ASC) return false;

	const FSubEffectData& SubEffect = NodeData->SubEffects[SubEffectIndex];
	return SubEffect.GrantedTag.IsValid() && ASC->HasMatchingGameplayTag(SubEffect.GrantedTag);
}

bool UZfSkillTreeWidgetHelper::CanUnlockSubEffect(
	APlayerState* PlayerState,
	UZfSkillTreeNodeData* NodeData,
	int32 SubEffectIndex)
{
	if (!PlayerState || !NodeData) return false;

	UZfSkillTreeComponent* TreeComp = GetTreeComponent(PlayerState);
	if (!TreeComp) return false;

	const TArray<int32> UnlockedIndices = TreeComp->GetUnlockedSubEffects(NodeData->NodeID);
	return UZfSkillTreeComponent::CanUnlockSubEffect(PlayerState, NodeData, SubEffectIndex, UnlockedIndices);
}

// =============================================================================
// Helpers de custo e recurso
// =============================================================================

float UZfSkillTreeWidgetHelper::GetCostForRank(const FSkillCostData& CostData, int32 Rank)
{
	return CostData.GetCostForRank(Rank);
}

FText UZfSkillTreeWidgetHelper::GetResourceTypeName(EResourceType ResourceType)
{
	switch (ResourceType)
	{
		case EResourceType::Mana:    return FText::FromString(TEXT("Mana"));
		case EResourceType::Stamina: return FText::FromString(TEXT("Stamina"));
		case EResourceType::Health:  return FText::FromString(TEXT("Health"));
		default:                     return FText::FromString(TEXT("?"));
	}
}

FLinearColor UZfSkillTreeWidgetHelper::GetResourceTypeColor(EResourceType ResourceType)
{
	switch (ResourceType)
	{
		case EResourceType::Mana:    return FLinearColor(0.f,  0.4f, 1.f,  1.f);
		case EResourceType::Stamina: return FLinearColor(1.f,  0.8f, 0.f,  1.f);
		case EResourceType::Health:  return FLinearColor(1.f,  0.1f, 0.1f, 1.f);
		default:                     return FLinearColor::White;
	}
}

float UZfSkillTreeWidgetHelper::GetRegionStateMaterialValue(ESkillNodeState RegionState)
{
	switch (RegionState)
	{
	case ESkillNodeState::Locked:    return 0.f;  // classe futura
	case ESkillNodeState::Available: return 1.f;  // acessível
	case ESkillNodeState::Excluded:  return 2.f;  // classe diferente
	default:                         return 1.f;  // fallback → Available
	}
}

float UZfSkillTreeWidgetHelper::GetNodeStateMaterialValue(ESkillNodeState NodeState)
{
	switch (NodeState)
	{
	case ESkillNodeState::Locked:            return 0.f;
	case ESkillNodeState::Available:         return 1.f;
	case ESkillNodeState::Unlocked:          return 2.f;
	case ESkillNodeState::Disabled:          return 3.f;
	case ESkillNodeState::AffordableUpgrade: return 4.f;
	case ESkillNodeState::Maxed:             return 5.f;
	case ESkillNodeState::Excluded:          return 6.f;
	default:                                 return 0.f;
	}
}

float UZfSkillTreeWidgetHelper::GetSubEffectStateMaterialValue(
	APlayerState* PlayerState,
	UZfSkillTreeNodeData* NodeData,
	int32 SubEffectIndex)
{
	if (!PlayerState || !NodeData) return 0.f;
	if (!NodeData->SubEffects.IsValidIndex(SubEffectIndex)) return 0.f;
 
	UAbilitySystemComponent* ASC = GetASC(PlayerState);
	if (!ASC) return 0.f;
 
	const FSubEffectData& SubEffect = NodeData->SubEffects[SubEffectIndex];
 
	// Verifica se está desbloqueado
	if (SubEffect.GrantedTag.IsValid() && ASC->HasMatchingGameplayTag(SubEffect.GrantedTag))
	{
		return 2.f; // Unlocked
	}
 
	// Verifica se está excluído por exclusividade
	if (SubEffect.ExclusiveTags.IsValid())
	{
		FGameplayTagContainer PlayerTags;
		ASC->GetOwnedGameplayTags(PlayerTags);
 
		if (PlayerTags.HasAny(SubEffect.ExclusiveTags))
		{
			return 3.f; // Excluded
		}
	}
 
	// Verifica se pode desbloquear agora
	UZfSkillTreeComponent* TreeComp = GetTreeComponent(PlayerState);
	if (TreeComp)
	{
		const TArray<int32> UnlockedIndices = TreeComp->GetUnlockedSubEffects(NodeData->NodeID);
		if (UZfSkillTreeComponent::CanUnlockSubEffect(PlayerState, NodeData, SubEffectIndex, UnlockedIndices))
		{
			return 1.f; // Available
		}
	}
 
	return 0.f; // Locked
}

// =============================================================================
// Slots
// =============================================================================

UZfSkillTreeNodeData* UZfSkillTreeWidgetHelper::GetNodeDataForSlot(
	APlayerState* PlayerState,
	int32 SlotIndex,
	UZfSkillTreeData* SkillTree)
{
	if (!PlayerState || !SkillTree) return nullptr;

	UZfSkillTreeComponent* TreeComp = GetTreeComponent(PlayerState);
	if (!TreeComp) return nullptr;

	const FSkillSlotLoadout& Loadout = TreeComp->GetLoadout();
	if (!Loadout.Slots.IsValidIndex(SlotIndex)) return nullptr;

	const FName NodeID = Loadout.Slots[SlotIndex];
	if (NodeID.IsNone()) return nullptr;

	return SkillTree->FindNode(NodeID);
}

UZfSkillTreeNodeData* UZfSkillTreeWidgetHelper::GetNodeDataForWeaponSlot(
	APlayerState* PlayerState,
	int32 SlotIndex,
	UZfSkillTreeData* SkillTree)
{
	if (!PlayerState || !SkillTree) return nullptr;

	UZfSkillTreeComponent* TreeComp = GetTreeComponent(PlayerState);
	if (!TreeComp) return nullptr;

	const TArray<FName>& WeaponSlots = TreeComp->GetWeaponLoadout();
	if (!WeaponSlots.IsValidIndex(SlotIndex)) return nullptr;

	const FName NodeID = WeaponSlots[SlotIndex];
	if (NodeID.IsNone()) return nullptr;

	return SkillTree->FindNode(NodeID);
}

bool UZfSkillTreeWidgetHelper::IsSlotLocked(APlayerState* PlayerState, int32 SlotIndex)
{
	if (!PlayerState) return true;

	AZfPlayerState* ZfPS = Cast<AZfPlayerState>(PlayerState);
	if (!ZfPS) return true;

	const UZfPrimaryDataAssetClass* ClassData = ZfPS->GetCharacterClassData();
	if (!ClassData) return true;

	return SlotIndex >= ClassData->MaxActiveSkillSlots;
}

bool UZfSkillTreeWidgetHelper::GetSlotCooldown(
	APlayerState* PlayerState,
	int32 SlotIndex,
	UZfSkillTreeData* SkillTree,
	float& OutTimeRemaining,
	float& OutDuration)
{
	OutTimeRemaining = 0.f;
	OutDuration = 0.f;

	UZfSkillTreeNodeData* NodeData = GetNodeDataForSlot(PlayerState, SlotIndex, SkillTree);
	if (!NodeData) return false;

	if (!NodeData->CooldownTag.IsValid()) return false;

	UAbilitySystemComponent* ASC = GetASC(PlayerState);
	if (!ASC) return false;

	FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(
		FGameplayTagContainer(NodeData->CooldownTag));

	TArray<float> Durations = ASC->GetActiveEffectsDuration(Query);
	TArray<float> StartTimes = ASC->GetActiveEffectsTimeRemaining(Query);

	if (StartTimes.Num() == 0) return false;

	OutTimeRemaining = StartTimes[0];
	OutDuration = Durations[0];

	return OutTimeRemaining > 0.f;
}

ESkillSlotState UZfSkillTreeWidgetHelper::GetSlotState(
	APlayerState* PlayerState,
	UZfSkillTreeNodeData* NodeData)
{
	if (!PlayerState || !NodeData) return ESkillSlotState::Normal;

	UAbilitySystemComponent* ASC = GetASC(PlayerState);
	if (!ASC) return ESkillSlotState::Normal;

	// Active — buff ativo (prioridade máxima)
	if (NodeData->BuffTag.IsValid() && ASC->HasMatchingGameplayTag(NodeData->BuffTag))
	{
		return ESkillSlotState::Active;
	}

	// OnCooldown
	if (NodeData->CooldownTag.IsValid() && ASC->HasMatchingGameplayTag(NodeData->CooldownTag))
	{
		return ESkillSlotState::OnCooldown;
	}

	// NoResource — verifica cada custo contra atributos atuais
	if (!NodeData->Costs.IsEmpty())
	{
		UZfSkillTreeComponent* TreeComp = GetTreeComponent(PlayerState);
		const int32 CurrentRank = TreeComp ? TreeComp->GetNodeRankFromASC(ASC, NodeData->NodeID) : 1;

		for (const FSkillCostData& Cost : NodeData->Costs)
		{
			if (!Cost.CostEffectClass) continue;

			const float CostValue = Cost.GetCostForRank(CurrentRank);
			if (CostValue <= 0.f) continue;

			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			if (!ASC->CanApplyAttributeModifiers(
				Cost.CostEffectClass->GetDefaultObject<UGameplayEffect>(),
				static_cast<float>(CurrentRank),
				Context))
			{
				return ESkillSlotState::NoResource;
			}
		}
	}

	return ESkillSlotState::Normal;
}

float UZfSkillTreeWidgetHelper::GetSlotStateMaterialValue(ESkillSlotState SlotState)
{
	switch (SlotState)
	{
	case ESkillSlotState::Normal:      return 0.f;
	case ESkillSlotState::NoResource:  return 1.f;
	case ESkillSlotState::OnCooldown:  return 2.f;
	case ESkillSlotState::Executing:   return 3.f;
	case ESkillSlotState::Active:      return 4.f;
	default:                           return 0.f;
	}
}