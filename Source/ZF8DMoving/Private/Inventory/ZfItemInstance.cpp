// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/ZfItemInstance.h"
#include "Inventory/ZfItemDefinition.h"
#include "Inventory/Fragments/ZfItemFragment.h"

void UZfItemInstance::InitializeFromDefinition(UZfItemDefinition* InDefinition)
{
	if (!InDefinition) return;

	ItemDefinition = InDefinition;
	ItemGuid = FGuid::NewGuid();

	// Copia os fragments da definição para essa instância
	for (UZfItemFragment* Fragment : InDefinition->Fragments)
	{
		if (!Fragment) continue;

		// Duplica o fragment para que cada item tenha sua própria cópia
		// sem isso todos os itens do mesmo tipo compartilhariam os mesmos dados
		UZfItemFragment* NewFragment = DuplicateObject<UZfItemFragment>(Fragment, this);
		if (NewFragment)
		{
			NewFragment->OnItemCreated(this);
			Fragments.Add(NewFragment);
		}
	}
}

UZfItemFragment* UZfItemInstance::FindFragment(TSubclassOf<UZfItemFragment> FragmentClass) const
{
	for (UZfItemFragment* Fragment : Fragments)
	{
		if (Fragment && Fragment->IsA(FragmentClass))
		{
			return Fragment;
		}
	}
	return nullptr;
}