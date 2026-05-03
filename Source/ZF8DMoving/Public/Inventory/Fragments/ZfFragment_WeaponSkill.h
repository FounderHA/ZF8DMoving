// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Fragments/ZfItemFragment.h"
#include "GameplayAbilitySpecHandle.h"
#include "ZfFragment_WeaponSkill.generated.h"

class UZfSkillTreeComponent;
class UAbilitySystemComponent;

// =============================================================================
// UZfFragment_WeaponSkill
// =============================================================================

/**
 * Fragment que define as abilities de uma arma.
 *
 * Responsabilidades:
 *   - Armazenar PrimaryAbilityClass e SecondaryAbilityClass da arma
 *   - Conceder as abilities ao ASC quando a arma é equipada
 *   - Revogar as abilities do ASC quando a arma é desequipada
 *   - Disparar RecalculateWeaponSlots no SkillTreeComponent após
 *     qualquer equip/unequip para redistribuir os WeaponSlots
 *
 * Redistribuição de WeaponSlots:
 *   Sempre que uma arma é equipada ou desequipada, o SkillTreeComponent
 *   recalcula quais abilities ocupam WeaponSlots[0] e WeaponSlots[1]
 *   com base nas armas atualmente equipadas no EquipmentComponent.
 *
 *   Regras:
 *     WeaponSlots[0] → Primary da arma no MainHand
 *                      Primary da arma no OffHand se MainHand não tiver
 *     WeaponSlots[1] → Secondary da arma no OffHand (prioridade)
 *                      Secondary da arma no MainHand se OffHand não tiver
 *     Conflito no slot 1 → OffHand vence, MainHand fica disponível no popup
 *
 * Popup de escolha na Skill Tree:
 *   Exibido apenas quando há conflito — dois candidatos para o mesmo slot.
 *   O jogador escolhe via UI qual ability quer no slot conflitante.
 *   O popup é responsabilidade da widget — o Fragment apenas fornece os dados.
 */
UCLASS(meta = (DisplayName = "Weapon Skill"))
class ZF8DMOVING_API UZfFragment_WeaponSkill : public UZfItemFragment
{
	GENERATED_BODY()

public:

	// ── Configuração ──────────────────────────────────────────────────────

	/**
	 * Ability ativada por IA_WeaponPrimary.
	 *
	 * Exemplos:
	 *   Espada      → GA_Sword_LightAttack
	 *   Arco        → GA_Bow_Shoot
	 *   Cajado      → GA_Staff_PhysicalAttack
	 *   Escudo      → nullptr (escudo não tem Primary)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponSkill")
	TSubclassOf<UGameplayAbility> PrimaryAbilityClass;

	/**
	 * Ability ativada por IA_WeaponSecondary.
	 *
	 * Exemplos:
	 *   Espada 2H   → GA_GreatSword_HeavyAttack
	 *   Escudo      → GA_Shield_Block
	 *   Espada 1H   → nullptr (secundário vai para o escudo se equipado)
	 *   Arco        → nullptr (ADS gerenciado internamente pela GA_Bow_Shoot)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponSkill")
	TSubclassOf<UGameplayAbility> SecondaryAbilityClass;

	// ── Acesso — consultado pelo SkillTreeComponent na redistribuição ─────

	/** Retorna true se esta arma tem ability primária configurada. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "WeaponSkill")
	bool HasPrimaryAbility() const { return PrimaryAbilityClass != nullptr; }

	/** Retorna true se esta arma tem ability secundária configurada. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "WeaponSkill")
	bool HasSecondaryAbility() const { return SecondaryAbilityClass != nullptr; }

	/**
	 * Retorna o handle da ability primária concedida ao ASC.
	 * Inválido se a arma não estiver equipada ou não tiver Primary.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "WeaponSkill")
	FGameplayAbilitySpecHandle GetPrimaryHandle() const { return PrimaryHandle; }

	/**
	 * Retorna o handle da ability secundária concedida ao ASC.
	 * Inválido se a arma não estiver equipada ou não tiver Secondary.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "WeaponSkill")
	FGameplayAbilitySpecHandle GetSecondaryHandle() const { return SecondaryHandle; }

	// ── Ciclo de vida ─────────────────────────────────────────────────────

	/**
	 * Chamado pelo EquipmentComponent ao equipar a arma.
	 * Concede PrimaryAbilityClass e SecondaryAbilityClass ao ASC.
	 * Dispara RecalculateWeaponSlots no SkillTreeComponent.
	 */
	virtual void OnItemEquipped(
		UZfItemInstance* OwningInstance,
		UZfEquipmentComponent* EquipmentComponent,
		AActor* EquippingActor) override;

	/**
	 * Chamado pelo EquipmentComponent ao desequipar a arma.
	 * Revoga as abilities do ASC.
	 * Dispara RecalculateWeaponSlots no SkillTreeComponent.
	 */
	virtual void OnItemUnequipped(
		UZfItemInstance* OwningInstance,
		UZfEquipmentComponent* EquipmentComponent,
		AActor* UnequippingActor) override;

private:

	/**
	 * Handle da ability primária concedida ao ASC.
	 * Armazenado para revogar corretamente ao desequipar.
	 * Existe apenas no servidor — não replicado.
	 */
	FGameplayAbilitySpecHandle PrimaryHandle;

	/**
	 * Handle da ability secundária concedida ao ASC.
	 * Armazenado para revogar corretamente ao desequipar.
	 * Existe apenas no servidor — não replicado.
	 */
	FGameplayAbilitySpecHandle SecondaryHandle;

	/** Busca o ASC a partir do Actor (Character → PlayerState). */
	UAbilitySystemComponent* GetASC(AActor* Actor) const;

	/** Busca o SkillTreeComponent a partir do Actor. */
	UZfSkillTreeComponent* GetSkillTreeComponent(AActor* Actor) const;
};