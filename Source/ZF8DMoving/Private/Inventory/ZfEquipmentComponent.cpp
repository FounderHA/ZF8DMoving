#include "Inventory/ZfEquipmentComponent.h"
#include "Inventory/ZfInventoryComponent.h"
#include "Inventory/ZfItemInstance.h"
#include "Inventory/ZfItemEquipped.h"
#include "Inventory/Fragments/ZfEquippableFragment.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"

UZfEquipmentComponent::UZfEquipmentComponent()
{
	SetIsReplicatedByDefault(true);
}

void UZfEquipmentComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UZfEquipmentComponent, EquippedItems);
}

FZfEquipmentEntry* UZfEquipmentComponent::FindEntry(EZfEquipSlot Slot)
{
	return EquippedItems.FindByPredicate([Slot](const FZfEquipmentEntry& E)
	{
		return E.Slot == Slot;
	});
}
void UZfEquipmentComponent::Server_EquipItem_Implementation(
	UZfItemInstance* InItem, UZfInventoryComponent* FromInventory)
{
	if (!InItem || !FromInventory) return;

	UZfEquippableFragment* Equippable =
		InItem->FindFragmentByClass<UZfEquippableFragment>();

	if (!Equippable || Equippable->EquipSlot == EZfEquipSlot::None) return;

	// Se slot ocupado desequipa primeiro
	if (IsSlotOccupied(Equippable->EquipSlot))
	{
		Server_UnequipItem_Implementation(Equippable->EquipSlot, FromInventory);
	}

	// Remove da Bag
	FromInventory->Server_RemoveItem_Implementation(InItem);

	// Pega o personagem dono
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character) return;

	// Spawna o AZfItemEquipped
	FActorSpawnParameters Params;
	Params.Owner = Character;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AZfItemEquipped* SpawnedActor = GetWorld()->SpawnActor<AZfItemEquipped>(
		AZfItemEquipped::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		Params
	);

	if (SpawnedActor)
	{
		SpawnedActor->InitializeWithItem(InItem,
			Character->GetMesh(), Equippable->SocketName);

		SpawnedActor->SetActorRelativeTransform(Equippable->AttachOffset);

		FZfEquipmentEntry NewEntry;
		NewEntry.Slot = Equippable->EquipSlot;
		NewEntry.Item = InItem;
		NewEntry.SpawnedActor = SpawnedActor;
		EquippedItems.Add(NewEntry);

		OnItemEquipped.Broadcast(Equippable->EquipSlot, InItem);
	}
}
void UZfEquipmentComponent::Server_UnequipItem_Implementation(
	EZfEquipSlot Slot, UZfInventoryComponent* ToInventory)
{
	if (Slot == EZfEquipSlot::None) return;

	FZfEquipmentEntry* Entry = FindEntry(Slot);
	if (!Entry || !Entry->Item) return;

	if (Entry->SpawnedActor)
	{
		Entry->SpawnedActor->Destroy();
		Entry->SpawnedActor = nullptr;
	}

	if (ToInventory)
	{
		ToInventory->Server_AddItem_Implementation(Entry->Item);
	}

	OnItemUnequipped.Broadcast(Slot, Entry->Item);

	EquippedItems.RemoveAll([Slot](const FZfEquipmentEntry& E)
	{
		return E.Slot == Slot;
	});
}

UZfItemInstance* UZfEquipmentComponent::GetEquippedItem(EZfEquipSlot Slot) const
{
	const FZfEquipmentEntry* Entry = EquippedItems.FindByPredicate(
		[Slot](const FZfEquipmentEntry& E) { return E.Slot == Slot; });
	if (!Entry) return nullptr;
	return Entry->Item;
}

bool UZfEquipmentComponent::IsSlotOccupied(EZfEquipSlot Slot) const
{
	const FZfEquipmentEntry* Entry = EquippedItems.FindByPredicate(
		[Slot](const FZfEquipmentEntry& E) { return E.Slot == Slot; });
	return Entry && Entry->Item != nullptr;
}