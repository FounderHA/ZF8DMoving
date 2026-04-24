// Fill out your copyright notice in the Description page of Project Settings.
// ZfGA_UseItem.cpp

#include "AbilitySystem/GameplayAbility/Misc/ZfGA_UseItem.h"
#include "Inventory/ZfItemInstance.h"
#include "Inventory/ZfItemDefinition.h"
#include "Inventory/ZfInventoryComponent.h"
#include "Inventory/ZfEquipmentComponent.h"
#include "Inventory/Fragments/ZfFragment_Consumable.h"
#include "Inventory/Fragments/ZfFragment_RecipeScroll.h"
#include "Inventory/Fragments/ZfFragment_Stackable.h"
#include "Player/ZfPlayerState.h"
#include "FunctionLibrary/ZfHelperLibrary.h"
#include "Tags/ZfCooldownTags.h"
#include "Tags/ZfUniqueItemTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameFramework/PlayerState.h"
#include "Engine/Engine.h"


DEFINE_LOG_CATEGORY_STATIC(LogZfUseItem, Log, All);


// ============================================================
// Constructor
// ============================================================

UZfGA_UseItem::UZfGA_UseItem()
{
	// Ativada por evento — nao por input direto.
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag    = ZfUniqueItemTags::ItemEvents::Item_Event_Use;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	// Instanciada por execucao — nao persiste entre usos.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

	// Nao replica — executa no servidor, resultados chegam via replicacao do inventario/ASC.
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
		UE_LOG(LogZfUseItem, Warning, TEXT("ZfGA_UseItem: ItemInstance invalido no EventData."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ─── Busca o Fragment_Consumable ──────────────────────────────────
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

	// ─── Inventario ───────────────────────────────────────────────────
	UZfInventoryComponent* Inventory = GetInventoryComponent(ActorInfo);
	if (!Inventory)
	{
		UE_LOG(LogZfUseItem, Warning, TEXT("ZfGA_UseItem: InventoryComponent nao encontrado."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ─── Pre-requisitos ───────────────────────────────────────────────
	if (!CheckPrerequisites(Item, Fragment, ZfPS, ActorInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ─── Aplica efeito de consumo ─────────────────────────────────────
	ApplyConsumptionEffect(Fragment, Handle, ActorInfo, ActivationInfo);

	// ─── Verifica RecipeScroll ────────────────────────────────────────
	TryLearnRecipe(Item, ZfPS);

	// ─── Consome o item ───────────────────────────────────────────────
	// EventMagnitude 1.0 = veio do slot de equipamento, 0.0 = veio do inventario.
	const bool bFromEquipmentSlot = TriggerEventData &&
		FMath::IsNearlyEqual(TriggerEventData->EventMagnitude, 1.f);

	if (Fragment->bConsumeOnUse)
	{
		ConsumeItem(Item, Inventory, bFromEquipmentSlot, ActorInfo);
	}

	// ─── Cooldowns ────────────────────────────────────────────────────
	ApplyGlobalCooldown(Handle, ActorInfo, ActivationInfo);

	if (Fragment->ItemCooldownSeconds > 0.f)
	{
		ApplyItemCooldown(Fragment->ItemCooldownSeconds, Item, Handle, ActorInfo, ActivationInfo);
	}

	// ─── Single Use ───────────────────────────────────────────────────
	if (Fragment->bIsSingleUsePerGame && ZfPS)
	{
		RegisterSingleUse(Fragment, ZfPS);
	}

	UE_LOG(LogZfUseItem, Log,
		TEXT("ZfGA_UseItem: Item '%s' usado com sucesso."),
		*Item->GetItemDefinition()->ItemName.ToString());

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}


// ============================================================
// ExtractItemInstance
// ============================================================

UZfItemInstance* UZfGA_UseItem::ExtractItemInstance(const FGameplayEventData* EventData) const
{
	if (!EventData) return nullptr;
	// OptionalObject e const — cast seguro porque nos apenas lemos o objeto
	// e o passamos para funcoes que esperam nao-const (ex: ConsumeItem).
	return Cast<UZfItemInstance>(const_cast<UObject*>(EventData->OptionalObject.Get()));
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
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC) return false;

	// ── Cooldown global ───────────────────────────────────────────────
	const FGameplayTag GlobalCDTag = ZfCooldownTags::Cooldown_Item_Global;
	if (ASC->HasMatchingGameplayTag(GlobalCDTag))
	{
		UE_LOG(LogZfUseItem, Log,
			TEXT("ZfGA_UseItem: Bloqueado pelo cooldown global."));
		return false;
	}

	// ── Cooldown por item ─────────────────────────────────────────────
	if (Fragment->ItemCooldownSeconds > 0.f && Item->GetItemDefinition())
	{
		// Tag de cooldown por item: Cooldown.Item.PerItem.<ItemDefinitionName>
		const FString ItemName    = Item->GetItemDefinition()->GetName();
		const FString CDTagString = FString::Printf(TEXT("Cooldown.Item.PerItem.%s"), *ItemName);
		const FGameplayTag ItemCDTag = FGameplayTag::RequestGameplayTag(
			FName(*CDTagString), /*ErrorIfNotFound=*/false);

		if (ItemCDTag.IsValid() && ASC->HasMatchingGameplayTag(ItemCDTag))
		{
			UE_LOG(LogZfUseItem, Log,
				TEXT("ZfGA_UseItem: Bloqueado pelo cooldown do item '%s'."), *ItemName);
			return false;
		}
	}

	// ── Single Use ────────────────────────────────────────────────────
	if (Fragment->bIsSingleUsePerGame && ZfPS)
	{
		if (!Fragment->UniqueItemTag.IsValid())
		{
			UE_LOG(LogZfUseItem, Warning,
				TEXT("ZfGA_UseItem: Item com bIsSingleUsePerGame=true mas UniqueItemTag invalida. Bloqueando."));
			return false;
		}

		// PENDENTE: adicionar UsedUniqueItemTags ao ZfPlayerState.h para ativar esta checagem.
		// Quando adicionado, descomente o bloco abaixo:
		//
		// if (ZfPS->UsedUniqueItemTags.HasTag(Fragment->UniqueItemTag))
		// {
		//     UE_LOG(LogZfUseItem, Log,
		//         TEXT("ZfGA_UseItem: Item unico '%s' ja foi usado."),
		//         *Fragment->UniqueItemTag.ToString());
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

	// Carrega o GE de forma sincrona (item acabou de ser usado — latencia aceitavel).
	TSubclassOf<UGameplayEffect> GEClass = Fragment->ConsumptionGameplayEffect.LoadSynchronous();
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

	// So tenta aprender se o item realmente tem o fragment de scroll.
	// Evita warning desnecessario em itens consumiveis normais.
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
// ConsumeItem
// ============================================================
// Nao precisa saber de onde o item veio.
// Procura no inventario — se nao achar, procura no equipment.
// Remove direto, sem intermediarios.
// ============================================================

void UZfGA_UseItem::ConsumeItem(
	UZfItemInstance* Item,
	UZfInventoryComponent* Inventory,
	bool /*bFromEquipmentSlot — ignorado, mantido por compatibilidade*/,
	const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!Item) return;

	UE_LOG(LogZfUseItem, Warning,
		TEXT("ZfGA_UseItem::ConsumeItem — iniciando para item '%s'"),
		*Item->GetItemDefinition()->ItemName.ToString());

	// ─── Tenta remover do inventario ─────────────────────────────────
	if (Inventory)
	{
		const int32 SlotIndex = Inventory->GetSlotIndexOfItem(Item);
		UE_LOG(LogZfUseItem, Warning,
			TEXT("ZfGA_UseItem::ConsumeItem — SlotIndex no inventario: %d"), SlotIndex);

		if (SlotIndex != INDEX_NONE)
		{
			const bool bStackable = Item->HasFragment<UZfFragment_Stackable>();
			if (bStackable)
				Inventory->ServerTryRemoveAmountFromStack(Item, 1);
			else
				Inventory->ServerTryRemoveItemFromInventory(SlotIndex);

			UE_LOG(LogZfUseItem, Log,
				TEXT("ZfGA_UseItem: '%s' consumido do inventario."),
				*Item->GetItemDefinition()->ItemName.ToString());
			return;
		}
	}
	else
	{
		UE_LOG(LogZfUseItem, Warning,
			TEXT("ZfGA_UseItem::ConsumeItem — Inventory e null!"));
	}

	// ─── Nao estava no inventario — tenta no EquipmentComponent ──────
	// EquipmentComponent vive no PlayerState — passa ele pra FindEquipmentComponent.
	AZfPlayerState* ZfPS = nullptr;
	if (ActorInfo && ActorInfo->PlayerController.IsValid())
	{
		ZfPS = ActorInfo->PlayerController->GetPlayerState<AZfPlayerState>();
	}

	if (!ZfPS) return;

	UZfEquipmentComponent* EquipComp =
		UZfHelperLibrary::FindEquipmentComponent(ZfPS);

	UE_LOG(LogZfUseItem, Warning,
		TEXT("ZfGA_UseItem::ConsumeItem — EquipComp: %s"),
		EquipComp ? TEXT("ENCONTRADO") : TEXT("NULL"));

	if (!EquipComp) return;

	// Acha o slot do item no equipment.
	FGameplayTag SlotTag;
	int32 SlotPos = 0;
	bool bFound = false;

	UE_LOG(LogZfUseItem, Warning,
		TEXT("ZfGA_UseItem::ConsumeItem — Iterando %d slots do equipment"),
		EquipComp->GetAllEquipmentSlots().Num());

	for (const FZfEquipmentSlotEntry& Entry : EquipComp->GetAllEquipmentSlots())
	{
		UE_LOG(LogZfUseItem, Warning,
			TEXT("  Slot [%s pos %d] -> Item: %s"),
			*Entry.SlotTag.ToString(),
			Entry.SlotPosition,
			Entry.ItemInstance ? *Entry.ItemInstance->GetItemDefinition()->ItemName.ToString() : TEXT("VAZIO"));

		if (Entry.ItemInstance == Item)
		{
			SlotTag = Entry.SlotTag;
			SlotPos = Entry.SlotPosition;
			bFound  = true;
			break;
		}
	}

	if (!bFound)
	{
		UE_LOG(LogZfUseItem, Warning,
			TEXT("ZfGA_UseItem: '%s' nao encontrado nem no inventario nem no equipment."),
			*Item->GetItemDefinition()->ItemName.ToString());
		return;
	}

	EquipComp->ServerTryRemoveItemFromEquipmentSlot(SlotTag, SlotPos);

	UE_LOG(LogZfUseItem, Log,
		TEXT("ZfGA_UseItem: '%s' consumido do equipment [%s pos %d]."),
		*Item->GetItemDefinition()->ItemName.ToString(),
		*SlotTag.ToString(), SlotPos);
}


// ============================================================
// ApplyGlobalCooldown
// ============================================================

void UZfGA_UseItem::ApplyGlobalCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (GlobalCooldownEffect.IsNull()) return;

	TSubclassOf<UGameplayEffect> GEClass = GlobalCooldownEffect.LoadSynchronous();
	if (!GEClass) return;

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
		Handle, ActorInfo, ActivationInfo, GEClass, 1);

	if (SpecHandle.IsValid())
	{
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
}


// ============================================================
// ApplyItemCooldown
// ============================================================

void UZfGA_UseItem::ApplyItemCooldown(
	float CooldownSeconds,
	UZfItemInstance* Item,
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC || !Item->GetItemDefinition()) return;

	// Monta a tag de cooldown por item dinamicamente.
	const FString ItemName    = Item->GetItemDefinition()->GetName();
	const FString CDTagString = FString::Printf(TEXT("Cooldown.Item.PerItem.%s"), *ItemName);

	// Registra a tag no sistema se ainda nao existir.
	// RequestGameplayTag com bErrorIfNotFound=false cria a tag em runtime.
	const FGameplayTag ItemCDTag = FGameplayTag::RequestGameplayTag(
		FName(*CDTagString), /*bErrorIfNotFound=*/false);

	if (!ItemCDTag.IsValid())
	{
		UE_LOG(LogZfUseItem, Warning,
			TEXT("ZfGA_UseItem: Nao foi possivel criar tag de cooldown para '%s'. "
				 "Declare 'Cooldown.Item.PerItem.%s' no ZfCooldownTags.h."),
			*ItemName, *ItemName);
		return;
	}

	// Cria um GE dinamico com duracao = CooldownSeconds e aplica a tag.
	// API UE 5.x: usa UTargetTagsGameplayEffectComponent em vez do
	// deprecated InheritableOwnedTagsContainer.
	UGameplayEffect* CooldownGE = NewObject<UGameplayEffect>(
		GetTransientPackage(), FName(TEXT("ItemCooldown_Dynamic")));

	CooldownGE->DurationPolicy    = EGameplayEffectDurationType::HasDuration;
	CooldownGE->DurationMagnitude = FScalableFloat(CooldownSeconds);

	// Adiciona a tag de cooldown via componente (API nao-deprecated).
	UTargetTagsGameplayEffectComponent& TagsComp =
		CooldownGE->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();

	FInheritedTagContainer InheritedTags;
	InheritedTags.AddTag(ItemCDTag);
	TagsComp.SetAndApplyTargetTagChanges(InheritedTags);

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
		Handle, ActorInfo, ActivationInfo,
		CooldownGE->GetClass(), 1);

	if (SpecHandle.IsValid())
	{
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);

		UE_LOG(LogZfUseItem, Log,
			TEXT("ZfGA_UseItem: Cooldown de %.1fs aplicado para item '%s'."),
			CooldownSeconds, *ItemName);
	}
}


// ============================================================
// RegisterSingleUse
// ============================================================

void UZfGA_UseItem::RegisterSingleUse(const UZfFragment_Consumable* Fragment, AZfPlayerState* ZfPS) const
{
	if (!Fragment || !ZfPS || !Fragment->UniqueItemTag.IsValid()) return;

	// PENDENTE: adicionar UsedUniqueItemTags ao ZfPlayerState.h para ativar este bloco.
	// Quando o campo existir no PlayerState, substitua este comentario por:
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

	UE_LOG(LogZfUseItem, Log,
		TEXT("ZfGA_UseItem: RegisterSingleUse pendente — adicione UsedUniqueItemTags ao ZfPlayerState.h."));
}


// ============================================================
// GetInventoryComponent
// ============================================================

UZfInventoryComponent* UZfGA_UseItem::GetInventoryComponent(
	const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!ActorInfo) return nullptr;

	// Inventario vive no PlayerState no projeto.
	if (ActorInfo->PlayerController.IsValid())
	{
		if (AZfPlayerState* ZfPS = ActorInfo->PlayerController->GetPlayerState<AZfPlayerState>())
		{
			return ZfPS->GetInventoryComponent();
		}
	}
	return nullptr;
}