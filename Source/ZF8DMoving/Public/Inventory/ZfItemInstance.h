// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Inventory/Fragments/ZfItemFragment.h"
#include "ZfItemInstance.generated.h"

class UZfItemDefinition;
class UZfItemFragment;

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class ZF8DMOVING_API UZfItemInstance : public UObject
{
	GENERATED_BODY()

public:
	// Servidor: cria os Fragments duplicando da Definition.
	// InOuter deve ser o Actor dono (PlayerState) — obrigatório para
	// AddReplicatedSubObject funcionar.
	void InitializeFromDefinition(UZfItemDefinition* InDefinition, UObject* InOuter);

	// Cliente: reconstrói os Fragments localmente a partir da ItemDefinition
	// já replicada. Chamado pelo PostReplicatedAdd do FastArray.
	// Os dados mutáveis (ex: CurrentStackSize) chegam depois via subobject replication
	// e sobrescrevem os valores default automaticamente.
	void InitializeFragmentsOnClient();

	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	UZfItemFragment* FindFragment(TSubclassOf<UZfItemFragment> FragmentClass) const;

	template<typename T>
	T* FindFragmentByClass() const
	{
		return Cast<T>(FindFragment(T::StaticClass()));
	}

	UPROPERTY(BlueprintReadOnly, Replicated)
	FGuid ItemGuid;

	UPROPERTY(BlueprintReadOnly, Replicated)
	TObjectPtr<UZfItemDefinition> ItemDefinition;

	// NÃO replicado via DOREPLIFETIME — o servidor registra cada Fragment
	// individualmente com AddReplicatedSubObject no InventoryComponent,
	// e o cliente os reconstrói localmente via InitializeFragmentsOnClient().
	UPROPERTY(BlueprintReadOnly)
	TArray<TObjectPtr<UZfItemFragment>> Fragments;
};