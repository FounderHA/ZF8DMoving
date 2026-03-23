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
#include "ZfItemDefinition.h"
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

UCLASS(ClassGroup = (Zf),BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class ZF8DMOVING_API UZfInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UZfInventoryComponent();

    // ----------------------------------------------------------
    // CONFIGURAÇÃO
    // ----------------------------------------------------------

    // Número inicial de slots do inventário.
    // Pode ser expandido via UZfFragment_InventoryExpansion (mochila).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Config", meta = (ClampMin = "1", ClampMax = "500"))
    int32 DefaultSlotCount = 30;

    // Número máximo absoluto de slots — nunca ultrapassa esse valor
    // mesmo com múltiplas mochilas equipadas.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Config", meta = (ClampMin = "1", ClampMax = "500"))
    int32 MaxAbsoluteSlotCount = 200;

    // ----------------------------------------------------------
    // DELEGATES — UI e outros sistemas se inscrevem aqui
    // ----------------------------------------------------------

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

    // ----------------------------------------------------------
    // FUNÇÕES PRINCIPAIS — ADICIONAR ITEM
    // ----------------------------------------------------------

    // Tenta adicionar um item ao inventário.
    // Se o item é stackável e já existe stack do mesmo tipo,
    // tenta empilhar antes de ocupar novo slot.
    // Deve ser chamado apenas no servidor.
    // @param ItemInstance — instância do item a adicionar
    // @param OutSlotIndex — slot onde foi adicionado (se Success)
    // @return resultado da operação
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory")
    EZfItemMechanicResult TryAddItemToInventory(UZfItemInstance* ItemInstance, int32& OutSlotIndex);

    // Tenta adicionar um item em um slot específico.
    // Se o slot já tiver item, troca os itens de lugar.
    // @param ItemInstance — instância do item a adicionar
    // @param SlotIndex — índice do slot desejado
    // @return resultado da operação
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory")
    EZfItemMechanicResult TryAddItemToSpecificSlot(UZfItemInstance* ItemInstance, int32 SlotIndex);

    // Cria e adiciona um novo item ao inventário a partir de um ItemDefinition.
    // Gera o ItemInstance, rola os modifiers e adiciona ao inventário.
    // Deve ser chamado apenas no servidor.
    // @param ItemDefinition — definição do item a criar
    // @param Tier — tier do item
    // @param Rarity — raridade do item
    // @param OutItemInstance — instância criada (se Success)
    // @return resultado da operação
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory")
    EZfItemMechanicResult CreateAndAddItemToInventory(UZfItemDefinition* ItemDefinition, int32 Tier, EZfItemRarity Rarity, UZfItemInstance*& OutItemInstance);

    // ----------------------------------------------------------
    // FUNÇÕES PRINCIPAIS — REMOVER ITEM
    // ----------------------------------------------------------

    // Remove um item do inventário pelo índice do slot.
    // @param SlotIndex — slot a remover
    // @param OutItemInstance — item removido
    // @return resultado da operação
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory")
    EZfItemMechanicResult RemoveItemAtSlot(int32 SlotIndex, UZfItemInstance*& OutItemInstance);

    // Remove um item do inventário pela referência do ItemInstance.
    // @param ItemInstance — item a remover
    // @return resultado da operação
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory")
    EZfItemMechanicResult RemoveItemInstance(UZfItemInstance* ItemInstance);

    // Remove uma quantidade de um item stackável.
    // Remove o item inteiro se a quantidade zerar.
    // @param ItemInstance — item stackável a remover
    // @param Amount — quantidade a remover
    // @return resultado da operação
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory")
    EZfItemMechanicResult RemoveAmountFromStack(UZfItemInstance* ItemInstance, int32 Amount);

    // ----------------------------------------------------------
    // FUNÇÕES PRINCIPAIS — MOVER ITEM
    // ----------------------------------------------------------

    // Move um item de um slot para outro.
    // Se o slot destino tiver item, troca os dois de lugar.
    // @param FromSlotIndex — slot de origem
    // @param ToSlotIndex — slot de destino
    // @return resultado da operação
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory")
    EZfItemMechanicResult MoveItemBetweenSlots(int32 FromSlotIndex, int32 ToSlotIndex);

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
    // FUNÇÕES PRINCIPAIS — ORGANIZAÇÃO
    // ----------------------------------------------------------

    // Ordena o inventário em ordem alfabética pelo nome do item.
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Sort")
    void SortInventoryAlphabetically();

    // Ordena o inventário por tipo de item (usando tags).
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Sort")
    void SortInventoryByItemType();

    // Ordena o inventário por raridade (mais raro primeiro).
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Sort")
    void SortInventoryByRarity();

    // Compacta o inventário — move todos os itens para os primeiros
    // slots disponíveis eliminando slots vazios entre itens.
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Sort")
    void CompactInventory();

    // ----------------------------------------------------------
    // FUNÇÕES DE CONSULTA
    // ----------------------------------------------------------

    // Retorna o ItemInstance em um slot específico.
    // Retorna nullptr se o slot estiver vazio ou inválido.
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    UZfItemInstance* GetItemAtSlot(int32 SlotIndex) const;

    // Retorna o índice do slot de um ItemInstance.
    // Retorna INDEX_NONE se não encontrado.
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    int32 GetSlotIndexOfItem(UZfItemInstance* ItemInstance) const;

    // Verifica se o inventário contém um ItemInstance específico.
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    bool ContainsItem(UZfItemInstance* ItemInstance) const;

    // Retorna todos os itens que possuem uma tag específica.
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    TArray<UZfItemInstance*> GetItemsByTag(const FGameplayTag& Tag) const;

    // Retorna o primeiro slot vazio disponível.
    // Retorna INDEX_NONE se o inventário estiver cheio.
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    int32 GetFirstEmptySlot() const;

    // Retorna o número de slots atualmente disponíveis (vazios).
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    int32 GetAvailableSlotCount() const;

    // Retorna o número total de slots do inventário (incluindo ocupados).
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    int32 GetTotalSlotCount() const;

    // Verifica se o inventário está cheio.
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    bool IsInventoryFull() const;

    // Verifica se um slot está vazio.
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    bool IsSlotEmpty(int32 SlotIndex) const;

    // Verifica se um índice de slot é válido.
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    bool IsValidSlotIndex(int32 SlotIndex) const;

    // Retorna todos os ItemInstances do inventário (sem slots vazios).
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    TArray<UZfItemInstance*> GetAllItems() const;

    // Retorna todos os slots do inventário (incluindo vazios).
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    TArray<FZfInventorySlot> GetAllSlots() const;

    // ----------------------------------------------------------
    // EXPANSÃO DE SLOTS
    // Chamado pelo EquipmentComponent quando uma mochila é
    // equipada/desequipada via UZfFragment_InventoryExpansion.
    // ----------------------------------------------------------

    // Adiciona slots extras ao inventário.
    // Respeita MaxAbsoluteSlotCount.
    // @param ExtraSlots — quantidade de slots a adicionar
    // @return quantidade de slots efetivamente adicionados
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Expansion")
    int32 AddExtraSlots(int32 ExtraSlots);

    // Remove slots extras do inventário.
    // Se houver itens nos slots que serão removidos,
    // tenta movê-los para slots livres anteriores.
    // @param SlotsToRemove — quantidade de slots a remover
    // @return resultado da operação
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Expansion")
    EZfItemMechanicResult RemoveExtraSlots(int32 SlotsToRemove);

    // ----------------------------------------------------------
    // RPCs — CLIENT → SERVER
    // Clientes não modificam o inventário diretamente.
    // Enviam requisições ao servidor via RPC.
    // ----------------------------------------------------------

    // Requisição do cliente para mover item entre slots
    UFUNCTION(Server, Reliable, WithValidation,
        BlueprintCallable, Category = "Zf|Inventory|RPC")
    void ServerRequestMoveItem(int32 FromSlotIndex, int32 ToSlotIndex);

    // Requisição do cliente para equipar item de um slot
    UFUNCTION(Server, Reliable, WithValidation,
        BlueprintCallable, Category = "Zf|Inventory|RPC")
    void ServerRequestEquipItem(int32 SlotIndex);

    // Requisição do cliente para dropar item no mundo
    UFUNCTION(Server, Reliable, WithValidation,
        BlueprintCallable, Category = "Zf|Inventory|RPC")
    void ServerRequestDropItem(int32 SlotIndex);

    // Requisição do cliente para ordenar o inventário
    UFUNCTION(Server, Reliable, WithValidation,
        BlueprintCallable, Category = "Zf|Inventory|RPC")
    void ServerRequestSortInventory(int32 SortTypeIndex);

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
    // REPLICAÇÃO
    // ----------------------------------------------------------

    // Registra as propriedades replicadas do componente
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ----------------------------------------------------------
    // CICLO DE VIDA DO COMPONENTE
    // ----------------------------------------------------------

    virtual void BeginPlay() override;

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

    // ----------------------------------------------------------
    // DADOS REPLICADOS
    // ----------------------------------------------------------

    // Lista de slots do inventário — replicada via FastArraySerializer
    UPROPERTY(Replicated)
    FZfInventoryList InventoryList;

    // Tamanho atual do inventário (número de slots disponíveis)
    // Replicado para que a UI do cliente possa exibir corretamente
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inventory")
    int32 CurrentSlotCount = 0;

    // ----------------------------------------------------------
    // REFERÊNCIAS INTERNAS
    // ----------------------------------------------------------

    // Referência ao EquipmentComponent no mesmo ator.
    // Resolvida no BeginPlay — não replicada.
    UPROPERTY()
    TObjectPtr<UZfEquipmentComponent> EquipmentComponent;

private:

    // ----------------------------------------------------------
    // FUNÇÕES INTERNAS
    // ----------------------------------------------------------

    // Inicializa os slots do inventário com o DefaultSlotCount.
    // Chamado no BeginPlay apenas no servidor.
    void Internal_InitializeSlots();

    // Busca o EquipmentComponent no ator dono.
    // Chamado no BeginPlay.
    void Internal_FindEquipmentComponent();

    // Tenta fazer stack de um item com stacks existentes.
    // Retorna true se o item foi completamente absorvido por stacks.
    // @param ItemInstance — item a empilhar
    bool Internal_TryStackWithExistingItems(UZfItemInstance* ItemInstance);

    // Adiciona um slot vazio ao array de inventário.
    // @param SlotIndex — índice do novo slot
    void Internal_AddEmptySlot(int32 SlotIndex);

    // Remove um item do slot sem notificações (uso interno).
    // @param SlotIndex — slot a limpar
    void Internal_ClearSlot(int32 SlotIndex);

    // Valida se uma operação pode ser executada no servidor.
    // Loga warning se chamada no cliente.
    bool Internal_CheckIsServer(const FString& FunctionName) const;

    // Valida se o EquipmentComponent está disponível.
    bool Internal_CheckEquipmentComponent(const FString& FunctionName) const;

    // Notifica os callbacks do FastArraySerializer após mudanças.
    void Internal_MarkInventoryDirty();
};