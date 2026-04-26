// Copyright ZfGame Studio. All Rights Reserved.
// ZfRefineryRecipe.h
// Primary Data Asset que representa uma receita de refinaria.
//
// CONCEITO:
// Cada receita é um asset individual criado no Content Browser.
// O designer configura os ingredientes, outputs e tempo de refino.
// O ZfRefineryComponent descobre as receitas disponíveis pela lista
// configurada diretamente na DA da máquina (AvailableRecipes).
//
// UMA RECEITA TEM:
// - Identificação: nome, descrição, ícone
// - Ingredientes: quais itens e quantidades mínimas são consumidos
// - Outputs: o que é produzido (quantidade base + chance de bônus)
// - Tempo: duração base do ciclo de refino em segundos
// - Prioridade: usada pelo sistema automático para escolher receitas
//
// PRIORIDADE DE EXECUÇÃO:
// O ZfRefineryComponent testa receitas na seguinte ordem:
// 1. Fila manual configurada pelo player (se houver)
// 2. Se nenhuma da fila manual for satisfeita, usa o sistema automático:
//    a. Maior Priority vence
//    b. Empate → receita com mais ingredientes distintos vence
//
// COMO CRIAR UMA NOVA RECEITA NO EDITOR:
// 1. Content Browser → Add → Miscellaneous → Data Asset
// 2. Selecione UZfRefineryRecipe como classe
// 3. Configure DisplayName, Ingredients e Outputs
// 4. Ajuste BaseCraftTime e Priority conforme necessário
// 5. Adicione esta receita ao array AvailableRecipes da DA da máquina

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Systems/RefinerySystem/ZfRefineryTypes.h"
#include "ZfRefineryRecipe.generated.h"

class UTexture2D;


// ============================================================
// UZfRefineryRecipe
// ============================================================

UCLASS(BlueprintType, Blueprintable, Const)
class ZF8DMOVING_API UZfRefineryRecipe : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UZfRefineryRecipe();

	// ============================================================
	// IDENTIFICAÇÃO
	// ============================================================

	// Nome legível exibido na UI da refinaria.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe|Identity")
	FText DisplayName = FText::FromString(TEXT("None"));

	// Descrição curta — tooltip da receita.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe|Identity", meta = (MultiLine = true))
	FText Description;

	// Ícone exibido na UI — pode ser o ícone do output principal.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe|Identity")
	TSoftObjectPtr<UTexture2D> Icon;

	// ============================================================
	// INGREDIENTES E OUTPUTS
	// ============================================================

	// Itens consumidos do slot de input a cada ciclo de refino.
	// Todos os ingredientes devem estar presentes nas quantidades
	// mínimas para o ciclo poder iniciar.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe|Ingredients")
	TArray<FZfRefineryIngredient> Ingredients;

	// Itens produzidos ao completar o ciclo com sucesso.
	// Depositados no slot de output ao final do ciclo.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe|Outputs")
	TArray<FZfRefineryOutput> Outputs;

	// ============================================================
	// TEMPO
	// ============================================================

	// Duração base de um ciclo de refino em segundos.
	// O tempo efetivo é reduzido pelo SpeedMultiplier do catalisador ativo:
	// TempoEfetivo = BaseCraftTime / SpeedMultiplier
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe|Time", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float BaseCraftTime = 10.0f;

	// ============================================================
	// PRIORIDADE
	// ============================================================

	// Prioridade usada pelo sistema automático de seleção de receita.
	// Maior valor = maior prioridade.
	// Em caso de empate, a receita com mais ingredientes distintos vence.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe|Priority", meta = (ClampMin = "0", UIMin = "0"))
	int32 Priority = 0;

	// ============================================================
	// OUTPUT BÔNUS
	// ============================================================

	// Chance de produzir itens extras ao completar o ciclo.
	// 0.0 = nunca ocorre | 1.0 = sempre ocorre
	// O roll acontece uma vez por ciclo completo, no momento
	// de depositar o output.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe|Bonus", meta = (ClampMin = "0.0", ClampMax = "0.0"))
	float BonusOutputChance = 0.0f;

	// Quantidade extra produzida quando o roll de bônus é bem-sucedido.
	// Aplicado ao primeiro output da lista (output principal).
	// Ignorado se BonusOutputChance == 0.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe|Bonus", meta = (ClampMin = "1", UIMin = "1", EditCondition = "BonusOutputChance > 0.0f"))
	int32 BonusOutputAmount = 1;

	// ============================================================
	// ASSETMANAGER — UPrimaryDataAsset overrides
	// ============================================================

	static const FPrimaryAssetType PrimaryAssetType;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// ============================================================
	// HELPERS
	// ============================================================

	// Retorna true se todos os ingredientes estão configurados corretamente.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|RefineryRecipe")
	bool HasValidIngredients() const;

	// Retorna true se todos os outputs estão configurados corretamente.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|RefineryRecipe")
	bool HasValidOutputs() const;

	// Retorna o número de ingredientes distintos — usado no desempate de prioridade.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zf|RefineryRecipe")
	int32 GetIngredientCount() const { return Ingredients.Num(); }

	// ============================================================
	// VALIDAÇÃO NO EDITOR
	// ============================================================

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};