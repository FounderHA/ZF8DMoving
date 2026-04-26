// Copyright ZfGame Studio. All Rights Reserved.
// ZfRefineryComponent.cpp

#include "Systems/RefinerySystem/ZfRefineryComponent.h"
#include "Systems/RefinerySystem/ZfRefineryData.h"
#include "Systems/RefinerySystem/ZfRefineryRecipe.h"
#include "Systems/RefinerySystem/ZfRefineryTypes.h"
#include "Inventory/Fragments/ZfFragment_Catalyst.h"
#include "Inventory/Fragments/ZfFragment_Stackable.h"
#include "Inventory/ZfItemInstance.h"
#include "Inventory/ZfItemDefinition.h"
#include "Inventory/ZfInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"


// ============================================================
// FZfRefinerySlot — Fast Array Callbacks
// ============================================================

void FZfRefinerySlot::PreReplicatedRemove(const FZfRefinerySlotList& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		if (UZfRefineryComponent* Comp = Cast<UZfRefineryComponent>(InArraySerializer.OwnerComponent))
		{
			Comp->OnRefinerySlotChanged.Broadcast(ItemInstance, SlotIndex);
		}
	}
}

void FZfRefinerySlot::PostReplicatedAdd(const FZfRefinerySlotList& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		if (UZfRefineryComponent* Comp = Cast<UZfRefineryComponent>(InArraySerializer.OwnerComponent))
		{
			Comp->OnRefinerySlotChanged.Broadcast(ItemInstance, SlotIndex);
		}
	}
}

void FZfRefinerySlot::PostReplicatedChange(const FZfRefinerySlotList& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		if (UZfRefineryComponent* Comp = Cast<UZfRefineryComponent>(InArraySerializer.OwnerComponent))
		{
			Comp->OnRefinerySlotChanged.Broadcast(ItemInstance, SlotIndex);
		}
	}
}


// ============================================================
// Constructor
// ============================================================

UZfRefineryComponent::UZfRefineryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
}


// ============================================================
// QuickTransferItemFromInventory
// ============================================================

void UZfRefineryComponent::QuickTransferItemFromInventory_Implementation(UZfItemInstance* ItemInstance, int32 FromInventorySlot, UZfInventoryComponent* InInventoryComponent)
{
	TryAddItem(InputSlots, ItemInstance, INDEX_NONE, InInventoryComponent);
}


// ============================================================
// BeginPlay
// ============================================================

void UZfRefineryComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner()->HasAuthority()) return;

	if (!RefineryData)
	{
		UE_LOG(LogZfRefinery, Error,
			TEXT("ZfRefineryComponent [%s]: RefineryData não configurado! O componente não funcionará."),
			*GetOwner()->GetName());
		return;
	}

	// Configura cada SlotList com seu tipo, capacidade e owner.
	// Slots não são pré-alocados — só existem quando têm item.
	InputSlots.SlotType    = EZfRefinerySlotType::Input;
	InputSlots.Capacity    = RefineryData->InputSlotCapacity;
	InputSlots.OwnerComponent = this;

	OutputSlots.SlotType   = EZfRefinerySlotType::Output;
	OutputSlots.Capacity   = RefineryData->OutputSlotCapacity;
	OutputSlots.OwnerComponent = this;

	CatalystSlots.SlotType = EZfRefinerySlotType::Catalyst;
	CatalystSlots.Capacity = RefineryData->CatalystSlotCount;
	CatalystSlots.OwnerComponent = this;
}


// ============================================================
// GetLifetimeReplicatedProps
// ============================================================

void UZfRefineryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UZfRefineryComponent, InputSlots);
	DOREPLIFETIME(UZfRefineryComponent, OutputSlots);
	DOREPLIFETIME(UZfRefineryComponent, CatalystSlots);
	DOREPLIFETIME(UZfRefineryComponent, bIsRefining);
	DOREPLIFETIME(UZfRefineryComponent, ActiveRecipe);
	DOREPLIFETIME(UZfRefineryComponent, RecipeElapsedTime);
	DOREPLIFETIME(UZfRefineryComponent, LastSnapshotTime);
	DOREPLIFETIME(UZfRefineryComponent, CurrentCycleTotalTime);
	DOREPLIFETIME(UZfRefineryComponent, ActiveCatalystState);
}


// ============================================================
// GetSlotListByType
// ============================================================

FZfRefinerySlotList& UZfRefineryComponent::GetSlotListByType(EZfRefinerySlotType SlotType)
{
	switch (SlotType)
	{
		case EZfRefinerySlotType::Output:   return OutputSlots;
		case EZfRefinerySlotType::Catalyst: return CatalystSlots;
		default:                            return InputSlots;
	}
}


// ============================================================
// GetSlotItem
// ============================================================

UZfItemInstance* UZfRefineryComponent::GetSlotItem(const FZfRefinerySlotList& SlotList, int32 SlotIndex) const
{
	for (const FZfRefinerySlot& Slot : SlotList.Slots)
	{
		if (Slot.SlotIndex == SlotIndex) return Slot.ItemInstance;
	}
	return nullptr;
}


// ============================================================
// GetFirstEmptySlot
// ============================================================

int32 UZfRefineryComponent::GetFirstEmptySlot(const FZfRefinerySlotList& SlotList) const
{
	TSet<int32> Occupied;
	for (const FZfRefinerySlot& Slot : SlotList.Slots)
	{
		Occupied.Add(Slot.SlotIndex);
	}

	for (int32 i = 0; i < SlotList.Capacity; i++)
	{
		if (!Occupied.Contains(i)) return i;
	}

	return INDEX_NONE;
}


// ============================================================
// FindPartialStack
// ============================================================

int32 UZfRefineryComponent::FindPartialStack(const FZfRefinerySlotList& SlotList, const UZfItemDefinition* ItemDef) const
{
	if (!ItemDef) return INDEX_NONE;

	for (const FZfRefinerySlot& Slot : SlotList.Slots)
	{
		if (!Slot.ItemInstance) continue;
		if (Slot.ItemInstance->GetItemDefinition() != ItemDef) continue;

		const UZfFragment_Stackable* StackFrag = Slot.ItemInstance->GetFragment<UZfFragment_Stackable>();
		if (StackFrag && Slot.ItemInstance->GetCurrentStack() < StackFrag->MaxStackSize)
		{
			return Slot.SlotIndex;
		}
	}

	return INDEX_NONE;
}


// ============================================================
// TryAddToStack
// ============================================================

int32 UZfRefineryComponent::TryAddToStack(FZfRefinerySlotList& SlotList, int32 SlotIndex, int32 Amount)
{
	if (!CheckIsServer(TEXT("TryAddToStack"))) return Amount;

	UZfItemInstance* Item = GetSlotItem(SlotList, SlotIndex);
	if (!Item) return Amount;

	const int32 Overflow = Item->AddToStack(Amount);

	for (FZfRefinerySlot& Slot : SlotList.Slots)
	{
		if (Slot.SlotIndex == SlotIndex)
		{
			SlotList.MarkItemDirty(Slot);
			break;
		}
	}

	OnRefinerySlotChanged.Broadcast(Item, SlotIndex);
	return Overflow;
}

bool UZfRefineryComponent::IsItemAllowedAt(FZfRefinerySlotList& SlotList, const FGameplayTagContainer& ItemTags, UZfItemInstance* ItemInstance) const
{
	if (SlotList.SlotType == EZfRefinerySlotType::Catalyst)
	{
		if (!RefineryData->IsItemAllowedAsCatalyst(ItemTags))
		{
			UE_LOG(LogZfRefinery, Warning,
				TEXT("ZfRefineryComponent: Item '%s' não tem ZfFragment_Catalyst."),
				*ItemInstance->GetItemDefinition()->GetName());
			return false;
		}
		if (!ItemInstance->GetFragment<UZfFragment_Catalyst>())
		{
			UE_LOG(LogZfRefinery, Warning,
				TEXT("ZfRefineryComponent: Item '%s' não tem ZfFragment_Catalyst."),
				*ItemInstance->GetItemDefinition()->GetName());
			return false;
		}
		return true;
	}
	
	if (RefineryData->IsItemAllowedInInput(ItemTags))return true;

	
	return false;
}


// ============================================================
// ApplyRemoveSideEffects
// ============================================================

void UZfRefineryComponent::ApplyRemoveSideEffects(FZfRefinerySlotList& SlotList)
{
	switch (SlotList.SlotType)
	{
		case EZfRefinerySlotType::Input:
			if (bIsRefining && ActiveRecipe && !CanSatisfyRecipe(ActiveRecipe))
			{
				StopRefining(true);
			}
			break;

		case EZfRefinerySlotType::Output:
			if (!bIsRefining) TryStartNextCycle();
			break;

		case EZfRefinerySlotType::Catalyst:
			if (bIsRefining && ActiveCatalystState.IsActive())
			{
				TakeSnapshot();
				ActiveCatalystState.Reset();
				RescheduleCycleTimer();
				OnCatalystConsumed.Broadcast(1.0f);
			}
			break;
	}
}


// ============================================================
// InternalTryStackWithExistingItems
// ============================================================

bool UZfRefineryComponent::InternalTryStackWithExistingItems(FZfRefinerySlotList& SlotList, UZfItemInstance* ItemInstance)
{
	if (!ItemInstance) return false;

	const UZfItemDefinition* IncomingDef = ItemInstance->GetItemDefinition();
	if (!IncomingDef) return false;

	for (FZfRefinerySlot& Slot : SlotList.Slots)
	{
		if (!Slot.ItemInstance) continue;
		if (Slot.ItemInstance->GetItemDefinition() != IncomingDef) continue;

		const int32 Overflow = Slot.ItemInstance->AddToStack(ItemInstance->GetCurrentStack());
		ItemInstance->SetCurrentStack(Overflow);
		SlotList.MarkItemDirty(Slot);
		OnRefinerySlotChanged.Broadcast(Slot.ItemInstance, Slot.SlotIndex);

		if (Overflow <= 0) return true;
	}

	return false;
}


// ============================================================
// InternalAddItemToSlot
// ============================================================

void UZfRefineryComponent::InternalAddItemToSlot(FZfRefinerySlotList& SlotList, UZfItemInstance* ItemInstance, int32 SlotIndex)
{
	check(ItemInstance != nullptr);

	FZfRefinerySlot NewSlot;
	NewSlot.SlotIndex    = SlotIndex;
	NewSlot.ItemInstance = ItemInstance;
	SlotList.Slots.Add(NewSlot);

	AddReplicatedSubObject(ItemInstance);
}


// ============================================================
// InternalRemoveItemFromSlot
// ============================================================

UZfItemInstance* UZfRefineryComponent::InternalRemoveItemFromSlot(FZfRefinerySlotList& SlotList, int32 SlotIndex)
{
	UZfItemInstance* Found = GetSlotItem(SlotList, SlotIndex);
	if (!Found) return nullptr;

	SlotList.Slots.RemoveAll([SlotIndex](const FZfRefinerySlot& Slot)
	{
		return Slot.SlotIndex == SlotIndex;
	});
	SlotList.MarkArrayDirty();

	RemoveReplicatedSubObject(Found);
	OnRefinerySlotChanged.Broadcast(nullptr, SlotIndex);
	return Found;
}


// ============================================================
// TryAddItem — genérico para Input, Output e Catalyst
// ============================================================

bool UZfRefineryComponent::TryAddItem(FZfRefinerySlotList& SlotList, UZfItemInstance* ItemInstance, int32 SlotIndex, UZfInventoryComponent* InventoryComp)
{
	if (!CheckIsServer(TEXT("TryAddItem"))) return false;
	if (!ItemInstance || !RefineryData) return false;

	const FGameplayTagContainer& ItemTags = ItemInstance->GetItemDefinition()->ItemTags;

	if (!IsItemAllowedAt(SlotList, ItemTags, ItemInstance)) return false;
	
	const bool bIsStackable = (ItemInstance->GetFragment<UZfFragment_Stackable>() != nullptr);

	if (bIsStackable)
	{
		const int32 OriginalStack = ItemInstance->GetCurrentStack();

		if (InternalTryStackWithExistingItems(SlotList, ItemInstance))
		{
			// Completamente absorvido por stacks existentes
			if (InventoryComp) InventoryComp->ServerTryRemoveAmountFromStack(ItemInstance, OriginalStack);
			if (!bIsRefining) TryStartNextCycle();
			return true;
		}

		// Overflow em ItemInstance->GetCurrentStack()
		const int32 Absorbed   = OriginalStack - ItemInstance->GetCurrentStack();
		const int32 TargetSlot = (SlotIndex == INDEX_NONE) ? GetFirstEmptySlot(SlotList) : SlotIndex;

		if (TargetSlot == INDEX_NONE)
		{
			// Sem slot disponível — remove do inventário apenas o que foi absorvido
			if (InventoryComp && Absorbed > 0)
				InventoryComp->ServerTryRemoveAmountFromStack(ItemInstance, Absorbed);
			return false;
		}

		InternalAddItemToSlot(SlotList, ItemInstance, TargetSlot);
		SlotList.MarkArrayDirty();
		OnRefinerySlotChanged.Broadcast(ItemInstance, SlotIndex);
		
		if (InventoryComp) InventoryComp->ServerTryRemoveAmountFromStack(ItemInstance, OriginalStack);
		
		if (!bIsRefining) TryStartNextCycle();
		return true;
	}

	// Não stackável — insere direto em slot vazio
	const int32 TargetSlot = (SlotIndex == INDEX_NONE) ? GetFirstEmptySlot(SlotList) : SlotIndex;
	if (TargetSlot == INDEX_NONE) return false;

	InternalAddItemToSlot(SlotList, ItemInstance, TargetSlot);
	
	SlotList.MarkArrayDirty();
	OnRefinerySlotChanged.Broadcast(ItemInstance, SlotIndex);
	
	if (!bIsRefining) TryStartNextCycle();
	return true;
}


// ============================================================
// TryRemoveItem — genérico para Input, Output e Catalyst
// ============================================================

bool UZfRefineryComponent::TryRemoveItem(FZfRefinerySlotList& SlotList, int32 SlotIndex)
{
	if (!CheckIsServer(TEXT("TryRemoveItem"))) return false;

	UZfItemInstance* Removed = InternalRemoveItemFromSlot(SlotList, SlotIndex);
	if (!Removed) return false;

	ApplyRemoveSideEffects(SlotList);
	return true;
}


// ============================================================
// TryTransferToInventory — genérico para Input, Output e Catalyst
// ============================================================

bool UZfRefineryComponent::TryTransferToInventory(FZfRefinerySlotList& SlotList, int32 SlotIndex, UZfInventoryComponent* InventoryComp)
{
	if (!CheckIsServer(TEXT("TryTransferToInventory"))) return false;
	if (!InventoryComp) return false;

	UZfItemInstance* Item = GetSlotItem(SlotList, SlotIndex);
	if (!Item) return false;

	const int32 OriginalStack = Item->GetCurrentStack();
	const int32 Overflow      = 0;//InventoryComp->TryAddPartial(Item);

	if (Overflow <= 0)
	{
		InternalRemoveItemFromSlot(SlotList, SlotIndex);
		ApplyRemoveSideEffects(SlotList);
	}
	// Se Overflow > 0, o item permanece no slot com CurrentStack já atualizado

	return Overflow < OriginalStack;
}


// ============================================================
// Server RPCs
// ============================================================

void UZfRefineryComponent::ServerTryAddItem_Implementation(EZfRefinerySlotType SlotType, UZfItemInstance* ItemInstance, int32 SlotIndex, UZfInventoryComponent* InventoryComp)
{
	TryAddItem(GetSlotListByType(SlotType), ItemInstance, SlotIndex, InventoryComp);
}

void UZfRefineryComponent::ServerTryRemoveItem_Implementation(EZfRefinerySlotType SlotType, int32 SlotIndex)
{
	TryRemoveItem(GetSlotListByType(SlotType), SlotIndex);
}

void UZfRefineryComponent::ServerTryTransferToInventory_Implementation(EZfRefinerySlotType SlotType, int32 SlotIndex, UZfInventoryComponent* InventoryComp)
{
	TryTransferToInventory(GetSlotListByType(SlotType), SlotIndex, InventoryComp);
}

void UZfRefineryComponent::ServerTrySetManualRecipeQueue_Implementation(const TArray<UZfRefineryRecipe*>& NewQueue)
{
	SetManualRecipeQueue(NewQueue);
}

void UZfRefineryComponent::ServerTryClearManualRecipeQueue_Implementation()
{
	ClearManualRecipeQueue();
}


// ============================================================
// SetManualRecipeQueue / ClearManualRecipeQueue
// ============================================================

void UZfRefineryComponent::SetManualRecipeQueue(const TArray<UZfRefineryRecipe*>& NewQueue)
{
	if (!CheckIsServer(TEXT("SetManualRecipeQueue"))) return;
	ManualRecipeQueue = TArray<TObjectPtr<UZfRefineryRecipe>>(NewQueue);
}

void UZfRefineryComponent::ClearManualRecipeQueue()
{
	if (!CheckIsServer(TEXT("ClearManualRecipeQueue"))) return;
	ManualRecipeQueue.Empty();
}


// ============================================================
// GetCurrentCycleProgress
// ============================================================

float UZfRefineryComponent::GetCurrentCycleProgress() const
{
	if (!bIsRefining || CurrentCycleTotalTime <= 0.0f) return 0.0f;

	const float Multiplier = ActiveCatalystState.IsActive() ? ActiveCatalystState.SpeedMultiplier : 1.0f;
	const UWorld* World    = GetWorld();
	const float Now        = World ? World->GetTimeSeconds() : 0.0f;
	const float Progress   = RecipeElapsedTime + ((Now - LastSnapshotTime) * Multiplier);

	return FMath::Clamp(Progress / CurrentCycleTotalTime, 0.0f, 1.0f);
}


// ============================================================
// TryStartNextCycle
// ============================================================

void UZfRefineryComponent::TryStartNextCycle()
{
	if (!GetOwner()->HasAuthority()) return;

	if (!HasOutputSpace())
	{
		UE_LOG(LogZfRefinery, Log, TEXT("ZfRefineryComponent [%s]: Output cheio — refino pausado."), *GetOwner()->GetName());
		return;
	}

	UZfRefineryRecipe* NextRecipe = FindNextRecipe();
	if (!NextRecipe)
	{
		UE_LOG(LogZfRefinery, Log, TEXT("ZfRefineryComponent [%s]: Nenhuma receita satisfeita — refino parado."), *GetOwner()->GetName());
		return;
	}

	StartCycle(NextRecipe);
}


// ============================================================
// StartCycle
// ============================================================

void UZfRefineryComponent::StartCycle(UZfRefineryRecipe* Recipe)
{
	if (!Recipe) return;

	RecipeElapsedTime = 0.0f;
	ActiveRecipe      = Recipe;
	bIsRefining       = true;

	if (!ActiveCatalystState.IsActive()) TryConsumeCatalyst();

	const UWorld* World   = GetWorld();
	LastSnapshotTime      = World ? World->GetTimeSeconds() : 0.0f;
	const float Mult      = ActiveCatalystState.IsActive() ? ActiveCatalystState.SpeedMultiplier : 1.0f;
	CurrentCycleTotalTime = Recipe->BaseCraftTime / Mult;

	if (World)
	{
		World->GetTimerManager().SetTimer(CycleTimerHandle, this, &UZfRefineryComponent::OnCycleCompleted, CurrentCycleTotalTime, false);
	}

	OnRefineryStateChanged.Broadcast();

	UE_LOG(LogZfRefinery, Log,
		TEXT("ZfRefineryComponent [%s]: Ciclo iniciado — '%s' | %.1fs | %.2fx"),
		*GetOwner()->GetName(), *Recipe->DisplayName.ToString(), CurrentCycleTotalTime, Mult);
}


// ============================================================
// StopRefining
// ============================================================

void UZfRefineryComponent::StopRefining(bool bSaveProgress)
{
	if (bSaveProgress && bIsRefining) TakeSnapshot();
	else RecipeElapsedTime = 0.0f;

	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(CycleTimerHandle);

	bIsRefining           = false;
	ActiveRecipe          = nullptr;
	CurrentCycleTotalTime = 0.0f;

	OnRefineryStateChanged.Broadcast();
}


// ============================================================
// OnCycleCompleted
// ============================================================

void UZfRefineryComponent::OnCycleCompleted()
{
	if (!GetOwner()->HasAuthority() || !ActiveRecipe) return;

	UZfRefineryRecipe* CompletedRecipe = ActiveRecipe;

	if (!DepositOutputs(CompletedRecipe))
	{
		UE_LOG(LogZfRefinery, Log, TEXT("ZfRefineryComponent [%s]: Output cheio ao completar — pausado."), *GetOwner()->GetName());
		StopRefining(false);
		return;
	}

	ConsumeIngredients(CompletedRecipe);
	OnRefineryCycleCompleted.Broadcast(CompletedRecipe);

	UE_LOG(LogZfRefinery, Log, TEXT("ZfRefineryComponent [%s]: Ciclo completo — '%s'"),
		*GetOwner()->GetName(), *CompletedRecipe->DisplayName.ToString());

	bIsRefining       = false;
	ActiveRecipe      = nullptr;
	RecipeElapsedTime = 0.0f;

	TryStartNextCycle();
}


// ============================================================
// FindNextRecipe
// ============================================================

UZfRefineryRecipe* UZfRefineryComponent::FindNextRecipe() const
{
	for (const TObjectPtr<UZfRefineryRecipe>& Recipe : ManualRecipeQueue)
	{
		if (Recipe && CanSatisfyRecipe(Recipe)) return Recipe;
	}

	for (UZfRefineryRecipe* Recipe : GetSortedRecipes())
	{
		if (CanSatisfyRecipe(Recipe)) return Recipe;
	}

	return nullptr;
}


// ============================================================
// CanSatisfyRecipe
// ============================================================

bool UZfRefineryComponent::CanSatisfyRecipe(const UZfRefineryRecipe* Recipe) const
{
	if (!Recipe || !Recipe->HasValidIngredients()) return false;

	for (const FZfRefineryIngredient& Ingredient : Recipe->Ingredients)
	{
		const UZfItemDefinition* ItemDef = Ingredient.ItemDefinition.Get();
		if (!ItemDef) return false;
		if (CountItemsInInput(ItemDef) < Ingredient.Quantity) return false;
	}

	return true;
}


// ============================================================
// GetSortedRecipes
// ============================================================

TArray<UZfRefineryRecipe*> UZfRefineryComponent::GetSortedRecipes() const
{
	TArray<UZfRefineryRecipe*> Result;
	if (!RefineryData) return Result;

	for (const TSoftObjectPtr<UZfRefineryRecipe>& SoftRecipe : RefineryData->AvailableRecipes)
	{
		if (UZfRefineryRecipe* Recipe = SoftRecipe.Get()) Result.Add(Recipe);
	}

	Result.Sort([](const UZfRefineryRecipe& A, const UZfRefineryRecipe& B)
	{
		if (A.Priority != B.Priority) return A.Priority > B.Priority;
		return A.GetIngredientCount() > B.GetIngredientCount();
	});

	return Result;
}


// ============================================================
// TryConsumeCatalyst
// ============================================================

bool UZfRefineryComponent::TryConsumeCatalyst()
{
	if (!RefineryData) return false;

	for (int32 i = 0; i < CatalystSlots.Capacity; ++i)
	{
		UZfItemInstance* CatalystItem = GetSlotItem(CatalystSlots, i);
		if (!CatalystItem) continue;

		const UZfFragment_Catalyst* Fragment = CatalystItem->GetFragment<UZfFragment_Catalyst>();
		if (!Fragment) continue;

		ActiveCatalystState.SpeedMultiplier        = Fragment->SpeedMultiplier;
		ActiveCatalystState.RemainingBoostDuration = Fragment->BoostDuration;

		InternalRemoveItemFromSlot(CatalystSlots, i);
		OnCatalystConsumed.Broadcast(ActiveCatalystState.SpeedMultiplier);

		UE_LOG(LogZfRefinery, Log,
			TEXT("ZfRefineryComponent [%s]: Catalisador consumido — Slot %d | %.2fx | %.1fs"),
			*GetOwner()->GetName(), i, ActiveCatalystState.SpeedMultiplier, ActiveCatalystState.RemainingBoostDuration);

		return true;
	}

	return false;
}


// ============================================================
// UpdateCatalystDuration
// ============================================================

void UZfRefineryComponent::UpdateCatalystDuration()
{
	if (!ActiveCatalystState.IsActive()) return;

	const UWorld* World = GetWorld();
	if (!World) return;

	ActiveCatalystState.RemainingBoostDuration -= (World->GetTimeSeconds() - LastSnapshotTime) * ActiveCatalystState.SpeedMultiplier;

	if (ActiveCatalystState.RemainingBoostDuration <= 0.0f)
	{
		ActiveCatalystState.Reset();
		if (!TryConsumeCatalyst())
		{
			UE_LOG(LogZfRefinery, Log,
				TEXT("ZfRefineryComponent [%s]: Nenhum catalisador disponível. Continuando sem boost."),
				*GetOwner()->GetName());
		}
	}
}


// ============================================================
// TakeSnapshot
// ============================================================

void UZfRefineryComponent::TakeSnapshot()
{
	const UWorld* World = GetWorld();
	if (!World) return;

	const float Now        = World->GetTimeSeconds();
	const float Multiplier = ActiveCatalystState.IsActive() ? ActiveCatalystState.SpeedMultiplier : 1.0f;

	RecipeElapsedTime += (Now - LastSnapshotTime) * Multiplier;
	LastSnapshotTime   = Now;
}


// ============================================================
// RescheduleCycleTimer
// ============================================================

void UZfRefineryComponent::RescheduleCycleTimer()
{
	if (!bIsRefining || !ActiveRecipe) return;

	UWorld* World = GetWorld();
	if (!World) return;

	World->GetTimerManager().ClearTimer(CycleTimerHandle);

	const float Mult      = ActiveCatalystState.IsActive() ? ActiveCatalystState.SpeedMultiplier : 1.0f;
	CurrentCycleTotalTime = ActiveRecipe->BaseCraftTime / Mult;
	const float Remaining = FMath::Max(CurrentCycleTotalTime - RecipeElapsedTime, 0.01f);

	World->GetTimerManager().SetTimer(CycleTimerHandle, this, &UZfRefineryComponent::OnCycleCompleted, Remaining, false);

	UE_LOG(LogZfRefinery, Log,
		TEXT("ZfRefineryComponent [%s]: Timer reagendado — %.2fs | %.2fx"),
		*GetOwner()->GetName(), Remaining, Mult);
}


// ============================================================
// ConsumeIngredients
// ============================================================

void UZfRefineryComponent::ConsumeIngredients(const UZfRefineryRecipe* Recipe)
{
	if (!Recipe) return;

	for (const FZfRefineryIngredient& Ingredient : Recipe->Ingredients)
	{
		const UZfItemDefinition* ItemDef = Ingredient.ItemDefinition.Get();
		if (!ItemDef) continue;

		int32 ToConsume = Ingredient.Quantity;

		// Itera uma cópia dos SlotIndexes para poder remover durante a iteração
		TArray<int32> SlotIndexes;
		for (const FZfRefinerySlot& Slot : InputSlots.Slots)
		{
			if (Slot.ItemInstance && Slot.ItemInstance->GetItemDefinition() == ItemDef)
				SlotIndexes.Add(Slot.SlotIndex);
		}

		for (const int32 Idx : SlotIndexes)
		{
			if (ToConsume <= 0) break;

			UZfItemInstance* Item = GetSlotItem(InputSlots, Idx);
			if (!Item) continue;

			const int32 StackCount = Item->GetCurrentStack();

			if (StackCount <= ToConsume)
			{
				ToConsume -= StackCount;
				InternalRemoveItemFromSlot(InputSlots, Idx);
			}
			else
			{
				Item->SetCurrentStack(StackCount - ToConsume);

				for (FZfRefinerySlot& Slot : InputSlots.Slots)
				{
					if (Slot.SlotIndex == Idx)
					{
						InputSlots.MarkItemDirty(Slot);
						break;
					}
				}

				ToConsume = 0;
			}
		}
	}
}


// ============================================================
// DepositOutputs
// ============================================================

bool UZfRefineryComponent::DepositOutputs(const UZfRefineryRecipe* Recipe)
{
	if (!Recipe || !HasOutputSpace()) return false;

	const bool bBonusRolled = Recipe->BonusOutputChance > 0.0f && FMath::FRand() < Recipe->BonusOutputChance;

	for (int32 i = 0; i < Recipe->Outputs.Num(); ++i)
	{
		const FZfRefineryOutput& Output = Recipe->Outputs[i];
		if (Output.ItemDefinition.IsNull()) continue;

		int32 Quantity = Output.Quantity;
		if (i == 0 && bBonusRolled) Quantity += Recipe->BonusOutputAmount;

		UZfItemInstance* NewItem = CreateItemInstance(Output.ItemDefinition, Quantity);
		if (!NewItem) continue;

		const int32 TargetSlot = GetFirstEmptySlot(OutputSlots);
		if (TargetSlot == INDEX_NONE) continue;

		InternalAddItemToSlot(OutputSlots, NewItem, TargetSlot);
	}

	if (bBonusRolled)
	{
		UE_LOG(LogZfRefinery, Log, TEXT("ZfRefineryComponent [%s]: Bônus de output rolado!"), *GetOwner()->GetName());
	}

	return true;
}


// ============================================================
// CreateItemInstance
// ============================================================

UZfItemInstance* UZfRefineryComponent::CreateItemInstance(TSoftObjectPtr<UZfItemDefinition> ItemDef, int32 Quantity) const
{
	UZfItemDefinition* Def = ItemDef.Get();
	if (!Def) return nullptr;

	UZfItemInstance* NewInstance = NewObject<UZfItemInstance>(GetOwner());
	if (!NewInstance) return nullptr;

	NewInstance->SetItemDefinition(Def);
	NewInstance->SetCurrentStack(Quantity);
	return NewInstance;
}


// ============================================================
// CountItemsInInput
// ============================================================

int32 UZfRefineryComponent::CountItemsInInput(const UZfItemDefinition* ItemDef) const
{
	int32 Count = 0;

	for (const FZfRefinerySlot& Slot : InputSlots.Slots)
	{
		if (!Slot.ItemInstance) continue;
		if (Slot.ItemInstance->GetItemDefinition() != ItemDef) continue;
		Count += Slot.ItemInstance->GetCurrentStack();
	}

	return Count;
}


// ============================================================
// HasOutputSpace
// ============================================================

bool UZfRefineryComponent::HasOutputSpace() const
{
	return OutputSlots.Slots.Num() < OutputSlots.Capacity;
}


// ============================================================
// CheckIsServer
// ============================================================

bool UZfRefineryComponent::CheckIsServer(const FString& FunctionName) const
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogZfRefinery, Warning,
			TEXT("ZfRefineryComponent::%s chamado no cliente — ignorado."), *FunctionName);
		return false;
	}
	return true;
}


// ============================================================
// Rep Notifies
// ============================================================

void UZfRefineryComponent::OnRep_RefineryState()
{
	OnRefineryStateChanged.Broadcast();
}

void UZfRefineryComponent::OnRep_CatalystState()
{
	OnCatalystConsumed.Broadcast(ActiveCatalystState.SpeedMultiplier);
}