#pragma once

#include "CoreMinimal.h"
#include "ZfEquipSlot.generated.h"

UENUM(BlueprintType)
enum class EZfEquipSlot : uint8
{
	None        UMETA(DisplayName = "None"),
	MainHand    UMETA(DisplayName = "Main Hand"),
	OffHand     UMETA(DisplayName = "Off Hand"),
	Head        UMETA(DisplayName = "Head"),
	Chest       UMETA(DisplayName = "Chest"),
	Legs        UMETA(DisplayName = "Legs"),
	Feet        UMETA(DisplayName = "Feet"),
	Hands       UMETA(DisplayName = "Hands"),
	Cape        UMETA(DisplayName = "Cape"),
	Backpack    UMETA(DisplayName = "Backpack"),
	Ring_1      UMETA(DisplayName = "Ring 1"),
	Ring_2      UMETA(DisplayName = "Ring 2"),
	Necklace    UMETA(DisplayName = "Necklace")
};