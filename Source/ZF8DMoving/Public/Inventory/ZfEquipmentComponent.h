// Copyright ZfGame Studio. All Rights Reserved.
// ZfEquipmentComponent.h
// Componente responsável por armazenar e gerenciar todos os
// itens equipados no personagem.
//
// CONCEITO:
// O EquipmentComponent é um UActorComponent que:
// - Armazena os ItemInstances equipados via FFastArraySerializer
// - Comunica com o InventoryComponent para trocar itens
// - Aplica/remove GameplayEffects dos modifiers via GAS
// - Gerencia a regra de duas mãos (TwoHanded)
// - Gerencia os bônus de Combo Sets
// - Desativa bônus de itens quebrados sem desequipá-los
//
// FLUXO DE EQUIPAR:
// 1. InventoryComponent::TryEquipItem chama EquipmentComponent::TryEquipItem
// 2. EquipmentComponent valida slot, two-handed, tags de requisito
// 3. Se já houver item no slot, devolve ao InventoryComponent
// 4. Remove o item do inventário e o armazena no EquipmentList
// 5. Aplica os GameplayEffects dos modifiers via GAS
// 6. Verifica e atualiza bônus de Combo Sets
//
// FLUXO DE DESEQUIPAR:
// 1. Remove os GameplayEffects dos modifiers
// 2. Atualiza bônus de Combo Sets
// 3. Devolve o item ao InventoryComponent via ReceiveUnequippedItem
//
// REPLICAÇÃO:
// FZfEquipmentList usa FFastArraySerializer.
// Replicado para todos — outros jogadores precisam ver
// os itens equipados para visuais e animações.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemInterface.h"
#include "ActiveGameplayEffectHandle.h"
#include "ZfInventoryTypes.h"
#include "ZfItemInstance.h"
#include "ZfItemDefinition.h"
#include "ZfEquipmentComponent.generated.h"

// Forward declarations
class UZfInventoryComponent;
class UZfItemInstance;
class UAbilitySystemComponent;
class UGameplayEffect;
class UZfFragment_Equippable;
class UZfFragment_SetPiece;

// ============================================================
// DELEGATES
// ============================================================

// Disparado quando um item é equipado com sucesso
// @param ItemInstance — item equipado
// @param SlotType — slot onde foi equipado
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemEquipped, UZfItemInstance*, ItemInstance, FGameplayTag, SlotTag);

// Disparado quando um item é desequipado
// @param ItemInstance — item desequipado
// @param SlotType — slot de onde foi removido
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemUnequipped, UZfItemInstance*, ItemInstance, FGameplayTag, SlotTag);

// Disparado quando um item quebra enquanto equipado
// @param ItemInstance — item que quebrou
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquippedItemBroken, UZfItemInstance*, ItemInstance);

// Disparado quando um item equipado é reparado
// @param ItemInstance — item reparado
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquippedItemRepaired, UZfItemInstance*, ItemInstance);

// Disparado quando os bônus de um Combo Set mudam
// @param SetIdentifierTag — tag do set que mudou
// @param ActivePieceCount — quantidade de peças atualmente equipadas
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSetBonusChanged, FGameplayTag, SetIdentifierTag, int32, ActivePieceCount);

// Disparado quando a mochila muda
// @param ActivePieceCount — quantidade de peças atualmente equipadas
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBackpackChanged, FGameplayTag, BackpackTag);



// ============================================================
// FZfActiveSetBonus
// Rastreia o estado ativo de um Combo Set em runtime.
// Armazena quantas peças estão equipadas e os handles
// dos GameplayEffects ativos.
// ============================================================
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfActiveSetBonus
{
    GENERATED_BODY()

    // Tag identificadora do set
    UPROPERTY(BlueprintReadOnly, Category = "Set|Active")
    FGameplayTag SetIdentifierTag;

    // Quantidade de peças deste set atualmente equipadas
    UPROPERTY(BlueprintReadOnly, Category = "Set|Active")
    int32 ActivePieceCount = 0;

    // Handles dos GameplayEffects de bônus ativos
    // Indexados pela quantidade de peças necessárias para ativação
    TMap<int32, FActiveGameplayEffectHandle> ActiveBonusHandles;
};

// ============================================================
// FAST ARRAY
// ============================================================


// -----------------------------------------------------------
// FZfEquipmentSlotEntry
// Uma entrada no sistema de equipamento.
// SlotIndex permite múltiplos slots do mesmo tipo (Ring_0, Ring_1).
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfEquipmentSlotEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

    // Tipo do slot de equipamento
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Slot")
    EZfEquipmentSlot SlotType = EZfEquipmentSlot::None;

    // Tag identificadora do slot de equipamento
    // Ex: EquipmentSlot.MainHand, EquipmentSlot.Ring
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Slot", meta = (Categories = "EquipmentSlot"))
    FGameplayTag SlotTag;
    
    // Índice para múltiplos slots do mesmo tipo
    // Ex: Ring → SlotIndex 0 e SlotIndex 1
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Slot")
    int32 SlotPosition = 0;

    // Item equipado (nullptr = slot vazio)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Slot")
    TObjectPtr<UZfItemInstance> ItemInstance = nullptr;

    // Callbacks de replicação
    void PreReplicatedRemove(const struct FZfEquipmentList& InArraySerializer);
    void PostReplicatedAdd(const FZfEquipmentList& InArraySerializer);
    void PostReplicatedChange(const FZfEquipmentList& InArraySerializer);
};

// -----------------------------------------------------------
// FZfEquipmentList
// FastArray de slots equipados.
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfEquipmentList : public FFastArraySerializer
{
    GENERATED_BODY()

    // Todos os slots de equipamento com seus itens
    UPROPERTY()
    TArray<FZfEquipmentSlotEntry> EquippedItems;

    // Referência ao componente dono
    UPROPERTY(NotReplicated)
    TObjectPtr<UZfEquipmentComponent> OwnerComponent = nullptr;

    // Replicação delta do equipamento
    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
    {
        return FastArrayDeltaSerialize<FZfEquipmentSlotEntry, FZfEquipmentList>(
            EquippedItems, DeltaParams, *this);
    }
};

template<>
struct TStructOpsTypeTraits<FZfEquipmentList> : public TStructOpsTypeTraitsBase2<FZfEquipmentList>
{
    enum { WithNetDeltaSerializer = true };
};















// ============================================================
// UZfEquipmentComponent
// ============================================================

UCLASS(ClassGroup = (Zf), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class ZF8DMOVING_API UZfEquipmentComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UZfEquipmentComponent();

    // ----------------------------------------------------------
    // CONFIGURAÇÃO
    // ----------------------------------------------------------

    // Configuração dos slots disponíveis neste equipamento.
    // Define quais slots existem e quantos de cada tipo.
    // Ex: [{Ring, 2}, {MainHand, 1}, {OffHand, 1}]
    // Configurado no editor por personagem/classe.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment|Config")
    TArray<FZfEquipmentSlotEntry> DefaultEquipmentSlots;

    // ----------------------------------------------------------
    // DELEGATES
    // ----------------------------------------------------------

    UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
    FOnItemEquipped OnItemEquipped;

    UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
    FOnItemUnequipped OnItemUnequipped;

    UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
    FOnEquippedItemBroken OnEquippedItemBroken;

    UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
    FOnEquippedItemRepaired OnEquippedItemRepaired;

    UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
    FOnSetBonusChanged OnSetBonusChanged;

    UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
    FOnBackpackChanged OnBackpackChanged;
    
    // ----------------------------------------------------------
    // FUNÇÕES PRINCIPAIS — DESEQUIPAR
    // ----------------------------------------------------------

    
    // Desequipa todos os itens equipados de uma vez.
    // Devolve todos ao inventário.
    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment")
    void UnequipAllItems();

    // ----------------------------------------------------------
    // FUNÇÕES DE TROCA RÁPIDA
    // Troca direta entre inventário e slot de equipamento.
    // ----------------------------------------------------------

    // Troca rápida — equipa item do inventário e devolve o
    // item atual do slot ao inventário em uma única operação.
    // @param InventorySlotIndex — slot do inventário
    // @param EquipSlotType — slot de equipamento alvo
    // @param EquipSlotIndex — índice do slot de equipamento
    // @return resultado da operação
    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment")
    EZfItemMechanicResult QuickSwapItem(int32 InventorySlotIndex, EZfEquipmentSlot EquipSlotType, int32 EquipSlotIndex = 0);

    // ----------------------------------------------------------
    // FUNÇÕES DE DURABILIDADE
    // Chamadas pelo ItemInstance ou sistemas externos para
    // notificar mudanças de durabilidade em itens equipados.
    // ----------------------------------------------------------

    // Notifica que um item equipado quebrou.
    // Desativa os GameplayEffects sem desequipar o item.
    // Dispara OnEquippedItemBroken para notificar a UI.
    // @param ItemInstance — item que quebrou
    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Durability")
    void NotifyEquippedItemBroken(UZfItemInstance* ItemInstance);

    // Notifica que um item equipado foi reparado.
    // Reativa os GameplayEffects do item.
    // @param ItemInstance — item reparado
    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Durability")
    void NotifyEquippedItemRepaired(UZfItemInstance* ItemInstance);

    // ----------------------------------------------------------
    // FUNÇÕES DE CONSULTA
    // ----------------------------------------------------------

    // Retorna o item equipado em um slot específico.
    // Retorna nullptr se o slot estiver vazio.
    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Query")
    UZfItemInstance* GetItemAtEquipmentSlot(EZfEquipmentSlot SlotType, int32 SlotIndex = 0) const;

    // Verifica se um item está equipado.
    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Query")
    bool IsItemEquipped(UZfItemInstance* ItemInstance) const;

    // Verifica se um slot específico está ocupado.
    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Query")
    bool IsEquipmentSlotOccupied(EZfEquipmentSlot SlotType, int32 SlotIndex = 0) const;

    // Verifica se um slot está bloqueado por arma de duas mãos.
    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Query")
    bool IsEquipmentSlotBlocked(EZfEquipmentSlot SlotType, int32 SlotIndex = 0) const;

    // Retorna todos os itens equipados.
    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Query")
    TArray<UZfItemInstance*> GetAllEquippedItems() const;

    // Retorna todos os itens equipados com uma tag específica.
    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Query")
    TArray<UZfItemInstance*> GetEquippedItemsByTag(const FGameplayTag& Tag) const;

    // Retorna o slot de um item equipado.
    // Retorna EZfEquipmentSlot::None se não encontrado.
    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Query")
    EZfEquipmentSlot GetEquipmentSlotOfItem(UZfItemInstance* ItemInstance) const;

    // Verifica se um item pode ser equipado neste componente.
    // Faz todas as validações sem efetivamente equipar.
    // @param ItemInstance — item a verificar
    // @param OutReason — motivo de falha se não puder equipar
    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Query")
    bool CanEquipItemOld(UZfItemInstance* ItemInstance, EZfItemMechanicResult& OutReason) const;

    // ----------------------------------------------------------
    // FUNÇÕES DE COMBO SET
    // ----------------------------------------------------------

    // Retorna a quantidade de peças equipadas de um set específico.
    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Set")
    int32 GetEquippedPieceCountForSet(const FGameplayTag& SetIdentifierTag) const;

    // Retorna todos os bônus de set ativos no momento.
    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Set")
    TArray<FZfActiveSetBonus> GetAllActiveSetBonuses() const;

    // ----------------------------------------------------------
    // EXPANSÃO DE SLOTS
    // Chamado pelo UZfFragment_InventoryExpansion ao equipar
    // ou desequipar uma mochila.
    // ----------------------------------------------------------

    // Adiciona slots extras ao InventoryComponent.
    // Chamado quando uma mochila é equipada.
    // @param ExtraSlots — quantidade de slots a adicionar
    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Expansion")
    void OnBackpackEquipped(int32 ExtraSlots);

    // Remove slots extras do InventoryComponent.
    // Chamado quando uma mochila é desequipada.
    // @param ExtraSlots — quantidade de slots a remover
    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Expansion")
    void OnBackpackUnequipped(int32 ExtraSlots);

    // ----------------------------------------------------------
    // RPCs — CLIENT → SERVER
    // ----------------------------------------------------------

    

    // Requisição do cliente para troca rápida
    UFUNCTION(Server, Reliable, WithValidation,
        BlueprintCallable, Category = "Zf|Equipment|RPC")
    void ServerRequestQuickSwap(int32 InventorySlotIndex, EZfEquipmentSlot EquipSlotType, int32 EquipSlotIndex);

    // ----------------------------------------------------------
    // REPLICAÇÃO
    // ----------------------------------------------------------

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ----------------------------------------------------------
    // CICLO DE VIDA
    // ----------------------------------------------------------

    virtual void BeginPlay() override;


protected:

    // ----------------------------------------------------------
    // DADOS REPLICADOS
    // ----------------------------------------------------------

    // Lista de slots equipados — replicada para todos os clientes
    // para visuais e animações de outros jogadores
    UPROPERTY(Replicated)
    FZfEquipmentList EquipmentList;

    // Se verdadeiro, o slot OffHand está bloqueado por
    // uma arma de duas mãos equipada no MainHand
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Equipment")
    bool bOffHandSlotBlocked = false;

    // ----------------------------------------------------------
    // DADOS NÃO REPLICADOS — apenas servidor
    // ----------------------------------------------------------

    // Referência ao InventoryComponent no mesmo ator
    UPROPERTY()
    TObjectPtr<UZfInventoryComponent> InventoryComponent;

    // Rastreamento dos bônus de set ativos em runtime
    // Não replicado — recalculado localmente em cada cliente
    // a partir do EquipmentList replicado
    TMap<FGameplayTag, FZfActiveSetBonus> ActiveSetBonuses;

private:

    // ----------------------------------------------------------
    // FUNÇÕES INTERNAS
    // ----------------------------------------------------------

    // Inicializa os slots de equipamento com base em DefaultEquipmentSlots
    void Internal_InitializeEquipmentSlots();

    // Busca o InventoryComponent no ator dono
    void Internal_FindInventoryComponent();

    // Busca o AbilitySystemComponent no ator dono
    UAbilitySystemComponent* Internal_GetAbilitySystemComponent() const;

    // Aplica todos os GameplayEffects dos modifiers de um item
    // @param ItemInstance — item cujos efeitos serão aplicados
    void Internal_ApplyItemGameplayEffects(UZfItemInstance* ItemInstance);

    // Remove todos os GameplayEffects dos modifiers de um item
    // @param ItemInstance — item cujos efeitos serão removidos
    void Internal_RemoveItemGameplayEffects(UZfItemInstance* ItemInstance);

    // Verifica e atualiza os bônus de Combo Set após equipar/desequipar
    // @param ItemInstance — item que foi equipado ou desequipado
    // @param bWasEquipped — true se equipou, false se desequipou
    void Internal_UpdateSetBonuses(UZfItemInstance* ItemInstance, bool bWasEquipped);

    // Aplica os bônus de set para um set específico baseado
    // na quantidade de peças atualmente equipadas
    // @param SetTag — tag identificadora do set
    // @param PieceCount — quantidade de peças equipadas
    void Internal_ApplySetBonuses(const FGameplayTag& SetTag, int32 PieceCount);

    // Remove todos os bônus de set de um set específico
    // @param SetTag — tag identificadora do set
    void Internal_RemoveAllSetBonuses(const FGameplayTag& SetTag);

    // Bloqueia o slot OffHand (arma de duas mãos equipada)
    void Internal_BlockOffHandSlot();

    // Libera o slot OffHand (arma de duas mãos removida)
    // Se houver item no OffHand ao liberar, mantém equipado
    void Internal_UnblockOffHandSlot();

    
    // Versão const de Internal_FindSlotEntry
    const FZfEquipmentSlotEntry* Internal_FindSlotEntryConst(EZfEquipmentSlot SlotType, int32 SlotIndex) const;

    // Gera uma ReplicationKey única para novos slots
    int32 Internal_GenerateReplicationKey() const;

    
    // Valida se o InventoryComponent está disponível
    bool Internal_CheckInventoryComponent(const FString& FunctionName) const;
















    

public:
    
    // ============================================================
    // FUNÇÕES SERVER - GERENCIAMENTO
    // ============================================================

    // Tenta equipar um item em seu slot correspondente.
    // Valida: slot disponível, two-handed, tags de requisito,
    //         item quebrado, slot bloqueado.
    // Se já houver item no slot alvo, devolve ao InventoryComponent.
    // @param ItemInstance — item a equipar
    // @param FromInventorySlot — slot de origem no inventário
    // @param Int32 — Slot Caso haja mais de um tipo de slot por tipo de item: Ring_1, RIng_2
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Equipment")
    void ServerTryEquipItem(UZfItemInstance* ItemInstance, int32 FromInventorySlot, int32 SlotPosition);

    // Tenta deequipar um item em seu slot correspondente.
    // Valida: slot disponível, two-handed, tags de requisito,
    //         item quebrado, slot bloqueado.
    // Se já houver item no slot alvo, devolve ao InventoryComponent.
    // @param ItemInstance — item a equipar
    // @param FromInventorySlot — slot de origem no inventário
    // @param Int32 — Slot Caso haja mais de um tipo de slot por tipo de item: Ring_1, RIng_2
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Equipment|RPC")
    void ServerTryUnequipItem(FGameplayTag SlotTag, int32 FromInventorySlot, int32 SlotPosition);
        
    // ============================================================
    // FUNÇÕES PRINCIPAIS - GERENCIAMENTO
    // ============================================================

    // Tenta equipar um item em seu slot correspondente.
    // Valida: slot disponível, two-handed, tags de requisito, item quebrado, slot bloqueado.
    // @param ItemInstance — item a equipar
    // @param InventorySlotIndex — slot de origem no inventário
    // @param Int32 — Slot Caso haja mais de um tipo de slot por tipo de item: Ring_1, RIng_2
    // @return resultado da operação
    UFUNCTION(Category = "Zf|Equipment")
    EZfItemMechanicResult TryEquipItem(UZfItemInstance* ItemInstance, int32 TagetInventorySlot, int32 SlotPosition);

    // Tenta equipar mochila
    // Se já houver item no slot alvo, devolve ao InventoryComponent.
    // @param ItemInstance — item a equipar
    // @param FromInventorySlot — slot de origem no inventário
    // @param Int32 — Slot Caso haja mais de um tipo de slot por tipo de item: Ring_1, RIng_2
    UFUNCTION(Category = "Zf|Equipment")
    EZfItemMechanicResult TryUnequipItem(FGameplayTag SlotTag, int32 TagetInventorySlot, int32 SlotPosition);

    // Tenta deequipar um item em seu slot correspondente.
    // Valida: slot disponível, two-handed, tags de requisito,
    //         item quebrado, slot bloqueado.
    // Se já houver item no slot alvo, devolve ao InventoryComponent.
    // @param ItemInstance — item a equipar
    // @param FromInventorySlot — slot de origem no inventário
    // @param Int32 — Slot Caso haja mais de um tipo de slot por tipo de item: Ring_1, RIng_2
    UFUNCTION(Category = "Zf|Equipment")
    EZfItemMechanicResult TryEquipBackpack(FGameplayTag SlotTag, int32 FromInventorySlot, int32 SlotPosition);

    // Tenta desequipar mochila
    // Se já houver item no slot alvo, devolve ao InventoryComponent.
    // @param ItemInstance — item a equipar
    // @param FromInventorySlot — slot de origem no inventário
    // @param Int32 — Slot Caso haja mais de um tipo de slot por tipo de item: Ring_1, RIng_2
    UFUNCTION(Category = "Zf|Equipment")
    EZfItemMechanicResult TryUnequipBackpack(FGameplayTag SlotTag, int32 FromInventorySlot, int32 SlotPosition);

    
    // ============================================================
    // FUNÇÕES DE CONSULTA
    // ============================================================

    // Verifica se Consigo Equipar o Item.
    // @param ItemInstance — item a equipar
    // @param FromInventorySlot — slot de origem no inventário
    // @param Int32 — Slot Caso haja mais de um tipo de slot por tipo de item: Ring_1, RIng_2
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    EZfItemMechanicResult CanEquipItem(UZfItemInstance* InItemInstance, int32 FromInventorySlot, int32 SlotPosition);

    // Pega EquipmentSlot Tag.
    // @param ItemInstance — item a Equipado
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    FGameplayTag GetEquipmentSlotTagOfItem(UZfItemInstance* ItemInstance) const;

    // Retorna o ItemInstance equipado em um slot pela sua tag.
    // @param SlotTag — EquipmentSlot Tag do Item
    // @param Int32 — Slot Caso haja mais de um tipo de slot por tipo de item: Ring_1, RIng_2    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Query")
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    UZfItemInstance* GetItemAtSlotTag(FGameplayTag SlotTag, int32 SlotPosition = 0) const;

    
private:

    // ============================================================
    // FUNÇÕES INTERNAS - GERENCIAMENTO
    // ============================================================
    
    // Equipa o item a partir do ItemInstance.
    // @param ItemInstance - Item a ser Adicionado
    void InternalEquipItem(UZfItemInstance* InItemInstance, int32 SlotPosition);
    
    // Equipa o item a partir do ItemInstance.
    // @param ItemInstance - Item a ser Adicionado
    // @param SlotPosition — Slot Caso haja mais de um tipo de slot por tipo de item: Ring_1, RIng_2
    void InternalUnequipItem(UZfItemInstance* InItemInstance, int32 SlotPosition);

        
    // Busca a entrada de slot no EquipmentList
    // @param SlotTag — Tag do slot
    // @param SlotPosition — Slot Caso haja mais de um tipo de slot por tipo de item: Ring_1, RIng_2
    FZfEquipmentSlotEntry* InternalFindSlotEntry(FGameplayTag SlotTag, int32 SlotPosition);

    // ============================================================
    // FUNÇÕES INTERNAS - ORGANIZAÇÃO
    // ============================================================

    // ============================================================
    // FUNÇÕES INTERNAS - VALIDAÇÃO
    // ============================================================

    // Valida se uma operação pode ser executada no servidor
    bool InternalCheckIsServer(const FString& FunctionName) const;






    


};