// Copyright ZfGame Studio. All Rights Reserved.
// ZfInteractionTypes.h
// Tipos centrais do sistema de interação.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ZfInteractionTypes.generated.h"

// -----------------------------------------------------------
// EZfInteractionMethod
// Define como o jogador interage com o objeto.
// -----------------------------------------------------------
UENUM(BlueprintType)
enum class EZfInteractionMethod : uint8
{
    Press   UMETA(DisplayName = "Press"),   // Clique simples
    Hold    UMETA(DisplayName = "Hold"),    // Segurar botão
};

// -----------------------------------------------------------
// EZfInteractionState
// Estado atual da interação com um objeto.
// -----------------------------------------------------------
UENUM(BlueprintType)
enum class EZfInteractionState : uint8
{
    Available   UMETA(DisplayName = "Available"),   // Pode interagir
    Blocked     UMETA(DisplayName = "Blocked"),     // Visível mas bloqueado
    Cooldown    UMETA(DisplayName = "Cooldown"),    // Em cooldown
    Unavailable UMETA(DisplayName = "Unavailable"), // Não pode interagir
};

// -----------------------------------------------------------
// FZfInteractionRequirements
// Requisitos para que a interação seja possível.
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfInteractionRequirements
{
    GENERATED_BODY()

    // Tags que o interator precisa ter para interagir
    // Ex: Tag.Quest.HasKey, Tag.Class.Rogue
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Requirements")
    FGameplayTagContainer RequiredTags;

    // Item necessário no inventário para interagir
    // Ex: Chave para abrir porta
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Requirements")
    FGameplayTag RequiredItemTag;

    // Nível mínimo para interagir (0 = sem requisito)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Requirements", meta = (ClampMin = "0"))
    int32 MinLevel = 0;
};

// -----------------------------------------------------------
// FZfInteractAction
// Uma ação disponível em um objeto interagível.
// Objetos podem ter múltiplas ações (ex: "Abrir", "Inspecionar").
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfInteractAction
{
    GENERATED_BODY()

    // Nome da ação exibido na UI
    // Ex: "Abrir", "Inspecionar", "Pegar"
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Action")
    FText ActionName;

    // Botão que dispara esta ação
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Action")
    FKey InteractionKey;

    // Método de interação desta ação
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Action")
    EZfInteractionMethod InteractionMethod = EZfInteractionMethod::Press;

    // Duração necessária para Hold (ignorado se Press)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Action", meta = (ClampMin = "0.1", UIMIN = "0.1", EditCondition = "InteractionMethod == EZfInteractionMethod::Hold"))
    float HoldDuration = 1.5f;

    // Cooldown após interagir (0 = sem cooldown)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Action", meta = (ClampMin = "0.0", UIMin = "0.0"))
    float Cooldown = 0.f;

    // Requisitos específicos desta ação
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Action")
    FZfInteractionRequirements Requirements;

    // Ícone exibido na UI para esta ação
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Action")
    TSoftObjectPtr<UTexture2D> ActionIcon;
};

// -----------------------------------------------------------
// FZfInteractionScore
// Score calculado para prioridade de foco entre 0 e 1.
// -----------------------------------------------------------
USTRUCT(BlueprintType)
struct ZF8DMOVING_API FZfInteractionScore
{
    GENERATED_BODY()

    // Score de distância — mais perto = mais alto (0 a 1)
    UPROPERTY(BlueprintReadOnly, Category = "Interaction|Score")
    float DistanceScore = 0.f;

    // Score de alinhamento com câmera — dot product normalizado (0 a 1)
    UPROPERTY(BlueprintReadOnly, Category = "Interaction|Score")
    float CameraAlignmentScore = 0.f;

    // Peso extra manual definido no próprio objeto (0 a 1)
    UPROPERTY(BlueprintReadOnly, meta =(ClampMin = "0", UIMin = "0", ClampMax = "1", UIMax = "1"), Category = "Interaction|Score")
    float ManualPriorityScore = 0.f;

    // Penalidade se bloqueado (reduz o score final)
    UPROPERTY(BlueprintReadOnly, Category = "Interaction|Score")
    float BlockedPenalty = 0.f;

    // Score final calculado — soma ponderada de todos os scores
    UPROPERTY(BlueprintReadOnly, Category = "Interaction|Score")
    float FinalScore = 0.f;
};