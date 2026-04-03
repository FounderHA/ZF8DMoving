// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "ZfAbilitySystemComponent.generated.h"

/**
 * AbilitySystemComponent customizado do projeto Zf.
 *
 * Responsabilidade única: garantir ReplicationMode correto na inicialização.
 *
 * A concessão de startup abilities foi movida para AZfCharacter::GrantStartupAbilities,
 * tornando as abilities configuráveis por Blueprint em qualquer subclasse de personagem.
 *
 * Replication modes:
 *   Full    → todos os clientes recebem todos os GEs  (AI simples)
 *   Mixed   → GEs só para o dono, Cues/Tags para todos (jogador com ASC no PlayerState)
 *   Minimal → nenhum GE replicado  (AI em jogos com muitos NPCs)
 */
UCLASS()
class ZF8DMOVING_API UZfAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
	
public:
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;
};
