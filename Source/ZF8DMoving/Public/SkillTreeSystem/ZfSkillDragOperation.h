// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "SkillTreeSystem/ZfSkillTreeNodeData.h"
#include "ZfSkillDragOperation.generated.h"

/**
 * Operação de drag and drop para skills da Skill Tree.
 *
 * Carrega os dados necessários durante o drag para que o widget
 * de destino (WBP_SkillSlot) saiba o que foi arrastado e de onde.
 *
 * Dois contextos de origem:
 *   SourceSlotIndex == -1  → drag veio de um nó da Skill Tree (WBP_SkillTree_Node)
 *   SourceSlotIndex >= 0   → drag veio de um slot existente (WBP_SkillSlot)
 *
 * Lógica de drop no WBP_SkillSlot:
 *   Veio da tree + slot vazio    → equipa
 *   Veio da tree + slot ocupado  → substitui (slot antigo fica vazio)
 *   Veio de slot + slot vazio    → move
 *   Veio de slot + slot ocupado  → troca
 */
UCLASS(BlueprintType)
class ZF8DMOVING_API UZfSkillDragOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:

	/**
	 * Data Asset do nó sendo arrastado.
	 * Usado pelo slot de destino para identificar a skill.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "SkillTree|DragDrop")
	TObjectPtr<UZfSkillTreeNodeData> NodeData;

	/**
	 * Índice do slot de origem.
	 * -1  → drag veio de um nó da Skill Tree
	 * >= 0 → drag veio de um slot existente
	 */
	UPROPERTY(BlueprintReadWrite, Category = "SkillTree|DragDrop")
	int32 SourceSlotIndex = -1;

	/**
	 * PlayerState do jogador que iniciou o drag.
	 * Necessário para chamar os Server RPCs de equip/unequip.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "SkillTree|DragDrop")
	TObjectPtr<APlayerState> PlayerState;
};