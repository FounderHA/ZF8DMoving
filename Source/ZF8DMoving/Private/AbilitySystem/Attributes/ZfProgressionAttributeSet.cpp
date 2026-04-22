// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Attributes/ZfProgressionAttributeSet.h"

#include "AbilitySystemComponent.h"
#include "Engine/CurveTable.h"
#include "GameplayEffectExtension.h"
#include "Tags/ZfGameplayTags.h"
#include "Net/UnrealNetwork.h"

// ─────────────────────────────────────────────────────────────────────────────
// Nome da linha na CurveTable que define XP necessário por nível.
// Formato da linha no CSV:  Progression.XPToNextLevel,300,500,750,...
// ─────────────────────────────────────────────────────────────────────────────

static const FName ProgressionXPRowName = FName(TEXT("Progression.XPToNextLevel"));

// =============================================================================
// Construtor
// =============================================================================

UZfProgressionAttributeSet::UZfProgressionAttributeSet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, LevelProgressionCurveTable(nullptr)
	, MaxLevel(60)
{
	// Valores iniciais — serão sobrescritos pelo GE de inicialização
	// aplicado pelo PlayerState/PlayerController ao possuir o pawn.
	InitLevel(1.f);
	InitXP(0.f);
	InitTotalXP(0.f);
	InitXPToNextLevel(300.f);
	InitAttributePoints(0.f);
	InitSkillPoints(0.f);
	InitStrengthPoints(0.f);
	InitDexterityPoints(0.f);
	InitIntelligencePoints(0.f);
	InitConstitutionPoints(0.f);
	InitConvictionPoints(0.f);
	InitIncomingXP(0.f);
}

// =============================================================================
// Replicação
// =============================================================================

void UZfProgressionAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// IncomingXP é meta-atributo — propositalmente NÃO replicado.
	DOREPLIFETIME_CONDITION_NOTIFY(UZfProgressionAttributeSet, Level,				COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfProgressionAttributeSet, XP,					COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfProgressionAttributeSet, XPToNextLevel,		COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfProgressionAttributeSet, TotalXP,				COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfProgressionAttributeSet, AttributePoints,		COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfProgressionAttributeSet, SkillPoints,			COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfProgressionAttributeSet, StrengthPoints,		COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfProgressionAttributeSet, DexterityPoints,		COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfProgressionAttributeSet, IntelligencePoints,	COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfProgressionAttributeSet, ConstitutionPoints,	COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZfProgressionAttributeSet, ConvictionPoints,	COND_None, REPNOTIFY_Always);
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
	else if (Attribute == GetSkillPointsAttribute())
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
	// XxxPoints nunca negativos
	else if (Attribute == GetStrengthPointsAttribute()    ||
			 Attribute == GetDexterityPointsAttribute()    ||
			 Attribute == GetIntelligencePointsAttribute() ||
			 Attribute == GetConstitutionPointsAttribute() ||
			 Attribute == GetConvictionPointsAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
	}
}

// =============================================================================
// PostGameplayEffectExecute — lógica de negócio (servidor only)
// =============================================================================

void UZfProgressionAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayAttribute& Attr = Data.EvaluatedData.Attribute;
	
	// Único ponto de entrada de XP: meta-atributo IncomingXP.
	// GE_GiveXP (Instant, SetByCaller) escreve aqui. Qualquer outra
	// fonte de XP (quest, item) usa o mesmo GE — nunca modifica XP diretamente.
	if (Attr == GetIncomingXPAttribute())
	{
		HandleIncomingXP(Data);
		return;
	}
	
	// ── Pontos de atributo mudaram → recalcular atributos tocados ─────────
	// Monta o set de atributos que foram modificados nesta execução.
	// Apenas os atributos cujos XxxPoints mudaram entram no recalculo.
	TSet<FGameplayTag> ChangedAttributes;
 
	if (Attr == GetStrengthPointsAttribute())
		ChangedAttributes.Add(ZfAttributeTags::ZfMainAttributeTags::Attribute_Strength);
	if (Attr == GetDexterityPointsAttribute())
		ChangedAttributes.Add(ZfAttributeTags::ZfMainAttributeTags::Attribute_Dexterity);
	if (Attr == GetIntelligencePointsAttribute())
		ChangedAttributes.Add(ZfAttributeTags::ZfMainAttributeTags::Attribute_Intelligence);
	if (Attr == GetConstitutionPointsAttribute())
		ChangedAttributes.Add(ZfAttributeTags::ZfMainAttributeTags::Attribute_Constitution);
	if (Attr == GetConvictionPointsAttribute())
		ChangedAttributes.Add(ZfAttributeTags::ZfMainAttributeTags::Attribute_Conviction);
 
	if (ChangedAttributes.Num() > 0)
	{
		ApplyRecalculateEffects(ChangedAttributes);
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

void UZfProgressionAttributeSet::OnRep_SkillPoints(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfProgressionAttributeSet, SkillPoints, OldValue);
}

void UZfProgressionAttributeSet::OnRep_StrengthPoints(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfProgressionAttributeSet, StrengthPoints, OldValue);
}
 
void UZfProgressionAttributeSet::OnRep_DexterityPoints(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfProgressionAttributeSet, DexterityPoints, OldValue);
}
 
void UZfProgressionAttributeSet::OnRep_IntelligencePoints(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfProgressionAttributeSet, IntelligencePoints, OldValue);
}
 
void UZfProgressionAttributeSet::OnRep_ConstitutionPoints(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfProgressionAttributeSet, ConstitutionPoints, OldValue);
}
 
void UZfProgressionAttributeSet::OnRep_ConvictionPoints(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZfProgressionAttributeSet, ConvictionPoints, OldValue);
}

// =============================================================================
// Lógica privada
// =============================================================================

void UZfProgressionAttributeSet::HandleIncomingXP(const FGameplayEffectModCallbackData& Data)
{
	const float Earned = GetIncomingXP();

	if (Earned <= 0.f) return;

	SetIncomingXP(0.f);
	SetTotalXP(GetTotalXP() + Earned);

	float AccumulatedXP = GetXP() + Earned;
	int32 LevelsGained  = 0;

	while (true)
	{
		const int32 CurrentLevel     = FMath::FloorToInt(GetLevel());
		const float CurrentThreshold = GetXPThresholdForLevel(CurrentLevel);

		// Sem threshold definido na CurveTable — nível máximo da tabela atingido.
		if (CurrentThreshold <= 0.f)
		{
			break;
		}

		// Verifica teto de nível — MaxLevel == 0 significa infinito.
		if (MaxLevel > 0 && CurrentLevel >= MaxLevel)
		{
			// XP trava no threshold do nível máximo.
			// Não zera, não passa — fica exatamente no valor do threshold.
			// TotalXP já foi acumulado antes do loop.
			AccumulatedXP = CurrentThreshold;
			break;
		}

		// XP insuficiente para o próximo nível.
		if (AccumulatedXP < CurrentThreshold)
		{
			break;
		}

		// Sobe de nível.
		AccumulatedXP -= CurrentThreshold;
		const float NewLevel = GetLevel() + 1.f;
		SetLevel(NewLevel);

		const float NextThreshold = GetXPThresholdForLevel(FMath::FloorToInt(NewLevel));
		SetXPToNextLevel(FMath::Max(1.f, NextThreshold));

		LevelsGained++;

		// Acabou de atingir o MaxLevel — trava o XP no threshold deste nível.
		if (MaxLevel > 0 && FMath::FloorToInt(NewLevel) >= MaxLevel)
		{
			AccumulatedXP = FMath::Max(1.f, NextThreshold);
			break;
		}
	}

	SetXP(AccumulatedXP);

	// Dispara UM único evento com o resultado consolidado do loop.
	// GA_LevelUp lida com todos os níveis ganhos de uma vez:
	//   EventMagnitude → nível final alcançado
	//   RawMagnitude   → quantos níveis foram ganhos (para calcular rewards)
	if (LevelsGained > 0)
	{
		if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
		{
			FGameplayEventData Payload;
			Payload.EventMagnitude = static_cast<float>(LevelsGained); // níveis ganhos
			Payload.Instigator     = ASC->GetAvatarActor();
			Payload.Target         = ASC->GetAvatarActor();

			// FinalLevel não é passado no payload — GA_LevelUp lê diretamente
			// do ProgressionAttributeSet via ASC, evitando o campo inexistente
			// RawMagnitude em FGameplayEventData.
			ASC->HandleGameplayEvent(ZfProgressionTags::LevelProgression_Event_Character_LevelUp, &Payload);
		}
	}
}

void UZfProgressionAttributeSet::ApplyRecalculateEffects(const TSet<FGameplayTag>& ChangedAttributes)
{
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!ASC) return;
 
	for (const FGameplayTag& AttrTag : ChangedAttributes)
	{
		const TSubclassOf<UGameplayEffect>* EffectClass = RecalculateEffects.Find(AttrTag);
		if (!EffectClass || !(*EffectClass))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("UZfProgressionAttributeSet: Nenhum GE de recalculo configurado "
					 "para a tag '%s'. Configure em RecalculateEffects no editor."),
				*AttrTag.ToString());
			continue;
		}
 
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(ASC->GetOwnerActor());
 
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(*EffectClass, 1.f, Context);
		if (Spec.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
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