// Copyright ZfGame Studio. All Rights Reserved.
// ZfItemSpawner.cpp

#include "Items//ZfItemSpawner.h"
#include "Inventory/ZfItemInstance.h"
#include "Inventory/ZfItemDefinition.h"
#include "Inventory/ZfItemPickup.h"
#include "Items/ZfModifierRollSystem.h"
#include "Inventory/Fragments/ZfFragment_ItemUnique.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
 
// ============================================================
// FUNÇÃO PRINCIPAL
// ============================================================

FZfSpawnItemResult UZfItemSpawner::SpawnItemPickup(
    const UObject* WorldContextObject,
    UZfItemDefinition* ItemDefinition,
    int32 Tier,
    EZfItemRarity Rarity,
    const FVector& SpawnLocation)
{
    FZfSpawnItemResult SpawnResult;

    // Valida servidor
    if (!Internal_CheckIsServer(WorldContextObject, TEXT("SpawnItemPickup")))
    {
        SpawnResult.Result = EZfItemMechanicResult::Failed_InvalidOperation;
        return SpawnResult;
    }

    // Valida ItemDefinition
    if (!ItemDefinition)
    {
        UE_LOG(LogZfInventory, Error,
            TEXT("UZfItemSpawner::SpawnItemPickup — "
                 "ItemDefinition é nulo."));
        SpawnResult.Result = EZfItemMechanicResult::Failed_ItemNotFound;
        return SpawnResult;
    }

    // Obtém o World
    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        UE_LOG(LogZfInventory, Error,
            TEXT("UZfItemSpawner::SpawnItemPickup — "
                 "GetWorld() retornou nulo."));
        SpawnResult.Result = EZfItemMechanicResult::Failed_InvalidOperation;
        return SpawnResult;
    }

    // Cria o ItemInstance
    // Usa o WorldSettings como Outer pois não temos um ator específico aqui
    AActor* Outer = World->GetWorldSettings();
    UZfItemInstance* NewInstance =
        Internal_CreateItemInstance(Outer, ItemDefinition, Tier, Rarity);

    if (!NewInstance)
    {
        UE_LOG(LogZfInventory, Error,
            TEXT("UZfItemSpawner::SpawnItemPickup — "
                 "Falha ao criar ItemInstance para '%s'."),
            *ItemDefinition->ItemName.ToString());
        SpawnResult.Result = EZfItemMechanicResult::Failed_InvalidOperation;
        return SpawnResult;
    }

    // Spawna o pickup no mundo
    AZfItemPickup* SpawnedPickup =
        Internal_SpawnPickupActor(World, NewInstance, SpawnLocation);

    if (!SpawnedPickup)
    {
        UE_LOG(LogZfInventory, Error,
            TEXT("UZfItemSpawner::SpawnItemPickup — "
                 "Falha ao spawnar AZfItemPickup para '%s'."),
            *ItemDefinition->ItemName.ToString());
        SpawnResult.Result = EZfItemMechanicResult::Failed_InvalidOperation;
        return SpawnResult;
    }

    // Preenche o resultado
    SpawnResult.SpawnedPickup       = SpawnedPickup;
    SpawnResult.CreatedItemInstance = NewInstance;
    SpawnResult.Result              = EZfItemMechanicResult::Success;

    UE_LOG(LogZfInventory, Log,
        TEXT("UZfItemSpawner::SpawnItemPickup — "
             "Item '%s' spawnado em %s | Tier: %d | Rarity: %s | "
             "Modifiers: %d"),
        *ItemDefinition->ItemName.ToString(),
        *SpawnLocation.ToString(),
        Tier,
        *UEnum::GetValueAsString(Rarity),
        NewInstance->AppliedModifiers.Num());

    return SpawnResult;
}

FZfSpawnItemResult UZfItemSpawner::SpawnItemPickupInFrontOfActor(
    AActor* OwnerActor,
    UZfItemDefinition* ItemDefinition,
    int32 Tier,
    EZfItemRarity Rarity,
    float DistanceAhead)
{
    FZfSpawnItemResult SpawnResult;

    if (!OwnerActor)
    {
        UE_LOG(LogZfInventory, Error,
            TEXT("UZfItemSpawner::SpawnItemPickupInFrontOfActor — "
                 "OwnerActor é nulo."));
        SpawnResult.Result = EZfItemMechanicResult::Failed_InvalidOperation;
        return SpawnResult;
    }

    // Calcula a posição à frente do ator
    const FVector SpawnLocation =
        Internal_CalculateSpawnLocationInFront(OwnerActor, DistanceAhead);

    // Delega para a função principal
    return SpawnItemPickup(OwnerActor, ItemDefinition, Tier, Rarity, SpawnLocation);
}

AZfItemPickup* UZfItemSpawner::DropItemPickup(
    AActor* OwnerActor,
    UZfItemInstance* ItemInstance,
    float DistanceAhead)
{
    if (!OwnerActor)
    {
        UE_LOG(LogZfInventory, Error,
            TEXT("UZfItemSpawner::DropItemPickup — "
                 "OwnerActor é nulo."));
        return nullptr;
    }

    if (!ItemInstance)
    {
        UE_LOG(LogZfInventory, Error,
            TEXT("UZfItemSpawner::DropItemPickup — "
                 "ItemInstance é nulo."));
        return nullptr;
    }

    if (!Internal_CheckIsServer(OwnerActor, TEXT("DropItemPickup")))
    {
        return nullptr;
    }

    UWorld* World = OwnerActor->GetWorld();
    if (!World)
    {
        return nullptr;
    }

    // Calcula posição à frente do ator
    const FVector SpawnLocation =
        Internal_CalculateSpawnLocationInFront(OwnerActor, DistanceAhead);

    // Spawna o pickup com o item já existente
    AZfItemPickup* SpawnedPickup =
        Internal_SpawnPickupActor(World, ItemInstance, SpawnLocation);

    if (SpawnedPickup)
    {
        UE_LOG(LogZfInventory, Log,
            TEXT("UZfItemSpawner::DropItemPickup — "
                 "Item '%s' dropado em %s."),
            *ItemInstance->GetItemName().ToString(),
            *SpawnLocation.ToString());
    }

    return SpawnedPickup;
}

// ============================================================
// FUNÇÕES INTERNAS
// ============================================================

UZfItemInstance* UZfItemSpawner::Internal_CreateItemInstance(
    AActor* Outer,
    UZfItemDefinition* ItemDefinition,
    int32 Tier,
    EZfItemRarity Rarity)
{
    if (!Outer || !ItemDefinition)
    {
        return nullptr;
    }

    // Cria o ItemInstance com o ator como Outer
    // O Outer deve ser um AActor para replicação correta
    UZfItemInstance* NewInstance =
        NewObject<UZfItemInstance>(Outer);

    if (!NewInstance)
    {
        UE_LOG(LogZfInventory, Error,
            TEXT("UZfItemSpawner::Internal_CreateItemInstance — "
                 "Falha ao criar UZfItemInstance."));
        return nullptr;
    }

    // Inicializa o item com definição, tier e raridade
    NewInstance->InitializeItemInstance(ItemDefinition, Tier, Rarity);

    // Rola modifiers — apenas para itens não únicos
    // Itens únicos já têm modifiers fixos inicializados no InitializeItemInstance
    if (!ItemDefinition->HasFragment<UZfFragment_ItemUnique>())
    {
        UZfModifierRollSystem::RollModifiersForItem(NewInstance);
    }

    UE_LOG(LogZfInventory, Verbose,
        TEXT("UZfItemSpawner::Internal_CreateItemInstance — "
             "ItemInstance criado | '%s' | GUID: %s | Modifiers: %d"),
        *ItemDefinition->ItemName.ToString(),
        *NewInstance->GetItemGuid().ToString(),
        NewInstance->AppliedModifiers.Num());

    return NewInstance;
}

AZfItemPickup* UZfItemSpawner::Internal_SpawnPickupActor(
    UWorld* World,
    UZfItemInstance* ItemInstance,
    const FVector& SpawnLocation)
{
    if (!World || !ItemInstance)
    {
        return nullptr;
    }

    UZfItemDefinition* ItemDefinition = ItemInstance->GetItemDefinition();
    if (!ItemDefinition)
    {
        UE_LOG(LogZfInventory, Error,
            TEXT("UZfItemSpawner::Internal_SpawnPickupActor — "
                 "ItemDefinition é nulo no ItemInstance."));
        return nullptr;
    }

    // Determina a classe do pickup a spawnar
    // Usa a classe configurada no ItemDefinition
    // Se não houver, usa AZfItemPickup como fallback
    TSubclassOf<AZfItemPickup> PickupClass = AZfItemPickup::StaticClass();

    if (!ItemDefinition->ItemPickupActorClass.IsNull())
    {
        TSubclassOf<AZfItemPickup> LoadedClass =
            ItemDefinition->ItemPickupActorClass.LoadSynchronous();

        if (LoadedClass)
        {
            PickupClass = LoadedClass;
        }
        else
        {
            UE_LOG(LogZfInventory, Warning,
                TEXT("UZfItemSpawner::Internal_SpawnPickupActor — "
                     "Falha ao carregar ItemPickupActorClass de '%s'. "
                     "Usando AZfItemPickup padrão."),
                *ItemDefinition->ItemName.ToString());
        }
    }

    // Configura os parâmetros de spawn
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // Spawna o ator no mundo
    AZfItemPickup* SpawnedPickup = World->SpawnActor<AZfItemPickup>(
        PickupClass,
        SpawnLocation,
        FRotator::ZeroRotator,
        SpawnParams);

    if (!SpawnedPickup)
    {
        UE_LOG(LogZfInventory, Error,
            TEXT("UZfItemSpawner::Internal_SpawnPickupActor — "
                 "SpawnActor falhou para '%s'."),
            *ItemDefinition->ItemName.ToString());
        return nullptr;
    }

    // Inicializa o pickup com o ItemInstance
    SpawnedPickup->InitializePickup(ItemInstance);

    return SpawnedPickup;
}

FVector UZfItemSpawner::Internal_CalculateSpawnLocationInFront(
    AActor* Actor,
    float DistanceAhead)
{
    if (!Actor)
    {
        return FVector::ZeroVector;
    }

    // Posição do ator + direção forward * distância
    const FVector ActorLocation  = Actor->GetActorLocation();
    const FVector ForwardVector  = Actor->GetActorForwardVector();

    return ActorLocation + (ForwardVector * DistanceAhead);
}

bool UZfItemSpawner::Internal_CheckIsServer(
    const UObject* WorldContextObject,
    const FString& FunctionName)
{
    if (!WorldContextObject)
    {
        UE_LOG(LogZfInventory, Error,
            TEXT("UZfItemSpawner::%s — WorldContextObject é nulo."),
            *FunctionName);
        return false;
    }

    const UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        UE_LOG(LogZfInventory, Error,
            TEXT("UZfItemSpawner::%s — GetWorld() retornou nulo."),
            *FunctionName);
        return false;
    }

    if (World->GetNetMode() == NM_Client)
    {
        UE_LOG(LogZfInventory, Warning,
            TEXT("UZfItemSpawner::%s — "
                 "Operação de servidor chamada no cliente!"),
            *FunctionName);
        return false;
    }

    return true;
}