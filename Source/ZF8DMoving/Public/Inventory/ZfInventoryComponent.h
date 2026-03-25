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
#include "ZfInventoryComponent.generated.h"

// Forward declarations
class UZfEquipmentComponent;
class UZfItemInstance;
class UZfItemDefinition;

// ============================================================
// DELEGATES
// Broadcast para notificar a UI e outros sistemas sobre
// mudanças no inventário sem criar dependências diretas.
// ============================================================

// Disparado quando um item é adicionado ao inventário
// @param ItemInstance — o item adicionado
// @param SlotIndex — índice do slot onde foi adicionado
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemAddedToInventory, UZfItemInstance*, ItemInstance, int32, SlotIndex);

// Disparado quando um item é removido do inventário
// @param ItemInstance — o item removido
// @param SlotIndex — índice do slot de onde foi removido
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemRemovedFromInventory, UZfItemInstance*, ItemInstance, int32, SlotIndex);

// Disparado quando um item é movido entre slots
// @param ItemInstance — o item movido
// @param FromSlot — slot de origem
// @param ToSlot — slot de destino
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnItemMovedInInventory, UZfItemInstance*, ItemInstance, int32, FromSlot, int32, ToSlot);

// Disparado quando o tamanho do inventário muda
// @param NewSize — novo tamanho total de slots
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventorySizeChanged, int32, NewSize);

// Disparado quando o inventário é completamente atualizado
// (reorganização, ordenação, etc.)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryRefreshed);

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
class ZF8DMOVING_API UZfInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    

    // ----------------------------------------------------------
    // FUNÇÕES PRINCIPAIS — ADICIONAR ITEM
    // ----------------------------------------------------------

    
    
    // ----------------------------------------------------------
    // FUNÇÕES PRINCIPAIS — REMOVER ITEM
    // ----------------------------------------------------------
    
    // ----------------------------------------------------------
    // FUNÇÕES PRINCIPAIS — MOVER ITEM
    // ----------------------------------------------------------
    
    // ----------------------------------------------------------
    // FUNÇÕES PRINCIPAIS — EQUIPAR
    // ----------------------------------------------------------

    // Tenta equipar um item do inventário no EquipmentComponent.
    // Remove o item do inventário e passa para o EquipmentComponent.
    // Faz todas as verificações necessárias (slot válido, two-handed, etc.)
    // @param SlotIndex — slot do item a equipar
    // @return resultado da operação
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory")
    EZfItemMechanicResult TryEquipItemFromSlot(int32 SlotIndex);

    // Tenta equipar um ItemInstance diretamente.
    // @param ItemInstance — item a equipar
    // @return resultado da operação
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory")
    EZfItemMechanicResult TryEquipItem(UZfItemInstance* ItemInstance);

    // Recebe um item desequipado do EquipmentComponent.
    // Tenta colocar no mesmo slot de origem ou no primeiro livre.
    // @param ItemInstance — item que foi desequipado
    // @param PreferredSlotIndex — slot preferido (INDEX_NONE = primeiro livre)
    // @return resultado da operação
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory")
    EZfItemMechanicResult ReceiveUnequippedItem(UZfItemInstance* ItemInstance, int32 PreferredSlotIndex);

    

    // ----------------------------------------------------------
    // FUNÇÕES DE CONSULTA
    // ----------------------------------------------------------
    
    






    
   

    

    

    // ----------------------------------------------------------
    // EXPANSÃO DE SLOTS
    // Chamado pelo EquipmentComponent quando uma mochila é
    // equipada/desequipada via UZfFragment_InventoryExpansion.
    // ----------------------------------------------------------


    // ----------------------------------------------------------
    // RPCs — CLIENT → SERVER
    // Clientes não modificam o inventário diretamente.
    // Enviam requisições ao servidor via RPC.
    // ----------------------------------------------------------
    
    // Requisição do cliente para equipar item de um slot
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Zf|Inventory|RPC")
    void ServerRequestEquipItem(int32 SlotIndex);

    // Requisição do cliente para dropar item no mundo
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Zf|Inventory|RPC")
    void ServerRequestDropItem(int32 SlotIndex);

    

    // ----------------------------------------------------------
    // RPCs — SERVER → CLIENT
    // Notificações do servidor para clientes específicos.
    // ----------------------------------------------------------

    // Notifica o cliente dono que o inventário foi atualizado
    UFUNCTION(Client, Reliable)
    void ClientNotifyInventoryUpdated();

    // Notifica o cliente que uma operação falhou
    // @param FailureResult — motivo da falha
    UFUNCTION(Client, Reliable)
    void ClientNotifyOperationFailed(EZfItemMechanicResult FailureResult);
    

    

    // ----------------------------------------------------------
    // DEBUG
    // ----------------------------------------------------------

    // Loga o estado completo do inventário
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Debug")
    void DebugLogInventory() const;

    // Desenha o estado do inventário na tela via DrawDebugString
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Debug")
    void DrawDebugInventory() const;

protected:

    // Referência ao EquipmentComponent no mesmo ator.
    // Resolvida no BeginPlay — não replicada.
    UPROPERTY()
    TObjectPtr<UZfEquipmentComponent> EquipmentComponent;

private:
    
    // Busca o EquipmentComponent no ator dono.
    // Chamado no BeginPlay.
    void Internal_FindEquipmentComponent();
    


    //============================================================================================================================================================








    

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

    // Chamado quando item é movido entre slots
    UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
    FOnItemMovedInInventory OnItemMoved;

    // Chamado quando o tamanho do inventário muda
    UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
    FOnInventorySizeChanged OnInventorySizeChanged;

    // Chamado após reorganização ou ordenação completa
    UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
    FOnInventoryRefreshed OnInventoryRefreshed;

    

protected:

    // ============================================================
    // CONFIGURAÇÃO
    // ============================================================

    // Número inicial de slots do inventário.
    // Pode ser expandido via UZfFragment_InventoryExpansion (mochila).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Config", meta = (ClampMin = "1", ClampMax = "100"))
    int32 DefaultSlotCount = 20;

    // Número máximo absoluto de slots — nunca ultrapassa esse valor
    // mesmo com múltiplas mochilas equipadas.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Config", meta = (ClampMin = "1", ClampMax = "100"))
    int32 MaxAbsoluteSlotCount = 100;
        
    // ============================================================
    // DADOS REPLICADOS
    // ============================================================

    // Lista de slots do inventário — replicada via FastArraySerializer
    UPROPERTY(Replicated)
    FZfInventoryList InventoryList;

    // Tamanho atual do inventário (número de slots disponíveis)
    // Replicado para que a UI do cliente possa exibir corretamente
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inventory")
    int32 CurrentSlotCount = 0;
    
public:

    // ============================================================
    // FUNÇÕES SERVER - GERENCIAMENTO
    // ============================================================

    // Adiciona um item do inventário pela instancia.
    // @param ItemInstance — item adicionado
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Inventory")
    void ServerTryAddItemToInventory(UZfItemInstance* ItemInstance);
   
    // Remove um item do inventário pelo índice do slot.
    // @param SlotIndex — slot a remover
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Inventory")
    void ServerTryRemoveItemFromInventory(int32 SlotIndex);

    // Move item entre Slots
    // @param int32 — Slot de onde veio seu item
    // @param int32 — Slot onde está indo seu item
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Inventory")
    void ServerTryMoveItem(int32 FromSlotIndex, int32 ToSlotIndex);

    // Remove uma quantidade de um item stackável.
    // Remove o item inteiro se a quantidade zerar.
    // @param ItemInstance — item stackável a remover
    // @param Amount — quantidade a remover
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Inventory")
    void ServerTryRemoveAmountFromStack(UZfItemInstance* ItemInstance, int32 Amount);
    
    // Requisição do cliente para ordenar o inventário
    // @param EZfInventorySortType — Tipo de Reorganização
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Inventory")
    void ServerTrySortInventory(EZfInventorySortType SortType);
    
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
    EZfItemMechanicResult TryAddItemToInventory(UZfItemInstance* ItemInstance);
   
    // Remove um item do inventário pelo índice do slot.
    // @param SlotIndex — slot a remover
    // @return resultado da operação
    UFUNCTION(Category = "Zf|Inventory")
    EZfItemMechanicResult TryRemoveItemFromInventory(int32 SlotIndex);

    // Remove uma quantidade de um item stackável.
    // Remove o item inteiro se a quantidade zerar.
    // @param ItemInstance — item stackável a remover
    // @param Amount — quantidade a remover
    // @return resultado da operação
    UFUNCTION(Category = "Zf|Inventory")
    EZfItemMechanicResult TryRemoveAmountFromStack(UZfItemInstance* ItemInstance, int32 Amount);

    // Adiciona slots extras ao inventário.
    // Respeita MaxAbsoluteSlotCount.
    // @param ExtraSlots — quantidade de slots a adicionar
    // @return quantidade de slots efetivamente adicionados
    UFUNCTION(Category = "Zf|Inventory|Expansion")
    int32 AddExtraSlots(int32 ExtraSlots);

    // Remove slots extras do inventário.
    // Se houver itens nos slots que serão removidos,
    // tenta movê-los para slots livres anteriores.
    // @param SlotsToRemove — quantidade de slots a remover
    // @return resultado da operação
    UFUNCTION(Category = "Zf|Inventory|Expansion")
    EZfItemMechanicResult RemoveExtraSlots(int32 SlotsToRemove);

    // ============================================================
    // FUNÇÕES DE CONSULTA
    // ============================================================

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
    
private:

    // ============================================================
    // FUNÇÕES INTERNAS - GERENCIAMENTO
    // ============================================================

    // Adiciona o item do inventário a partir do ItemInstance.
    // @param ItemInstance - Item a ser Adicionado
    // @param int32 - Slot do Item a ser Adicionado
    void InternalAddItem(UZfItemInstance* InItemInstance, int32 TargetSlot);

    // Remove o item do inventário a partir do ItemInstance.
    // @param ItemInstance - Item a ser Removido
    void InternalRemoveItemFromInstance(UZfItemInstance* OutItemInstance);

    // Remove o item do inventário a partir do Slot.
    // @param int32 - Item a ser Removido
    void InternalRemoveItemAtSlot(int32 SlotIndex);

    // Tenta fazer stack de um item com stacks existentes.
    // Retorna true se o item foi completamente absorvido por stacks.
    // @param ItemInstance — item a empilhar
    bool InternalTryStackWithExistingItems(UZfItemInstance* ItemInstance);

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
    
    // Ordena o inventário em ordem alfabética pelo nome do item.
    UFUNCTION(Category = "Zf|Inventory|Sort")
    void InternalSortInventoryAlphabetically();

    // Ordena o inventário por tipo de item (usando tags).
    UFUNCTION(Category = "Zf|Inventory|Sort")
    void InternalSortInventoryByItemType();

    // Ordena o inventário por raridade (mais raro primeiro).
    UFUNCTION(Category = "Zf|Inventory|Sort")
    void InternalSortInventoryByRarity();

    // Compacta o inventário — move todos os itens para os primeiros
    // slots disponíveis eliminando slots vazios entre itens.
    UFUNCTION(Category = "Zf|Inventory|Sort")
    void InternalCompactInventory();
    
    // ============================================================
    // FUNÇÕES INTERNAS - VALIDAÇÃO
    // ============================================================
    
    // Valida se uma operação pode ser executada no servidor.
    // Loga warning se chamada no cliente.
    bool InternalCheckIsServer(const FString& FunctionName) const;
};