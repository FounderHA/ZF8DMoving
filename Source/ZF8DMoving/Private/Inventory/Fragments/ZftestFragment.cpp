// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/Fragments/ZfTestFragment.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"

void UZfTestFragment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UZfTestFragment, TestValue);
}

void UZfTestFragment::SetTestValue(int32 NewValue)
{
	// Proteção — só roda no servidor
	UWorld* World = GetOuter() ? GetOuter()->GetWorld() : nullptr;
	if (!World || World->GetNetMode() == NM_Client) return;

	TestValue = NewValue;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Green,
			FString::Printf(TEXT("[SERVER] ZfTestFragment::SetTestValue → %d | Outer: %s"),
				TestValue, *GetOuter()->GetName()));
	}
}

void UZfTestFragment::OnRep_TestValue()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Cyan,
			FString::Printf(TEXT("[CLIENT] ZfTestFragment::OnRep_TestValue → %d | Outer: %s"),
				TestValue, *GetOuter()->GetName()));
	}
}


