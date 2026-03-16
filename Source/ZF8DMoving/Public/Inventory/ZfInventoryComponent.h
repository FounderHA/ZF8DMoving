// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Inventory/ZfItemInstance.h"
#include "ZfInventoryComponent.generated.h"

class UZfItemDefinition;
class UZfInventoryComponent;

USTRUCT(BlueprintType)
struct FZfInventoryEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UZfItemInstance> Item = nullptr;

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

    void AddItem(UZfItemInstance* Item);
    void RemoveItem(UZfItemInstance* Item);

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

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Server, Reliable)
    void Server_AddItemFromDefinition(UZfItemDefinition* ItemDefinition);

    UFUNCTION(BlueprintCallable, Server, Reliable)
    void Server_AddItem(UZfItemInstance* InItem);

    UFUNCTION(BlueprintCallable, Server, Reliable)
    void Server_RemoveItem(UZfItemInstance* InItem);

    UFUNCTION(BlueprintCallable, Server, Reliable)
    void Server_DropItem(UZfItemInstance* InItem, FVector Location);

    UFUNCTION(BlueprintCallable)
    TArray<UZfItemInstance*> GetAllItems() const;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryChanged, UZfItemInstance*, AffectedItem);

    UPROPERTY(BlueprintAssignable)
    FOnInventoryChanged OnItemAdded;

    UPROPERTY(BlueprintAssignable)
    FOnInventoryChanged OnItemRemoved;

private:
    UPROPERTY(Replicated)
    FZfInventoryList InventoryList;
};