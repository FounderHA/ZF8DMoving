// Copyright ZfGame Studio. All Rights Reserved.
// ZfGatherSkillCheckWidget.h
//
// Widget do QTE de coleta de recursos.
// Gerencia o ponteiro giratório, as zonas Good/Perfect no material,
// o timer de volta completa e o envio do resultado via GameplayEvent.
//
// CICLO DE VIDA:
// 1. GA cria a widget e chama InitSkillCheck() uma vez
// 2. A cada golpe, GA chama StartRound()
// 3. Widget gira o ponteiro, detecta input e envia o resultado
// 4. GA recebe o evento, processa o golpe e chama StartRound() novamente
// 5. GA destrói a widget no EndAbility

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GatheringSystem/ZfGatherTypes.h"
#include "ZfGatherSkillCheckWidget.generated.h"

// Forward declarations
class UImage;
class UTextBlock;
class UMaterialInstanceDynamic;
class UZfGatherResourceData;
struct FZfGatherToolEntry;

// ============================================================
// EZfSkillCheckState
// Estado interno da widget — controla o que o Tick faz.
// ============================================================

UENUM(BlueprintType)
enum class EZfSkillCheckState : uint8
{
    Idle,           // Aguardando InitSkillCheck()
    Running,        // Ponteiro girando — aguarda input
    ShowingResult,  // Exibindo feedback visual do resultado
    Finished        // Volta completa sem input (Missed) — aguarda GA
};

// ============================================================
// UZfGatherSkillCheckWidget
// ============================================================

UCLASS()
class ZF8DMOVING_API UZfGatherSkillCheckWidget : public UUserWidget
{
    GENERATED_BODY()

public:

    // ----------------------------------------------------------
    // INICIALIZAÇÃO
    // Chamada pela GA uma única vez após criar a widget.
    // Guarda referências, cria o DynamicMaterial e seta
    // os parâmetros fixos (AngleOffset, cores, ring).
    // ----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Zf|SkillCheck")
    void InitSkillCheck(
        UZfGatherResourceData* InResourceData,
        const FGameplayTag&    InToolTag,
        AActor*                InOwnerActor
    );

    // ----------------------------------------------------------
    // CONTROLE DE ROUND
    // StartRound() é chamado pela GA a cada novo golpe.
    // Randomiza GoodStart, recalcula PerfectStart, atualiza
    // o material e começa a girar o ponteiro.
    // ----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Zf|SkillCheck")
    void StartRound();

    // ----------------------------------------------------------
    // INPUT
    // Chamado pelo EnhancedInput binding da GA ou do PlayerController.
    // Só processa se State == Running.
    // ----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Zf|SkillCheck")
    void OnInteractPressed();

    // ----------------------------------------------------------
    // BLUEPRINT EVENTS
    // Implementados no WBP para feedback visual por resultado.
    // Chamados automaticamente pelo C++ após cada golpe.
    // ----------------------------------------------------------

    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|SkillCheck")
    void OnResultPerfect();

    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|SkillCheck")
    void OnResultGood();

    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|SkillCheck")
    void OnResultBad();

    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|SkillCheck")
    void OnResultMissed();

    // ----------------------------------------------------------
    // CONFIGURAÇÃO EXPOSTA AO BLUEPRINT
    // ----------------------------------------------------------

    // Offset de rotação para posicionar o ponto 0 no topo.
    // 0.25 = topo (12h). Exposto para ajuste visual no WBP.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillCheck|Config")
    float AngleOffset = 0.25f;

    // Duração em segundos do feedback visual antes do próximo round.
    // Controlado pelo Blueprint via OnResult* events.
    // O C++ usa este valor para agendar o próximo StartRound() se
    // a GA optar por auto-continuar.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillCheck|Config")
    float ResultFeedbackDuration = 0.4f;

protected:

    // ----------------------------------------------------------
    // BIND WIDGETS
    // Nomes devem bater exatamente com os da hierarquia do WBP.
    // ----------------------------------------------------------

    // Imagem que recebe o MI_GatheringSkillCheck.
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UImage> ZoneImage;

    // Imagem da seta/ponteiro — rotacionada via RenderTransform.
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UImage> PointerImage;

    // ----------------------------------------------------------
    // OVERRIDES
    // ----------------------------------------------------------

    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float DeltaTime) override;

private:

    // ----------------------------------------------------------
    // ESTADO INTERNO
    // ----------------------------------------------------------

    EZfSkillCheckState State = EZfSkillCheckState::Idle;

    // Ângulo atual do ponteiro normalizado (0.0 a 1.0).
    // 0 = topo, 0.5 = baixo, 1.0 = volta completa.
    float CurrentAngle = 0.0f;

    // Velocidade de rotação em voltas por segundo.
    // Calculado em InitSkillCheck(): 1.0f / QTEWindowSeconds.
    float AngularSpeed = 0.0f;

    // Timer para sair do estado ShowingResult.
    float ResultFeedbackTimer = 0.0f;

    // ----------------------------------------------------------
    // DADOS DO ROUND ATUAL
    // ----------------------------------------------------------

    float CurrentGoodStart   = 0.0f;
    float CurrentGoodSize    = 0.0f;
    float CurrentPerfectStart = 0.0f;
    float CurrentPerfectSize  = 0.0f;

    // ----------------------------------------------------------
    // REFERÊNCIAS
    // ----------------------------------------------------------

    UPROPERTY()
    TObjectPtr<UZfGatherResourceData> ResourceData;

    UPROPERTY()
    TObjectPtr<AActor> OwnerActor;

    FGameplayTag ToolTag;

    // Entrada da ferramenta resolvida no InitSkillCheck().
    // Guardada para evitar busca a cada StartRound().
    const FZfGatherToolEntry* ToolEntry = nullptr;

    // Material dinâmico criado a partir do MI_GatheringSkillCheck.
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> SkillCheckMID;

    // ----------------------------------------------------------
    // FUNÇÕES INTERNAS
    // ----------------------------------------------------------

    // Avalia o ângulo atual e retorna o resultado do QTE.
    EZfGatherHitResult EvaluateHit() const;

    // Verifica se um ângulo está dentro de um range circular (0..1).
    bool IsAngleInRange(float Angle, float Start, float Size) const;

    // Atualiza os parâmetros do material para o round atual.
    void UpdateMaterialZones() const;

    // Processa o resultado: dispara o evento GAS e o feedback visual.
    void ProcessResult(EZfGatherHitResult Result);

    // Envia o GameplayEvent para a GA via SendGameplayEventToActor.
    void BroadcastResultEvent(EZfGatherHitResult Result) const;

    // Retorna a GameplayTag correspondente ao resultado.
    static FGameplayTag GetTagForResult(EZfGatherHitResult Result);
};