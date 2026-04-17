// Copyright ZfGame Studio. All Rights Reserved.
// ZfGatherSkillCheckWidget.h
//
// Widget PURAMENTE VISUAL. Não sabe nada sobre ângulos, zonas ou resultados.
// Faz bind nos delegates do ZfGatheringComponent e reage visualmente.
//
// RESPONSABILIDADES:
// - Girar o ponteiro (recebe o ângulo via delegate)
// - Exibir as zonas (recebe posições via delegate)
// - Animar feedback (recebe resultado via delegate)
//
// QUEM RASTREIA O ÂNGULO:  ZfGatheringComponent (Tick)
// QUEM AVALIA O HIT:       ZfGatheringComponent
// QUEM REGISTRA O CLIQUE:  ZfGA_GatheringBase::RegisterGatherHit()
// QUEM ENVIA O EVENT:      ZfGatheringComponent

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GatheringSystem/ZfGatheringTypes.h"
#include "ZfGatheringSkillCheckWidget.generated.h"

class UZfGatheringComponent;

UCLASS()
class ZF8DMOVING_API UZfGatheringSkillCheckWidget : public UUserWidget
{
    GENERATED_BODY()

public:

    // ----------------------------------------------------------
    // INICIALIZAÇÃO
    // Chamada pela GA após criar a widget.
    // Faz bind nos delegates do component — a partir daqui tudo é reativo.
    // ----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Zf|SkillCheck")
    void InitSkillCheck(UZfGatheringComponent* InGatherComponent);

protected:

    virtual void NativeDestruct() override;

    // ==========================================================
    // EVENTOS BLUEPRINT — implementar no WBP
    // ==========================================================

    // Chamado no InitSkillCheck — setup visual inicial.
    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|SkillCheck")
    void BP_OnSkillCheckReady();

    // Novo round — posicione as zonas no material.
    // Valores normalizados (0.0–1.0). Graus = valor * 360.
    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|SkillCheck")
    void BP_OnRoundStarted(
        float InGoodStart,
        float InGoodSize,
        float InPerfectStart,
        float InPerfectSize);

    // Ângulo atualizado — gire o ponteiro: Angle * 360 = graus.
    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|SkillCheck")
    void BP_OnAngleUpdated(float InAngle);

    // Resultado do hit — exiba a animação de feedback.
    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|SkillCheck")
    void BP_OnResultPerfect();

    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|SkillCheck")
    void BP_OnResultGood();

    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|SkillCheck")
    void BP_OnResultBad();

    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|SkillCheck")
    void BP_OnResultMissed();

private:

    UPROPERTY()
    TObjectPtr<UZfGatheringComponent> GatherComponent;

    // Handlers dos delegates do component
    UFUNCTION()
    void Internal_OnRoundBegun(
        float GoodStart, float GoodSize,
        float PerfectStart, float PerfectSize);

    UFUNCTION()
    void Internal_OnAngleUpdated(float NormalizedAngle);

    UFUNCTION()
    void Internal_OnHitEvaluated(EZfGatherHitResult Result);
};