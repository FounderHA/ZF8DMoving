// Copyright ZfGame Studio. All Rights Reserved.
// ZfItemDefinition.cpp

#include "Inventory/ZfItemDefinition.h"
#include "Inventory/ZfInventoryTypes.h"
#include "Inventory/Fragments/ZfFragment_ItemRarity.h"
#include "Inventory/Fragments/ZfFragment_ItemUnique.h"
#include "Inventory/Fragments/ZfFragment_Modifiers.h"
#include "Misc/DataValidation.h"

// ============================================================
// Constructor
// ============================================================

UZfItemDefinition::UZfItemDefinition()
{
    // Nenhuma inicialização especial necessária.
    // Os valores padrão são definidos nas UPROPERTY acima.
}

// Define o tipo primário — deve bater exatamente com o configurado
// em Project Settings → Asset Manager → Primary Asset Types to Scan
const FPrimaryAssetType UZfItemDefinition::PrimaryAssetType("ZfItemDefinition");

// ============================================================
// IsDataValid — Validação no Editor
// ============================================================

#if WITH_EDITOR
EDataValidationResult UZfItemDefinition::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    // Valida que o item tem nome configurado
    if (ItemName.IsEmpty())
    {
        Context.AddWarning(FText::FromString(TEXT("ZfItemDefinition: ItemName não pode estar vazio.")));
        Result = EDataValidationResult::Invalid;
    }

    // Valida que o item tem ao menos uma tag de tipo
    if (ItemTags.IsEmpty())
    {
        Context.AddWarning(FText::FromString(TEXT("ZfItemDefinition: ItemTags não pode estar vazio." "Adicione ao menos uma tag de tipo de item.")));
        Result = EDataValidationResult::Invalid;
    }

    // Valida consistência de item Único
    if (const UZfFragment_ItemUnique* UniqueFragment = FindFragment<UZfFragment_ItemUnique>())
    {
        // Item Único deve ter ao menos um modifier fixo
        if (UniqueFragment->UniqueModifiers.IsEmpty())
        {
            Context.AddWarning(FText::FromString(TEXT("ZfItemDefinition: Item Único sem UniqueModifiers configurados.")));
        }
    }

    // Valida que não há fragments duplicados do mesmo tipo
    TSet<UClass*> FragmentClasses;
    for (const TObjectPtr<UZfItemFragment>& Fragment : Fragments)
    {
        if (!Fragment)
        {
            Context.AddWarning(FText::FromString(
                TEXT("ZfItemDefinition: Array Fragments contém entradas nulas.")));
            continue;
        }

        UClass* FragmentClass = Fragment->GetClass();
        if (FragmentClasses.Contains(FragmentClass))
        {
            Context.AddError(FText::FromString(FString::Printf(
                TEXT("ZfItemDefinition: Fragment duplicado encontrado: %s. " "Cada tipo de fragment deve aparecer apenas uma vez."),
                *FragmentClass->GetName())));
            Result = EDataValidationResult::Invalid;
        }
        FragmentClasses.Add(FragmentClass);
    }

    if (HasFragment<UZfFragment_Modifiers>() && HasFragment<UZfFragment_ItemUnique>())
    {
        Context.AddError(FText::FromString(FString::Printf(TEXT("ZfItemDefinition: Não é Possível ter os Fragmentos de Modifiers e ItemUnique em conjunto"))));
    }
    if (HasFragment<UZfFragment_ItemRarity>() && HasFragment<UZfFragment_ItemUnique>())
    {
        Context.AddError(FText::FromString(FString::Printf(TEXT("ZfItemDefinition: Não é Possível ter os Fragmentos de ItemRarity e ItemUnique em conjunto"))));
    }
    return Result;
}
#endif

// ============================================================
// GetDebugString
// ============================================================

FString UZfItemDefinition::GetDebugString() const
{
    // Monta string legível com os dados principais do item
    FString DebugInfo = FString::Printf(
        TEXT("=== ZfItemDefinition Debug ===\n")
        TEXT("Name: %s\n")
        TEXT("Tags: %s\n")
        TEXT("Fragments (%d):\n"),
        *ItemName.ToString(),
        *ItemTags.ToString(),
        Fragments.Num());

    // Lista todos os fragments e seus dados de debug
    for (int32 Index = 0; Index < Fragments.Num(); Index++)
    {
        if (Fragments[Index])
        {
            DebugInfo += FString::Printf(
                TEXT("  [%d] %s\n"),
                Index,
                *Fragments[Index]->GetDebugString());
        }
    }

    // Lista modifiers únicos se aplicável
    if (const UZfFragment_ItemUnique* UniqueItem = FindFragment<UZfFragment_ItemUnique>())
    {
        if (UniqueItem->UniqueModifiers.Num() > 0)
        {
            DebugInfo += FString::Printf(TEXT("UniqueModifiers (%d):\n"), UniqueItem->UniqueModifiers.Num());

            for (const FDataTableRowHandle& Handle : UniqueItem->UniqueModifiers)
            {
                if (GEngine)
                {
                    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
                        FString::Printf(TEXT("Item Equipado: %s"), *Handle.RowName.ToString()));
                }
            }
        }
    }
    return DebugInfo;
}