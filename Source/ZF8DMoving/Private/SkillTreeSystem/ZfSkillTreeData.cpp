// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillTreeSystem/ZfSkillTreeData.h"

UZfSkillTreeNodeData* UZfSkillTreeData::FindNode(FName NodeID) const
{
	if (NodeID.IsNone()) return nullptr;

	if (UZfSkillTreeNodeData* Node = FindNodeInRegion(NoviceRegion, NodeID))
		return Node;

	for (const FSkillTreeRegion& Region : ClassRegions)
	{
		if (UZfSkillTreeNodeData* Node = FindNodeInRegion(Region, NodeID))
			return Node;
	}

	if (UZfSkillTreeNodeData* Node = FindNodeInRegion(SpecialRegion, NodeID))
		return Node;

	UE_LOG(LogTemp, Warning,
		TEXT("UZfSkillTreeData::FindNode: NodeID '%s' não encontrado."),
		*NodeID.ToString());

	return nullptr;
}

TArray<UZfSkillTreeNodeData*> UZfSkillTreeData::GetAllNodes() const
{
	TArray<UZfSkillTreeNodeData*> AllNodes;

	auto CollectFromRegion = [&](const FSkillTreeRegion& Region)
	{
		for (UZfSkillTreeNodeData* Node : Region.Nodes)
		{
			if (Node) AllNodes.Add(Node);
		}
	};

	CollectFromRegion(NoviceRegion);
	for (const FSkillTreeRegion& Region : ClassRegions)
		CollectFromRegion(Region);
	CollectFromRegion(SpecialRegion);

	return AllNodes;
}

UZfSkillTreeNodeData* UZfSkillTreeData::FindNodeInRegion(const FSkillTreeRegion& Region, FName NodeID) const
{
	for (UZfSkillTreeNodeData* Node : Region.Nodes)
	{
		if (Node && Node->NodeID == NodeID)
			return Node;
	}
	return nullptr;
}