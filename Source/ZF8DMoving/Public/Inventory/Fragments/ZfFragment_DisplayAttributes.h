// Copyright ZfGame Studio. All Rights Reserved.
// ZfFragment_DisplayAttributes.h
// Fragment que define quais atributos GAS serão exibidos na widget do item.
// Os valores são absolutos por qualidade — sem fórmulas, total controle por item.

#pragma once

#include "CoreMinimal.h"
#include "ZfItemFragment.h"
#include "AttributeSet.h"
#include "Inventory/ZfInventoryTypes.h"
#include "ZfFragment_DisplayAttributes.generated.h"

// -----------------------------------------------------------
// FZfDisplayAttributeEntry
// Um atributo a ser exibido na widget do item.
// Contém o nome, o atributo GAS e os valores por qualidade.
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfDisplayAttributeEntry
{
    GENERATED_BODY()

    // Nome legível exibido na UI do inventário
    // Ex: "Dano Físico", "Velocidade de Ataque"
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display|Attribute")
    FText DisplayName;

    // Atributo GAS que este entry representa
    // Ex: UZfCombatAttributeSet::GetPhysicalDamageAttribute()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display|Attribute")
    FGameplayAttribute Attribute;

    // Valores absolutos do atributo por nível de qualidade.
    // Índice 0 = Quality 0, Índice 1 = Quality 1, etc.
    // Ex: [10, 15, 22, 30, 40, 52, 65, 80, 96, 115]
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display|Attribute")
    TArray<float> ValuesPerQuality;

    // Retorna o valor do atributo para a qualidade fornecida.
    // @param Quality — qualidade atual do item
    // @return valor do atributo ou 0 se não configurado
    float GetValueForQuality(int32 Quality) const
    {
        if (ValuesPerQuality.IsValidIndex(Quality))
            return ValuesPerQuality[Quality];

        UE_LOG(LogZfInventory, Warning,
            TEXT("FZfDisplayAttributeEntry: Quality %d não configurada."), Quality);
        return 0.f;
    }
};

// -----------------------------------------------------------
// UZfFragment_DisplayAttributes
// Fragment que define quais atributos GAS serão exibidos
// na widget do item e seus valores por qualidade.
// -----------------------------------------------------------
UCLASS(DisplayName = "Fragment: Display Attributes")
class ZF8DMOVING_API UZfFragment_DisplayAttributes : public UZfItemFragment
{
    GENERATED_BODY()

public:

    // Lista de atributos a exibir na widget deste item.
    // Cada entry define o nome, atributo GAS e valores por qualidade.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Display")
    TArray<FZfDisplayAttributeEntry> AttributesToDisplay;
};