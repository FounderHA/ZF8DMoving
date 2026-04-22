// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillTreeSystem/ZfAbilityTreeData.h"

UZfAbilityNodeData* UZfAbilityTreeData::FindNode(FName NodeID) const
{
	if (NodeID.IsNone()) return nullptr;

	if (UZfAbilityNodeData* Node = FindNodeInRegion(NoviceRegion, NodeID))
		return Node;

	for (const FAbilityTreeRegion& Region : ClassRegions)
	{
		if (UZfAbilityNodeData* Node = FindNodeInRegion(Region, NodeID))
			return Node;
	}

	if (UZfAbilityNodeData* Node = FindNodeInRegion(SpecialRegion, NodeID))
		return Node;

	UE_LOG(LogTemp, Warning,
		TEXT("UZfAbilityTreeData::FindNode: NodeID '%s' não encontrado."),
		*NodeID.ToString());

	return nullptr;
}

TArray<UZfAbilityNodeData*> UZfAbilityTreeData::GetAllNodes() const
{
	TArray<UZfAbilityNodeData*> AllNodes;

	auto CollectFromRegion = [&](const FAbilityTreeRegion& Region)
	{
		for (UZfAbilityNodeData* Node : Region.Nodes)
		{
			if (Node) AllNodes.Add(Node);
		}
	};

	CollectFromRegion(NoviceRegion);
	for (const FAbilityTreeRegion& Region : ClassRegions)
		CollectFromRegion(Region);
	CollectFromRegion(SpecialRegion);

	return AllNodes;
}

UZfAbilityNodeData* UZfAbilityTreeData::FindNodeInRegion(const FAbilityTreeRegion& Region, FName NodeID) const
{
	for (UZfAbilityNodeData* Node : Region.Nodes)
	{
		if (Node && Node->NodeID == NodeID)
			return Node;
	}
	return nullptr;
}