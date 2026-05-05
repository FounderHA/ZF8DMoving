// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemReplicationProxyInterface.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "Inventory/ZfInventoryComponent.h"
#include "Inventory/ZfEquipmentComponent.h"
#include "Player/Class/ZfClassBaseSettings.h"
#include "CraftingSystem/ZfCraftTypes.h"
#include "SkillTreeSystem/ZfSkillTreeComponent.h"
#include "ZfPlayerState.generated.h"

class UZfMainAttributeSet;
class UZfResourceAttributeSet;
class UZfProgressionAttributeSet;
class UZfOffensiveAttributeSet;
class UZfDefensiveAttributeSet;
class UZfUtilityAttributeSet;
class UZfMovementAttributeSet;
class UZfCraftingComponent;

// ============================================================
// DELEGATES — CRAFT
// ============================================================

// Disparado quando o jogador aprende uma nova receita.
// Chamado em todos os clientes via OnRep_KnownRecipeTags.
// Use na UI para atualizar a lista de receitas disponiveis.
// @param RecipeTag — tag da receita aprendida
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRecipeLearned, FGameplayTag, RecipeTag);

// Disparado no cliente que iniciou um craft, com o resultado final.
// Rebroadcast a partir do Client_ReceiveCraftResult (servidor → cliente).
// A UI faz bind aqui para atualizar animacao/texto/som de feedback.
// @param Result — resultado completo do craft
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCraftResultReceived, const FZfCraftResult&, Result);

UCLASS()
class ZF8DMOVING_API AZfPlayerState : public APlayerState, public IAbilitySystemInterface, public IZfInventoryReceiverInterface
{
	GENERATED_BODY()
	
public:
	AZfPlayerState();
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UZfResourceAttributeSet* GetResourceAttributeSet() const;
	UZfMainAttributeSet* GetMainAttributeSet() const;
	UZfProgressionAttributeSet* GetProgressionAttributeSet() const;
	UZfOffensiveAttributeSet* GetOffensiveAttributeSet() const;
	UZfDefensiveAttributeSet* GetResistanceAttributeSet() const;
	UZfUtilityAttributeSet* GetUtilityAttributeSet() const;
	UZfMovementAttributeSet* GetMovementAttributeSet() const;

	UFUNCTION(BlueprintCallable)
	UZfInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	/** Retorna o Data Asset de classe do personagem. */
	UFUNCTION(BlueprintCallable, Category = "Player|Class")
	UZfPrimaryDataAssetClass* GetCharacterClassData() const { return CharacterClassData; }
	
protected:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category="Class")
	TObjectPtr<UZfPrimaryDataAssetClass> CharacterClassData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UZfInventoryComponent> InventoryComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillTree")
	TObjectPtr<UZfSkillTreeComponent> SkillTreeComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UZfEquipmentComponent> EquipmentComponent;
	
	UPROPERTY()
	TObjectPtr<UZfResourceAttributeSet> ResourceAttributeSet;
	
	UPROPERTY()
	TObjectPtr<UZfMainAttributeSet> MainAttributeSet; 

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|AttributeSets")
	TObjectPtr<UZfProgressionAttributeSet> ProgressionAttributeSet;
	
	UPROPERTY()
	TObjectPtr<UZfOffensiveAttributeSet> OffensiveAttributeSet;
	
	UPROPERTY()
	TObjectPtr<UZfDefensiveAttributeSet> ResistanceAttributeSet;
	
	UPROPERTY()
	TObjectPtr<UZfUtilityAttributeSet> UtilityAttributeSet;
	
	UPROPERTY()
	TObjectPtr<UZfMovementAttributeSet> MovementAttributeSet;


private:
	
	/** 
	* Define se o personagem está sendo criado pela primeira vez.
	* true  → InitializeDefaults() será chamado após InitializeAttributes()
	* false → load do save, defaults ignorados.
	* Setado pelo GameMode antes da posse. Não replicado — servidor only.
	*/
	bool bIsNewCharacter = true;
	
public:

	UFUNCTION(BlueprintCallable)
	UZfSkillTreeComponent* GetSkillTreeComponent() const { return SkillTreeComponent; }

	/** Retorna true se o personagem está sendo criado pela primeira vez.
	*/
	UFUNCTION(BlueprintCallable, Category = "Player|Save")
	bool GetIsNewCharacter() const { return bIsNewCharacter; }
	
	/** Chamado pelo sistema de save quando um save existente é encontrado.
	* Após chamar este método, InitializeDefaults() não será executado.
	*/
	UFUNCTION(BlueprintCallable, Category = "Player|Save")
	void SetIsNewCharacterFalse() { bIsNewCharacter = false; }
	
	// ─────────────────────────────────────────────────────────────────────
	// Server RPCs — ponte entre widget (cliente) e abilities (servidor)
	// Passa valores primitivos int32 para evitar problemas de replicação
	// de UObjects pela rede. O objeto Request é criado no servidor.
	// ─────────────────────────────────────────────────────────────────────
 
	/** Distribui pontos nos atributos. Valores >= 0. */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Progression")
	void Server_SpendAttributePoints(int32 Strength, int32 Dexterity, int32 Intelligence, int32 Constitution, int32 Conviction);
 
	/** Remove pontos individualmente. Valores >= 0 (quanto remover de cada). */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Progression")
	void Server_RefundAttributePoint(int32 Strength, int32 Dexterity, int32 Intelligence, int32 Constitution, int32 Conviction);
 
	/** Reseta todos os pontos distribuídos de volta ao pool. Sem parâmetros. */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Progression")
	void Server_ResetAttributePoints();

	// ─────────────────────────────────────────────────────────────────────
	// Skill Tree System - RPC
	// ─────────────────────────────────────────────────────────────────────
	
	/** Desbloqueia um nó da skill tree. */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "SkillTree")
	void Server_UnlockSkillNode(const FName& NodeID);
 
	/** Evolui um nó já desbloqueado da skill tree. */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "SkillTree")
	void Server_UpgradeSkillNode(const FName& NodeID);
 
	/** Desbloqueia um sub-efeito de um nó. */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "SkillTree")
	void Server_UnlockSubEffect(const FName& NodeID, int32 SubEffectIndex);
 
	/** Reseta toda a skill tree devolvendo os SkillPoints gastos. */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "SkillTree")
	void Server_RespecAbilityTree();
 
	/** Equipa uma ability desbloqueada em um slot ativo. */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "SkillTree")
	void Server_EquipSkillInSlot(const FName& NodeID, int32 SlotIndex);
 
	/** Remove a ability de um slot ativo. */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "SkillTree")
	void Server_UnequipSkillFromSlot(int32 SlotIndex);

	/** Equipa a skill de uma arma num slot de arma (0=primário, 1=secundário). */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "SkillTree")
	void Server_EquipWeaponSkill(const FName& NodeID, int32 SlotIndex);

	/** Remove a skill de arma de um slot de arma. */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "SkillTree")
	void Server_UnequipWeaponSkill(int32 SlotIndex);
	
	UFUNCTION(Client, Reliable, Category = "SkillTree")
	void Client_NotifySkillUpgraded(const FName& NodeID, int32 NewRank);
	
	// =====================================================================
	// CRAFT — RECEITAS CONHECIDAS
	// =====================================================================
	//
	// Container de tags das receitas que este player ja aprendeu.
	// Persistido no Profile Save (separado do World Save) para que as
	// receitas viagem com o personagem entre mundos diferentes.
	//
	// REPLICACAO:
	// Replicado para todos os clientes via OnRep_KnownRecipeTags.
	// O OnRep dispara OnRecipeLearned para cada tag nova detectada.
	//
	// FLUXO DE APRENDIZADO:
	//   Gatilho (scroll, quest, dialogo, level up)
	//   → Server_LearnRecipe(Tag) no PlayerState do jogador que descobriu
	//   → Propaga para todos os outros players online (LearnRecipe interno)
	//   → Replica para clientes via OnRep
	//   → UI atualiza via OnRecipeLearned
	//
	// NOTA: Usado como filtro pela UI de craft via
	// UZfCraftingSubsystem::GetAvailableRecipesForPlayer.
	// =====================================================================

	/** Retorna (por referencia) o container de tags de receitas conhecidas. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player|Craft")
	const FGameplayTagContainer& GetKnownRecipeTags() const { return KnownRecipeTags; }

	/** Retorna true se o player ja aprendeu a receita com a tag dada. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player|Craft")
	bool HasLearnedRecipe(const FGameplayTag& RecipeTag) const;

	/**
	 * RPC: cliente pede ao servidor para registrar uma receita como aprendida.
	 * O servidor valida, adiciona a tag e PROPAGA para todos os players online.
	 *
	 * Tipicamente chamado via um dos gatilhos de descoberta:
	 *   - Uso de scroll/item de receita
	 *   - Conclusao de quest
	 *   - Dialogo com NPC
	 *   - Subir de nivel
	 *   - Trigger de exploracao
	 *
	 * @param RecipeTag — tag da receita descoberta
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Player|Craft")
	void Server_LearnRecipe(FGameplayTag RecipeTag);

	/** Delegate disparada quando uma receita e aprendida (replicada para a UI). */
	UPROPERTY(BlueprintAssignable, Category = "Player|Craft")
	FOnRecipeLearned OnRecipeLearned;

	// =====================================================================
	// CRAFT — RESULTADO DE TENTATIVA
	// =====================================================================
	//
	// O UZfCraftingComponent vive num NPC nao-possuido pelo player, entao
	// Client RPC direto la nao funciona (nao ha NetConnection propria).
	// A rota correta e passar pelo PlayerController/PlayerState do
	// instigator, que tem conexao natural com seu cliente.
	//
	// FLUXO:
	//   1. Cliente clica craftar na UI
	//   2. UI chama Server_RequestCraft no CraftingComponent do NPC
	//   3. Servidor valida e executa
	//   4. Servidor chama Client_ReceiveCraftResult no PlayerState do
	//      jogador originador
	//   5. Cliente do jogador recebe e faz broadcast de OnCraftResultReceived
	//   6. UI escuta o delegate e atualiza o feedback
	// =====================================================================

	/** Envia o resultado do craft para o cliente originador. Chamado pelo CraftingComponent. */
	UFUNCTION(Client, Reliable, Category = "Player|Craft")
	void Client_ReceiveCraftResult(const FZfCraftResult& Result);

	// =====================================================================
	// CRAFT — ROTEAMENTO DE REQUEST
	// =====================================================================
	//
	// RPC roteador: cliente chama no PROPRIO PlayerState (que ele possui —
	// tem NetConnection natural). O PlayerState, ja no servidor, chama
	// CraftingComponent->RequestCraft diretamente.
	//
	// Isso resolve o problema de "No owning connection" que aconteceria
	// se o cliente tentasse chamar RPC direto no componente do NPC,
	// ja que NPCs nao sao possuidos por players.
	// =====================================================================

	/** RPC: cliente pede ao servidor para executar um craft no NPC dado. */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Player|Craft")
	void Server_RequestCraft(UZfCraftingComponent* CraftingComponent, FGameplayTag RecipeTag);

	/** Delegate disparada no cliente ao receber o resultado de um craft. */
	UPROPERTY(BlueprintAssignable, Category = "Player|Craft")
	FOnCraftResultReceived OnCraftResultReceived;
	
	// =============================================================================
	// ROTEAMENTO PARA TRANSFERIR ITENS
	// =============================================================================

	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Refinery")
	void Server_RequestAddItem(UObject* TargetReceiver, UObject* ItemComesFrom, UZfItemInstance* InItemInstance, int32 AmountToAdd,
		int32 SlotIndexComesFrom, int32 TargetSlotIndex,
		EZfRefinerySlotType SlotTypeComesFrom, EZfRefinerySlotType TargetSlotType,
		FGameplayTag SlotTagComesFrom, FGameplayTag TargetSlotTag);

	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Refinery")
	void Server_RequestRemoveItem(UObject* TargetReceiver, int32 ItemAmountToRemove, int32 TargetSlotIndex, EZfRefinerySlotType TargetSlotType, FGameplayTag TargetSlotTag);

	// =============================================================================
	// DROPAR ITENS
	// =============================================================================

	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Inventory")
	void Server_RequestDropItem(UObject* ItemComesFrom, UZfItemInstance* InItemInstance, int32 ItemAmountToRemove, int32 TargetSlotIndex, EZfRefinerySlotType TargetSlotType, FGameplayTag TargetSlotTag);
	
	// =====================================================================
	// UNIQUE ITEMS
	// =====================================================================

	// Container de tags de itens unicos ja usados por este player.
	// Replicado — cliente precisa saber pra UI desabilitar o botao "Usar".
	UPROPERTY(ReplicatedUsing = OnRep_UsedUniqueItemTags)
	FGameplayTagContainer UsedUniqueItemTags;

	UFUNCTION()
	void OnRep_UsedUniqueItemTags();

	// Checa se o player ja usou este item unico.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player|Items")
	bool HasUsedUniqueItem(FGameplayTag UniqueItemTag) const;

	// RPC — registra o uso de um item unico (chamado pela ZfGA_UseItem).
	UFUNCTION(Server, Reliable)
	void Server_RegisterUniqueItemUsed(FGameplayTag UniqueItemTag);
	

protected:

	/**
	 * Adiciona a tag ao container localmente (servidor).
	 * Chamado por:
	 *  - Server_LearnRecipe no PlayerState do descobridor
	 *  - Propagacao vinda do PlayerState do descobridor para os demais
	 *
	 * Nao propaga — propagacao e responsabilidade do Server_LearnRecipe.
	 * @param RecipeTag — tag a adicionar
	 * @return true se foi adicionada (nao estava la); false se ja existia
	 */
	bool LearnRecipe_Internal(const FGameplayTag& RecipeTag);

	/** OnRep do KnownRecipeTags — detecta tags novas e dispara OnRecipeLearned. */
	UFUNCTION()
	void OnRep_KnownRecipeTags(const FGameplayTagContainer& OldTags);

private:

	/** Tags das receitas que o player aprendeu. Replicado para todos. */
	UPROPERTY(ReplicatedUsing = OnRep_KnownRecipeTags)
	FGameplayTagContainer KnownRecipeTags;
};