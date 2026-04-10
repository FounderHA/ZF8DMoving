// Copyright ZfGame Studio. All Rights Reserved.
// ZfModifierRule.h
// Classe base abstrata para regras de modifier dinâmico.
//
// CONCEITO:
// Uma Rule encapsula a fórmula que transforma o CurrentValue (resultado
// do roll) em um FinalValue calculado em runtime a partir de uma fonte
// externa (atributo do GAS, variável do ItemInstance, etc.).
//
// CICLO DE VIDA:
// 1. EquipmentComponent instancia a subclasse via NewObject ao equipar
// 2. BindToSource() conecta aos delegates da fonte
// 3. Calculate() gera o FinalValue inicial
// 4. Fonte muda → OnRuleValueChanged dispara → EquipmentComponent reaplica
// 5. UnbindFromSource() desconecta tudo ao desequipar
// 6. Instância é destruída (GC)
//
// COMO CRIAR UMA NOVA RULE:
// 1. Crie uma subclasse de UZfModifierRule
// 2. Override BindToSource_Implementation — conecte ao delegate da fonte
// 3. Override UnbindFromSource_Implementation — limpe todos os handles
// 4. Override Calculate_Implementation — implemente a fórmula
// 5. Chame NotifyValueChanged() sempre que a fonte mudar

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "ZfModifierRule.generated.h"

class UAbilitySystemComponent;
class UZfItemInstance;

// ============================================================
// FZfModifierRuleContext
// ============================================================
// Contexto passado para Calculate() com tudo que uma Rule
// pode precisar para calcular o FinalValue.
// Expandível sem quebrar a assinatura de Calculate.
// ============================================================

USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfModifierRuleContext
{
    GENERATED_BODY()

    // ASC do personagem que está usando o item
    UPROPERTY(BlueprintReadOnly, Category = "Rule|Context")
    TObjectPtr<UAbilitySystemComponent> ASC = nullptr;

    // ItemInstance que contém o modifier com esta Rule
    UPROPERTY(BlueprintReadOnly, Category = "Rule|Context")
    TObjectPtr<UZfItemInstance> ItemInstance = nullptr;
};

// ============================================================
// Delegate — disparado quando o FinalValue precisa ser
// recalculado porque a fonte mudou.
// EquipmentComponent escuta este delegate para reaplica o GE
// ou a propriedade do item com o novo FinalValue.
// ============================================================

DECLARE_MULTICAST_DELEGATE(FOnModifierRuleValueChanged);

// ============================================================
// UZfModifierRule
// ============================================================

UCLASS(Abstract, Blueprintable, BlueprintType)
class ZF8DMOVING_API UZfModifierRule : public UObject
{
    GENERATED_BODY()

public:

    // ----------------------------------------------------------
    // INTERFACE PRINCIPAL
    // Chamadas pelo EquipmentComponent no ciclo de vida do modifier.
    // ----------------------------------------------------------

    // Conecta a Rule à sua fonte de dados.
    // Cada subclasse implementa o bind correto:
    //   - Atributo GAS → ASC->GetGameplayAttributeValueChangeDelegate()
    //   - Durabilidade  → ItemInstance->OnDurabilityChanged
    // Armazene o contexto internamente para uso em Calculate.
    // @param InContext — ASC e ItemInstance do equipador
    UFUNCTION(BlueprintNativeEvent, Category = "Zf|ModifierRule")
    void BindToSource(const FZfModifierRuleContext& InContext);
    virtual void BindToSource_Implementation(const FZfModifierRuleContext& InContext);

    // Desconecta todos os delegates registrados em BindToSource.
    // Sempre chamado antes de destruir a instância.
    UFUNCTION(BlueprintNativeEvent, Category = "Zf|ModifierRule")
    void UnbindFromSource();
    virtual void UnbindFromSource_Implementation();

    // Calcula o FinalValue a partir do CurrentValue (fator do roll)
    // e do estado atual da fonte.
    // Regra: se a fonte for inválida, retorne CurrentValue sem modificar.
    // @param CurrentValue — valor rolado na geração do modifier
    // @return FinalValue  — valor a ser aplicado no destino
    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Zf|ModifierRule")
    float Calculate(float CurrentValue) const;
    virtual float Calculate_Implementation(float CurrentValue) const;

    // ----------------------------------------------------------
    // DELEGATE PÚBLICO
    // O EquipmentComponent faz bind aqui ao instanciar a Rule.
    // A Rule dispara este delegate via NotifyValueChanged()
    // sempre que a fonte muda e um novo Calculate() é necessário.
    // ----------------------------------------------------------
    FOnModifierRuleValueChanged OnRuleValueChanged;

protected:

    // Contexto armazenado no BindToSource — disponível para Calculate
    // e para os callbacks internos dos delegates da fonte.
    UPROPERTY(BlueprintReadOnly, Category = "Rule|Internal")
    FZfModifierRuleContext CachedContext;

    // Dispara OnRuleValueChanged.
    // Subclasses chamam isso dentro dos callbacks da fonte.
    // Ex: no callback de OnDurabilityChanged → NotifyValueChanged()
    void NotifyValueChanged();
};