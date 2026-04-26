// Copyright ZfGame Studio. All Rights Reserved.
// ZfRefineryData.h
// Primary Data Asset que configura uma máquina refinadora.
//
// CONCEITO:
// Cada tipo de máquina (Fornalha, Serraria, etc.) tem sua própria
// DA herdando desta classe. Ela define a identidade visual da máquina,
// capacidades dos slots, filtros de itens aceitos e receitas disponíveis.
//
// SLOTS:
// - Input:     onde o player deposita os itens a refinar
// - Output:    onde saem os itens refinados (só remoção pelo player)
// - Catalyst:  onde o player insere itens que aceleram o refino
//
// Cada slot tem capacidade configurável e um FGameplayTagQuery para
// filtrar quais itens podem ser inseridos. A query é avaliada contra
// o ItemTags do ItemDefinition do item.
//
// RECEITAS:
// Lista de receitas que esta máquina pode executar.
// O ZfRefineryComponent usa esta lista para descobrir qual receita
// satisfaz os itens atuais no slot de input.
//
// COMO CRIAR UMA MÁQUINA NO EDITOR:
// 1. Content Browser → Add → Miscellaneous → Data Asset
// 2. Selecione UZfRefineryData como classe
// 3. Configure DisplayName, capacidades dos slots e queries de filtro
// 4. Adicione as receitas disponíveis em AvailableRecipes
// 5. Atribua esta DA ao ZfRefineryComponent do ator no mundo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Systems/RefinerySystem/ZfRefineryTypes.h"
#include "ZfRefineryData.generated.h"

class UZfRefineryRecipe;
class UTexture2D;


// ============================================================
// UZfRefineryData
// ============================================================

UCLASS(BlueprintType, Blueprintable, Const)
class ZF8DMOVING_API UZfRefineryData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	// ============================================================
	// IDENTIFICAÇÃO
	// ============================================================

	// Nome legível exibido na UI ao interagir com a máquina.
	// Ex: "Fornalha", "Serraria", "Bancada de Joalheria"
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refinery|Identity")
	FText DisplayName = FText::FromString(TEXT("None"));

	// Descrição curta exibida na UI — tooltip da máquina.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refinery|Identity", meta = (MultiLine = true))
	FText Description;

	// Ícone da máquina exibido na UI.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refinery|Identity")
	TSoftObjectPtr<UTexture2D> Icon;

	// ============================================================
	// SLOT — INPUT
	// ============================================================

	// Número máximo de stacks distintos que o slot de input suporta.
	// Ex: 5 = pode ter até 5 tipos diferentes de itens no input.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refinery|Slots|Input", meta = (ClampMin = "1", UIMin = "1"))
	int32 InputSlotCapacity = 5;

	// Query avaliada contra o ItemTags do item ao tentar inserir no input.
	// Apenas itens cujos ItemTags satisfaçam esta query são aceitos.
	//
	// Exemplos:
	// - "HasTag(ItemType.CraftMaterial.RawOre)" → aceita qualquer minério bruto
	// - "HasAnyTags(ItemType.CraftMaterial.RawOre, ItemType.CraftMaterial.Jewel.Ruby)"
	//   → aceita minério bruto OU aquela joia específica
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refinery|Slots|Input")
	FGameplayTagQuery InputSlotQuery;

	// ============================================================
	// SLOT — OUTPUT
	// ============================================================

	// Número máximo de stacks distintos que o slot de output suporta.
	// Quando cheio, o refinador pausa e aguarda espaço.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refinery|Slots|Output", meta = (ClampMin = "1", UIMin = "1"))
	int32 OutputSlotCapacity = 5;

	// ============================================================
	// SLOT — CATALYST
	// ============================================================

	// Número de slots de catalisador disponíveis nesta máquina.
	// O consumo sempre começa pelo slot de menor índice (índice 0).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refinery|Slots|Catalyst", meta = (ClampMin = "1", UIMin = "1"))
	int32 CatalystSlotCount = 1;

	// Query avaliada contra o ItemTags do item ao tentar inserir no slot de catalisador.
	// Permite restringir quais catalisadores esta máquina aceita.
	//
	// Exemplos:
	// - "HasTag(ItemType.Catalyst.Ore)"  → aceita só catalisadores de minério
	// - "HasTag(ItemType.Catalyst.Wood)" → aceita só catalisadores de madeira
	// - "HasTag(ItemType.Catalyst)"      → aceita qualquer catalisador
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refinery|Slots|Catalyst")
	FGameplayTagQuery CatalystSlotQuery;

	// ============================================================
	// RECEITAS
	// ============================================================

	// Receitas que esta máquina pode executar.
	// O ZfRefineryComponent testa estas receitas para encontrar
	// a que pode ser satisfeita com os itens atuais no input,
	// respeitando a fila manual do player ou o sistema automático.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refinery|Recipes")
	TArray<TSoftObjectPtr<UZfRefineryRecipe>> AvailableRecipes;

	// ============================================================
	// ASSETMANAGER
	// ============================================================

	static const FPrimaryAssetType PrimaryAssetType;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// ============================================================
	// HELPERS
	// ============================================================

	// Verifica se um item pode ser inserido no slot de input.
	// Avalia o InputSlotQuery contra as tags fornecidas.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|RefineryData")
	bool IsItemAllowedInInput(const FGameplayTagContainer& ItemTags) const;

	// Verifica se um item pode ser inserido em um slot de catalisador.
	// Avalia o CatalystSlotQuery contra as tags fornecidas.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|RefineryData")
	bool IsItemAllowedAsCatalyst(const FGameplayTagContainer& ItemTags) const;

	// ============================================================
	// VALIDAÇÃO NO EDITOR
	// ============================================================

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	// ============================================================
	// DEBUG
	// ============================================================

	UFUNCTION(BlueprintCallable, Category = "Zf|RefineryData|Debug")
	FString GetDebugString() const;
};