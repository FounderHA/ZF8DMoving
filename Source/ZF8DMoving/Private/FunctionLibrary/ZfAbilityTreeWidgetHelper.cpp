// Fill out your copyright notice in the Description page of Project Settings.

#include "FunctionLibrary/ZfAbilityTreeWidgetHelper.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "SkillTreeSystem/ZfAbilityTreeComponent.h"
#include "SkillTreeSystem/ZfAbilityTreeData.h"
#include "AbilitySystem/GameplayAbility/SkillTreeSystem/ZfGameplayAbilitySkill.h"
#include "Player/ZfPlayerState.h"
#include "Player/Class/ZfClassBaseSettings.h"

// =============================================================================
// Privados
// =============================================================================

UZfAbilityTreeComponent* UZfAbilityTreeWidgetHelper::GetTreeComponent(APlayerState* PlayerState)
{
	if (!PlayerState) return nullptr;
	AZfPlayerState* ZfPS = Cast<AZfPlayerState>(PlayerState);
	return ZfPS ? ZfPS->GetAbilityTreeComponent() : nullptr;
}

UAbilitySystemComponent* UZfAbilityTreeWidgetHelper::GetASC(APlayerState* PlayerState)
{
	if (!PlayerState) return nullptr;
	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PlayerState);
	return ASI ? ASI->GetAbilitySystemComponent() : nullptr;
}

// =============================================================================
// Tooltip — Nó
// =============================================================================

void UZfAbilityTreeWidgetHelper::GetNodeTooltipData(
	APlayerState* PlayerState,
	UZfAbilityNodeData* NodeData,
	FNodeTooltipData& OutData)
{
	OutData = FNodeTooltipData();

	if (!PlayerState || !NodeData) return;

	UZfAbilityTreeComponent* TreeComp = GetTreeComponent(PlayerState);
	UAbilitySystemComponent* ASC = GetASC(PlayerState);
	if (!TreeComp || !ASC) return;

	// ── Dados básicos ─────────────────────────────────────────────────────
	OutData.Icon        = NodeData->Icon;
	OutData.DisplayName = NodeData->DisplayName;
	OutData.Description = NodeData->Description;
	OutData.CurrentRank = TreeComp->GetNodeRankFromASC(ASC, NodeData->NodeID);
	OutData.NodeState   = UZfAbilityTreeComponent::DeriveNodeState(PlayerState, NodeData, OutData.CurrentRank);

	// ── Próximo rank para exibição do custo de ativação ───────────────────
	// Se Maxed → mostra custo do rank atual (não tem próximo)
	// Caso contrário → mostra custo do próximo rank
	if (OutData.NodeState == EAbilityNodeState::Maxed)
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
		case EAbilityNodeState::Locked:
		case EAbilityNodeState::Available:
			OutData.SkillPointCost = NodeData->UnlockCost;
			break;

		case EAbilityNodeState::Unlocked:
		case EAbilityNodeState::AffordableUpgrade:
			OutData.SkillPointCost = NodeData->GetRankUpCost(OutData.CurrentRank);
			break;

		case EAbilityNodeState::Maxed:
		case EAbilityNodeState::Excluded:
			OutData.SkillPointCost = 0;
			break;
	}

	// ── Custos de ativação — apenas se a ability for ativa ────────────────
	OutData.ActivationCosts = NodeData->Costs;

	// ── Requisitos não atendidos — apenas se Locked ───────────────────────
	if (OutData.NodeState == EAbilityNodeState::Locked)
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

void UZfAbilityTreeWidgetHelper::GetSubEffectTooltipData(
	APlayerState* PlayerState,
	UZfAbilityNodeData* NodeData,
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

EAbilityNodeState UZfAbilityTreeWidgetHelper::GetNodeState(
	APlayerState* PlayerState,
	UZfAbilityNodeData* NodeData)
{
	if (!PlayerState || !NodeData) return EAbilityNodeState::Locked;

	UZfAbilityTreeComponent* TreeComp = GetTreeComponent(PlayerState);
	UAbilitySystemComponent* ASC = GetASC(PlayerState);
	if (!TreeComp || !ASC) return EAbilityNodeState::Locked;

	const int32 CurrentRank = TreeComp->GetNodeRankFromASC(ASC, NodeData->NodeID);
	return UZfAbilityTreeComponent::DeriveNodeState(PlayerState, NodeData, CurrentRank);
}

int32 UZfAbilityTreeWidgetHelper::GetNodeRank(
	APlayerState* PlayerState,
	UZfAbilityNodeData* NodeData)
{
	if (!PlayerState || !NodeData) return 0;

	UZfAbilityTreeComponent* TreeComp = GetTreeComponent(PlayerState);
	UAbilitySystemComponent* ASC = GetASC(PlayerState);
	if (!TreeComp || !ASC) return 0;

	return TreeComp->GetNodeRankFromASC(ASC, NodeData->NodeID);
}

bool UZfAbilityTreeWidgetHelper::IsSubEffectUnlocked(
	APlayerState* PlayerState,
	UZfAbilityNodeData* NodeData,
	int32 SubEffectIndex)
{
	if (!PlayerState || !NodeData) return false;
	if (!NodeData->SubEffects.IsValidIndex(SubEffectIndex)) return false;

	UAbilitySystemComponent* ASC = GetASC(PlayerState);
	if (!ASC) return false;

	const FSubEffectData& SubEffect = NodeData->SubEffects[SubEffectIndex];
	return SubEffect.GrantedTag.IsValid() && ASC->HasMatchingGameplayTag(SubEffect.GrantedTag);
}

bool UZfAbilityTreeWidgetHelper::CanUnlockSubEffect(
	APlayerState* PlayerState,
	UZfAbilityNodeData* NodeData,
	int32 SubEffectIndex)
{
	if (!PlayerState || !NodeData) return false;

	UZfAbilityTreeComponent* TreeComp = GetTreeComponent(PlayerState);
	if (!TreeComp) return false;

	const TArray<int32> UnlockedIndices = TreeComp->GetUnlockedSubEffects(NodeData->NodeID);
	return UZfAbilityTreeComponent::CanUnlockSubEffect(PlayerState, NodeData, SubEffectIndex, UnlockedIndices);
}

// =============================================================================
// Helpers de custo e recurso
// =============================================================================

float UZfAbilityTreeWidgetHelper::GetCostForRank(const FAbilityCostData& CostData, int32 Rank)
{
	return CostData.GetCostForRank(Rank);
}

FText UZfAbilityTreeWidgetHelper::GetResourceTypeName(EResourceType ResourceType)
{
	switch (ResourceType)
	{
		case EResourceType::Mana:    return FText::FromString(TEXT("Mana"));
		case EResourceType::Stamina: return FText::FromString(TEXT("Stamina"));
		case EResourceType::Health:  return FText::FromString(TEXT("Health"));
		default:                     return FText::FromString(TEXT("?"));
	}
}

FLinearColor UZfAbilityTreeWidgetHelper::GetResourceTypeColor(EResourceType ResourceType)
{
	switch (ResourceType)
	{
		case EResourceType::Mana:    return FLinearColor(0.f,  0.4f, 1.f,  1.f);
		case EResourceType::Stamina: return FLinearColor(1.f,  0.8f, 0.f,  1.f);
		case EResourceType::Health:  return FLinearColor(1.f,  0.1f, 0.1f, 1.f);
		default:                     return FLinearColor::White;
	}
}

float UZfAbilityTreeWidgetHelper::GetNodeStateMaterialValue(EAbilityNodeState NodeState)
{
	switch (NodeState)
	{
	case EAbilityNodeState::Locked:            return 0.f;
	case EAbilityNodeState::Available:         return 1.f;
	case EAbilityNodeState::Unlocked:          return 2.f;
	case EAbilityNodeState::AffordableUpgrade: return 3.f;
	case EAbilityNodeState::Maxed:             return 4.f;
	case EAbilityNodeState::Excluded:          return 5.f;
	default:                                   return 0.f;
	}
}

float UZfAbilityTreeWidgetHelper::GetSubEffectStateMaterialValue(
	APlayerState* PlayerState,
	UZfAbilityNodeData* NodeData,
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
	UZfAbilityTreeComponent* TreeComp = GetTreeComponent(PlayerState);
	if (TreeComp)
	{
		const TArray<int32> UnlockedIndices = TreeComp->GetUnlockedSubEffects(NodeData->NodeID);
		if (UZfAbilityTreeComponent::CanUnlockSubEffect(PlayerState, NodeData, SubEffectIndex, UnlockedIndices))
		{
			return 1.f; // Available
		}
	}
 
	return 0.f; // Locked
}

// =============================================================================
// Slots
// =============================================================================

UZfAbilityNodeData* UZfAbilityTreeWidgetHelper::GetNodeDataForSlot(
	APlayerState* PlayerState,
	int32 SlotIndex,
	UZfAbilityTreeData* AbilityTree)
{
	if (!PlayerState || !AbilityTree) return nullptr;

	UZfAbilityTreeComponent* TreeComp = GetTreeComponent(PlayerState);
	if (!TreeComp) return nullptr;

	const FAbilitySlotLoadout& Loadout = TreeComp->GetLoadout();
	if (!Loadout.Slots.IsValidIndex(SlotIndex)) return nullptr;

	const FName NodeID = Loadout.Slots[SlotIndex];
	if (NodeID.IsNone()) return nullptr;

	return AbilityTree->FindNode(NodeID);
}

bool UZfAbilityTreeWidgetHelper::IsSlotLocked(APlayerState* PlayerState, int32 SlotIndex)
{
	if (!PlayerState) return true;

	AZfPlayerState* ZfPS = Cast<AZfPlayerState>(PlayerState);
	if (!ZfPS) return true;

	const UZfPrimaryDataAssetClass* ClassData = ZfPS->GetCharacterClassData();
	if (!ClassData) return true;

	return SlotIndex >= ClassData->MaxActiveAbilitySlots;
}

bool UZfAbilityTreeWidgetHelper::GetSlotCooldown(
	APlayerState* PlayerState,
	int32 SlotIndex,
	UZfAbilityTreeData* AbilityTree,
	float& OutTimeRemaining,
	float& OutDuration)
{
	OutTimeRemaining = 0.f;
	OutDuration = 0.f;

	UZfAbilityNodeData* NodeData = GetNodeDataForSlot(PlayerState, SlotIndex, AbilityTree);
	if (!NodeData || !NodeData->AbilityClass) return false;

	UAbilitySystemComponent* ASC = GetASC(PlayerState);
	if (!ASC) return false;

	// Busca o spec da ability no ASC
	FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(NodeData->AbilityClass);
	if (!Spec) return false;

	UGameplayAbility* AbilityInstance = Spec->GetPrimaryInstance();
	if (!AbilityInstance) return false;

	// Consulta o cooldown via GAS nativo
	const FGameplayAbilityActorInfo* ActorInfo = ASC->AbilityActorInfo.Get();
	if (!ActorInfo) return false;

	AbilityInstance->GetCooldownTimeRemainingAndDuration(
		Spec->Handle, ActorInfo, OutTimeRemaining, OutDuration);

	return OutTimeRemaining > 0.f;
}