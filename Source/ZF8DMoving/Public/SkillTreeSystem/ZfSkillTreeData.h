// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SkillTreeSystem/ZfSkillTreeTypes.h"
#include "ZfSkillTreeData.generated.h"

/**
 * Data Asset da Skill Tree universal do projeto Zf.
 *
 * Contém a estrutura de regiões da tree. Cada nó é um asset individual
 * do tipo UZfSkillTreeNodeData — referenciado pelo array Nodes de cada região.
 *
 * Como criar no editor:
 *   Content Browser → botão direito → Miscellaneous → Data Asset
 *   → selecione UZfSkillTreeData → nomeie como DA_AbilityTree
 */
UCLASS(BlueprintType)
class ZF8DMOVING_API UZfSkillTreeData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	// ── Regiões ───────────────────────────────────────────────────────────

	/** Nós disponíveis para todos os personagens independente de classe. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree|Regions")
	FAbilityTreeRegion NoviceRegion;

	/**
	 * Uma entrada por classe do jogo.
	 * Cada região tem RequiredClassTag definida.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree|Regions")
	TArray<FAbilityTreeRegion> ClassRegions;

	/** Nós de quest, eventos e habilidades únicas. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree|Regions")
	FAbilityTreeRegion SpecialRegion;

	// ── Utilitários de lookup ─────────────────────────────────────────────

	/**
	 * Busca um nó pelo NodeID em todas as regiões.
	 * Retorna nullptr se não encontrado.
	 *
	 * @param NodeID  Identificador do nó buscado
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	UZfSkillTreeNodeData* FindNode(FName NodeID) const;

	/**
	 * Retorna todos os nós de todas as regiões em um único array.
	 * Útil para a widget iterar todos os nós disponíveis.
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	TArray<UZfSkillTreeNodeData*> GetAllNodes() const;

private:

	/** Busca um nó dentro de uma região específica. */
	UZfSkillTreeNodeData* FindNodeInRegion(const FAbilityTreeRegion& Region, FName NodeID) const;
};