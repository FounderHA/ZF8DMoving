// Copyright ZfGame Studio. All Rights Reserved.
// ZfGatherResourceData.h
// Primary Data Asset que define todas as informações de um recurso coletável.
//
// SISTEMA DE DANO:
// O recurso tem ResourceHP — o loop de coleta continua até que o
// dano acumulado dos golpes zere o HP. O número de golpes é dinâmico:
// uma ferramenta mais forte termina em menos golpes,
// acertos Perfeitos causam mais dano e também terminam mais rápido.
//
// HERANÇA:
// Classe base para todos os recursos. Recursos com comportamento
// especial herdam desta e adicionam apenas os dados extras.
// Ex: UZfGatherResourceData_Fish adiciona a fase de espera.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GatheringSystem/ZfGatheringTypes.h"
#include "ZfGatheringResourceData.generated.h"

// ============================================================
// UZfGatherResourceData
// ============================================================

UCLASS(BlueprintType, Blueprintable)
class ZF8DMOVING_API UZfGatheringResourceData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:

    // ----------------------------------------------------------
    // IDENTIFICAÇÃO
    // ----------------------------------------------------------

    // Nome legível exibido na UI ao mirar no recurso.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Identity")
    FText ResourceName = FText::FromString(TEXT("None"));
    
    // Tag que identifica o tipo deste recurso.
    // Ex: "Resource.Ore.Iron", "Resource.Wood.Oak"
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Identity", meta = (GameplayTagFilter = "Resource"))
    FGameplayTag ResourceTag;

    // ----------------------------------------------------------
    // HP DO RECURSO
    // Define a "durabilidade" do nó — o total de dano necessário
    // para esgotá-lo. O loop termina quando HP acumulado chega a 0.
    //
    // EXEMPLO:
    // ResourceHP = 100, BaseDamage ferramenta = 20
    // Acerto Bom (1.1x) = 22 de dano → ~5 golpes para esgotar
    // Acerto Perfeito (1.4x) = 28 de dano → ~4 golpes para esgotar
    // ----------------------------------------------------------

    // HP total do recurso — quantidade de dano para esgotá-lo.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Identity", meta = (ClampMin = "1.0", UIMin = "1.0"))
    float ResourceHP = 100.0f;

    // ----------------------------------------------------------
    // QTE
    // ----------------------------------------------------------

    // Tempo em segundos para a agulha completar uma volta (360°).
    // Ao completar a volta sem clique, o QTE falha como Missed.
    // Menor = agulha mais rápida = mais difícil.
    // Ex: 2.0 = fácil | 1.5 = normal | 1.0 = difícil
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Identity", meta = (ClampMin = "0.5", UIMin = "0.5"))
    float NeedleRotationTime = 1.5f;

    // ----------------------------------------------------------
    // FERRAMENTAS ACEITAS
    // Cada entrada define quais ferramentas coletam este recurso
    // e com qual eficiência (DamageMultiplier e ZoneSize do QTE).
    // Ferramentas com tag ausente não podem coletar o recurso.
    // ----------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Tools")
    TArray<FZfGatherToolEntry> AllowedTools;

    // ----------------------------------------------------------
    // LOOT TABLE
    // Avaliada ao fim da coleta usando o score acumulado.
    // Score alto desbloqueia entradas com ScoreMinimum maior.
    // ----------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Loot")
    TArray<FZfGatherLootEntry> LootTable;

    // ----------------------------------------------------------
    // RESPAWN
    // ----------------------------------------------------------

    // Se verdadeiro, o nó reaparece após RespawnTime segundos.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Respawn")
    bool bCanRespawn = true;

    // Tempo em segundos até o nó reaparecer. Ignorado se bCanRespawn = false.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Respawn",
        meta = (ClampMin = "1.0", UIMin = "1.0", EditCondition = "bCanRespawn"))
    float RespawnTime = 300.0f;

    // Offset em unidades de mundo para posicionar a widget de HP acima do recurso.
    // Ajuste conforme o tamanho visual de cada recurso.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|UI")
    FVector HPWidgetOffset = FVector(0.f, 0.f, 100.f);
    
    // ----------------------------------------------------------
    // FUNÇÕES DE ACESSO
    // ----------------------------------------------------------

    // Busca a entrada de ferramenta pelo ToolTag.
    // Retorna nullptr se a ferramenta não for aceita.
    // C++ only — UHT não permite UFUNCTION retornando ponteiro para USTRUCT.
    const FZfGatherToolEntry* FindToolEntry(const FGameplayTag& ToolTag) const;

    // Verifica se uma ferramenta pode coletar este recurso.
    UFUNCTION(BlueprintCallable, Category = "Zf|GatherResource")
    bool IsToolAllowed(const FGameplayTag& ToolTag) const;

    // ----------------------------------------------------------
    // ASSETMANAGER
    // ----------------------------------------------------------

    static const FPrimaryAssetType PrimaryAssetType;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

    // ----------------------------------------------------------
    // DEBUG
    // ----------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Zf|GatherResource|Debug")
    FString GetDebugString() const;
};