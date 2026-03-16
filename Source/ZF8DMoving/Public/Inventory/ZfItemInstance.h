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
	// Inicializa o item a partir de uma definição
	void InitializeFromDefinition(UZfItemDefinition* InDefinition);

	// Busca fragment por tipo — uso na Blueprint
	UFUNCTION(BlueprintCallable)
	UZfItemFragment* FindFragment(TSubclassOf<UZfItemFragment> FragmentClass) const;

	// Busca fragment por tipo — uso no C++
	template<typename T>
	T* FindFragmentByClass() const
	{
		return Cast<T>(FindFragment(T::StaticClass()));
	}

	// ID único dessa instância — útil para salvar/carregar
	UPROPERTY(BlueprintReadOnly)
	FGuid ItemGuid;

	// Definição que originou esse item
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UZfItemDefinition> ItemDefinition;

	// Fragments instanciados em runtime
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<TObjectPtr<UZfItemFragment>> Fragments;
};
