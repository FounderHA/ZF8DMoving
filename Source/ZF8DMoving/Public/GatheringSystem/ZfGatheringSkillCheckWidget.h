// Copyright ZfGame Studio. All Rights Reserved.
// ZfGatherSkillCheckWidget.h
//
// Widget PURAMENTE VISUAL. Não sabe nada sobre ângulos reais, zonas ou resultados.
// Faz bind nos delegates do ZfGatheringComponent e reage visualmente.
//
// RESPONSABILIDADES:
// - Girar o ponteiro via tick visual LOCAL (não recebe mais ângulo via delegate)
// - Exibir as zonas quando OnSkillCheckRoundBegun é recebido via RepNotify
// - Animar feedback quando OnSkillCheckHitEvaluated é recebido via RepNotify
//
// TICK VISUAL LOCAL:
// Ao receber OnSkillCheckRoundBegun, a widget inicia seu próprio NativeTick
// avançando LocalAngle na mesma velocidade que o servidor (NeedleRotTime).
// O ângulo local é APENAS COSMÉTICO — o servidor usa seu próprio ângulo
// interno para avaliar hits. Pequenas divergências de timing são aceitáveis.
//
// INICIALIZAÇÃO TARDIA:
// Se a widget for criada após o OnRep_RoundData já ter chegado (o componente
// já tem dados de round válidos), InitSkillCheck exibe o round imediatamente
// sem precisar aguardar o próximo RepNotify.
//
// QUEM RASTREIA O ÂNGULO REAL:  ZfGatheringComponent (servidor)
// QUEM AVALIA O HIT:            ZfGatheringComponent (servidor)
// QUEM REGISTRA O CLIQUE:       ZfGA_GatheringBase::Server_RegisterHit()
// QUEM ENVIA O EVENTO:          ZfGatheringComponent (servidor → GameplayEvent)

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
    // Chamada pela GA no cliente após criar a widget.
    // Faz bind nos delegates do component — a partir daqui tudo é reativo.
    //
    // Se o componente já tiver dados de round válidos (RoundData chegou
    // antes da widget ser criada), exibe imediatamente sem aguardar RepNotify.
    // ----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Zf|SkillCheck")
    void InitSkillCheck(UZfGatheringComponent* InGatherComponent);

protected:

    virtual void NativeDestruct() override;

    // ----------------------------------------------------------
    // TICK VISUAL LOCAL
    // Avança LocalAngle a cada frame e chama BP_OnAngleUpdated.
    // Ativo apenas enquanto bLocalTickActive = true.
    // ----------------------------------------------------------

    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

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

    // Ângulo visual atualizado a cada frame — gire o ponteiro: Angle * 360 = graus.
    // Este ângulo é LOCAL e cosmético — não é o ângulo real do servidor.
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

    // ----------------------------------------------------------
    // TICK VISUAL LOCAL
    // ----------------------------------------------------------

    // true enquanto o ponteiro deve girar visualmente
    bool  bLocalTickActive  = false;

    // Ângulo visual atual (0.0–1.0) — cosmético, não é o ângulo do servidor
    float LocalAngle        = 0.0f;

    // Voltas por segundo (1.0 / NeedleRotTime)
    float LocalAngularSpeed = 0.0f;

    // Inicia o tick visual com a velocidade do round atual
    void StartLocalTick(float InNeedleRotTime);

    // Para o tick visual (após hit ou destruição)
    void StopLocalTick();

    // ----------------------------------------------------------
    // HANDLERS DOS DELEGATES DO COMPONENT
    // ----------------------------------------------------------

    UFUNCTION()
    void Internal_OnRoundBegun(
        float GoodStart, float GoodSize,
        float PerfectStart, float PerfectSize,
        float NeedleRotTime);

    UFUNCTION()
    void Internal_OnHitEvaluated(EZfGatherHitResult Result);
};