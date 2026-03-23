// Copyright ZfGame Studio. All Rights Reserved.
// ZfFragment_SetPiece.h
// Fragment que define a qual combo set este item pertence.
// O EquipmentComponent monitora quantas peças do mesmo set
// estão equipadas e aplica/remove os GameplayEffects de bônus.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Fragments/ZfItemFragment.h"
#include "Inventory/ZfInventoryTypes.h"
#include "ZfFragment_SetPiece.generated.h"

UCLASS(DisplayName = "Fragment: Set Piece")
class ZF8DMOVING_API UZfFragment_SetPiece : public UZfItemFragment
{
    GENERATED_BODY()

public:

    // Identificador único do set ao qual este item pertence.
    // Todos os itens do mesmo set devem ter o mesmo SetIdentifier.
    // Ex: "Set.DragonSlayer", "Set.ShadowRogue"
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|SetPiece")
    FGameplayTag SetIdentifier;

    // Nome exibido do set na UI do inventário.
    // Ex: "Conjunto do Mata-Dragões"
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|SetPiece")
    FText SetDisplayName;

    // Lista de bônus do set ordenada por quantidade de peças necessárias.
    // Cada entrada é um GameplayEffect aplicado ao atingir X peças equipadas.
    // Ex: [{2 peças, GE_SetBonus_2}, {4 peças, GE_SetBonus_4}]
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|SetPiece")
    TArray<FZfSetBonusEntry> SetBonuses;

    // Quantidade total de peças que compõem o set completo.
    // Usado na UI para mostrar "X/TotalPieces equipadas".
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|SetPiece",
        meta = (ClampMin = "1"))
    int32 TotalSetPieces = 5;

    // Retorna os bônus ativos para uma quantidade de peças equipadas.
    // Ex: Com 3 peças equipadas, retorna todos os bônus com
    // RequiredPieceCount <= 3.
    UFUNCTION(BlueprintCallable, Category = "Zf|Fragment|SetPiece")
    TArray<FZfSetBonusEntry> GetActiveBonusesForPieceCount(int32 EquippedPieceCount) const
    {
        TArray<FZfSetBonusEntry> ActiveBonuses;
        for (const FZfSetBonusEntry& Bonus : SetBonuses)
        {
            if (EquippedPieceCount >= Bonus.RequiredPieceCount)
            {
                ActiveBonuses.Add(Bonus);
            }
        }
        return ActiveBonuses;
    }

// ----------------------------------------------------------
// DEBUG
// ----------------------------------------------------------	
    
    virtual FString GetDebugString() const override
    {
        return FString::Printf(
            TEXT("[Fragment_SetPiece] Set: %s | Bonuses: %d | TotalPieces: %d"),
            *SetIdentifier.ToString(),
            SetBonuses.Num(),
            TotalSetPieces);
    }
};