#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZfInteractionWidget.generated.h"

class UZfInteractionComponent;

UCLASS()
class ZF8DMOVING_API UZfInteractionWidget : public UUserWidget
{
    GENERATED_BODY()

public:

    // Chamado pelo InteractionComponent ao criar a widget
    UFUNCTION(BlueprintCallable, Category = "Zf|Interaction|Widget")
    void InitializeWidget(AActor* InInteractableActor, UZfInteractionComponent* InInteractionComponent);

    // Retorna o ator interagível dono desta widget
    UFUNCTION(BlueprintPure, Category = "Zf|Interaction|Widget")
    AActor* GetInteractableActor() const { return InteractableActor; }

    // Retorna o progresso do Hold (0 a 1)
    UFUNCTION(BlueprintPure, Category = "Zf|Interaction|Widget")
    float GetHoldProgress() const;

    // Retorna se está em Hold no momento
    UFUNCTION(BlueprintPure, Category = "Zf|Interaction|Widget")
    bool IsHolding() const;

    // Retorna o tempo restante do Hold em segundos
    UFUNCTION(BlueprintPure, Category = "Zf|Interaction|Widget")
    float GetHoldTimeRemaining() const;

    // Retorna a duração total do Hold da ação atual
    UFUNCTION(BlueprintPure, Category = "Zf|Interaction|Widget")
    float GetHoldDuration() const;

    // Retorna as ações disponíveis no objeto
    UFUNCTION(BlueprintPure, Category = "Zf|Interaction|Widget")
    TArray<FZfInteractAction> GetInteractActions() const;

    // Retorna o estado atual da interação
    UFUNCTION(BlueprintPure, Category = "Zf|Interaction|Widget")
    EZfInteractionState GetInteractionState() const;

    // Chamado quando a widget deve aparecer
    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|Interaction|Widget")
    void OnWidgetShow();

    // Chamado quando a widget deve sumir — implemente a animação aqui
    // Chame HideComplete() quando a animação terminar
    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|Interaction|Widget")
    void OnWidgetHide();

    // Chame este ao final da animação de saída no Blueprint
    UFUNCTION(BlueprintCallable, Category = "Zf|Interaction|Widget")
    void HideComplete();
    
protected:

    // Chamado quando a widget é inicializada — override no Blueprint
    UFUNCTION(BlueprintImplementableEvent, Category = "Zf|Interaction|Widget")
    void OnWidgetInitialized();
    
    // Ator interagível dono desta widget
    UPROPERTY(BlueprintReadOnly, Category = "Zf|Interaction|Widget")
    TObjectPtr<AActor> InteractableActor = nullptr;

    // Referência ao componente de interação do player
    UPROPERTY(BlueprintReadOnly, Category = "Zf|Interaction|Widget")
    TObjectPtr<UZfInteractionComponent> InteractionComponent = nullptr;
};