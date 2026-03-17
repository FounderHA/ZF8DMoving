// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/Fragments/ZfItemFragment.h"

void UZfItemFragment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// base não tem props — subclasses chamam Super e adicionam as suas
}