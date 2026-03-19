// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Fragments/ZfItemFragment.h"
#include "ZfTestFragment.generated.h"

/**
 * Fragment de teste para verificar replicação de dados mutáveis em runtime.
 * Adicione este Fragment em qualquer ItemDefinition no editor para testar.
 */
UCLASS(BlueprintType, meta=(DisplayName="Fragment: TEST (Debug)"))
class ZF8DMOVING_API UZfTestFragment : public UZfItemFragment
{
	GENERATED_BODY()

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Valor editável no DataAsset — modificado em runtime pelo servidor
	UPROPERTY(EditDefaultsOnly, ReplicatedUsing=OnRep_TestValue, BlueprintReadOnly, Category = "Test")
	int32 TestValue = 0;

	// Muda o valor no servidor e dispara a replicação para os clientes
	UFUNCTION(BlueprintCallable, Category = "Test|Debug")
	void SetTestValue(int32 NewValue);

protected:

	UFUNCTION()
	void OnRep_TestValue();
};