// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ZfCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ZfResourceAttributeSet.h"

// Sets default values
AZfCharacter::AZfCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

UAbilitySystemComponent* AZfCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AZfCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	InitAbilityActorInfo();
}


void AZfCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


void AZfCharacter::InitAbilityActorInfo()
{
	// Caminho NPC/AI: ASC vive no próprio Character.
	// Não chamado por AZfPlayerCharacter — ele tem seu próprio caminho
	// via PossessedBy/OnRep_PlayerState apontando para o ASC do PlayerState.
	if (!IsValid(AbilitySystemComponent) || !HasAuthority())
	{
		return;
	}
 
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
 
	// Concede as startup abilities após o ActorInfo estar configurado.
	GrantStartupAbilities();
	InitializeAttributes();
	RegisterAttributeRefreshDelegates();
	RegisterResourceSyncDelegates();
	InitializeDependentAttributes();
}

void AZfCharacter::GrantStartupAbilities()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !HasAuthority()) return;
 
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : InitializationGameplayAbilities)
	{
		if (!AbilityClass)
		{
			continue;
		}
 
		// Guard contra dupla concessão: se a ability já existe no ASC, pula.
		// Cobre respawns (novo pawn, ASC no PlayerState persiste) e
		// re-chamadas acidentais de InitAbilityActorInfo.
		if (ASC->FindAbilitySpecFromClass(AbilityClass))
		{
			continue;
		}
 
		ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
	}
}

void AZfCharacter::InitializeDefaultsAttributes()
{
	if (!HasAuthority()) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	if (AttributeDefaultEffects.IsEmpty())
	{
		return;
	}

	for (const TSubclassOf<UGameplayEffect>& EffectClass : AttributeDefaultEffects)
	{
		if (!EffectClass)
		{
			continue;
		}

		FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
		ContextHandle.AddSourceObject(this);

		const FGameplayEffectSpecHandle SpecHandle =
			ASC->MakeOutgoingSpec(EffectClass, 1.f, ContextHandle);

		if (!SpecHandle.IsValid())
		{
			continue;
		}

		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void AZfCharacter::InitializeAttributes()
{
	if (!HasAuthority()) return;
	
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	if (InitializationAttributeEffects.IsEmpty())
	{
		return;
	}

	for (const TSubclassOf<UGameplayEffect>& EffectClass : InitializationAttributeEffects)
	{
		if (!EffectClass)
		{
			continue;
		}

		FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
		ContextHandle.AddSourceObject(this);

		const FGameplayEffectSpecHandle SpecHandle =
			ASC->MakeOutgoingSpec(EffectClass, 1.f, ContextHandle);

		if (!SpecHandle.IsValid())
		{
			continue;
		}

		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void AZfCharacter::RegisterAttributeRefreshDelegates()
{
	if (!HasAuthority()) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;

	if (AttributeRefreshDependencies.IsEmpty()) return;

	for (const FZfAttributeRefreshDependency& Dependency : AttributeRefreshDependencies)
	{
		if (!Dependency.SourceAttribute.IsValid() || !Dependency.DependentEffect)
		{
			continue;
		}

		TSubclassOf<UGameplayEffect> EffectClass = Dependency.DependentEffect;

		ASC->GetGameplayAttributeValueChangeDelegate(Dependency.SourceAttribute)
			.AddLambda([this, EffectClass](const FOnAttributeChangeData& Data)
			{
				RefreshEffect(EffectClass);
			});
	}
}

void AZfCharacter::RegisterResourceSyncDelegates()
{
    if (!HasAuthority()) return;

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC) return;

    const UZfResourceAttributeSet* ResourceSet = ASC->GetSet<UZfResourceAttributeSet>();
    if (!ResourceSet) return;

    // ── MaxHealth → CurrentHealth ─────────────────────────────────────────
    ASC->GetGameplayAttributeValueChangeDelegate(
        UZfResourceAttributeSet::GetMaxHealthAttribute())
        .AddLambda([this](const FOnAttributeChangeData& Data)
        {
            UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
            if (!ASC) return;

            const UZfResourceAttributeSet* Set = ASC->GetSet<UZfResourceAttributeSet>();
            if (!Set) return;

            const float OldMax = Data.OldValue;
            const float NewMax = Data.NewValue;
            const float Current = Set->GetCurrentHealth();

            if (OldMax > 0.f)
            {
                const float Proportion = Current / OldMax;
                ASC->SetNumericAttributeBase(UZfResourceAttributeSet::GetCurrentHealthAttribute(),
                    FMath::Clamp(Proportion * NewMax, 0.f, NewMax));
            }
            else
            {
                if (Current <= 0.f)
                    // Personagem novo — começa no máximo
                    ASC->SetNumericAttributeBase(UZfResourceAttributeSet::GetCurrentHealthAttribute(), NewMax);
                else
                    // Load do save — apenas clampa
                    ASC->SetNumericAttributeBase(UZfResourceAttributeSet::GetCurrentHealthAttribute(),
                        FMath::Clamp(Current, 0.f, NewMax));
            }
        });

    // ── MaxMana → CurrentMana ─────────────────────────────────────────────
    ASC->GetGameplayAttributeValueChangeDelegate(
        UZfResourceAttributeSet::GetMaxManaAttribute())
        .AddLambda([this](const FOnAttributeChangeData& Data)
        {
            UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
            if (!ASC) return;

            const UZfResourceAttributeSet* Set = ASC->GetSet<UZfResourceAttributeSet>();
            if (!Set) return;

            const float OldMax = Data.OldValue;
            const float NewMax = Data.NewValue;
            const float Current = Set->GetCurrentMana();

            if (OldMax > 0.f)
            {
                const float Proportion = Current / OldMax;
                ASC->SetNumericAttributeBase(UZfResourceAttributeSet::GetCurrentManaAttribute(), FMath::Clamp(Proportion * NewMax, 0.f, NewMax));
            }
            else
            {
                if (Current <= 0.f)
                    ASC->SetNumericAttributeBase(UZfResourceAttributeSet::GetCurrentManaAttribute(), NewMax);
                else
                    ASC->SetNumericAttributeBase(UZfResourceAttributeSet::GetCurrentManaAttribute(),
                        FMath::Clamp(Current, 0.f, NewMax));
            }
        });

    // ── MaxStamina → CurrentStamina ───────────────────────────────────────
    ASC->GetGameplayAttributeValueChangeDelegate(
        UZfResourceAttributeSet::GetMaxStaminaAttribute())
        .AddLambda([this](const FOnAttributeChangeData& Data)
        {
            UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
            if (!ASC) return;

            const UZfResourceAttributeSet* Set = ASC->GetSet<UZfResourceAttributeSet>();
            if (!Set) return;

            const float OldMax = Data.OldValue;
            const float NewMax = Data.NewValue;
            const float Current = Set->GetCurrentStamina();

            if (OldMax > 0.f)
            {
                const float Proportion = Current / OldMax;
                ASC->SetNumericAttributeBase(UZfResourceAttributeSet::GetCurrentStaminaAttribute(), FMath::Clamp(Proportion * NewMax, 0.f, NewMax));
            }
            else
            {
                if (Current <= 0.f)
                    ASC->SetNumericAttributeBase(UZfResourceAttributeSet::GetCurrentStaminaAttribute(), NewMax);
                else
                    ASC->SetNumericAttributeBase(UZfResourceAttributeSet::GetCurrentStaminaAttribute(),
                        FMath::Clamp(Current, 0.f, NewMax));
            }
        });
}

void AZfCharacter::InitializeDependentAttributes()
{
	if (!HasAuthority()) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;

	if (AttributeRefreshDependencies.IsEmpty()) return;

	// Coleta apenas os DependentEffects únicos — evita aplicar o mesmo
	// GE múltiplas vezes caso ele apareça em mais de uma dependência
	TSet<TSubclassOf<UGameplayEffect>> UniqueEffects;

	for (const FZfAttributeRefreshDependency& Dependency : AttributeRefreshDependencies)
	{
		if (Dependency.DependentEffect)
		{
			UniqueEffects.Add(Dependency.DependentEffect);
		}
	}

	for (const TSubclassOf<UGameplayEffect>& EffectClass : UniqueEffects)
	{
		
		FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
		ContextHandle.AddSourceObject(this);

		const FGameplayEffectSpecHandle SpecHandle =
			ASC->MakeOutgoingSpec(EffectClass, 1.f, ContextHandle);

		if (!SpecHandle.IsValid())
		{
			continue;
		}

		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void AZfCharacter::RefreshEffect(TSubclassOf<UGameplayEffect> EffectClass)
{
	if (!HasAuthority()) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !EffectClass) return;

	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle =
		ASC->MakeOutgoingSpec(EffectClass, 1.f, ContextHandle);

	if (!SpecHandle.IsValid())
	{
		return;
	}

	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}
