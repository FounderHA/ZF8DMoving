// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZfItemDefinition.generated.h"

class UZfItemFragment;


UCLASS(BlueprintType)
class ZF8DMOVING_API UZfItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	
	// Nome de exibição
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText ItemName;
	
	// Ícone na UI
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UTexture2D> ItemIcon;

	// Descrição
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText Description;

	// Mesh que aparece no mundo quando dropado
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMesh> StaticMesh;

	// Mesh que aparece equipado no personagem
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<USkeletalMesh> SkeletalMesh;
	
	// Fragments que esse tipo de item possui
	// EditInlineNew permite criar e editar direto no DataAsset
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fragments", Instanced)
	TArray<TObjectPtr<UZfItemFragment>> Fragments;

	// Busca um fragment pelo tipo — útil para ler dados padrão
	UFUNCTION(BlueprintCallable)
	const UZfItemFragment* FindFragment(TSubclassOf<UZfItemFragment> FragmentClass) const;

	template<typename T>
	const T* FindFragmentByClass() const
	{
		return Cast<T>(FindFragment(T::StaticClass()));
	}

	// Necessário para o AssetManager funcionar
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("ItemDefinition", GetFName());
	}
};
