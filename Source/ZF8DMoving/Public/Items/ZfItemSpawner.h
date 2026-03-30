// Copyright ZfGame Studio. All Rights Reserved.
// ZfItemSpawner.h
// Sistema responsável por criar itens e spawnar seus pickups no mundo.
//
// CONCEITO:
// Classe utilitária com funções estáticas que:
// - Cria um ItemInstance a partir de um ItemDefinition
// - Rola os modifiers via UZfModifierRollSystem
// - Spawna o AZfItemPickup correto no mundo
// - Inicializa o pickup com o ItemInstance criado
//
// COMO USAR:
// UZfItemSpawner::SpawnItemPickup(WorldContextObject, ItemDefinition, Tier, Rarity, SpawnLocation);
//
// QUANDO CHAMAR:
// - Ao matar um inimigo (drop de loot)
// - Ao dropar um item do inventário
// - Em qualquer ponto do jogo que precise spawnar um item no mundo
// Deve ser chamado APENAS no servidor.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Inventory/ZfInventoryTypes.h"
#include "ZfItemSpawner.generated.h"

// Forward declarations
class UZfItemInstance;
class UZfItemDefinition;
class AZfItemPickup;

// ============================================================
// FZfSpawnItemResult
// ============================================================
// Resultado do spawn de um item no mundo.
// Contém o pickup spawnado e o item criado.
// ============================================================
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfSpawnItemResult
{
    GENERATED_BODY()

    // Pickup spawnado no mundo
    // nullptr se o spawn falhou
    UPROPERTY(BlueprintReadOnly, Category = "Spawn|Result")
    TObjectPtr<AZfItemPickup> SpawnedPickup = nullptr;

    // ItemInstance criado e inicializado
    // nullptr se a criação falhou
    UPROPERTY(BlueprintReadOnly, Category = "Spawn|Result")
    TObjectPtr<UZfItemInstance> CreatedItemInstance = nullptr;

    // Resultado da operação
    UPROPERTY(BlueprintReadOnly, Category = "Spawn|Result")
    EZfItemMechanicResult Result = EZfItemMechanicResult::Failed_InvalidOperation;

    // Retorna true se o spawn foi bem sucedido
    bool IsSuccess() const
    {
        return Result == EZfItemMechanicResult::Success &&
               SpawnedPickup != nullptr &&
               CreatedItemInstance != nullptr;
    }
};

// ============================================================
// UZfItemSpawner
// ============================================================

UCLASS()
class ZF8DMOVING_API UZfItemSpawner : public UObject
{
    GENERATED_BODY()

public:

    // ----------------------------------------------------------
    // FUNÇÃO PRINCIPAL
    // ----------------------------------------------------------

    // Cria um item e spawna seu pickup no mundo.
    // Ponto de entrada principal do sistema.
    // Deve ser chamado APENAS no servidor.
    //
    // @param WorldContextObject — qualquer UObject com acesso ao World
    //                             (ex: GameMode, Character, Enemy)
    // @param ItemDefinition     — definição do item a criar
    // @param Tier               — tier do item (0 a 5)
    // @param Rarity             — raridade do item
    // @param SpawnLocation      — posição no mundo onde spawnar
    // @return FZfSpawnItemResult com pickup, instance e resultado
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemSpawner",
        meta = (WorldContext = "WorldContextObject"))
    static FZfSpawnItemResult SpawnItemPickup(
        const UObject* WorldContextObject,
        UZfItemDefinition* ItemDefinition,
        int32 Tier,
        EZfItemRarity Rarity,
        const FVector& SpawnLocation);

    // Cria um item e spawna na frente do ator fornecido.
    // Calcula automaticamente a posição baseada na direção do ator.
    //
    // @param OwnerActor     — ator de referência para calcular posição
    // @param ItemDefinition — definição do item a criar
    // @param Tier           — tier do item
    // @param Rarity         — raridade do item
    // @param DistanceAhead  — distância à frente do ator (padrão: 100cm)
    // @return FZfSpawnItemResult com pickup, instance e resultado
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemSpawner")
    static FZfSpawnItemResult SpawnItemPickupInFrontOfActor(
        AActor* OwnerActor,
        UZfItemDefinition* ItemDefinition,
        int32 Tier,
        EZfItemRarity Rarity,
        float DistanceAhead = 100.0f);

    // Dropa um ItemInstance já existente no mundo.
    // Usado ao dropar item do inventário — não cria novo item,
    // apenas spawna o pickup com o item já existente.
    //
    // @param OwnerActor    — ator que está dropando o item
    // @param ItemInstance  — item a ser dropado
    // @param DistanceAhead — distância à frente do ator
    // @return AZfItemPickup spawnado ou nullptr se falhou
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemSpawner")
    static AZfItemPickup* DropItemPickup(
        AActor* OwnerActor,
        UZfItemInstance* ItemInstance,
        float DistanceAhead = 100.0f);

private:

    // ----------------------------------------------------------
    // FUNÇÕES INTERNAS
    // ----------------------------------------------------------

    // Cria e inicializa um ItemInstance a partir de um ItemDefinition.
    // Rola modifiers via UZfModifierRollSystem.
    // @param Outer          — Outer do UObject (deve ser o OwnerActor)
    // @param ItemDefinition — definição do item
    // @param Tier           — tier do item
    // @param Rarity         — raridade do item
    // @return ItemInstance criado ou nullptr se falhou
    static UZfItemInstance* Internal_CreateItemInstance(
        AActor* Outer,
        UZfItemDefinition* ItemDefinition,
        int32 Tier,
        EZfItemRarity Rarity);

    // Spawna o AZfItemPickup correto no mundo.
    // Usa o ItemPickupActorClass configurado no ItemDefinition.
    // Se não houver classe configurada, usa AZfItemPickup padrão.
    // @param World         — mundo onde spawnar
    // @param ItemInstance  — item que o pickup vai carregar
    // @param SpawnLocation — posição de spawn
    // @return AZfItemPickup spawnado ou nullptr se falhou
    static AZfItemPickup* Internal_SpawnPickupActor(
        UWorld* World,
        UZfItemInstance* ItemInstance,
        const FVector& SpawnLocation);

    // Calcula a posição à frente de um ator.
    // @param Actor         — ator de referência
    // @param DistanceAhead — distância à frente
    // @return posição calculada no mundo
    static FVector Internal_CalculateSpawnLocationInFront(
        AActor* Actor,
        float DistanceAhead);

    // Valida se a operação pode ser executada no servidor.
    // @param WorldContextObject — contexto de mundo
    // @param FunctionName       — nome da função para log
    static bool Internal_CheckIsServer(
        const UObject* WorldContextObject,
        const FString& FunctionName);
};