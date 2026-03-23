// Copyright ZfGame Studio. All Rights Reserved.
// ZfItemFragment.h
// Classe base abstrata para todos os Fragmentos de Item.
//
// CONCEITO:
// Um Fragment é um objeto que adiciona dados e comportamento
// a um ItemInstance sem precisar criar subclasses de ItemInstance.
// Inspirado no sistema de Fragments do Lyra (Epic Games).
//
// IMPORTANTE SOBRE REPLICAÇÃO:
// Fragments NÃO são replicados. Eles vivem no ItemDefinition (PDA)
// como configuração estática (ex: "durabilidade máxima = 100").
// Os dados dinâmicos (ex: "durabilidade atual = 73") vivem no
// ItemInstance e são replicados via FFastArraySerializer
// pelo InventoryComponent.
//
// EXEMPLO DE USO:
// Um item "Bota de Corrida" tem os fragments:
//   - UZfFragment_Equippable  → define que equipa no slot Feet
//   - UZfFragment_Durability  → define durabilidade máxima 120
//   - UZfFragment_Quality     → define qualidade máxima +6
//
// COMO CRIAR UM NOVO FRAGMENT:
// 1. Crie uma nova classe C++ herdando de UZfItemFragment
// 2. Adicione as variáveis de dados estáticos que precisar
// 3. Override as funções virtuais necessárias
// 4. Adicione ao array Fragments no ItemDefinition (PDA)

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Inventory/ZfInventoryTypes.h"
#include "ZfItemFragment.generated.h"

// Forward declarations
class UZfItemInstance;
class UZfInventoryComponent;
class UZfEquipmentComponent;
class AActor;

UCLASS(Abstract,        // Não pode ser instanciado diretamente — apenas subclasses
    Blueprintable,      // Permite criar subclasses em Blueprint se necessário
    BlueprintType,      // Permite usar como variável em Blueprint
    EditInlineNew,      // Permite criar instâncias inline no PDA (ItemDefinition)
    DefaultToInstanced, // Instanciado automaticamente quando adicionado ao array
    CollapseCategories) // UI mais limpa no editor

class ZF8DMOVING_API UZfItemFragment : public UObject
{
    GENERATED_BODY()

public:

    // ----------------------------------------------------------
    // CICLO DE VIDA DO FRAGMENT
    // Funções virtuais chamadas pelo ItemInstance nos momentos
    // corretos. Cada fragment faz override apenas do que precisa.
    // Nenhuma dessas funções replica dados — apenas reagem a
    // eventos que já foram replicados pelo ItemInstance.
    // ----------------------------------------------------------

    // Chamado quando o ItemInstance é criado e inicializado.
    // Use para leitura dos dados estáticos do fragment.
    // @param OwningInstance — o ItemInstance que contém este fragment
    virtual void OnInstanceCreated(UZfItemInstance* OwningInstance);

    // Chamado quando o item é adicionado ao inventário.
    // @param OwningInstance — o ItemInstance dono
    // @param InventoryComponent — componente que recebeu o item
    virtual void OnItemAddedToInventory(UZfItemInstance* OwningInstance, UZfInventoryComponent* InventoryComponent);

    // Chamado quando o item é removido do inventário.
    // @param OwningInstance — o ItemInstance dono
    // @param InventoryComponent — componente que perdeu o item
    virtual void OnItemRemovedFromInventory(UZfItemInstance* OwningInstance, UZfInventoryComponent* InventoryComponent);

    // Chamado quando o item é equipado no EquipmentComponent.
    // Executado apenas no servidor — efeitos são replicados via GAS.
    // @param OwningInstance — o ItemInstance dono
    // @param EquipmentComponent — componente de equipamento
    // @param EquippingActor — ator que equipou (normalmente o Character)
    virtual void OnItemEquipped(UZfItemInstance* OwningInstance, UZfEquipmentComponent* EquipmentComponent, AActor* EquippingActor);

    // Chamado quando o item é desequipado.
    // Executado apenas no servidor — remoção de efeitos via GAS.
    // @param OwningInstance — o ItemInstance dono
    // @param EquipmentComponent — componente de equipamento
    // @param UnequippingActor — ator que desequipou
    virtual void OnItemUnequipped(UZfItemInstance* OwningInstance, UZfEquipmentComponent* EquipmentComponent, AActor* UnequippingActor);

    // Chamado quando a durabilidade do item chega a zero.
    // O item permanece equipado mas todos os bônus são desativados.
    // A notificação visual ao player é feita via Widget no cliente.
    // @param OwningInstance — o ItemInstance dono
    virtual void OnItemBroken(UZfItemInstance* OwningInstance);

    // Chamado quando o item é reparado (durabilidade > 0 após estar em 0).
    // Reativa os bônus do item.
    // @param OwningInstance — o ItemInstance dono
    virtual void OnItemRepaired(UZfItemInstance* OwningInstance);

    // Chamado quando o item é dropado no mundo.
    // @param OwningInstance — o ItemInstance dono
    // @param DropLocation — posição no mundo onde o item foi dropado
    virtual void OnItemDropped(UZfItemInstance* OwningInstance, const FVector& DropLocation);

    // ----------------------------------------------------------
    // DEBUG
    // ----------------------------------------------------------

    // Retorna string com informações do fragment para debug.
    // Cada subclasse deve fazer override para incluir seus dados.
    UFUNCTION(BlueprintCallable, Category = "Zf|Fragment|Debug")
    virtual FString GetDebugString() const
    {
        return FString::Printf(TEXT("[Fragment] %s"), *GetClass()->GetName());
    }

    // ----------------------------------------------------------
    // UTILITÁRIO
    // ----------------------------------------------------------

    // Retorna o nome da classe do fragment para identificação.
    // Útil para logs e busca de fragment por tipo no ItemInstance.
    UFUNCTION(BlueprintCallable, Category = "Zf|Fragment")
    FName GetFragmentTypeName() const
    {
        return GetClass()->GetFName();
    }
};