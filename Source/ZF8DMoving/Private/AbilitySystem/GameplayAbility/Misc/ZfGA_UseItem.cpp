// Fill out your copyright notice in the Description page of Project Settings.
// ZfGA_UseItem.cpp

#include "AbilitySystem/GameplayAbility/Misc/ZfGA_UseItem.h"
#include "Inventory/ZfItemInstance.h"
#include "Inventory/ZfItemDefinition.h"
#include "Inventory/ZfInventoryComponent.h"
#include "Inventory/Fragments/ZfFragment_Consumable.h"
#include "Inventory/Fragments/ZfFragment_RecipeScroll.h"
#include "Player/ZfPlayerState.h"
#include "Tags/ZfUniqueItemTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffect.h"
#include "GameFramework/PlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogZfUseItem, Log, All);


// ============================================================
// Constructor
// ============================================================

UZfGA_UseItem::UZfGA_UseItem()
{
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag    = ZfUniqueItemTags::ItemEvents::Item_Event_Use;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}


// ============================================================
// ActivateAbility
// ============================================================

void UZfGA_UseItem::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// ─── Extrai ItemInstance ──────────────────────────────────────────
	UZfItemInstance* Item = ExtractItemInstance(TriggerEventData);
	if (!Item)
	{
		UE_LOG(LogZfUseItem, Warning,
			TEXT("ZfGA_UseItem: ItemInstance invalido no EventData."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ─── Fragment_Consumable ──────────────────────────────────────────
	const UZfFragment_Consumable* Fragment = Item->GetFragment<UZfFragment_Consumable>();
	if (!Fragment)
	{
		UE_LOG(LogZfUseItem, Warning,
			TEXT("ZfGA_UseItem: Item '%s' nao tem Fragment_Consumable."),
			*Item->GetItemDefinition()->ItemName.ToString());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ─── PlayerState ─────────────────────────────────────────────────
	AZfPlayerState* ZfPS = nullptr;
	if (ActorInfo && ActorInfo->PlayerController.IsValid())
	{
		ZfPS = ActorInfo->PlayerController->GetPlayerState<AZfPlayerState>();
	}

	// ─── Pre-requisitos ───────────────────────────────────────────────
	if (!CheckPrerequisites(Item, Fragment, ZfPS, ActorInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ─── Aplica GE de consumo ─────────────────────────────────────────
	ApplyConsumptionEffect(Fragment, Handle, ActorInfo, ActivationInfo);

	// ─── Aprende receita (se for scroll) ─────────────────────────────
	TryLearnRecipe(Item, ZfPS);

	// ─── Single Use ───────────────────────────────────────────────────
	if (Fragment->bIsSingleUsePerGame && ZfPS)
	{
		RegisterSingleUse(Fragment, ZfPS);
	}

	UE_LOG(LogZfUseItem, Log,
		TEXT("ZfGA_UseItem: Item '%s' processado com sucesso."),
		*Item->GetItemDefinition()->ItemName.ToString());

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}


// ============================================================
// ExtractItemInstance
// ============================================================

UZfItemInstance* UZfGA_UseItem::ExtractItemInstance(
	const FGameplayEventData* EventData) const
{
	if (!EventData) return nullptr;
	return Cast<UZfItemInstance>(
		const_cast<UObject*>(EventData->OptionalObject.Get()));
}


// ============================================================
// CheckPrerequisites
// ============================================================

bool UZfGA_UseItem::CheckPrerequisites(
	UZfItemInstance* Item,
	const UZfFragment_Consumable* Fragment,
	AZfPlayerState* ZfPS,
	const FGameplayAbilityActorInfo* ActorInfo)
{
	// ── Single Use ────────────────────────────────────────────────────
	if (Fragment->bIsSingleUsePerGame && ZfPS)
	{
		if (!Fragment->UniqueItemTag.IsValid())
		{
			UE_LOG(LogZfUseItem, Warning,
				TEXT("ZfGA_UseItem: bIsSingleUsePerGame=true mas UniqueItemTag invalida."));
			return false;
		}

		// Descomente quando UsedUniqueItemTags for adicionado ao ZfPlayerState:
		// if (ZfPS->UsedUniqueItemTags.HasTag(Fragment->UniqueItemTag))
		// {
		//     UE_LOG(LogZfUseItem, Log, TEXT("ZfGA_UseItem: Item unico ja usado."));
		//     return false;
		// }
	}

	return true;
}


// ============================================================
// ApplyConsumptionEffect
// ============================================================

void UZfGA_UseItem::ApplyConsumptionEffect(
	const UZfFragment_Consumable* Fragment,
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (Fragment->ConsumptionGameplayEffect.IsNull()) return;

	TSubclassOf<UGameplayEffect> GEClass =
		Fragment->ConsumptionGameplayEffect.LoadSynchronous();
	if (!GEClass) return;

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
		Handle, ActorInfo, ActivationInfo, GEClass, Fragment->EffectLevel);

	if (SpecHandle.IsValid())
	{
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);

		UE_LOG(LogZfUseItem, Log,
			TEXT("ZfGA_UseItem: GE '%s' aplicado (Level %d)."),
			*GEClass->GetName(), Fragment->EffectLevel);
	}
}


// ============================================================
// TryLearnRecipe
// ============================================================

void UZfGA_UseItem::TryLearnRecipe(UZfItemInstance* Item, AZfPlayerState* ZfPS) const
{
	if (!ZfPS || !Item) return;
	if (!Item->HasFragment<UZfFragment_RecipeScroll>()) return;

	const bool bLearned = UZfFragment_RecipeScroll::TryLearnRecipeFromItem(Item, ZfPS);
	if (bLearned)
	{
		UE_LOG(LogZfUseItem, Log,
			TEXT("ZfGA_UseItem: Receita aprendida via scroll '%s'."),
			*Item->GetItemDefinition()->ItemName.ToString());
	}
}


// ============================================================
// RegisterSingleUse
// ============================================================

void UZfGA_UseItem::RegisterSingleUse(
	const UZfFragment_Consumable* Fragment, AZfPlayerState* ZfPS) const
{
	if (!Fragment || !ZfPS || !Fragment->UniqueItemTag.IsValid()) return;

	// Descomente quando UsedUniqueItemTags for adicionado ao ZfPlayerState:
	//
	// UWorld* World = ZfPS->GetWorld();
	// if (!World) return;
	// AGameStateBase* GS = World->GetGameState();
	// if (!GS) return;
	// for (APlayerState* PS : GS->PlayerArray)
	// {
	//     AZfPlayerState* OtherPS = Cast<AZfPlayerState>(PS);
	//     if (!OtherPS || OtherPS->UsedUniqueItemTags.HasTag(Fragment->UniqueItemTag)) continue;
	//     OtherPS->UsedUniqueItemTags.AddTag(Fragment->UniqueItemTag);
	// }
}


// ============================================================
// GetInventoryComponent
// ============================================================

UZfInventoryComponent* UZfGA_UseItem::GetInventoryComponent(
	const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!ActorInfo) return nullptr;

	if (ActorInfo->PlayerController.IsValid())
	{
		if (AZfPlayerState* ZfPS =
			ActorInfo->PlayerController->GetPlayerState<AZfPlayerState>())
		{
			return ZfPS->GetInventoryComponent();
		}
	}
	return nullptr;
}