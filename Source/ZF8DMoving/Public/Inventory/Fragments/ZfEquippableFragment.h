// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "Inventory/Fragments/ZfItemFragment.h"
#include "Inventory/ZfEquipSlot.h"
#include "ZfEquippableFragment.generated.h"

UCLASS(BlueprintType, meta=(DisplayName="Fragment: Equippable"))
class ZF8DMOVING_API UZfEquippableFragment : public UZfItemFragment
{
	GENERATED_BODY()

public:
	// Qual slot esse item ocupa
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equip")
	EZfEquipSlot EquipSlot = EZfEquipSlot::None;

	// Socket do personagem onde o item será anexado
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equip")
	FName SocketName = NAME_None;

	// Ajuste de posição/rotação/escala ao equipar
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equip")
	FTransform AttachOffset;
};
