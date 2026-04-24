// Fill out your copyright notice in the Description page of Project Settings.
// ZfGA_UseItem.h
// GameplayAbility que processa o uso de um item consumivel.
//
// RESPONSABILIDADE:
// Esta GA NAO consome o item — apenas processa os efeitos do uso:
//   1. Valida pre-requisitos (single use)
//   2. Aplica ConsumptionGameplayEffect no ASC do player
//   3. Aprende receita se for um RecipeScroll
//   4. Registra single use
//
// CONSUMO DO ITEM:
// Quem dispara o evento ja consumiu o item antes:
//   - UZfInventoryComponent::TryUseItemAtSlot()  → consome do inventario
//   - UZfEquipmentComponent::UseQuickSlot()       → consome do equipment
//
// COOLDOWN:
// Sem cooldown de consumiveis. Se quiser cooldown em algum item especifico,
// configure CooldownGameplayEffect no CDO do Blueprint filho desta GA.
//
// ATIVACAO:
// Ativada via GameplayEvent com tag Item.Event.Use.
// O payload EventData.OptionalObject deve conter o UZfItemInstance usado.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ZfGA_UseItem.generated.h"

class UZfItemInstance;
class UZfFragment_Consumable;
class UZfInventoryComponent;
class AZfPlayerState;


UCLASS()
class ZF8DMOVING_API UZfGA_UseItem : public UGameplayAbility
{
	GENERATED_BODY()

public:

	UZfGA_UseItem();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:

	UZfItemInstance* ExtractItemInstance(const FGameplayEventData* EventData) const;

	bool CheckPrerequisites(
		UZfItemInstance* Item,
		const UZfFragment_Consumable* Fragment,
		AZfPlayerState* ZfPS,
		const FGameplayAbilityActorInfo* ActorInfo);

	void ApplyConsumptionEffect(
		const UZfFragment_Consumable* Fragment,
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo);

	void TryLearnRecipe(UZfItemInstance* Item, AZfPlayerState* ZfPS) const;

	void RegisterSingleUse(const UZfFragment_Consumable* Fragment, AZfPlayerState* ZfPS) const;

	UZfInventoryComponent* GetInventoryComponent(const FGameplayAbilityActorInfo* ActorInfo) const;
};