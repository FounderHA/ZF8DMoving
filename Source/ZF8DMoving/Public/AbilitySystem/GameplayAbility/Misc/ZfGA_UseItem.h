// Fill out your copyright notice in the Description page of Project Settings.
// ZfGA_UseItem.h
// GameplayAbility que processa o uso/consumo de um item.
//
// ATIVACAO:
// Ativada via GameplayEvent com tag Item.Event.Use.
// O payload EventData.OptionalObject deve conter o UZfItemInstance a ser usado.
// Quem dispara o evento:
//   - InventoryComponent::UseItemAtSlot()    → context action no menu do inventario
//   - EquipmentComponent::UseQuickSlot(N)    → tecla de atalho 1/2/3
//
// PIPELINE (servidor):
//   1. Extrai ItemInstance do EventData
//   2. Busca UZfFragment_Consumable — se nao tem, cancela
//   3. Checa cooldown global (tag Cooldown.Item.Global no ASC)
//   4. Checa cooldown por item (tag Cooldown.Item.PerItem no ASC)
//   5. Checa single use (PlayerState::UsedUniqueItemTags)
//   6. Carrega e aplica ConsumptionGameplayEffect no ASC do player
//   7. Verifica UZfFragment_RecipeScroll → aprende receita se tiver
//   8. Consome o item (bConsumeOnUse)
//   9. Aplica cooldown global (GE_Cooldown_ItemGlobal)
//  10. Aplica cooldown por item se ItemCooldownSeconds > 0
//  11. Registra single use em PlayerState se bIsSingleUsePerGame
//
// COOLDOWN GLOBAL:
// Configurado via GE_Cooldown_ItemGlobal — um Blueprint filho de
// UGameplayEffect com duracao fixa e GrantedTags: Cooldown.Item.Global.
// Referenciado em GlobalCooldownEffect (UPROPERTY abaixo).
//
// COOLDOWN POR ITEM:
// Criado dinamicamente em runtime com duracao = Fragment.ItemCooldownSeconds.
// Aplica tag Cooldown.Item.PerItem.<ItemDefinitionName> no ASC.
// Nao requer asset Blueprint — gerado via FGameplayEffectSpec.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
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

	// ----------------------------------------------------------
	// CONFIGURACAO (editor)
	// ----------------------------------------------------------

	// GameplayEffect de cooldown global entre consumiveis.
	// Crie um Blueprint filho de UGameplayEffect com:
	//   - DurationPolicy: HasDuration
	//   - Duration: tempo desejado (ex: 1.0s)
	//   - GrantedTags: Cooldown.Item.Global
	// Referencie o asset aqui.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UseItem|Cooldown")
	TSoftClassPtr<UGameplayEffect> GlobalCooldownEffect;

	// ----------------------------------------------------------
	// ATIVACAO
	// ----------------------------------------------------------

	// Ativada via GameplayEvent. Extrai o ItemInstance do payload.
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:

	// ----------------------------------------------------------
	// PIPELINE INTERNO
	// ----------------------------------------------------------

	// Extrai e valida o ItemInstance do EventData.
	UZfItemInstance* ExtractItemInstance(const FGameplayEventData* EventData) const;

	// Verifica se todos os pre-requisitos estao satisfeitos.
	// Retorna false e chama EndAbility se algum falhar.
	bool CheckPrerequisites(
		UZfItemInstance* Item,
		const UZfFragment_Consumable* Fragment,
		AZfPlayerState* ZfPS,
		const FGameplayAbilityActorInfo* ActorInfo);

	// Carrega o ConsumptionGameplayEffect de forma sincrona e aplica.
	void ApplyConsumptionEffect(
		const UZfFragment_Consumable* Fragment,
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo);

	// Verifica e processa o RecipeScroll se o item tiver o fragment.
	void TryLearnRecipe(UZfItemInstance* Item, AZfPlayerState* ZfPS) const;

	// Consome o item do inventario ou do slot de equipamento.
	// bFromEquipmentSlot = true quando veio do UseQuickSlot.
	void ConsumeItem(
		UZfItemInstance* Item,
		UZfInventoryComponent* Inventory,
		bool bFromEquipmentSlot,
		const FGameplayAbilityActorInfo* ActorInfo) const;

	// Aplica cooldown global via GlobalCooldownEffect.
	void ApplyGlobalCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo);

	// Cria e aplica cooldown dinamico por item.
	void ApplyItemCooldown(
		float CooldownSeconds,
		UZfItemInstance* Item,
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo);

	// Registra o item como usado (single use).
	void RegisterSingleUse(const UZfFragment_Consumable* Fragment, AZfPlayerState* ZfPS) const;

	// Helper: retorna o InventoryComponent do actor info.
	UZfInventoryComponent* GetInventoryComponent(const FGameplayAbilityActorInfo* ActorInfo) const;
};