// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ZfItemFragment.generated.h"


class UZfItemInstance;

UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class ZF8DMOVING_API UZfItemFragment : public UObject
{
	GENERATED_BODY()

public:
	// ✅ Necessário para replicar como referência
	virtual bool IsSupportedForNetworking() const override { return true; }
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// Chamado quando o item é criado a partir da definição
	virtual void OnItemCreated(UZfItemInstance* OwnerItem) {}

	// Chamado quando o item entra no inventário
	virtual void OnItemAdded(UZfItemInstance* OwnerItem) {}

	// Chamado quando o item sai do inventário
	virtual void OnItemRemoved(UZfItemInstance* OwnerItem) {}
};
