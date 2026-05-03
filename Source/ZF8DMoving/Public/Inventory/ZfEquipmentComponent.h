// Copyright ZfGame Studio. All Rights Reserved.
// ZfEquipmentComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemInterface.h"
#include "ActiveGameplayEffectHandle.h"
#include "ZfInventoryTypes.h"
#include "ZfItemInstance.h"
#include "ZfItemDefinition.h"
#include "Inventory/ZfInventoryReceiverInterface.h"
#include "Inventory/Modifiers/ZfModifierRule.h"
#include "Inventory/Fragments/ZfFragment_Modifiers.h"
#include "ZfEquipmentComponent.generated.h"

enum class EZfRefinerySlotType : uint8;
// Forward declarations
class UZfInventoryComponent;
class UZfItemInstance;
class UAbilitySystemComponent;
class UGameplayEffect;
class UZfFragment_Equippable;
class UZfFragment_SetPiece;
class UZfModifierRule;

// ============================================================
// DELEGATES
// ============================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemEquipped,   UZfItemInstance*, ItemInstance, FGameplayTag, SlotTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemUnequipped, UZfItemInstance*, ItemInstance, FGameplayTag, SlotTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquippedItemBroken,   UZfItemInstance*, ItemInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquippedItemRepaired, UZfItemInstance*, ItemInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSetBonusChanged, FGameplayTag, SetIdentifierTag, int32, ActivePieceCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStackChanged, UZfItemInstance*, ItemInstance, FGameplayTag, SlotTag);

// ============================================================
// FZfActiveSetBonus
// ============================================================

USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfActiveSetBonus
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Set|Active")
    FGameplayTag SetIdentifierTag;

    UPROPERTY(BlueprintReadOnly, Category = "Set|Active")
    int32 ActivePieceCount = 0;

    TMap<int32, FActiveGameplayEffectHandle> ActiveBonusHandles;
};

// ============================================================
// FAST ARRAY
// ============================================================

USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfEquipmentSlotEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

    friend class UZfInventoryComponent;

    // Tag única que identifica este slot — ex: EquipmentSlot.Slot.Ring.1
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Slot", meta = (Categories = "EquipmentSlot"))
    FGameplayTag SlotTag;

    // Item equipado (nullptr = slot vazio)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Slot")
    TObjectPtr<UZfItemInstance> ItemInstance = nullptr;

    void PreReplicatedRemove(const struct FZfEquipmentList& InArraySerializer);
    void PostReplicatedAdd(const struct FZfEquipmentList& InArraySerializer);
    void PostReplicatedChange(const struct FZfEquipmentList& InArraySerializer);
};

USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfEquipmentList : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FZfEquipmentSlotEntry> EquippedItems;

    UPROPERTY(NotReplicated)
    TObjectPtr<UZfEquipmentComponent> OwnerComponent = nullptr;

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
class ZF8DMOVING_API UZfEquipmentComponent : public UActorComponent, public IZfInventoryReceiverInterface
{
    GENERATED_BODY()

    friend class UZfInventoryComponent;
    
public:

    UZfEquipmentComponent();

    virtual void AddItemToTargetInterface_Implementation(UObject* ItemComesFrom, UZfItemInstance* InItemInstance, int32 AmountToAdd,
        int32 SlotIndexComesFrom, int32 TargetSlotIndex,
        EZfRefinerySlotType SlotTypeComesFrom, EZfRefinerySlotType TargetSlotType,
        FGameplayTag SlotTagComesFrom, FGameplayTag TargetSlotTag) override;
    virtual void RemoveItemFromTargetInterface_Implementation(UObject* ItemComesFrom, int32 ItemAmountToRemove, int32 TargetSlotIndex, EZfRefinerySlotType TargetSlotType, FGameplayTag TargetSlotTag) override;
	virtual bool CanITransferBack_Implementation(UZfItemInstance* InItemInstance,
	    EZfRefinerySlotType SlotTypeComesFrom, EZfRefinerySlotType TargetSlotType, 
	    FGameplayTag SlotTagComesFrom, FGameplayTag TargetSlotTag) override;
    
    // ----------------------------------------------------------
    // CONFIGURAÇÃO
    // ----------------------------------------------------------

    // Slots padrão desta máquina — usado por Internal_InitializeEquipmentSlots.
    // Com o novo sistema de tags, este array é opcional — os slots existem
    // dinamicamente via EquipmentList conforme os itens são equipados.
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
    FOnStackChanged OnStackChanged;

    // ----------------------------------------------------------
    // DESEQUIPAR TODOS
    // ----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment")
    void UnequipAllItems();

    // ----------------------------------------------------------
    // TROCA RÁPIDA
    // ----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment")
    EZfItemMechanicResult QuickSwapItem(int32 InventorySlotIndex, FGameplayTag EquipmentSlotTag, int32 EquipSlotIndex = 0);

    // ----------------------------------------------------------
    // DURABILIDADE
    // ----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Durability")
    void NotifyEquippedItemBroken(UZfItemInstance* ItemInstance);

    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Durability")
    void NotifyEquippedItemRepaired(UZfItemInstance* ItemInstance);

    // ----------------------------------------------------------
    // RPCs
    // ----------------------------------------------------------

    UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Zf|Equipment|RPC")
    void ServerRequestQuickSwap(int32 InventorySlotIndex, FGameplayTag EquipmentSlotTag, int32 EquipSlotIndex);

    // ----------------------------------------------------------
    // SERVER RPCs — GERENCIAMENTO
    // ----------------------------------------------------------

    // Equipa um item. SlotTag deve ser a tag filha exata (ex: Slot_Ring_1).
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Equipment")
    void ServerTryEquipItem(UZfItemInstance* ItemInstance, int32 FromInventorySlot, FGameplayTag SlotTag);

    // Desequipa o item no SlotTag especificado.
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Equipment")
    void ServerTryUnequipItem(FGameplayTag SlotTag, int32 TargetInventorySlot);

    // Usa o consumível no slot especificado.
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Equipment")
    void ServerTryUseQuickSlot(FGameplayTag SlotTag);

    // Remove 1 do stack do item no slot. Remove o item se zerar.
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Zf|Equipment")
    void ServerTryRemoveItemStackFromEquipmentSlot(FGameplayTag SlotTag);

    // ----------------------------------------------------------
    // FUNÇÕES PRINCIPAIS — GERENCIAMENTO
    // ----------------------------------------------------------

    // Equipa o item no slot definido pelo Fragment_Equippable do item.
    // @param SlotTag — tag filha exata (ex: Slot_Ring_1). Se inválida, usa a do fragment.
    UFUNCTION(Category = "Zf|Equipment")
    EZfItemMechanicResult TryEquipItem(UZfItemInstance* ItemInstance, int32 FromInventorySlot, FGameplayTag SlotTag);

    UFUNCTION(Category = "Zf|Equipment")
    EZfItemMechanicResult TryEquipItemInterface(UObject* ItemComesFrom, UZfItemInstance* InItemInstance, int32 AmountToAdd,
        int32 SlotIndexComesFrom, int32 TargetSlotIndex,
        EZfRefinerySlotType SlotTypeComesFrom, EZfRefinerySlotType TargetSlotType,
        FGameplayTag SlotTagComesFrom, FGameplayTag TargetSlotTag);

    UFUNCTION(Category = "Zf|Equipment")
    EZfItemMechanicResult TryUnequipItem(FGameplayTag SlotTag, int32 TargetInventorySlot);

    UFUNCTION(Category = "Zf|Equipment")
    EZfItemMechanicResult TryUnequipItemInterface(UObject* ItemComesFrom, int32 ItemAmountToRemove, int32 TargetSlotIndex, EZfRefinerySlotType TargetSlotType, FGameplayTag TargetSlotTag);

    UFUNCTION(Category = "Zf|Equipment")
    EZfItemMechanicResult TryEquipBackpack(FGameplayTag SlotTag, UZfItemInstance* InItemInstance, int32 FromInventorySlot);

    UFUNCTION(Category = "Zf|Equipment")
    EZfItemMechanicResult TryUnequipBackpack(FGameplayTag SlotTag, int32 TargetInventorySlot);

    UFUNCTION(Category = "Zf|Equipment")
    void TryUseQuickSlot(FGameplayTag SlotTag);

    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment")
    void TryRemoveItemStackFromEquipmentSlot(FGameplayTag SlotTag);

    // Tenta adicionar ao stack de um item já existente no slot especificado.
    // Retorna o overflow (quantidade que não coube). 0 = tudo coube.
    UFUNCTION(Category = "Zf|Refinery")
    int32 TryAddToStack(FGameplayTag SlotTag, int32 Amount);

    // ----------------------------------------------------------
    // FUNÇÕES DE CONSULTA
    // ----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    bool CanEquipItem(UZfItemInstance* InItemInstance, FGameplayTag SlotTagComesFrom, FGameplayTag TargetSlotTag);

    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    bool CanUnequipItem(FGameplayTag TargetSlotTag);

    
    
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    FGameplayTag GetEquipmentSlotTagOfItem(UZfItemInstance* ItemInstance) const;

    // Retorna o item equipado na SlotTag exata fornecida.
    UFUNCTION(BlueprintCallable, Category = "Zf|Inventory|Query")
    UZfItemInstance* GetItemAtSlotTag(FGameplayTag SlotTag) const;

    // Compatibilidade — busca pelo SlotTag ignorando SlotIndex.
    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Query")
    UZfItemInstance* GetItemAtEquipmentSlot(FGameplayTag EquipmentSlotTag, int32 SlotIndex = 0) const;

    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Query")
    bool IsItemEquipped(UZfItemInstance* ItemInstance) const;

    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Query")
    bool IsEquipmentSlotOccupied(FGameplayTag EquipmentSlotTag, int32 SlotIndex = 0) const;

    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Query")
    bool IsEquipmentSlotBlocked(FGameplayTagContainer EquipmentTags, int32 SlotIndex = 0) const;

    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Query")
    TArray<UZfItemInstance*> GetAllEquippedItems() const;

    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Query")
    TArray<UZfItemInstance*> GetEquippedItemsByTag(const FGameplayTag& Tag) const;

    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Query")
    FGameplayTag GetEquipmentSlotOfItem(UZfItemInstance* ItemInstance) const;

    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Query")
    const TArray<FZfEquipmentSlotEntry>& GetAllEquipmentSlots() const;

    FZfEquipmentSlotEntry* FindSlotEntryByItem(UZfItemInstance* Item);

    // Retorna a primeira tag filha vazia de ParentTag.
    // Ex: GetFirstEmptySlotByParentTag(Slot_Ring) → Slot_Ring_1 ou Slot_Ring_2
    // Usa RequestGameplayTagChildren para encontrar os filhos automaticamente.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|Equipment|Query")
    FGameplayTag GetFirstEmptySlotByParentTag(FGameplayTag ParentTag) const;

    // ----------------------------------------------------------
    // COMBO SETS
    // ----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Set")
    int32 GetEquippedPieceCountForSet(const FGameplayTag& SetIdentifierTag) const;

    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Set")
    TArray<FZfActiveSetBonus> GetAllActiveSetBonuses() const;

    // ----------------------------------------------------------
    // EXPANSÃO DE SLOTS
    // ----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Expansion")
    void OnBackpackEquipped(int32 ExtraSlots);

    UFUNCTION(BlueprintCallable, Category = "Zf|Equipment|Expansion")
    void OnBackpackUnequipped(int32 ExtraSlots);

    // ----------------------------------------------------------
    // REPLICAÇÃO / CICLO DE VIDA
    // ----------------------------------------------------------

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void BeginPlay() override;

protected:

    UPROPERTY(Replicated)
    FZfEquipmentList EquipmentList;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Equipment")
    bool bOffHandSlotBlocked = false;

    UPROPERTY()
    TObjectPtr<UZfInventoryComponent> InventoryComponent;

    TMap<FGameplayTag, FZfActiveSetBonus> ActiveSetBonuses;

private:

    // ----------------------------------------------------------
    // FUNÇÕES INTERNAS — GERENCIAMENTO
    // ----------------------------------------------------------

    void Internal_InitializeEquipmentSlots();
    UAbilitySystemComponent* Internal_GetAbilitySystemComponent() const;
    void Internal_ApplyItemGameplayEffects(UZfItemInstance* ItemInstance);
    void Internal_RemoveItemGameplayEffects(UZfItemInstance* ItemInstance);
    void Internal_UpdateSetBonuses(UZfItemInstance* ItemInstance, bool bWasEquipped);
    void Internal_ApplySetBonuses(const FGameplayTag& SetTag, int32 PieceCount);
    void Internal_RemoveAllSetBonuses(const FGameplayTag& SetTag);
    void Internal_BlockOffHandSlot();
    void Internal_UnblockOffHandSlot();
    int32 InternalTryStackWithExistingItems(UZfItemInstance* ItemInstance, FGameplayTag SlotTag);

    const FZfEquipmentSlotEntry* Internal_FindSlotEntryConst(FGameplayTag EquipmentSlotTag) const;
    int32 Internal_GenerateReplicationKey() const;

    void InternalEquipItem(UZfItemInstance* InItemInstance, FGameplayTag ResolvedSlotTag = FGameplayTag());
    void InternalUnequipItem(UZfItemInstance* InItemInstance, FGameplayTag ResolvedSlotTag);
    FZfEquipmentSlotEntry* InternalFindSlotEntry(FGameplayTag SlotTag);

    // ----------------------------------------------------------
    // MODIFIER RULES
    // ----------------------------------------------------------

    TMap<TObjectPtr<UZfItemInstance>, TMap<FName, TObjectPtr<UZfModifierRule>>> ActiveModifierRules;

    void Internal_DeactivateModifierRules(UZfItemInstance* ItemInstance);
    void Internal_OnModifierRuleValueChanged(UZfItemInstance* ItemInstance, FName ModifierRowName, UAbilitySystemComponent* ASC);
    void Internal_ReapplyModifier(UZfItemInstance* ItemInstance, FZfAppliedModifier& Modifier, float NewFinalValue, UAbilitySystemComponent* ASC);

    // ----------------------------------------------------------
    // VALIDAÇÃO
    // ----------------------------------------------------------

    bool InternalCheckIsServer(const FString& FunctionName) const;
};