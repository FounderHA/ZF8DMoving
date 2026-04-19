// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "ZfCharacter.generated.h"

class UZfResourceAttributeSet;

/** Define a dependência entre um atributo fonte e um GE que deve
 * ser reaplicado quando esse atributo muda.
 * Ex: Constitution muda → GE_CalculateMaxHealth é reaplicado. */
USTRUCT(BlueprintType)
struct FZfAttributeRefreshDependency
{
	GENERATED_BODY()

	/** Atributo que quando muda dispara o recálculo. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dependency")
	FGameplayAttribute SourceAttribute;

	/** GE Instant a ser reaplicado quando SourceAttribute mudar. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dependency")
	TSubclassOf<UGameplayEffect> DependentEffect;
};

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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Initialization")
	TArray<TSubclassOf<UGameplayAbility>> InitializationGameplayAbilities;
	
	// -----------------------------------------------------------------------
	// Atributos — Inicialização
	// -----------------------------------------------------------------------
	
	/** 
	* GEs Instant aplicados APENAS na criação do personagem novo.
	* Nunca aplicados no load — o save já restaura esses valores.
	* Use para definir estado inicial: Health = MaxHealth, Mana = MaxMana, etc.
	* Configure no Blueprint filho desta classe.
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Initialization")
	TArray<TSubclassOf<UGameplayEffect>> AttributeDefaultEffects;
	
	/** 
	* GEs Infinite com MMC aplicados na posse do pawn.
	* Um GE por atributo (Strength, Dexterity, MaxHealth, etc.).
	* Configure no Blueprint filho desta classe.
	* Ordem não importa — cada GE é independente. 
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Initialization")
	TArray<TSubclassOf<UGameplayEffect>> InitializationAttributeEffects;

	// -----------------------------------------------------------------------
	// Atributos — Atualização de Dependencias de Attributo
	// -----------------------------------------------------------------------
	
	/** Define quais GEs devem ser reaplicados quando um atributo fonte muda.
	* Ex: Constitution → GE_CalculateMaxHealth
	*     Intelligence → GE_CalculateMaxMana
	* Configure no Blueprint filho desta classe. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Initialization")
	TArray<FZfAttributeRefreshDependency> AttributeRefreshDependencies;
	
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
	
	/** 
	* Aplica todos os GEs de AttributeDefaultEffects no ASC.
	* Chamado no PossessedBy após InitAbilityActorInfo.
	* Seguro contra lista vazia e entradas nulas.
	* Chamado apenas na criação do personagem, personagem existente ignora
	*/
	void InitializeDefaultsAttributes();
	
	/** 
	* Aplica todos os GEs de AttributeInitializationEffects no ASC.
	* Chamado no PossessedBy após InitAbilityActorInfo.
	* Seguro contra lista vazia e entradas nulas.  
	*/
	void InitializeAttributes();
	
	/**
	* Registra um delegate por dependência configurada em AttributeRefreshDependencies.
	* Cada delegate observa o SourceAttribute e reaplica o DependentEffect ao mudar.
	* Chamado no PossessedBy após InitializeAttributes().
	*/
	void RegisterAttributeRefreshDelegates();
	
	/** 
	* Registra delegates que observam MaxHealth, MaxMana e MaxStamina.
	* Quando qualquer um muda, sincroniza o Current correspondente
	* proporcionalmente ao valor anterior.
	* Chamado no PossessedBy após RegisterAttributeRefreshDelegates(). 
	*/
	void RegisterResourceSyncDelegates();
	
	/**
	* Aplica uma vez todos os DependentEffects configurados em
	* AttributeRefreshDependencies. Chamado após RegisterAttributeRefreshDelegates()
	* para garantir que atributos dependentes sejam calculados na inicialização.
	*/
	void InitializeDependentAttributes();
	
	/**
	 * Registra o delegate que observa MoveSpeed e aplica o valor ao CMC.
	 *
	 * Intencionalmente SEM guard de HasAuthority — deve rodar tanto no
	 * servidor quanto no owning client:
	 *   Servidor      → aplica quando o GE de inicialização seta MoveSpeed
	 *   Owning Client → aplica quando o atributo chega replicado (predição local)
	 *
	 * Inclui safety net: lê o valor atual e aplica imediatamente caso o
	 * atributo já tenha chegado antes do bind (race condition de replicação).
	 *
	 * Simulated proxies recebem MaxWalkSpeed via replicação nativa do CMC —
	 * nenhum código adicional necessário para eles.
	 */
	void RegisterMovementSyncDelegate();

	/**
	 * Callback do delegate de MoveSpeed.
	 * Chamado sempre que MoveSpeed muda — tanto no servidor quanto no owning client.
	 */
	void OnMoveSpeedChanged(const FOnAttributeChangeData& Data);

	/**
	* Reaplica um GE Instant para forçar o MMC a recalcular
	* com os valores atualizados das fontes.
	* Chamado pelos delegates registrados em RegisterAttributeRefreshDelegates().
	*/
	void RefreshEffect(TSubclassOf<UGameplayEffect> EffectClass);

};
