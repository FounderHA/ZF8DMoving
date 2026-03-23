// Copyright ZfGame Studio. All Rights Reserved.
// ZfFragment_Consumable.h
// Fragment que define o comportamento ao consumir o item.
// Ao consumir, aplica um GameplayEffect no ator consumidor
// e reduz o stack (ou remove o item se stack = 1).

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Fragments/ZfItemFragment.h"
#include "ZfFragment_Consumable.generated.h"

class UGameplayEffect;
class UGameplayAbility;

UCLASS(DisplayName = "Fragment: Consumable")
class ZF8DMOVING_API UZfFragment_Consumable : public UZfItemFragment
{
    GENERATED_BODY()

public:

    // GameplayEffect aplicado ao consumir o item.
    // Ex: Poção de vida → GE_RestoreHealth
    // Soft reference para carregamento assíncrono.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|Consumable")
    TSoftClassPtr<UGameplayEffect> ConsumptionGameplayEffect;

    // Nível do GameplayEffect ao aplicar.
    // Permite escalamento de efeitos por nível.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|Consumable",
        meta = (ClampMin = "1"))
    int32 EffectLevel = 1;

    // Se verdadeiro, o item é destruído ao consumir
    // (reduz stack em 1, remove o item se chegar a 0).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|Consumable")
    bool bConsumeOnUse = true;

    // Se verdadeiro, só pode ser usado uma única vez no jogo inteiro
    // (pergaminhos únicos, itens de progressão, etc.)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|Consumable")
    bool bIsSingleUsePerGame = false;

    // Cooldown em segundos antes de poder consumir novamente.
    // 0.0 = sem cooldown.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|Consumable",
        meta = (ClampMin = "0.0"))
    float ConsumptionCooldownSeconds = 0.0f;

// ----------------------------------------------------------
// DEBUG
// ----------------------------------------------------------	
    
    virtual FString GetDebugString() const override
    {
        return FString::Printf(
            TEXT("[Fragment_Consumable] Effect: %s | ConsumeOnUse: %s | Cooldown: %.1fs"),
            *ConsumptionGameplayEffect.ToString(),
            bConsumeOnUse ? TEXT("Yes") : TEXT("No"),
            ConsumptionCooldownSeconds);
    }
};