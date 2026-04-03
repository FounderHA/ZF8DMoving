// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Attributes/ZfProgressionAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/CurveTable.h"
#include "GameplayEffectExtension.h"
#include "GameplayTagsModule.h"
#include "Tags/ZfGameplayTags.h"
#include "Net/UnrealNetwork.h"


// ─────────────────────────────────────────────────────────────────────────────
// Nome da linha na CurveTable que define XP necessário por nível.
// Formato da linha no CSV:  Progression.XPToNextLevel,300,500,750,...
// ─────────────────────────────────────────────────────────────────────────────
static const FName ProgressionXPRowName = FName(TEXT("Progression.XPToNextLevel"));

// =============================================================================
UZfProgressionAttributeSet::UZfProgressionAttributeSet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, LevelProgressionCurveTable(nullptr)
{
	// Valores iniciais — serão sobrescritos pelo GE de inicialização
	// aplicado pelo PlayerState/PlayerController ao possuir o pawn.
	InitLevel(1.f);
	InitXP(0.f);
	InitTotalXP(0.f);
	InitXPToNextLevel(300.f); // fallback caso a CurveTable não esteja configurada
	InitAttributePoints(0.f);
	InitIncomingXP(0.f);
}

// =============================================================================
// Replicação
// =============================================================================

void UZfProgressionAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// IncomingXP é meta-atributo — propositalmente NÃO replicado.
	DOREPLIFETIME_CONDITION_NOTIFY(UZfProgressionAttributeSet, Level,          COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfProgressionAttributeSet, XP,             COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfProgressionAttributeSet, XPToNextLevel,  COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfProgressionAttributeSet, TotalXP,        COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfProgressionAttributeSet, AttributePoints,COND_None, REPNOTIFY_Always);
}

// =============================================================================
// Pre-change: clamping de valores antes de serem aplicados
// =============================================================================

void UZfProgressionAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetLevelAttribute())
	{
		// Nível mínimo: 1. Sem limite máximo definido em código —
		// use a CurveTable como barreira natural (retorna 0 além do máximo).
		NewValue = FMath::Max(1.f, NewValue);
	}
	else if (Attribute == GetXPAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
	}
	else if (Attribute == GetXPToNextLevelAttribute())
	{
		// Threshold nunca pode ser zero: causaria loop infinito em HandleIncomingXP.
		NewValue = FMath::Max(1.f, NewValue);
	}
	else if (Attribute == GetAttributePointsAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
	}
	else if (Attribute == GetTotalXPAttribute())
	{
		// TotalXP nunca pode diminuir — é um acumulador histórico.
		NewValue = FMath::Max(GetTotalXP(), NewValue);
	}
	else if (Attribute == GetIncomingXPAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
	}
}

// =============================================================================
// Post-effect: ponto central de lógica de negócio (servidor only)
// =============================================================================

void UZfProgressionAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// Único ponto de entrada de XP: meta-atributo IncomingXP.
	// GE_GiveXP (Instant, SetByCaller) escreve aqui. Qualquer outra
	// fonte de XP (quest, item) usa o mesmo GE — nunca modifica XP diretamente.
	if (Data.EvaluatedData.Attribute == GetIncomingXPAttribute())
	{
		HandleIncomingXP(Data);
	}
}

// =============================================================================
// Post-change: chamado após qualquer mudança de atributo (servidor e cliente)
// =============================================================================

void UZfProgressionAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// Extensão futura: validações adicionais, telemetria, etc.
	// Não replique lógica de level-up aqui — use PostGameplayEffectExecute.
}

// =============================================================================
// RepNotifies
// =============================================================================

void UZfProgressionAttributeSet::OnRep_Level(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfProgressionAttributeSet, Level, OldValue);
}

void UZfProgressionAttributeSet::OnRep_XP(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfProgressionAttributeSet, XP, OldValue);
}

void UZfProgressionAttributeSet::OnRep_TotalXP(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfProgressionAttributeSet, TotalXP, OldValue);
}

void UZfProgressionAttributeSet::OnRep_XPToNextLevel(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfProgressionAttributeSet, XPToNextLevel, OldValue);
}

void UZfProgressionAttributeSet::OnRep_AttributePoints(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfProgressionAttributeSet, AttributePoints, OldValue);
}

// =============================================================================
// Lógica privada
// =============================================================================

void UZfProgressionAttributeSet::HandleIncomingXP(const FGameplayEffectModCallbackData& Data)
	{
	const float Earned = GetIncomingXP();

	// Sem XP para processar — nada a fazer.
	if (Earned <= 0.f)
	{
		return;
	}

	// Consome o meta-atributo imediatamente.
	// Isso garante que o valor não vaze para uma segunda chamada acidental.
	SetIncomingXP(0.f);

	// Acumula no histórico total imediatamente — antes do loop de level-up.
	// TotalXP nunca é decrementado, independentemente de penalidades ou respec.
	SetTotalXP(GetTotalXP() + Earned);

	float AccumulatedXP = GetXP() + Earned;

	// ─────────────────────────────────────────────────────────────────────
	// Loop de level-up: suporta múltiplos níveis no mesmo frame.
	// Exemplo: jogador no nível 3 ganha XP suficiente para ir direto ao 5.
	// ─────────────────────────────────────────────────────────────────────
	while (true)
	{
		const int32 CurrentLevel    = FMath::FloorToInt(GetLevel());
		const float CurrentThreshold = GetXPThresholdForLevel(CurrentLevel);

		// CurveTable não configurada, nível máximo atingido (threshold == 0)
		// ou XP insuficiente para o próximo nível.
		if (CurrentThreshold <= 0.f || AccumulatedXP < CurrentThreshold)
		{
			break;
		}

		// ── Sobe de nível ──────────────────────────────────────────────
		AccumulatedXP -= CurrentThreshold;
		const float NewLevel = GetLevel() + 1.f;
		SetLevel(NewLevel);

		// Atualiza XPToNextLevel inline para a próxima iteração do while.
		// GA_LevelUp NÃO precisa atualizar este atributo — já está correto aqui.
		const float NextThreshold = GetXPThresholdForLevel(FMath::FloorToInt(NewLevel));
		SetXPToNextLevel(FMath::Max(1.f, NextThreshold));

		// ── Notifica GA_LevelUp via GameplayEvent ──────────────────────
		// GA_LevelUp (ServerOnly, ativada por evento) cuida de:
		//   • Conceder AttributePoints via LR_AttributePoints
		//   • Escalar atributos via LR_ScaleAttributes + CurveTable
		//   • Disparar GameplayCue (efeitos visuais — tratados em Blueprint)
		//   • Qualquer recompensa futura adicionada ao TArray de ULevelReward
		if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
		{
			FGameplayEventData Payload;
			Payload.EventMagnitude = NewLevel;      // nível recém-alcançado
			Payload.Instigator     = ASC->GetAvatarActor();
			Payload.Target         = ASC->GetAvatarActor();

			ASC->HandleGameplayEvent(ZfProgressionTags::LevelProgression_Event_Character_LevelUp, &Payload);
		}
	}

	// Persiste o XP restante (possível sobra após o último level-up).
	SetXP(AccumulatedXP);
}

float UZfProgressionAttributeSet::GetXPThresholdForLevel(int32 InLevel) const
	{
	if (!LevelProgressionCurveTable)
	{
		// CurveTable não atribuída no CDO do Blueprint filho.
		// Retorna 0 → level-up bloqueado até a tabela ser configurada.
		UE_LOG(LogTemp, Warning,
			TEXT("UZfProgressionAttributeSet: LevelProgressionCurveTable não está configurada. "
			     "Atribua a CurveTable no CDO do Blueprint filho desta classe."));
		return 0.f;
	}

	static const FString ContextString = TEXT("UZfProgressionAttributeSet::GetXPThresholdForLevel");

	const FRealCurve* Curve = LevelProgressionCurveTable->FindCurve(ProgressionXPRowName, ContextString);
	if (!Curve)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UZfProgressionAttributeSet: Linha '%s' não encontrada na CurveTable '%s'."),
			*ProgressionXPRowName.ToString(),
			*LevelProgressionCurveTable->GetName());
		return 0.f;
	}

	// O índice da curva corresponde ao nível atual (coluna na CurveTable).
	return Curve->Eval(static_cast<float>(InLevel));
}