#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Inventory/ZfItemInstance.h"
#include "ZfInventoryComponent.generated.h"

class UZfItemDefinition;
class UZfInventoryComponent;
class AZfItemPickup;

USTRUCT(BlueprintType)
struct FZfInventoryEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()
    
    UPROPERTY()
    TObjectPtr<UZfItemInstance> Item = nullptr;

    UPROPERTY()
    int32 SlotIndex = -1;

    void PreReplicatedRemove(const struct FZfInventoryList& InArraySerializer);
    void PostReplicatedAdd(const struct FZfInventoryList& InArraySerializer);
    void PostReplicatedChange(const struct FZfInventoryList& InArraySerializer);
};

USTRUCT()
struct FZfInventoryList : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FZfInventoryEntry> Entries;

    UPROPERTY(NotReplicated)
    TObjectPtr<UZfInventoryComponent> OwnerComponent = nullptr;

    // Cria uma nova instância a partir da definição e adiciona
    UZfItemInstance* AddItem(UZfItemDefinition* ItemDefinition, int32 SlotIndex);   
    
    // Adiciona uma instância já existente (ex: vinda do equipment)
    void AddItem(UZfItemInstance* Item, int32 SlotIndex);
    
    // Remove por instância
    void RemoveItem(UZfItemInstance* Item);
    
    // Remove por definição (remove o primeiro encontrado)
    void RemoveItem(UZfItemDefinition* ItemDefinition);

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FZfInventoryEntry, FZfInventoryList>(
            Entries, DeltaParams, *this
        );
    }
};

template<>
struct TStructOpsTypeTraits<FZfInventoryList> : public TStructOpsTypeTraitsBase2<FZfInventoryList>
{
    enum { WithNetDeltaSerializer = true };
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ZF8DMOVING_API UZfInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UZfInventoryComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Adiciona item a partir de uma definição (cria a instância no servidor)
    UFUNCTION(BlueprintCallable, Server, Reliable)
    void Server_AddItem(UZfItemDefinition* InItemDefinition);

    // Adiciona item em slot específico
    UFUNCTION(BlueprintCallable, Server, Reliable)
    void Server_AddItemAtSlot(UZfItemInstance* InItem, int32 SlotIndex);

    // Move item de um slot para outro
    UFUNCTION(BlueprintCallable, Server, Reliable)
    void Server_MoveItem(int32 FromSlot, int32 ToSlot);

    UFUNCTION(BlueprintCallable, Server, Reliable)
    void Server_RemoveItem(UZfItemInstance* InItem);

    UFUNCTION(BlueprintCallable, Server, Reliable)
    void Server_DropItem(UZfItemInstance* InItem, float DistanceDrop);

    // Expande slots — chamado pelo EquipmentComponent ao equipar mochila
    void AddExtraSlots(int32 Amount);

    // Verifica se Consigo Reduzir Slots slots — retorna false se itens nos slots extras
    bool CanRemoveExtraSlots(int32 Amount, int32 ReservedSlots = 0) const;

    // Tenta reduzir slots — retorna false se itens nos slots extras
    bool TryRemoveExtraSlots(int32 Amount, int32 ReservedSlots = 0);

    UFUNCTION(BlueprintCallable)
    TArray<UZfItemInstance*> GetAllItems() const;

    UFUNCTION(BlueprintCallable, BlueprintPure)
    UZfItemInstance* GetItemAtSlot(int32 SlotIndex) const;

    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool IsSlotEmpty(int32 SlotIndex) const;

    UFUNCTION(BlueprintCallable, BlueprintPure)
    int32 GetMaxSlots() const { return MaxSlots; }

    UFUNCTION(BlueprintCallable, BlueprintPure)
    int32 GetFirstEmptySlot() const;

    void MarkItemDirty(UZfItemInstance* InItem);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryChanged, UZfItemInstance*, AffectedItem);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotsChanged, int32, NewMaxSlots);

    UPROPERTY(BlueprintAssignable)
    FOnInventoryChanged OnItemAdded;

    UPROPERTY(BlueprintAssignable)
    FOnInventoryChanged OnItemRemoved;

    UPROPERTY(BlueprintAssignable)
    FOnSlotsChanged OnSlotsChanged;

private:
    UPROPERTY(Replicated)
    FZfInventoryList InventoryList;

    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    int32 BaseSlots = 20;

    UPROPERTY(ReplicatedUsing = OnRep_MaxSlots)
    int32 MaxSlots = 20;

    UFUNCTION()
    void OnRep_MaxSlots();

    
public:
    // Debug inventory
    UFUNCTION(BlueprintCallable)
    void DebugInventory();

    // Debug Fragment
    UFUNCTION(BlueprintCallable, Server, Reliable)
    void Server_DebugSetTestValue(int32 NewValue);
    
    
};