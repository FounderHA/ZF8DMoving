// Copyright ZfGame Studio. All Rights Reserved.
// ZfRefineryComponent.h
// Componente principal do sistema de refinaria. Toda a lógica roda no servidor.
//
// CONCEITO:
// O componente é instalado no ator da refinadora no mundo.
// Ele gerencia três conjuntos de slots independentes:
// - Input:    itens a serem refinados
// - Output:   itens já refinados aguardando retirada
// - Catalyst: itens que aceleram o tempo de refino
//
// FLUXO DE REFINO:
// 1. Player insere itens no slot de input via Server RPC
// 2. O componente testa as receitas disponíveis (fila manual → automático)
// 3. Encontrada uma receita satisfeita, inicia o ciclo:
//    a. Registra LastSnapshotTime e agenda o FTimerHandle de disparo único
//    b. Se há catalisador disponível, consome e ativa o boost
// 4. Timer dispara → processa output (roll de bônus) → tenta próximo ciclo
//
// SISTEMA DE TEMPO:
// TempoEfetivo = BaseCraftTime / SpeedMultiplier
// O timer é agendado pelo tempo restante (TempoEfetivo - RecipeElapsedTime).
// Quando o multiplicador muda (novo catalisador ou boost acabou):
//   RecipeElapsedTime += (Agora - LastSnapshotTime) * MultiplicadorAnterior
//   LastSnapshotTime = Agora
//   Timer é cancelado e reagendado com o novo tempo restante.
//
// CATALISADOR:
// Consumido no momento que o boost começa (não ao inserir no slot).
// O item é removido do slot de catalisador de menor índice disponível.
// O boost só desconta enquanto o refinador está ativamente processando.
// Quando RemainingBoostDuration chega a 0, tenta consumir o próximo.
//
// SLOTS:
// Slots só existem quando têm item — sem pré-alocação, igual ao inventário.
// SlotIndex é o identificador do slot, não a posição no array.
// A capacidade máxima é definida no RefineryData e armazenada em
// FZfRefinerySlotList::Capacity para validação de inserção e
// para encontrar o primeiro slot livre.
//
// PRIORIDADE DE RECEITAS:
// 1. Fila manual do player (ManualRecipeQueue) — testada em ordem
// 2. Fallback automático — ordena por Priority desc, depois por
//    número de ingredientes desc para desempate
//
// REPLICAÇÃO:
// Slots replicados via FFastArraySerializer (delta eficiente).
// Estado do refino replicado para o cliente calcular progresso na UI.

#pragma once

#include "CoreMinimal.h"
#include "ZfRefineryTypes.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Inventory/ZfInventoryReceiverInterface.h"
#include "ZfRefineryComponent.generated.h"

// Forward declarations
class UZfRefineryData;
class UZfRefineryRecipe;
class UZfItemInstance;
class UZfItemDefinition;
class UZfFragment_Catalyst;
class UZfInventoryComponent;

// ============================================================
// FAST ARRAY — SLOT DE REFINARIA
// Slots só existem quando têm item — sem pré-alocação.
// SlotIndex é o identificador do slot, não a posição no array.
// ============================================================

USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfRefinerySlot : public FFastArraySerializerItem
{
	GENERATED_BODY()

	// Identificador único deste slot (0 a Capacity-1)
	UPROPERTY(BlueprintReadOnly)
	int32 SlotIndex = INDEX_NONE;

	// Item neste slot
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UZfItemInstance> ItemInstance = nullptr;

	void PreReplicatedRemove(const struct FZfRefinerySlotList& InArraySerializer);
	void PostReplicatedAdd(const struct FZfRefinerySlotList& InArraySerializer);
	void PostReplicatedChange(const struct FZfRefinerySlotList& InArraySerializer);
};

USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfRefinerySlotList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FZfRefinerySlot> Slots;

	// Referência ao componente dono — usada nos callbacks de replicação
	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent = nullptr;

	// Tipo deste slot — define filtros e efeitos colaterais nas funções genéricas
	UPROPERTY(NotReplicated)
	EZfRefinerySlotType SlotType = EZfRefinerySlotType::Input;

	// Capacidade máxima — definida no BeginPlay via RefineryData.
	// Usada para validar inserção e encontrar o primeiro slot livre.
	UPROPERTY(NotReplicated)
	int32 Capacity = 0;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FastArrayDeltaSerialize<FZfRefinerySlot, FZfRefinerySlotList>(Slots, DeltaParams, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FZfRefinerySlotList> : public TStructOpsTypeTraitsBase2<FZfRefinerySlotList>
{
	enum { WithNetDeltaSerializer = true };
};


// ============================================================
// DELEGATES
// ============================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRefineryStateChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRefineryCycleCompleted, UZfRefineryRecipe*, Recipe);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCatalystConsumed, float, NewSpeedMultiplier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRefineryItemRemoved, EZfRefinerySlotType, SlotType, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnRefinerySlotChanged, EZfRefinerySlotType, SlotType, UZfItemInstance*, ItemInstance, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnRefineryItemAdded, EZfRefinerySlotType, SlotType, UZfItemInstance*, ItemInstance, int32, SlotIndex);

// ============================================================
// UZfRefineryComponent
// ============================================================

UCLASS(ClassGroup = (Refinery), meta = (BlueprintSpawnableComponent))
class ZF8DMOVING_API UZfRefineryComponent : public UActorComponent, public IZfInventoryReceiverInterface
{
	GENERATED_BODY()

public:

	UZfRefineryComponent();

protected:

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void AddItemToTargetInterface_Implementation(UObject* ItemComesFrom, UZfItemInstance* InItemInstance, int32 AmountToAdd,
		int32 SlotIndexComesFrom, int32 TargetSlotIndex,
		EZfRefinerySlotType SlotTypeComesFrom, EZfRefinerySlotType TargetSlotType,
		FGameplayTag SlotTagComesFrom, FGameplayTag TargetSlotTag) override;
	virtual void RemoveItemFromTargetInterface_Implementation(int32 ItemAmountToRemove, int32 TargetSlotIndex, EZfRefinerySlotType TargetSlotType, FGameplayTag TargetSlotTag) override;
	
public:
	
	// ============================================================
	// CONFIGURAÇÃO
	// ============================================================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refinery|Config")
	TObjectPtr<UZfRefineryData> RefineryData;

	// ============================================================
	// DELEGATES
	// ============================================================

	UPROPERTY(BlueprintAssignable, Category = "Refinery|Events")
	FOnRefineryStateChanged OnRefineryStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Refinery|Events")
	FOnRefineryCycleCompleted OnRefineryCycleCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Refinery|Events")
	FOnCatalystConsumed OnCatalystConsumed;

	UPROPERTY(BlueprintAssignable, Category = "Refinery|Events")
	FOnRefinerySlotChanged OnRefinerySlotChanged;

	UPROPERTY(BlueprintAssignable, Category = "Refinery|Events")
	FOnRefinerySlotChanged OnRefineryItemAdded;

	UPROPERTY(BlueprintAssignable, Category = "Refinery|Events")
	FOnRefineryItemRemoved OnRefineryItemRemoved;
	
	// ============================================================
	// SERVER RPCs — SLOTS
	// ============================================================

	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Refinery|RPC")
	void ServerTryAddItem(UObject* ItemComesFrom, UZfItemInstance* InItemInstance, int32 AmountToAdd,
		int32 SlotIndexComesFrom, int32 TargetSlotIndex,
		EZfRefinerySlotType SlotTypeComesFrom, EZfRefinerySlotType TargetSlotType,
		FGameplayTag SlotTagComesFrom, FGameplayTag TargetSlotTag);

	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Refinery|RPC")
	void ServerTryRemoveItem(int32 ItemAmountToRemove, int32 TargetSlotIndex, EZfRefinerySlotType TargetSlotType, FGameplayTag TargetSlotTag);
	
	// ============================================================
	// SERVER RPCs — FILA MANUAL DE RECEITAS
	// ============================================================

	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Refinery|RPC")
	void ServerTrySetManualRecipeQueue(const TArray<UZfRefineryRecipe*>& NewQueue);

	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Refinery|RPC")
	void ServerTryClearManualRecipeQueue();

	// ============================================================
	// API PÚBLICA — SLOTS (genéricas, servidor apenas)
	// ============================================================

	// Tenta inserir um item no slot especificado de qualquer SlotList.
	// Trata stackable, overflow e sincronização com o inventário do player.
	// @param SlotIndex     — índice alvo; INDEX_NONE = primeiro slot vazio disponível
	// @param InventoryComp — inventário de origem (opcional); usado para debitar o stack
	UFUNCTION(Category = "Zf|Refinery")
	void TryAddItem(UObject* ItemComesFrom, UZfItemInstance* InItemInstance, int32 AmountToAdd,
		int32 SlotIndexComesFrom, int32 TargetSlotIndex,
		EZfRefinerySlotType SlotTypeComesFrom, EZfRefinerySlotType TargetSlotType,
		FGameplayTag SlotTagComesFrom, FGameplayTag TargetSlotTag);

	// Tenta Remover quantidade do Item, se for tudo, remove Item.
	// @param OriginalSlotIndexBeforeSplit — slot de origem antes do Split
	// @param Amount        — quantidade a separar do stack original
	UFUNCTION(Category = "Zf|Inventory")
	void TryRemoveItem(int32 ItemAmountToRemove, int32 TargetSlotIndex, EZfRefinerySlotType TargetSlotType, FGameplayTag TargetSlotTag);
	
	// ============================================================
	// API PÚBLICA — FILA MANUAL DE RECEITAS
	// ============================================================

	UFUNCTION(BlueprintCallable, Category = "Zf|Refinery")
	void SetManualRecipeQueue(const TArray<UZfRefineryRecipe*>& NewQueue);

	UFUNCTION(BlueprintCallable, Category = "Zf|Refinery")
	void ClearManualRecipeQueue();

	// ============================================================
	// API PÚBLICA — QUERIES (cliente ou servidor)
	// ============================================================

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|Refinery|Query")
	bool IsRefining() const { return bIsRefining; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|Refinery|Query")
	UZfRefineryRecipe* GetActiveRecipe() const { return ActiveRecipe; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|Refinery|Query")
	FZfActiveCatalystState GetActiveCatalystState() const { return ActiveCatalystState; }

	// Retorna o progresso atual do ciclo (0.0 a 1.0) — seguro chamar no cliente.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|Refinery|Query")
	float GetCurrentCycleProgress() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|Refinery|Query")
	float GetCurrentCycleTotalTime() const { return CurrentCycleTotalTime; }

	// Retorna o item de um slot pelo SlotIndex (nullptr se não existir).
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|Refinery|Query")
	UZfItemInstance* GetSlotItem(const FZfRefinerySlotList& SlotList, int32 SlotIndex) const;

	// Retorna o primeiro SlotIndex livre dentro da Capacity do SlotList.
	// Retorna INDEX_NONE se todos os slots estiverem ocupados.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|Refinery|Query")
	int32 GetFirstEmptySlot(const FZfRefinerySlotList& SlotList) const;

	// Resolve o SlotList correto a partir do tipo de slot.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|Refinery|Query")
	FZfRefinerySlotList& GetSlotListByType(EZfRefinerySlotType SlotType);

	// Retorna o primeiro SlotIndex com o mesmo ItemDefinition e stack não cheio.
	// Retorna INDEX_NONE se não encontrar.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|Refinery|Query")
	int32 FindPartialStack(const FZfRefinerySlotList& SlotList, const UZfItemDefinition* ItemDef) const;

	// Tenta adicionar ao stack de um item já existente no slot especificado.
	// Retorna o overflow (quantidade que não coube). 0 = tudo coube.
	UFUNCTION(BlueprintCallable, Category = "Zf|Refinery")
	int32 TryAddToStack(FZfRefinerySlotList& SlotList, int32 SlotIndex, int32 Amount);
	
	// Verifica se SlotList é Compativel
	UFUNCTION(BlueprintCallable, Category = "Zf|Refinery")
	bool IsItemAllowedAt(FZfRefinerySlotList& SlotList, UZfItemInstance* ItemInstance) const;
	
	

	// ============================================================
	// ACESSO DIRETO ÀS SLOTLISTS — leitura no BP/UI
	// ============================================================

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|Refinery|Query")
	FZfRefinerySlotList& GetInputSlots() { return InputSlots; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|Refinery|Query")
	FZfRefinerySlotList& GetOutputSlots() { return OutputSlots; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|Refinery|Query")
	FZfRefinerySlotList& GetCatalystSlots() { return CatalystSlots; }



private:

	// ============================================================
	// SLOTS — DADOS REPLICADOS
	// ============================================================

	UPROPERTY(Replicated)
	FZfRefinerySlotList InputSlots;

	UPROPERTY(Replicated)
	FZfRefinerySlotList OutputSlots;

	UPROPERTY(Replicated)
	FZfRefinerySlotList CatalystSlots;

	// ============================================================
	// ESTADO DO REFINO — REPLICADO PARA A UI
	// ============================================================

	UPROPERTY(ReplicatedUsing = OnRep_RefineryState)
	bool bIsRefining = false;

	UPROPERTY(ReplicatedUsing = OnRep_RefineryState)
	TObjectPtr<UZfRefineryRecipe> ActiveRecipe = nullptr;

	// Progresso acumulado em segundos efetivos até o último snapshot.
	UPROPERTY(Replicated)
	float RecipeElapsedTime = 0.0f;

	// GetTimeSeconds() no momento do último snapshot.
	// Progresso = RecipeElapsedTime + (Agora - LastSnapshotTime) * Multiplicador
	UPROPERTY(Replicated)
	float LastSnapshotTime = 0.0f;

	// Tempo total efetivo do ciclo atual (BaseCraftTime / SpeedMultiplier).
	UPROPERTY(Replicated)
	float CurrentCycleTotalTime = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_CatalystState)
	FZfActiveCatalystState ActiveCatalystState;

	// ============================================================
	// ESTADO INTERNO — SERVIDOR APENAS
	// ============================================================

	FTimerHandle CycleTimerHandle;
	TArray<TObjectPtr<UZfRefineryRecipe>> ManualRecipeQueue;

	// ============================================================
	// PIPELINE DE REFINO
	// ============================================================

	void TryStartNextCycle();
	void StartCycle(UZfRefineryRecipe* Recipe);
	void StopRefining(bool bSaveProgress = false);
	void OnCycleCompleted();

	// ============================================================
	// SELEÇÃO DE RECEITA
	// ============================================================

	UZfRefineryRecipe* FindNextRecipe() const;
	bool CanSatisfyRecipe(const UZfRefineryRecipe* Recipe) const;
	TArray<UZfRefineryRecipe*> GetSortedRecipes() const;

	// ============================================================
	// SISTEMA DE CATALISADOR
	// ============================================================

	bool TryConsumeCatalyst();
	void UpdateCatalystDuration();

	// ============================================================
	// SNAPSHOT — SISTEMA DE TEMPO
	// ============================================================

	void TakeSnapshot();
	void RescheduleCycleTimer();

	// ============================================================
	// CONSUMO E DEPÓSITO
	// ============================================================

	void ConsumeIngredients(const UZfRefineryRecipe* Recipe);
	bool DepositOutputs(const UZfRefineryRecipe* Recipe);
	UZfItemInstance* CreateItemInstance(TSoftObjectPtr<UZfItemDefinition> ItemDef, int32 Quantity) const;

	// ============================================================
	// HELPERS DE SLOT (internos)
	// ============================================================

	// Cria e adiciona um novo FZfRefinerySlot no array com o item dado.
	void InternalAddItemToSlot(FZfRefinerySlotList& SlotList, UZfItemInstance* ItemInstance, int32 SlotIndex);

	// Remove o FZfRefinerySlot do array pelo SlotIndex. Retorna o item ou nullptr.
	UZfItemInstance* InternalRemoveItemFromSlot(FZfRefinerySlotList& SlotList, int32 SlotIndex);

	// Tenta empilhar o item em stacks existentes do mesmo tipo no SlotList.
	// Retorna true se o item foi completamente absorvido (sem overflow).
	int32 InternalTryStackWithExistingItems(FZfRefinerySlotList& SlotList, const UZfItemInstance* ItemInstance, int32 AmountToRemove);

	// Conta quantos itens de um tipo específico existem no input.
	int32 CountItemsInInput(const UZfItemDefinition* ItemDef) const;

	// Retorna true se o output tem espaço para pelo menos mais um item.
	bool HasOutputSpace() const;

	// Aplica os efeitos colaterais de remoção conforme o SlotType do SlotList.
	void ApplyRemoveSideEffects(FZfRefinerySlotList& SlotList);

	bool CheckIsServer(const FString& FunctionName) const;

	// ============================================================
	// REP NOTIFIES
	// ============================================================

	UFUNCTION()
	void OnRep_RefineryState();

	UFUNCTION()
	void OnRep_CatalystState();
};