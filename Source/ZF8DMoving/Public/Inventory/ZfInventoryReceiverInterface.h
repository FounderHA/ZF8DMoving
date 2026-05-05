#pragma once
#include "CoreMinimal.h"
#include "ZfItemInstance.h"
#include "UObject/Interface.h"
#include "ZfInventoryReceiverInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UZfInventoryReceiverInterface : public UInterface
{
	GENERATED_BODY()
};

class ZF8DMOVING_API IZfInventoryReceiverInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Zf|DragDrop")
	void AddItemToTargetInterface(UObject* ItemComesFrom, UZfItemInstance* InItemInstance, int32 AmountToAdd,
		int32 SlotIndexComesFrom, int32 TargetSlotIndex,
		EZfRefinerySlotType SlotTypeComesFrom, EZfRefinerySlotType TargetSlotType,
		FGameplayTag SlotTagComesFrom, FGameplayTag TargetSlotTag);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Zf|DragDrop")
	bool RemoveItemFromTargetInterface(int32 ItemAmountToRemove, int32 TargetSlotIndex, EZfRefinerySlotType TargetSlotType, FGameplayTag TargetSlotTag);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Zf|DragDrop")
	void AddItemBackToTargetInterface(UZfItemInstance* InItemInstance, int32 TargetSlotIndex, EZfRefinerySlotType TargetSlotType, FGameplayTag TargetSlotTag);
};