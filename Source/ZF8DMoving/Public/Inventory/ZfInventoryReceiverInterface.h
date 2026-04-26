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
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Zf|Inventory")
	void QuickTransferItemFromInventory(UZfItemInstance* ItemInstance, int32 FromInventorySlot, UZfInventoryComponent* InventoryComponent);
};