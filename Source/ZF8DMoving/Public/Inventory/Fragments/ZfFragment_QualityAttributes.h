// Copyright ZfGame Studio. All Rights Reserved.
// ZfFragment_QualityAttributes.h
// Fragment que define os atributos do item que evoluem por qualidade.
// Configure os atributos e seus valores por qualidade diretamente no ItemDefinition.

#pragma once

#include "CoreMinimal.h"
#include "ZfItemFragment.h"
#include "GameplayTagContainer.h"
#include "Inventory/ZfInventoryTypes.h"
#include "ZfFragment_QualityAttributes.generated.h"

// -----------------------------------------------------------
// FZfQualityAttributesEntry
// Um atributo do item com seus valores por qualidade.
// -----------------------------------------------------------

USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfQualityAttributesEntry
{
	GENERATED_BODY()

	// Nome legível exibido na UI
	// Ex: "Dano Físico", "Velocidade de Ataque"
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "QualityAttributes")
	FText DisplayName;

	// Tag que identifica este atributo
	// Ex: Attribute.Combat.PhysicalDamage
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "QualityAttributes", meta =(GameplayTagFilter = "GameplayEffect.type.AttributeSet"))
	FGameplayTag AttributeTag;

	// Valores absolutos por qualidade — índice 0 = Quality 0
	// Ex: [10, 15, 22, 30, 40, 52, 65, 80, 96, 115]
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "QualityAttributes")
	TArray<float> ValuesPerQuality;

	// Retorna o valor para a qualidade fornecida.
	// @param Quality — qualidade atual do item
	// @return valor do atributo ou 0 se não configurado
	float GetValueForQuality(int32 Quality) const
	{
		if (ValuesPerQuality.IsValidIndex(Quality))
			return ValuesPerQuality[Quality];

		UE_LOG(LogZfInventory, Warning,
			TEXT("FZfQualityAttributesEntry: Quality %d não configurada."), Quality);
		return 0.f;
	}
};

// -----------------------------------------------------------
// UZfFragment_QualityAttributes
// -----------------------------------------------------------
UCLASS(DisplayName = "Fragment: Quality Attributes")
class ZF8DMOVING_API UZfFragment_QualityAttributes : public UZfItemFragment
{
	GENERATED_BODY()

public:

	// Lista de atributos deste item com seus valores por qualidade.
	// Configure manualmente para cada ItemDefinition.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "QualityAttributes")
	TArray<FZfQualityAttributesEntry> Entries;
};