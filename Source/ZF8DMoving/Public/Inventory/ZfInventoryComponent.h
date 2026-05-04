// Copyright ZfGame Studio. All Rights Reserved.
// ZfInventoryComponent.h
// Componente de inventário do jogador.
//
// CONCEITO:
// O InventoryComponent é um UActorComponent responsável por:
// - Armazenar todos os ItemInstances do jogador via FFastArraySerializer
// - Gerenciar slots (adicionar, remover, mover, reorganizar)
// - Comunicar com o EquipmentComponent para equipar/desequipar
// - Garantir replicação correta em multiplayer
//
// REPLICAÇÃO:
// O FZfInventoryList usa FFastArraySerializer — replica apenas
// o que mudou (delta), sendo muito eficiente em rede.
// Todas as operações de escrita devem ocorrer no servidor.
// Clientes fazem requisições via RPCs.
//
// SLOTS:
// - Slots são identificados por índice (0 a MaxSlots-1)
// - Slots podem estar vazios (ItemInstance = nullptr)
// - Não há obrigatoriedade de sequência — slots podem ter
//   itens em qualquer posição independente dos anteriores
// - O tamanho pode expandir via UZfFragment_InventoryExpansion
//
// COMUNICAÇÃO COM EQUIPMENTCOMPONENT:
// O InventoryComponent se comunica diretamente com o
// UZfEquipmentComponent no mesmo ator para equipar/desequipar.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ZfInventoryTypes.h"
#include "ZfItemInstance.h"
#include "Inventory/ZfInventoryReceiverInterface.h"
#include "Items/ZfItemPickup.h"
#include "ZfInventoryComponent.generated.h"

enum class EZfRefinerySlotType : uint8;
// Forward declarations
class UZfEquipmentComponent;
class UZfItemInstance;
class UZfItemDefinition;
class UZfItemPickup;

// ============================================================
// DELEGATES
// Broadcast para notificar a UI e outros sistemas sobre
// mudanças no inventário sem criar dependências diretas.
// ============================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventorySizeChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryRefreshed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemRemovedFromInventory, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemAddedToInventory, UZfItemInstance*, ItemInstance, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemStackChanged, UZfItemInstance*, ItemInstance, int32, SlotIndex);


// ============================================================
// UZfInventoryComponent
// ============================================================

UENUM(BlueprintType)
enum class EZfInventorySortType : uint8
{
    Alphabetical    UMETA(DisplayName = "Alphabetical"),
    ByItemType      UMETA(DisplayName = "By Item Type"),
    ByRarity        UMETA(DisplayName = "By Rarity"),
    ByTier          UMETA(DisplayName = "By Tier"),
    ByQuantity      UMETA(DisplayName = "By Quantity"),
    ByQuality       UMETA(DisplayName = "By Quality"),
    Compact         UMETA(DisplayName = "Compact"),
};

// -----------------------------------------------------------
// FZfInventorySlot
// Um slot no inventário com seu índice e item.
// Herda de FFastArraySerializerItem para replicação delta eficiente.
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfInventorySlot : public FFastArraySerializerItem
{
    GENERATED_BODY()

    // Índice único deste slot no inventário (0 a MaxSlots-1)
    // INDEX_NONE = slot inválido
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Slot")
    int32 SlotIndex = INDEX_NONE;

    // Item neste slot (nullptr = vazio)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Slot")
    TObjectPtr<UZfItemInstance> ItemInstance = nullptr;

    // Callbacks do FastArraySerializer — notificam mudanças nos clientes
    void PreReplicatedRemove(const struct FZfInventoryList& InArraySerializer);
    void PostReplicatedAdd(const FZfInventoryList& InArraySerializer);
    void PostReplicatedChange(const FZfInventoryList& InArraySerializer);
};

// -----------------------------------------------------------
// FZfInventoryList
// FastArray de slots do inventário.
// Usa replicação delta — só envia o que mudou pela rede.
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfInventoryList : public FFastArraySerializer
{
    GENERATED_BODY()

    // Todos os slots do inventário
    UPROPERTY()
    TArray<FZfInventorySlot> Slots;

    // Referência ao componente dono — usado nos callbacks de replicação
    // NotReplicated pois é apenas referência local
    UPROPERTY(NotReplicated)
    TObjectPtr<UZfInventoryComponent> OwnerComponent = nullptr;

    // Função obrigatória do FFastArraySerializer para replicação delta
    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
    {
        return FastArrayDeltaSerialize<FZfInventorySlot, FZfInventoryList>(Slots, DeltaParams, *this);
    }
};

// Trait obrigatório para habilitar a replicação delta no FZfInventoryList
template<>
struct TStructOpsTypeTraits<FZfInventoryList> : public TStructOpsTypeTraitsBase2<FZfInventoryList>
{
    enum { WithNetDeltaSerializer = true };
};

UCLASS(ClassGroup = (Zf),BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class ZF8DMOVING_API UZfInventoryComponent : public UActorComponent, public IZfInventoryReceiverInterface
{
    GENERATED_BODY()

    friend class UZfEquipmentComponent;
    friend class UZfRefineryComponent;

    virtual void AddItemToTargetInterface_Implementation(UObject* ItemComesFrom, UZfItemInstance* InItemInstance, int32 AmountToAdd,
        int32 SlotIndexComesFrom, int32 TargetSlotIndex,
        EZfRefinerySlotType SlotTypeComesFrom, EZfRefinerySlotType TargetSlotType,
        FGameplayTag SlotTagComesFrom, FGameplayTag TargetSlotTag) override;
    virtual void RemoveItemFromTargetInterface_Implementation(int32 ItemAmountToRemove, int32 TargetSlotIndex, EZfRefinerySlotType TargetSlotType, FGameplayTag TargetSlotTag) override;
    
private:

    // ============================================================
    // CONFIGURAÇÃO
    // ============================================================

    // Número inicial de slots do inventário.
    // Pode ser expandido via UZfFragment_InventoryExpansion (mochila).
    UPROPERTY()
    int32 DefaultSlotCount = 5;

    // Número máximo absoluto de slots — nunca ultrapassa esse valor
    // mesmo com múltiplas mochilas equipadas.
    UPROPERTY()
    int32 MaxAbsoluteSlotCount = 100;
        
    // ============================================================
    // DADOS REPLICADOS
    // ============================================================

    // Lista de slots do inventário — replicada via FastArraySerializer
    UPROPERTY(Replicated)
    FZfInventoryList InventoryList;

    // Tamanho atual do inventário (número de slots disponíveis)
    // Replicado para que a UI do cliente possa exibir corretamente
    UPROPERTY(Replicated)
    int32 CurrentSlotCount = 0;
    
    // Referência ao EquipmentComponent no mesmo ator.
    // Resolvida no BeginPlay — não replicada.
    UPROPERTY()
    TObjectPtr<UZfEquipmentComponent> EquipmentComponent;

public:

    UZfInventoryComponent();
    
    // Registra as propriedades replicadas do componente
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void BeginPlay() override;
    
    // ============================================================
    // DELEGATES — UI e outros sistemas se inscrevem aqui
    // ============================================================

    // Chamado quando item é adicionado
    UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
    FOnItemAddedToInventory OnItemAdded;

    // Chamado quando item é removido
    UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
    FOnItemRemovedFromInventory OnItemRemoved;

    // Chamado quando o tamanho do inventário muda
    UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
    FOnInventorySizeChanged OnInventorySizeChanged;

    // Chamado após reorganização ou ordenação completa
    UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
    FOnInventoryRefreshed OnInventoryRefreshed;

    // Chamado Quando Stack Muda
    UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
    FOnItemStackChanged OnItemStackChanged;

    // ============================================================
    // FUNÇÕES SERVER - GERENCIAMENTO
    // ============================================================

    // ============================================================
    // FUNÇÕES DE CONSULTA
    // ============================================================

    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    int32 GetDefaultSlotCount() const { return DefaultSlotCount; }

    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    int32 GetMaxAbsoluteSlotCount() const { return MaxAbsoluteSlotCount; }

    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    int32 GetCurrentSlotCount() const { return CurrentSlotCount; }
    
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    FZfInventoryList GetInventoryList() const { return InventoryList; }

    
    // Retorna o ItemInstance em um slot específico.
    // Retorna nullptr se o slot estiver vazio ou inválido.
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    UZfItemInstance* GetItemAtSlot(int32 SlotIndex) const;

    // Retorna o índice do slot de um ItemInstance.
    // Retorna INDEX_NONE se não encontrado.
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    int32 GetSlotIndexOfItem(UZfItemInstance* ItemInstance) const;

    // Retorna o primeiro slot vazio disponível.
    // Retorna INDEX_NONE se o inventário estiver cheio.
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    int32 GetFirstEmptySlot() const;

    // Retorna todos os itens que possuem uma tag específica.
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    TArray<UZfItemInstance*> GetItemsByTag(const FGameplayTag& Tag) const;

    // Retorna o número de slots atualmente disponíveis (vazios).
    //@param int32 - Para verificar quantos itens tem na mochila passe 
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    int32 GetAvailableSlots() const;

    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    int32 GetAvailableDefaultSlots() const;
    
    // Retorna quantos slots estão disponíveis considerando a capacidade padrão
    // mais os slots extras da mochila sendo equipada.
    // @param BackpackInstance — instância da mochila sendo equipada
    // @return quantidade de slots livres no range expandido
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    int32 GetAvailableSlotsWithExpansion(UZfItemInstance* BackpackInstance) const;

    // Retorna quantos itens existem a partir de um slot específico até o final.
    // Útil para verificar se há itens nos slots que serão perdidos
    // ao trocar uma mochila maior por uma menor.
    // @param InitialSlotIndex — slot inicial para contar
    // @return quantidade de itens nos slots a partir de FromSlotIndex
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    int32 GetItemCountFromInitialSlot(int32 InitialSlotIndex) const;

    // Move itens dos slots acima da nova capacidade para os primeiros slots livres.
    // @param NewCapacity — nova capacidade total após troca de mochila
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory")
    void RelocateItemsAboveCapacity(int32 NewCapacity);
    
    // Retorna o número total de slots do inventário (incluindo ocupados).
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    int32 GetTotalSlots() const;

    // Verifica se um slot está vazio.
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    bool IsSlotEmpty(int32 SlotIndex) const;

    // Retorna todos os ItemInstances do inventário (sem slots vazios).
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    TArray<UZfItemInstance*> GetAllItems() const;

    // Verifica se um índice de slot é válido.
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    bool IsValidSlotIndex(int32 SlotIndex) const;

    FZfInventorySlot* FindSlotByIndex(int32 SlotIndex);
    
    // Adiciona um item do inventário pela instancia.
    // @param ItemInstance — item adicionado
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Inventory")
    void ServerTryAddItem(UObject* ItemComesFrom, UZfItemInstance* InItemInstance, int32 AmountToAdd,
        int32 SlotIndexComesFrom, int32 TargetSlotIndex,
        EZfRefinerySlotType SlotTypeComesFrom, EZfRefinerySlotType TargetSlotType,
        FGameplayTag SlotTagComesFrom, FGameplayTag TargetSlotTag);
   
    // Remove um item do inventário pelo índice do slot.
    // @param SlotIndex — slot a remover
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Inventory")
    void ServerTryRemoveItem(int32 ItemAmountToRemove, int32 TargetSlotIndex, EZfRefinerySlotType TargetSlotType, FGameplayTag TargetSlotTag);

    // Adiciona um item do inventário pela instancia.
    // @param ItemInstance — item adicionado
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Inventory")
    void ServerTryAddItemToInventory(UZfItemInstance* ItemInstance);

    // Remove um item do inventário pelo índice do slot.
    // @param SlotIndex — slot a remover
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Inventory")
    void ServerTryRemoveItemFromInventory(int32 SlotIndex);

    // Remove uma quantidade de um item stackável.
    // Remove o item inteiro se a quantidade zerar.
    // @param ItemInstance — item stackável a remover
    // @param Amount — quantidade a remover
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Inventory")
    void ServerTryRemoveAmountFromStack(UZfItemInstance* ItemInstance, int32 Amount);
    
    // Divide um stack em dois.
    // @param FromSlotIndex — slot de origem
    // @param ToSlotIndex   — slot de destino (pode ser INDEX_NONE = primeiro slot livre)
    // @param Amount        — quantidade a separar do stack original
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Inventory")
    void ServerTrySplitStack(int32 FromSlotIndex, int32 ToSlotIndex, int32 Amount);

    // Dropa item do inventário
    //@param SlotIndex — Index do item no inventário
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Inventory|RPC")
    void ServerTryDropItem(UZfItemInstance* ItemInstance, int32 SlotIndex = -1);

    
    // Requisição do cliente para ordenar o inventário
    // @param EZfInventorySortType — Tipo de Reorganização
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Inventory")
    void ServerTrySortInventory(EZfInventorySortType SortType);

    // Dispara o evento Item.Event.Use para o item no slot dado.
    // A ZfGA_UseItem sera ativada automaticamente via trigger.
    // Chamado pelo context action do menu de inventario.
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Inventory")
    void ServerTryUseItemAtSlot(int32 SlotIndex);

    UFUNCTION(Category = "Zf|Inventory")
    EZfItemMechanicResult TryPickupItem(UZfItemInstance* ItemInstance, AZfItemPickup* ItemPickup);
    

private:
    
    // ============================================================
    // FUNÇÕES PRINCIPAIS - GERENCIAMENTO
    // ============================================================

    // Tenta adicionar um item ao inventário.
    // Se o item é stackável e já existe stack do mesmo tipo,
    // tenta empilhar antes de ocupar novo slot.
    // Deve ser chamado apenas no servidor.
    // @param ItemInstance — instância do item a adicionar
    // @return resultado da operação
    UFUNCTION(Category = "Zf|Inventory")
    EZfItemMechanicResult TryAddItem(UObject* ItemComesFrom, UZfItemInstance* InItemInstance, int32 AmountToAdd,
        int32 SlotIndexComesFrom, int32 TargetSlotIndex,
        EZfRefinerySlotType SlotTypeComesFrom, EZfRefinerySlotType TargetSlotType,
        FGameplayTag SlotTagComesFrom, FGameplayTag TargetSlotTag);
   
    // Remove um item do inventário pelo índice do slot.
    // @param SlotIndex — slot a remover
    // @return resultado da operação
    UFUNCTION(Category = "Zf|Inventory")
    EZfItemMechanicResult TryRemoveItem(int32 ItemAmountToRemove, int32 TargetSlotIndex, EZfRefinerySlotType TargetSlotType, FGameplayTag TargetSlotTag);

    // Tenta adicionar um item ao inventário.
    // Se o item é stackável e já existe stack do mesmo tipo,
    // tenta empilhar antes de ocupar novo slot.
    // Deve ser chamado apenas no servidor.
    // @param ItemInstance — instância do item a adicionar
    // @return resultado da operação
    UFUNCTION(Category = "Zf|Inventory")
    EZfItemMechanicResult TryAddItemToInventory(UZfItemInstance* ItemInstance);

    // Remove um item do inventário pelo índice do slot.
    // @param SlotIndex — slot a remover
    // @return resultado da operação
    UFUNCTION(Category = "Zf|Inventory")
    EZfItemMechanicResult TryRemoveItemFromInventory(int32 SlotIndex);

    // Tenta adicionar um item a um slot específico do inventário.
    // @param ItemInstance — item a adicionar
    // @param TargetSlotIndex — slot alvo
    // @return resultado da operação
    UFUNCTION(Category = "Zf|Inventory")
    EZfItemMechanicResult TryAddItemToSpecificSlot(UZfItemInstance* InItemInstance, int32 TargetSlotIndex);
    
    // Remove uma quantidade de um item stackável.
    // Remove o item inteiro se a quantidade zerar.
    // @param ItemInstance — item stackável a remover
    // @param Amount — quantidade a remover
    // @return resultado da operação
    UFUNCTION(Category = "Zf|Inventory")
    EZfItemMechanicResult TryRemoveAmountFromStack(UZfItemInstance* ItemInstance, int32 Amount);
    
    // Tenta adicionar ao stack de um item já existente no slot especificado.
    // Retorna o overflow (quantidade que não coube). 0 = tudo coube.
    UFUNCTION(Category = "Zf|Refinery")
    int32 TryAddToStack(int32 SlotIndex, int32 Amount);
    
    UFUNCTION(Category = "Zf|Inventory")
    void TrySplitStack(int32 FromSlotIndex, int32 ToSlotIndex, int32 Amount);

    // Spawna pickupclass com inicialização do ItemInstance
    // @param ItemInstance — Informações do Item para o pickup
    UFUNCTION(Category = "Zf|Inventory")
    void TrySpawnPickupItem(UZfItemInstance* ItemInstance) const;

    // Dispara o evento Item.Event.Use para o item no slot dado.
    // A ZfGA_UseItem sera ativada automaticamente via trigger.
    // Chamado pelo context action do menu de inventario.
    UFUNCTION(Category = "Zf|Inventory")
    void TryUseItemAtSlot(int32 SlotIndex);
    
    // Atualiza o CurrentSlotCount baseado na mochila atualmente equipada.
    UFUNCTION(Category = "Zf|Inventory")
    void UpdateSlotCountFromEquippedBackpack();

    // Tenta Pegar Item do chão
    UFUNCTION(Category = "Zf|Inventory")
    EZfItemMechanicResult TryAddItemFromPickup(UZfItemInstance* ItemInstance, AZfItemPickup* ItemPickup);

    // ============================================================
    // FUNÇÕES INTERNAS - GERENCIAMENTO
    // ============================================================

    // Adiciona o item do inventário a partir do ItemInstance.
    // @param ItemInstance - Item a ser Adicionado
    // @param int32 - Slot do Item a ser Adicionado
    void InternalAddItem(UZfItemInstance* InItemInstance, int32 TargetSlot);

    // Remove o item do inventário a partir do Slot.
    // @param int32 - Item a ser Removido
    void InternalRemoveItem(int32 SlotIndex);

    // Tenta fazer stack de um item com stacks existentes.
    // Retorna true se o item foi completamente absorvido por stacks.
    // @param ItemInstance — item a empilhar
    int32 InternalTryStackWithExistingItems(UZfItemInstance* ItemInstance, int32 AmountToRemove);

    // Move um item de um slot para outro.
    // Se o slot destino tiver item, troca os dois de lugar.
    // @param FromSlotIndex — slot de origem
    // @param ToSlotIndex — slot de destino
    // @return resultado da operação
    UFUNCTION()
    EZfItemMechanicResult InternalMoveItemBetweenSlots(int32 FromSlotIndex, int32 ToSlotIndex);

    
    // ============================================================
    // FUNÇÕES INTERNAS - ORGANIZAÇÃO
    // ============================================================
    
    // Ordena o inventário de acordo com o tipo selecionado.
    UFUNCTION(Category = "Zf|Inventory|Sort")
    void InternalSortInventoryBySelected(EZfInventorySortType SortType);
    
    // ============================================================
    // FUNÇÕES INTERNAS - VALIDAÇÃO
    // ============================================================
    
    // Valida se uma operação pode ser executada no servidor.
    // Loga warning se chamada no cliente.
    bool InternalCheckIsServer(const FString& FunctionName) const;
};