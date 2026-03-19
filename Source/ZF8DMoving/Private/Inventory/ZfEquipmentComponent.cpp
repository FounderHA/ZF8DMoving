#include "Inventory/ZfEquipmentComponent.h"
#include "Inventory/ZfInventoryComponent.h"
#include "Inventory/ZfItemInstance.h"
#include "Inventory/ZfItemEquipped.h"
#include "Inventory/Fragments/ZfEquippableFragment.h"
#include "Inventory/Fragments/ZfBackpackFragment.h"
#include "Player/ZfPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Tests/ToolMenusTestUtilities.h"

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

void UZfEquipmentComponent::Server_EquipItem_Implementation(UZfItemInstance* InItem, UZfInventoryComponent* FromInventory, EZfEquipSlot TargetSlot)
{
    if (!InItem || !FromInventory) return;

    UZfEquippableFragment* Equippable = InItem->FindFragmentByClass<UZfEquippableFragment>();

    if (!Equippable || Equippable->EquipSlot == EZfEquipSlot::None) return;

    // Verifica se o slot do item é compatível com o slot de destino
    if (Equippable->EquipSlot != TargetSlot) return;
        
    // Se slot ocupado desequipa primeiro
    if (IsSlotOccupied(Equippable->EquipSlot))
    {
        Server_UnequipItem_Implementation(Equippable->EquipSlot, FromInventory);
    }

    // Remove da Bag
    FromInventory->Server_RemoveItem_Implementation(InItem);

    // Pega o personagem dono
    AZfPlayerState* PS = Cast<AZfPlayerState>(GetOwner());
    if (!PS) return;
    
    ACharacter* Character = Cast<ACharacter>(PS->GetPawn());
    if (!Character) return;

    // Spawna o AZfItemEquipped
    FActorSpawnParameters Params;
    Params.Owner = Character;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AZfItemEquipped* SpawnedActor = GetWorld()->SpawnActor<AZfItemEquipped>(AZfItemEquipped::StaticClass(),FVector::ZeroVector,FRotator::ZeroRotator,Params);

    if (SpawnedActor)
    {
        SpawnedActor->InitializeWithItem(InItem,Character->GetMesh(), Equippable->SocketName);

        SpawnedActor->SetActorRelativeTransform(Equippable->AttachOffset);

        FZfEquipmentEntry NewEntry;
        NewEntry.Slot = Equippable->EquipSlot;
        NewEntry.Item = InItem;
        NewEntry.SpawnedActor = SpawnedActor;
        EquippedItems.Add(NewEntry);

        // Verifica se é mochila e expande o inventário
        UZfBackpackFragment* Backpack = InItem->FindFragmentByClass<UZfBackpackFragment>();
        if (Backpack)
        {
            FromInventory->AddExtraSlots(Backpack->ExtraSlots);
        }

        OnItemEquipped.Broadcast(Equippable->EquipSlot, InItem);
    }
}

void UZfEquipmentComponent::Server_UnequipItem_Implementation(EZfEquipSlot Slot, UZfInventoryComponent* ToInventory)
{
    if (Slot == EZfEquipSlot::None) return;

    FZfEquipmentEntry* Entry = FindEntry(Slot);
    if (!Entry || !Entry->Item) return;

    // Verifica se é mochila antes de desequipar
    UZfBackpackFragment* Backpack = Entry->Item->FindFragmentByClass<UZfBackpackFragment>();
    if (Backpack && ToInventory)
    {
        bool bCanUnequip = ToInventory->TryRemoveExtraSlots(Backpack->ExtraSlots);
        if (!bCanUnequip)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
                TEXT("Mova os itens dos slots extras antes de desequipar a mochila!"));
            return;
        }
    }

    if (Entry->SpawnedActor)
    {
        Entry->SpawnedActor->Destroy();
        Entry->SpawnedActor = nullptr;
    }

    if (ToInventory)
    {
        // Passar Item Definition e não Item Instance.
        //ToInventory->Server_AddItem(Entry->Item);
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

//
//============================ Debug Inventory ============================
//

#include "Inventory/ZfItemDefinition.h"

void UZfEquipmentComponent::DebugEquipament()
{
    for (const FZfEquipmentEntry& Entry : EquippedItems)
    {
        if (!Entry.Item) continue;

        FString ItemName = TEXT("sem definicao");
        if (Entry.Item->ItemDefinition)
        {
            UZfItemDefinition* Def = Entry.Item->ItemDefinition.Get();
            if (Def)
            {
                ItemName = Def->ItemName.ToString();
            }
        }

        FString SlotName = UEnum::GetValueAsString(Entry.Slot);

        FString ActorInfo = TEXT("sem actor spawnado");
        if (Entry.SpawnedActor)
        {
            ActorInfo = Entry.SpawnedActor->GetName();
        }

        GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow,
            FString::Printf(TEXT("[Slot %s] Item: %s | Fragments: %d | Actor: %s"),
                *SlotName,
                *ItemName,
                Entry.Item->Fragments.Num(),
                *ActorInfo));

        for (int32 i = 0; i < Entry.Item->Fragments.Num(); i++)
        {
            UZfItemFragment* Fragment = Entry.Item->Fragments[i];
            FString FragName = Fragment ? Fragment->GetClass()->GetName() : TEXT("nullptr");

            GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Cyan,
                FString::Printf(TEXT("  Fragment[%d]: %s"), i, *FragName));
        }
    }
}