// Copyright ZfGame Studio. All Rights Reserved.
// ZfFragment_InventoryExpansion.h
// Fragment exclusivo de mochilas — expande o número de slots
// do inventário enquanto estiver equipado.
// Ao desequipar: slots extras são removidos.
// Ao equipar: slots extras são adicionados via GameplayEffect.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Fragments/ZfItemFragment.h"
#include "ZfFragment_InventoryExpansion.generated.h"

class UGameplayEffect;

UCLASS(DisplayName = "Fragment: Inventory Expansion")
class ZF8DMOVING_API UZfFragment_InventoryExpansion : public UZfItemFragment
{
	GENERATED_BODY()

public:

	// Quantidade de slots extras adicionados ao inventário.
	// Ex: Mochila pequena = 9, Mochila grande = 18
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|InventoryExpansion", meta = (ClampMin = "1", UIMin = "1"))
	int32 ExtraSlotCount = 9;

	// GameplayEffect que aplica a expansão de slots via GAS.
	// O GE deve modificar o atributo de slots máximos do inventário.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|InventoryExpansion")
	TSoftClassPtr<UGameplayEffect> ExpansionGameplayEffect;

	// Chamado ao equipar a mochila — aplica o GE de expansão
	virtual void OnItemEquipped(UZfItemInstance* OwningInstance, UZfEquipmentComponent* EquipmentComponent, AActor* EquippingActor) override;

	// Chamado ao desequipar a mochila — remove o GE de expansão
	virtual void OnItemUnequipped(UZfItemInstance* OwningInstance, UZfEquipmentComponent* EquipmentComponent, AActor* UnequippingActor) override;

// ----------------------------------------------------------
// DEBUG
// ----------------------------------------------------------
	
	virtual FString GetDebugString() const override
	{
		return FString::Printf(
			TEXT("[Fragment_InventoryExpansion] ExtraSlots: %d | Effect: %s"),
			ExtraSlotCount,
			*ExpansionGameplayEffect.ToString());
	}
};