// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SkillTreeSystem/ZfSkillTreeNodeData.h"
#include "ZfSkillTreeTypes.generated.h"

// =============================================================================
// ESkillNodeState
// =============================================================================

/**
 * Estado visual de um nó na skill tree.
 * Nunca armazenado — sempre derivado em runtime por UZfSkillTreeComponent::DeriveNodeState.
 */
UENUM(BlueprintType)
enum class ESkillNodeState : uint8
{
	Locked              UMETA(DisplayName = "Locked"),
	Available           UMETA(DisplayName = "Available"),
	Unlocked            UMETA(DisplayName = "Unlocked"),
	Disabled            UMETA(DisplayName = "Disabled"),
	AffordableUpgrade   UMETA(DisplayName = "Affordable Upgrade"),
	Maxed               UMETA(DisplayName = "Maxed"),
	Excluded            UMETA(DisplayName = "Excluded"),
};

// =============================================================================
// ENodeRequerimentType
// =============================================================================

UENUM(BlueprintType)
enum class ENodeRequerimentType : uint8
{
	Attribute           UMETA(DisplayName = "Attribute"),
	Tag					UMETA(DisplayName = "Tag"),
};

// =============================================================================
// FSkillTreeRegion
// =============================================================================

/**
 * Região da skill tree universal — agrupa nós de uma classe ou contexto.
 *
 * Regiões existentes:
 *   Novice   → sem RequiredClassTag, acessível por todos
 *   Classes  → RequiredClassTag define qual classe tem acesso
 *   Especial → nós de quest/evento, RequiredTags específicas por nó
 */
USTRUCT(BlueprintType)
struct FSkillTreeRegion
{
	GENERATED_BODY()

	/** Identificador único da região. Ex: "Novice", "Mage", "Warrior" */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree|Region")
	FName RegionID;

	/** Nome exibido na orb da tela principal. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree|Region")
	FText DisplayName;
	
	// Icon do Orb
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree|Region")
	TObjectPtr<UTexture2D> OrbIcon;

	/**
	 * Tag de classe necessária para acessar esta região.
	 * Vazio = região livre (Novice, Especial).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree|Region",
		meta = (Categories = "SkillTree.Class"))
	FGameplayTag RequiredClassTag;

	/**
	 * Nós desta região — referências aos Data Assets individuais.
	 * O editor exibe o nome do asset automaticamente.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree|Region")
	TArray<TObjectPtr<UZfSkillTreeNodeData>> Nodes;
};

// =============================================================================
// FSubEffectIndexList
// =============================================================================

/**
 * Wrapper necessário para uso como valor em TMap com UPROPERTY.
 * O Unreal não suporta TArray diretamente como valor de TMap em propriedades refletidas.
 */
USTRUCT(BlueprintType)
struct FSubEffectIndexList
{
	GENERATED_BODY()

	/** Índices dos sub-efeitos desbloqueados para um nó específico. */
	UPROPERTY(BlueprintReadOnly, Category = "SkillTree|Save")
	TArray<int32> Indices;
};

// =============================================================================
// FCharacterTreeSaveData
// =============================================================================

/**
 * Dados de progressão da skill tree por personagem.
 *
 * O que é salvo:
 *   UnlockedNodes      → NodeIDs desbloqueados
 *   NodeRanks          → rank atual de cada nó desbloqueado
 *   UnlockedSubEffects → índices dos sub-efeitos desbloqueados por nó
 *
 * O que NÃO é salvo (derivado em runtime):
 *   ESkillNodeState  → calculado por DeriveNodeState a cada consulta
 *   SkillPoints        → vive no ProgressionAttributeSet (já replicado)
 *   Tags concedidas    → restauradas pelo RestoreFromSaveData via ASC
 */
USTRUCT(BlueprintType)
struct FCharacterTreeSaveData
{
	GENERATED_BODY()

	/** NodeIDs de todos os nós desbloqueados pelo personagem. */
	UPROPERTY(BlueprintReadOnly, Category = "SkillTree|Save")
	TSet<FName> UnlockedNodes;

	/**
	 * Rank atual de cada nó desbloqueado.
	 * Nós não presentes aqui assumem Rank 1.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "SkillTree|Save")
	TMap<FName, int32> NodeRanks;

	/**
	 * Índices dos sub-efeitos desbloqueados por nó.
	 * Índice corresponde à posição em UZfSkillTreeNodeData::SubEffects.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "SkillTree|Save")
	TMap<FName, FSubEffectIndexList> UnlockedSubEffects;
};