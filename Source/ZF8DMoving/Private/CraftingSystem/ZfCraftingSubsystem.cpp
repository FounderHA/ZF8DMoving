// Fill out your copyright notice in the Description page of Project Settings.
// ZfCraftingSubsystem.cpp

#include "CraftingSystem/ZfCraftingSubsystem.h"
#include "CraftingSystem/ZfCraftRecipe.h"
#include "Inventory/ZfItemDefinition.h"
#include "Player/ZfPlayerState.h"
#include "Tags/ZfCraftTags.h"
#include "Engine/AssetManager.h"
#include "GameFramework/PlayerState.h"


// ============================================================
// Initialize
// ============================================================
// Carrega o catalogo inteiro no boot. Feito uma unica vez;
// receitas nao sao adicionadas nem removidas em runtime.
// ============================================================

void UZfCraftingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogZfCraft, Log, TEXT("ZfCraftingSubsystem: Initialize — carregando catalogo de receitas..."));

	BuildCatalog();

	UE_LOG(LogZfCraft, Log,
		TEXT("ZfCraftingSubsystem: Catalogo pronto — %d receitas indexadas."),
		RecipeByTag.Num());
}


// ============================================================
// Deinitialize
// ============================================================

void UZfCraftingSubsystem::Deinitialize()
{
	UE_LOG(LogZfCraft, Log, TEXT("ZfCraftingSubsystem: Deinitialize."));

	ClearCatalog();

	Super::Deinitialize();
}


// ============================================================
// BuildCatalog
// ============================================================
// Usa o AssetManager para listar todos os PrimaryAssets do tipo
// ZfCraftRecipe, carregar cada um e indexar.
//
// Carga sincrona aqui — aceitavel porque acontece uma unica vez
// no boot do servidor/cliente. Se crescer muito (~milhares de
// receitas), migrar para LoadPrimaryAssets assincrono + callback.
// ============================================================

void UZfCraftingSubsystem::BuildCatalog()
{
	UAssetManager* AssetMgr = UAssetManager::GetIfInitialized();
	if (!AssetMgr)
	{
		UE_LOG(LogZfCraft, Error,
			TEXT("ZfCraftingSubsystem: AssetManager nao disponivel."));
		return;
	}

	// Lista todos os PrimaryAssetIds do tipo ZfCraftRecipe.
	TArray<FPrimaryAssetId> RecipeIds;
	AssetMgr->GetPrimaryAssetIdList(UZfCraftRecipe::PrimaryAssetType, RecipeIds);

	if (RecipeIds.IsEmpty())
	{
		UE_LOG(LogZfCraft, Warning,
			TEXT("ZfCraftingSubsystem: Nenhuma receita encontrada. "
			     "Verifique PrimaryAssetTypesToScan no DefaultGame.ini."));
		return;
	}

	UE_LOG(LogZfCraft, Log,
		TEXT("ZfCraftingSubsystem: Encontradas %d receitas para carregar."),
		RecipeIds.Num());

	// Carrega e indexa cada uma.
	for (const FPrimaryAssetId& Id : RecipeIds)
	{
		UObject* LoadedAsset = AssetMgr->GetPrimaryAssetObject(Id);

		// Se o asset ainda nao esta carregado, forca carga sincrona.
		if (!LoadedAsset)
		{
			TSharedPtr<FStreamableHandle> Handle = AssetMgr->LoadPrimaryAsset(Id);
			if (Handle.IsValid())
			{
				Handle->WaitUntilComplete();
				LoadedAsset = AssetMgr->GetPrimaryAssetObject(Id);
			}
		}

		UZfCraftRecipe* Recipe = Cast<UZfCraftRecipe>(LoadedAsset);
		if (!Recipe)
		{
			UE_LOG(LogZfCraft, Warning,
				TEXT("ZfCraftingSubsystem: Falha ao carregar receita '%s'."),
				*Id.ToString());
			continue;
		}

		IndexRecipe(Recipe);
	}
}


// ============================================================
// IndexRecipe
// ============================================================
// Adiciona a receita aos 4 indices. Detecta colisao de RecipeTag
// e loga como Error — nao aborta (a segunda receita sobrescreve
// no indice por tag, mas continua indexada nos outros lookups).
// ============================================================

void UZfCraftingSubsystem::IndexRecipe(UZfCraftRecipe* Recipe)
{
	if (!Recipe)
	{
		return;
	}

	if (!Recipe->RecipeTag.IsValid())
	{
		UE_LOG(LogZfCraft, Error,
			TEXT("ZfCraftingSubsystem: Receita '%s' sem RecipeTag — ignorada."),
			*Recipe->GetName());
		return;
	}

	// ─── Indice principal (por tag) ────────────────────────────────────
	if (TObjectPtr<UZfCraftRecipe>* Existing = RecipeByTag.Find(Recipe->RecipeTag))
	{
		UE_LOG(LogZfCraft, Error,
			TEXT("ZfCraftingSubsystem: COLISAO de RecipeTag '%s'. "
			     "Existente: '%s' | Novo: '%s'. Novo assume — verifique no editor!"),
			*Recipe->RecipeTag.ToString(),
			*(*Existing)->GetName(),
			*Recipe->GetName());
	}
	RecipeByTag.Add(Recipe->RecipeTag, Recipe);

	// ─── Indice por categoria ──────────────────────────────────────────
	for (const FGameplayTag& CategoryTag : Recipe->CategoryTags)
	{
		FZfRecipeList& List = RecipesByCategory.FindOrAdd(CategoryTag);
		List.Recipes.AddUnique(Recipe);
	}

	// ─── Indice por output (para "como obtenho este item?") ───────────
	for (const FZfCraftOutput& Output : Recipe->Outputs)
	{
		if (!Output.ItemDefinition.IsNull())
		{
			const FSoftObjectPath Path = Output.ItemDefinition.ToSoftObjectPath();
			TArray<TWeakObjectPtr<UZfCraftRecipe>>& List = RecipesByOutput.FindOrAdd(Path);
			List.AddUnique(Recipe);
		}
	}

	// ─── Indice por ingrediente (para "para que serve este item?") ───
	for (const FZfCraftIngredient& Ingredient : Recipe->Ingredients)
	{
		if (!Ingredient.ItemDefinition.IsNull())
		{
			const FSoftObjectPath Path = Ingredient.ItemDefinition.ToSoftObjectPath();
			TArray<TWeakObjectPtr<UZfCraftRecipe>>& List = RecipesByIngredient.FindOrAdd(Path);
			List.AddUnique(Recipe);
		}
	}

	UE_LOG(LogZfCraft, Verbose,
		TEXT("ZfCraftingSubsystem: Indexada receita '%s' (tag: %s)"),
		*Recipe->GetName(), *Recipe->RecipeTag.ToString());
}


// ============================================================
// ClearCatalog
// ============================================================

void UZfCraftingSubsystem::ClearCatalog()
{
	RecipeByTag.Empty();
	RecipesByCategory.Empty();
	RecipesByOutput.Empty();
	RecipesByIngredient.Empty();
}


// ============================================================
// GetRecipeByTag
// ============================================================

UZfCraftRecipe* UZfCraftingSubsystem::GetRecipeByTag(const FGameplayTag& RecipeTag) const
{
	if (const TObjectPtr<UZfCraftRecipe>* Found = RecipeByTag.Find(RecipeTag))
	{
		return Found->Get();
	}
	return nullptr;
}


// ============================================================
// GetAllRecipes
// ============================================================

TArray<UZfCraftRecipe*> UZfCraftingSubsystem::GetAllRecipes() const
{
	TArray<UZfCraftRecipe*> Result;
	Result.Reserve(RecipeByTag.Num());

	for (const TPair<FGameplayTag, TObjectPtr<UZfCraftRecipe>>& Pair : RecipeByTag)
	{
		if (Pair.Value)
		{
			Result.Add(Pair.Value);
		}
	}

	return Result;
}


// ============================================================
// GetRecipesByCategory
// ============================================================
// Match hierarquico — se o designer passa "Recipe.Category.Weapon",
// captura tambem "Recipe.Category.Weapon.Sword" e filhos.
// ============================================================

TArray<UZfCraftRecipe*> UZfCraftingSubsystem::GetRecipesByCategory(const FGameplayTag& CategoryTag) const
{
	TArray<UZfCraftRecipe*> Result;

	if (!CategoryTag.IsValid())
	{
		return Result;
	}

	for (const TPair<FGameplayTag, FZfRecipeList>& Pair : RecipesByCategory)
	{
		// MatchesTag checa se Pair.Key herda de CategoryTag.
		if (Pair.Key.MatchesTag(CategoryTag))
		{
			for (const TObjectPtr<UZfCraftRecipe>& R : Pair.Value.Recipes)
			{
				if (R)
				{
					Result.AddUnique(R);
				}
			}
		}
	}

	return Result;
}


// ============================================================
// GetRecipesProducingItem
// ============================================================

TArray<UZfCraftRecipe*> UZfCraftingSubsystem::GetRecipesProducingItem(UZfItemDefinition* ItemDefinition) const
{
	TArray<UZfCraftRecipe*> Result;

	if (!ItemDefinition)
	{
		return Result;
	}

	const FSoftObjectPath Path(ItemDefinition);
	if (const TArray<TWeakObjectPtr<UZfCraftRecipe>>* Found = RecipesByOutput.Find(Path))
	{
		Result.Reserve(Found->Num());
		for (const TWeakObjectPtr<UZfCraftRecipe>& Weak : *Found)
		{
			if (UZfCraftRecipe* R = Weak.Get())
			{
				Result.Add(R);
			}
		}
	}

	return Result;
}


// ============================================================
// GetRecipesConsumingItem
// ============================================================

TArray<UZfCraftRecipe*> UZfCraftingSubsystem::GetRecipesConsumingItem(UZfItemDefinition* ItemDefinition) const
{
	TArray<UZfCraftRecipe*> Result;

	if (!ItemDefinition)
	{
		return Result;
	}

	const FSoftObjectPath Path(ItemDefinition);
	if (const TArray<TWeakObjectPtr<UZfCraftRecipe>>* Found = RecipesByIngredient.Find(Path))
	{
		Result.Reserve(Found->Num());
		for (const TWeakObjectPtr<UZfCraftRecipe>& Weak : *Found)
		{
			if (UZfCraftRecipe* R = Weak.Get())
			{
				Result.Add(R);
			}
		}
	}

	return Result;
}


// ============================================================
// GetAvailableRecipesForPlayer
// ============================================================

TArray<UZfCraftRecipe*> UZfCraftingSubsystem::GetAvailableRecipesForPlayer(
	APlayerState* PlayerState,
	const FGameplayTagQuery& CrafterAcceptedQuery,
	bool bIncludeForbidden) const
{
	TArray<UZfCraftRecipe*> Result;

	if (!PlayerState)
	{
		UE_LOG(LogZfCraft, Warning,
			TEXT("ZfCraftingSubsystem::GetAvailableRecipesForPlayer: PlayerState nulo."));
		return Result;
	}

	AZfPlayerState* ZfPS = Cast<AZfPlayerState>(PlayerState);
	if (!ZfPS)
	{
		UE_LOG(LogZfCraft, Warning,
			TEXT("ZfCraftingSubsystem::GetAvailableRecipesForPlayer: "
			     "PlayerState nao e AZfPlayerState."));
		return Result;
	}

	// NOTA: KnownRecipeTags sera adicionado ao AZfPlayerState no proximo passo.
	// Por enquanto, assumimos que sera exposto via getter const:
	//   const FGameplayTagContainer& GetKnownRecipeTags() const;
	const FGameplayTagContainer& KnownTags = ZfPS->GetKnownRecipeTags();

	for (const TPair<FGameplayTag, TObjectPtr<UZfCraftRecipe>>& Pair : RecipeByTag)
	{
		UZfCraftRecipe* Recipe = Pair.Value;
		if (!Recipe)
		{
			continue;
		}

		// ─── Filtra por NPC ────────────────────────────────────────────
		// Query vazia = NPC aceita qualquer coisa (caso raro).
		if (!CrafterAcceptedQuery.IsEmpty() &&
			!CrafterAcceptedQuery.Matches(Recipe->CategoryTags))
		{
			continue;
		}

		// ─── Filtra Forbidden ─────────────────────────────────────────
		if (Recipe->IsForbidden() && !bIncludeForbidden)
		{
			continue;
		}

		// ─── Filtra Hidden ────────────────────────────────────────────
		if (Recipe->IsHidden())
		{
			continue;
		}

		// ─── Filtra por descoberta ────────────────────────────────────
		const bool bKnown = Recipe->bIsKnownByDefault || KnownTags.HasTag(Recipe->RecipeTag);
		if (!bKnown)
		{
			continue;
		}

		Result.Add(Recipe);
	}

	return Result;
}


// ============================================================
// LogCatalog — util para debug
// ============================================================

void UZfCraftingSubsystem::LogCatalog() const
{
	UE_LOG(LogZfCraft, Log, TEXT("======== CATALOGO DE RECEITAS ========"));
	UE_LOG(LogZfCraft, Log, TEXT("Total: %d receitas"), RecipeByTag.Num());

	for (const TPair<FGameplayTag, TObjectPtr<UZfCraftRecipe>>& Pair : RecipeByTag)
	{
		if (Pair.Value)
		{
			UE_LOG(LogZfCraft, Log,
				TEXT("  [%s] %s — %d ingredientes, %d outputs, KnownByDefault=%s"),
				*Pair.Key.ToString(),
				*Pair.Value->DisplayName.ToString(),
				Pair.Value->Ingredients.Num(),
				Pair.Value->Outputs.Num(),
				Pair.Value->bIsKnownByDefault ? TEXT("SIM") : TEXT("NAO"));
		}
	}

	UE_LOG(LogZfCraft, Log, TEXT("======================================"));
}