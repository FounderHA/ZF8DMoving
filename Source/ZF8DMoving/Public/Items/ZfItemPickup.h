// Copyright ZfGame Studio. All Rights Reserved.
// ZfItemPickup.h
// Actor responsável pela representação física do item no mundo.
//
// CONCEITO:
// O ItemPickup é o ator visível no mundo que o jogador pode
// coletar. Ele carrega uma referência ao ItemInstance e ao
// ItemDefinition para exibir o mesh correto e permitir a coleta.
//
// FLUXO DE SPAWN:
// 1. Servidor spawna o ZfItemPickup via InventoryComponent::ServerRequestDropItem
// 2. O pickup recebe o ItemInstance e carrega o mesh do ItemDefinition
// 3. O mesh é replicado visualmente para todos os clientes
// 4. Ao coletar, o ItemInstance é transferido ao InventoryComponent do coletor
//
// FLUXO DE COLETA:
// Dois modos de coleta:
// - Overlap automático: ao entrar na área de coleta, o item é coletado
// - Interação direta: o jogador precisa pressionar uma tecla para coletar
// Configurável por item via bRequiresInteractionToPickup.
//
// REPLICAÇÃO:
// - O AActor replica automaticamente Transform (posição/rotação)
// - O ItemInstance é replicado para que clientes vejam o item correto
// - O mesh é carregado localmente em cada cliente via AssetManager
//
// SEGURANÇA:
// - Apenas o servidor pode efetivamente coletar o item
// - Clientes enviam RPC ao servidor solicitando coleta
// - O servidor valida antes de processar

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Inventory/ZfInventoryTypes.h"
#include "Inventory/ZfItemInstance.h"
#include "Inventory/ZfItemDefinition.h"
#include "InteractionSystem/ZfInteractionInterface.h"
#include "ZfItemPickup.generated.h"

// Forward declarations
class UZfInventoryComponent;
class USphereComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UWidgetComponent;
class URotatingMovementComponent;

// ============================================================
// DELEGATES
// ============================================================

// Disparado quando o item é coletado por um ator
// @param CollectorActor — ator que coletou o item
// @param ItemInstance — instância do item coletado
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemPickedUp, AActor*, CollectorActor, UZfItemInstance*, ItemInstance);

// ============================================================
// AZfItemPickup
// ============================================================

UCLASS(BlueprintType, Blueprintable)
class ZF8DMOVING_API AZfItemPickup : public AActor, public IZfInteractionInterface
{
    GENERATED_BODY()

public:

    AZfItemPickup();

    // ----------------------------------------------------------
    // CONFIGURAÇÃO
    // ----------------------------------------------------------

    // Se verdadeiro, o jogador precisa pressionar uma tecla
    // para coletar o item (interação direta).
    // Se falso, o item é coletado automaticamente ao entrar
    // na área de coleta (overlap).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Config")
    bool bRequiresInteractionToPickup = false;

    // Raio da área de coleta (SphereComponent).
    // Usado tanto para overlap automático quanto para
    // detectar se o jogador está próximo o suficiente
    // para interação direta.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Config", meta = (ClampMin = "10.0"))
    float PickupRadius = 100.0f;

    // Tempo em segundos antes do pickup ser destruído
    // automaticamente se não for coletado.
    // 0.0 = nunca destroi automaticamente.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Config", meta = (ClampMin = "0.0"))
    float AutoDestroyAfterSeconds = 300.0f;

    // Se verdadeiro, o pickup rotaciona suavemente no eixo Z
    // para dar destaque visual ao item no mundo.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Config")
    bool bRotateInWorld = true;

    // ----------------------------------------------------------
    // DELEGATES
    // ----------------------------------------------------------

    UPROPERTY(BlueprintAssignable, Category = "Pickup|Events")
    FOnItemPickedUp OnItemPickedUp;

    // ----------------------------------------------------------
    // INICIALIZAÇÃO
    // ----------------------------------------------------------

    // Inicializa o pickup com um ItemInstance já existente.
    // Chamado pelo InventoryComponent ao dropar um item.
    // Deve ser chamado apenas no servidor após o spawn.
    // @param InItemInstance — instância do item a exibir
    UFUNCTION(BlueprintCallable, Category = "Zf|Pickup")
    void InitializePickup(UZfItemInstance* InItemInstance);

    // ----------------------------------------------------------
    // COLETA
    // ----------------------------------------------------------

    // Tenta coletar o item para o ator fornecido.
    // Valida se o ator tem InventoryComponent e espaço disponível.
    // Deve ser chamado apenas no servidor.
    // @param CollectorActor — ator que está coletando o item
    // @return resultado da operação
    UFUNCTION(BlueprintCallable, Category = "Zf|Pickup")
    EZfItemMechanicResult TryCollectItem(AActor* CollectorActor);

    // ----------------------------------------------------------
    // CONSULTA
    // ----------------------------------------------------------

    // Retorna o ItemInstance armazenado neste pickup.
    UFUNCTION(BlueprintCallable, Category = "Zf|Pickup|Query")
    UZfItemInstance* GetItemInstance() const { return ItemInstance; }

    // Retorna o ItemDefinition do item neste pickup.
    UFUNCTION(BlueprintCallable, Category = "Zf|Pickup|Query")
    UZfItemDefinition* GetItemDefinition() const;

    // Verifica se o pickup ainda tem um item válido.
    UFUNCTION(BlueprintCallable, Category = "Zf|Pickup|Query")
    bool HasValidItem() const { return ItemInstance != nullptr; }

    // ----------------------------------------------------------
    // RPCs — CLIENT → SERVER
    // ----------------------------------------------------------

    // Requisição do cliente para coletar o item via interação direta.
    // O servidor valida se o jogador está próximo o suficiente.
    UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Zf|Pickup|RPC")
    void ServerRequestPickup(AActor* RequestingActor);

    // ----------------------------------------------------------
    // RPCs — SERVER → ALL CLIENTS (Multicast)
    // ----------------------------------------------------------

    // Notifica todos os clientes que o item foi coletado.
    // Dispara efeitos visuais/sonoros de coleta.
    UFUNCTION(NetMulticast, Reliable)
    void MulticastOnItemCollected(AActor* CollectorActor);

    // ----------------------------------------------------------
    // REPLICAÇÃO
    // ----------------------------------------------------------

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ----------------------------------------------------------
    // CICLO DE VIDA DO ATOR
    // ----------------------------------------------------------

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // ----------------------------------------------------------
    // DEBUG
    // ----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Zf|Pickup|Debug")
    void DrawDebugPickupInfo() const;

protected:

    // ----------------------------------------------------------
    // COMPONENTES
    // ----------------------------------------------------------

    // Componente raiz — esfera de colisão para overlap e coleta
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup|Components")
    TObjectPtr<USphereComponent> CollisionSphere;

    // Mesh estática do item no mundo
    // Carregado assincronamente via AssetManager
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup|Components")
    TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

    // Mesh esquelética (alternativa ao StaticMesh para alguns itens)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup|Components")
    TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;

    // Widget 3D exibido sobre o item com nome e raridade
    // Visível apenas quando o jogador está próximo
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup|Components")
    TObjectPtr<UWidgetComponent> ItemInfoWidget;
    
    virtual void PostInitializeComponents() override;

    // ----------------------------------------------------------
    // DADOS REPLICADOS
    // ----------------------------------------------------------

    // ItemInstance replicado para todos os clientes
    // Permite que clientes exibam informações corretas do item
    UPROPERTY(ReplicatedUsing = OnRep_ItemInstance, BlueprintReadOnly, Category = "Pickup")
    TObjectPtr<UZfItemInstance> ItemInstance;

    
    // ----------------------------------------------------------
    // REP NOTIFIES
    // ----------------------------------------------------------

    // Chamado nos clientes quando ItemInstance é replicado.
    // Atualiza o mesh e o widget com os dados do novo item.
    UFUNCTION()
    void OnRep_ItemInstance();

    // ----------------------------------------------------------
    // CALLBACKS DE OVERLAP
    // ----------------------------------------------------------

    // Chamado quando um ator entra na área de coleta.
    // Se bRequiresInteractionToPickup = false, coleta automaticamente.
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, 
        int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    // Chamado quando um ator sai da área de coleta.
    // Esconde o widget de interação se visível.
    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:

    // ----------------------------------------------------------
    // FUNÇÕES INTERNAS
    // ----------------------------------------------------------

    // Configura o widget com nome e raridade do item.
    void Internal_UpdateItemInfoWidget();

    // Inicia o timer de auto-destruição se configurado.
    void Internal_StartAutoDestroyTimer();

    // Callback do timer de auto-destruição.
    void Internal_OnAutoDestroyTimerExpired();

    // Valida se o ator coletor tem um InventoryComponent válido.
    // @param CollectorActor — ator a validar
    // @param OutInventory — InventoryComponent encontrado
    bool Internal_GetCollectorInventory(AActor* CollectorActor, UZfInventoryComponent*& OutInventory) const;

    // Valida se uma operação pode ser executada no servidor.
    bool Internal_CheckIsServer(const FString& FunctionName) const;

    // Handle do timer de auto-destruição
    FTimerHandle AutoDestroyTimerHandle;

    // Handle do asset de mesh carregado assincronamente
    // Guardado para cancelar carregamento se o pickup for destruído
    TSharedPtr<struct FStreamableHandle> MeshStreamingHandle;
};