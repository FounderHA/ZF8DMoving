// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/GameplayAbility/SkillTreeSystem/ZfAbility_Active.h"

#include "AbilitySystemComponent.h"
#include "SkillTreeSystem/ZfSkillTreeNodeData.h"
#include "SkillTreeSystem/ZfSkillAimIndicator.h"
#include "SkillTreeSystem/ZfSkillTreeComponent.h"
#include "Player/ZfPlayerState.h"
#include "GameFramework/Pawn.h"
#include "Tags/ZfGameplayTags.h"

UZfAbility_Active::UZfAbility_Active()
{
	// InstancedPerExecution — suporta execuções sobrepostas e estado por execução
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

	// ServerOnly — custo e efeitos sempre validados no servidor
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

// =============================================================================
// CheckCost — verifica recursos antes de ativar
// =============================================================================

bool UZfAbility_Active::CheckCost(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	// Guard de segurança — bloqueia ativação se atributos não atendem
	// os requisitos mínimos do nó. Caso principal é tratado pelo
	// OnRequiredAttributeChanged que desequipa o slot automaticamente.
	if (!AreAttributeRequirementsMet(ActorInfo)) return false;

	// Sem NodeData — sem custo, libera ativação
	if (!NodeData) return true;

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC) return false;

	const int32 CurrentRank = GetAbilityLevel();

	for (const FSkillCostData& Cost : NodeData->Costs)
	{
		if (!Cost.CostEffectClass) continue;

		const float CostValue = Cost.GetCostForRank(CurrentRank);
		if (CostValue <= 0.f) continue;

		// Cria spec temporário com o valor do rank atual via SetByCaller
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(
			Cost.CostEffectClass, static_cast<float>(CurrentRank), Context);

		if (!Spec.IsValid()) continue;

		Spec.Data->SetSetByCallerMagnitude(
			ZfAbilityTreeTags::SkillTree_Data_AbilityCost, CostValue);

		// CanApplyAttributeModifiers checa se o recurso é suficiente
		// respeitando os clamps do PreAttributeChange do AttributeSet
		if (!ASC->CanApplyAttributeModifiers(
			Cost.CostEffectClass->GetDefaultObject<UGameplayEffect>(),
			static_cast<float>(CurrentRank),
			Context))
		{
			UE_LOG(LogTemp, Verbose,
				TEXT("UZfAbility_Active::CheckCost: recurso insuficiente em '%s'."),
				*GetName());
			return false;
		}
	}

	return true;
}

// =============================================================================
// ApplyCost — aplica GEs de custo com valores do NodeData
// =============================================================================

void UZfAbility_Active::ApplyCost(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!NodeData) return;

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC) return;

	const int32 CurrentRank = GetAbilityLevel();

	for (const FSkillCostData& Cost : NodeData->Costs)
	{
		if (!Cost.CostEffectClass) continue;

		const float CostValue = Cost.GetCostForRank(CurrentRank);
		if (CostValue <= 0.f) continue;

		FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(
			Handle, ActorInfo, ActivationInfo,
			Cost.CostEffectClass,
			static_cast<float>(CurrentRank));

		if (!Spec.IsValid())
		{
			UE_LOG(LogTemp, Warning,
				TEXT("UZfAbility_Active::ApplyCost: falha ao criar spec para '%s'."),
				*Cost.CostEffectClass->GetName());
			continue;
		}

		// Passa o custo base via SetByCaller — MMC aplica a redução do atributo
		Spec.Data->SetSetByCallerMagnitude(
			ZfAbilityTreeTags::SkillTree_Data_AbilityCost, CostValue);

		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
	}
}

// =============================================================================
// ApplyCooldown — aplica GE de cooldown com duração do NodeData
// =============================================================================

void UZfAbility_Active::ApplyCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	// Sem NodeData ou sem GE de cooldown — sem cooldown
	if (!NodeData || !GetCooldownGameplayEffect()) return;

	const int32 CurrentRank = GetAbilityLevel();
	const float Duration = NodeData->GetCooldownDuration(CurrentRank);

	// Duração zero ou negativa — sem cooldown para este rank
	if (Duration <= 0.f) return;

	FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(
		Handle, ActorInfo, ActivationInfo,
		GetCooldownGameplayEffect()->GetClass(),
		static_cast<float>(CurrentRank));

	if (!Spec.IsValid()) return;

	// Passa a duração dinamicamente via SetByCaller.
	// O GE de cooldown deve usar Duration Policy: Has Duration
	// com Magnitude: SetByCaller e tag Cooldown.Duration.
	Spec.Data->SetSetByCallerMagnitude(
		ZfAbilityTreeTags::Cooldown_Duration,
		Duration);

	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
}

// =============================================================================
// ActivateAbility — fluxo principal
// =============================================================================

void UZfAbility_Active::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ActorInfo)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// CommitAbility chama ApplyCost() + ApplyCooldown() — ambos implementados acima
	// Se falhar (recurso insuficiente ou cooldown ativo), cancela a ability
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Reproduz montage se configurada
	if (AbilityMontage)
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		if (ASC)
		{
			ASC->PlayMontage(this, ActivationInfo, AbilityMontage, 1.f);
		}
	}

	// Blueprint implementa a lógica da ability aqui
	OnAbilityActivated(Handle, *ActorInfo, ActivationInfo);
}
// =============================================================================
// Targeting em runtime
// =============================================================================

float UZfAbility_Active::GetEffectiveAimRadius_Implementation() const
{
	// Fallback: valor base do NodeData para o rank atual.
	// Override no Blueprint filho para aplicar modificadores de sub-efeito.
	if (!NodeData) return 300.f;
	return NodeData->GetAimRadiusForRank(GetAbilityLevel());
}

float UZfAbility_Active::GetEffectiveMaxRange_Implementation() const
{
	// Fallback: valor base do NodeData para o rank atual.
	// Override no Blueprint filho para aplicar modificadores de sub-efeito.
	if (!NodeData) return 1500.f;
	return NodeData->GetMaxRangeForRank(GetAbilityLevel());
}

// =============================================================================
// AimMode
// =============================================================================

void UZfAbility_Active::EnterAimMode()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo) return;

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC) return;

	// Guard: ja esta em AimMode
	if (IsValid(ActiveAimIndicator)) return;

	// Adiciona tag ao ASC — BP_PlayerController intercepta
	// WeaponPrimary e ESC enquanto esta tag estiver ativa
	ASC->AddLooseGameplayTag(ZfAbilityTreeTags::SkillTree_AimMode_Active);

	// Spawna o indicador apenas se configurado e apenas no cliente local
	// O servidor nunca spawna o indicador — puramente visual
	if (!AimIndicatorClass) return;

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	if (!AvatarActor) return;

	// Apenas o owning client spawna o indicador
	const ENetRole LocalRole = AvatarActor->GetLocalRole();
	if (LocalRole != ROLE_AutonomousProxy && !AvatarActor->HasAuthority())
	{
		// Simulated proxy — sem indicador
		return;
	}

	// Spawna no origin do avatar — InitializeIndicator posiciona corretamente
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = AvatarActor;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ActiveAimIndicator = AvatarActor->GetWorld()->SpawnActor<AZfSkillAimIndicator>(
		AimIndicatorClass,
		AvatarActor->GetActorLocation(),
		FRotator::ZeroRotator,
		SpawnParams);

	if (!IsValid(ActiveAimIndicator)) return;

	// Inicializa com os valores efetivos do rank e sub-efeitos atuais.
	// O tipo do indicador e definido pelo Blueprint filho via EditDefaultsOnly —
	// BP_AimIndicator_GroundCircle configura GroundCircle,
	// BP_AimIndicator_Reticle configura Reticle.
	APawn* OwnerPawn = Cast<APawn>(AvatarActor);
	ActiveAimIndicator->InitializeIndicator(
		GetEffectiveMaxRange(),
		GetEffectiveAimRadius(),
		ActiveAimIndicator->IndicatorType,
		OwnerPawn);

	// Registra no SkillTreeComponent para que o BP_PlayerController
	// possa acessar HitLocation/HitNormal e NodeID sem referência direta à GA
	if (OwnerPawn)
	{
		if (AZfPlayerState* PS = OwnerPawn->GetPlayerState<AZfPlayerState>())
		{
			if (UZfSkillTreeComponent* TreeComp = PS->GetSkillTreeComponent())
			{
				const FName NodeID = NodeData ? NodeData->NodeID : NAME_None;
				TreeComp->SetActiveAimIndicator(ActiveAimIndicator, NodeID);
			}
		}
	}
}

void UZfAbility_Active::ExitAimMode()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo) return;

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (ASC)
	{
		// Remove a tag — BP_PlayerController volta ao comportamento normal
		ASC->RemoveLooseGameplayTag(ZfAbilityTreeTags::SkillTree_AimMode_Active);
	}

	// Limpa a referência no SkillTreeComponent antes de destruir o indicador
	if (IsValid(ActiveAimIndicator))
	{
		AActor* AvatarActor = ActorInfo->AvatarActor.Get();
		if (APawn* OwnerPawn = Cast<APawn>(AvatarActor))
		{
			if (AZfPlayerState* PS = OwnerPawn->GetPlayerState<AZfPlayerState>())
			{
				if (UZfSkillTreeComponent* TreeComp = PS->GetSkillTreeComponent())
				{
					TreeComp->ClearActiveAimIndicator();
				}
			}
		}

		ActiveAimIndicator->Destroy();
		ActiveAimIndicator = nullptr;
	}
}