// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/ZfItemDefinition.h"
#include "Inventory/Fragments/ZfItemFragment.h"

const UZfItemFragment* UZfItemDefinition::FindFragment(TSubclassOf<UZfItemFragment> FragmentClass) const
{
	for (const UZfItemFragment* Fragment : Fragments)
	{
		if (Fragment && Fragment->IsA(FragmentClass))
		{
			return Fragment;
		}
	}
	return nullptr;
}