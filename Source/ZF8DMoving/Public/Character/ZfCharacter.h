// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "ZfCharacter.generated.h"

class UZfResourceAttributeSet;

UCLASS()
class ZF8DMOVING_API AZfCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AZfCharacter();
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void InitAbilityActorInfo();
	
	/**
	* Abilities concedidas automaticamente ao personagem após InitAbilityActorInfo.
	*
	* Configurável por Blueprint em qualquer subclasse:
	*   AZfCharacter          → abilities comuns a todos os personagens
	*   AZfPlayerCharacter    → abilities exclusivas do jogador (ex: ReceiveXP, LevelUp)
	*   BP_ZfEnemyCharacter   → abilities do inimigo específico
	*
	* A concessão ocorre apenas no servidor e apenas se a ability
	* ainda não estiver registrada no ASC — seguro contra dupla concessão
	* em respawns e re-inicializações.
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Startup")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;
	
protected:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	virtual void Tick(float DeltaSeconds) override;
	
	/**
	* Concede as abilities da lista StartupAbilities ao ASC.
	* Deve ser chamado após InitAbilityActorInfo.
	*
	* Usa FindAbilitySpecFromClass como guard:
	*   - Ability já concedida → ignorada silenciosamente
	*   - Novo pawn (respawn) → ability não existe ainda → concedida corretamente
	*/
	void GrantStartupAbilities();
};
