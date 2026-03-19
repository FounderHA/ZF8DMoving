// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/ZfItemInstance.h"
#include "Inventory/ZfItemDefinition.h"
#include "Inventory/Fragments/ZfItemFragment.h"
#include "Net/UnrealNetwork.h"

void UZfItemInstance::InitializeFromDefinition(UZfItemDefinition* InDefinition, UObject* InOuter)
{
	if (!InDefinition) return;

	ItemDefinition = InDefinition;
	ItemGuid = FGuid::NewGuid();

	UObject* FragmentOuter = InOuter ? InOuter : GetOuter();

	for (UZfItemFragment* Fragment : InDefinition->Fragments)
	{
		if (!Fragment) continue;

		// Outer dos Fragments = Actor (PlayerState), não o UZfItemInstance.
		// Isso é obrigatório para AddReplicatedSubObject funcionar.
		UZfItemFragment* NewFragment = DuplicateObject<UZfItemFragment>(Fragment, FragmentOuter);
		if (NewFragment)
		{
			NewFragment->OnItemCreated(this);
			Fragments.Add(NewFragment);
		}
	}
}

void UZfItemInstance::InitializeFragmentsOnClient()
{
	// Só executa no cliente e apenas se ainda não tem Fragments
	if (!ItemDefinition || Fragments.Num() > 0) return;

	// O Outer no cliente também deve ser o Actor dono
	UObject* FragmentOuter = GetOuter();

	for (UZfItemFragment* Fragment : ItemDefinition->Fragments)
	{
		if (!Fragment) continue;

		UZfItemFragment* NewFragment = DuplicateObject<UZfItemFragment>(Fragment, FragmentOuter);
		if (NewFragment)
		{
			Fragments.Add(NewFragment);
		}
	}
	// Nota: os dados mutáveis (CurrentStackSize etc.) chegam via subobject
	// replication logo após e sobrescrevem os valores default dos Fragments.
}

void UZfItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UZfItemInstance, ItemGuid);
	DOREPLIFETIME(UZfItemInstance, ItemDefinition);
	// Fragments intencionalmente fora — veja comentário no .h
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