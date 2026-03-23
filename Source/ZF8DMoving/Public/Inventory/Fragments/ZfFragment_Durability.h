// Copyright ZfGame Studio. All Rights Reserved.
// ZfFragment_Durability.h
// Fragment que define a configuração ESTÁTICA de durabilidade.
//
// RESPONSABILIDADE DESTE FRAGMENT:
// Armazena apenas os dados de configuração (MaxDurability, LossPerUse, etc.)
// que são definidos no ItemDefinition (PDA) e nunca mudam.
//
// ONDE FICAM OS DADOS DINÂMICOS:
// CurrentDurability fica no UZfItemInstance e é replicado
// via FFastArraySerializer pelo UZfInventoryComponent.
//
// COMPORTAMENTO AO CHEGAR EM ZERO:
// - Todos os bônus são desativados (como se desequipado)
// - O item permanece equipado fisicamente
// - Widget notifica o player visualmente
// - O item pode ser reparado se bIsRepairable = true

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Fragments/ZfItemFragment.h"
#include "ZfFragment_Durability.generated.h"

UCLASS(DisplayName = "Fragment: Durability")
class ZF8DMOVING_API UZfFragment_Durability : public UZfItemFragment
{
    GENERATED_BODY()

public:

    // Durabilidade máxima do item — configurada no PDA, nunca muda.
    // O valor atual (CurrentDurability) vive no ItemInstance.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|Durability", meta = (ClampMin = "1.0"))
    float MaxDurability = 100.0f;

    // Quantidade de durabilidade perdida por uso/golpe.
    // Ex: 1.0 = perde 1 ponto por uso.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|Durability", meta = (ClampMin = "0.0"))
    float DurabilityLossPerUse = 1.0f;

    // Chamado pelo ItemInstance quando CurrentDurability chega a 0.
    // Aqui o fragment pode reagir ao evento (ex: log, VFX, etc.)
    // A desativação dos bônus é feita pelo EquipmentComponent.
    virtual void OnItemBroken(UZfItemInstance* OwningInstance) override;

    // Chamado pelo ItemInstance quando CurrentDurability volta acima de 0.
    // A reativação dos bônus é feita pelo EquipmentComponent.
    virtual void OnItemRepaired(UZfItemInstance* OwningInstance) override;

    
// ----------------------------------------------------------
// DEBUG
// ----------------------------------------------------------    
    
    virtual FString GetDebugString() const override
    {
        return FString::Printf(
            TEXT("[Fragment_Durability] MaxDurability: %.1f | LossPerUse: %.2f"), 
            MaxDurability, DurabilityLossPerUse);
    }
};