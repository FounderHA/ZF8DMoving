// Copyright ZfGame Studio. All Rights Reserved.
// ZfInteractionInterface.h
// Interface que define um objeto interagível no mundo.
// Adicione esta interface a qualquer ator para torná-lo interagível.
// Tudo que o sistema precisa saber sobre a interação vem daqui.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ZfInteractionTypes.h"
#include "ZfInteractionInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class UZfInteractionInterface : public UInterface
{
    GENERATED_BODY()
};

class ZF8DMOVING_API IZfInteractionInterface
{
    GENERATED_BODY()

public:

    // ----------------------------------------------------------
    // ESTADO
    // ----------------------------------------------------------

    // Retorna se este objeto pode ser interagido no momento.
    // @param Interactor — ator que está tentando interagir
    // @return estado atual da interação
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Zf|Interaction")
    EZfInteractionState GetInteractionState(AActor* Interactor) const;

    // ----------------------------------------------------------
    // CONFIGURAÇÃO
    // ----------------------------------------------------------

    // Retorna todas as ações disponíveis neste objeto.
    // Cada ação tem seu próprio botão, método e requisitos.
    // @return array de ações disponíveis
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Zf|Interaction")
    TArray<FZfInteractAction> GetInteractActions() const;

    // Retorna o peso manual de prioridade deste objeto (0 a 1).
    // Usado no cálculo de score para definir foco.
    // @return peso de prioridade manual
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Zf|Interaction")
    float GetManualPriorityWeight() const;

    // Retorna a widget que aparece sobre o objeto quando em range.
    // @return classe da widget de indicação
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Zf|Interaction")
    TSubclassOf<UUserWidget> GetIndicatorWidgetClass() const;

    // Retorna a widget que aparece quando o objeto é o foco atual.
    // @return classe da widget de prompt de interação
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Zf|Interaction")
    TSubclassOf<UUserWidget> GetInteractionWidgetClass() const;

    // ----------------------------------------------------------
    // CALLBACKS — chamados pelo InteractionComponent
    // ----------------------------------------------------------

    // Chamado quando este objeto se torna o foco atual do player.
    // Use para destacar visualmente o objeto.
    // @param Interactor — ator que focou este objeto
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Zf|Interaction")
    void OnInteractionFocused(AActor* Interactor);

    // Chamado quando este objeto perde o foco do player.
    // Use para remover o destaque visual.
    // @param Interactor — ator que perdeu o foco
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Zf|Interaction")
    void OnInteractionUnfocused(AActor* Interactor);

    // Chamado quando o player interage com este objeto.
    // Executado no servidor via RPC.
    // @param Interactor — ator que interagiu
    // @param ActionIndex — índice da ação em GetInteractActions()
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Zf|Interaction")
    void OnInteract(AActor* Interactor, int32 ActionIndex);

    // Chamado quando o player entra no range deste objeto.
    // @param Interactor — ator que entrou no range
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Zf|Interaction")
    void OnInteractorEnterRange(AActor* Interactor);

    // Chamado quando o player sai do range deste objeto.
    // @param Interactor — ator que saiu do range
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Zf|Interaction")
    void OnInteractorExitRange(AActor* Interactor);

    // Chamado quando o Hold é cancelado antes de completar.
    // @param Interactor — ator que cancelou
    // @param ActionIndex — índice da ação cancelada
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Zf|Interaction")
    void OnInteractionHoldCancelled(AActor* Interactor, int32 ActionIndex);

    // Retorna o offset de posição da widget de indicação no espaço local do objeto.
    // @return offset em unidades do mundo
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Zf|Interaction")
    FVector GetIndicatorOffset() const;

    // Retorna o offset de posição da widget de prompt no espaço local do objeto.
    // @return offset em unidades do mundo
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Zf|Interaction")
    FVector GetPromptOffset() const;
};