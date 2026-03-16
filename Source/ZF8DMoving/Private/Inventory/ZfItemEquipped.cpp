// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/ZfItemEquipped.h"
#include "Inventory/ZfItemInstance.h"
#include "Inventory/ZfItemDefinition.h"

AZfItemEquipped::AZfItemEquipped()
{
	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = MeshComponent;
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AZfItemEquipped::InitializeWithItem(UZfItemInstance* InItem, USceneComponent* AttachTarget, FName SocketName)
{
	if (!InItem || !AttachTarget) return;

	Item = InItem;

	// Atualiza o visual
	if (Item->ItemDefinition && Item->ItemDefinition->StaticMesh)
	{
		MeshComponent->SetStaticMesh(Item->ItemDefinition->StaticMesh);
	}

	// Anexa no socket do personagem
	AttachToComponent(AttachTarget, 
		FAttachmentTransformRules::SnapToTargetIncludingScale, SocketName);
}
	