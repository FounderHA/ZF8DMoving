// Fill out your copyright notice in the Description page of Project Settings.
// ZfCraftingComponent.cpp

#include "CraftingSystem/ZfCraftingComponent.h"
#include "CraftingSystem/ZfCraftRecipe.h"
#include "CraftingSystem/ZfCraftingSubsystem.h"
#include "CraftingSystem/Requirements/ZfCraftRequirement.h"
#include "Player/ZfPlayerState.h"
#include "Inventory/ZfInventoryComponent.h"
#include "Inventory/ZfItemDefinition.h"
#include "Inventory/ZfItemInstance.h"
#include "Inventory/Fragments/ZfFragment_Stackable.h"
#include "Inventory/ZfInventoryTypes.h"
#include "Tags/ZfCraftTags.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"


// ============================================================
// Constructor / BeginPlay
// ============================================================

UZfCraftingComponent::UZfCraftingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false); // Estado ephemeral por-craft, sem estado replicavel.
}

void UZfCraftingComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		UE_LOG(LogZfCraft, Log,
			TEXT("CraftingComponent: NPC '%s' pronto. Tags: [%s] | MaxRange: %.0f"),
			*GetOwner()->GetName(),
			*CrafterTags.ToStringSimple(),
			MaxCraftRange);
	}
}


// ============================================================
// Server_RequestCraft — ponto de entrada do cliente
// ============================================================

void UZfCraftingComponent::RequestCraft(APlayerController* PC, FGameplayTag RecipeTag)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		UE_LOG(LogZfCraft, Warning,
			TEXT("CraftingComponent::RequestCraft chamado sem authority — ignorando."));
		return;
	}

	// ─── Valida parametros basicos ────────────────────────────────────
	if (!PC || !PC->PlayerState)
	{
		UE_LOG(LogZfCraft, Warning,
			TEXT("CraftingComponent: Server_RequestCraft recebido com PC invalido."));
		return;
	}

	// ─── Resolve a receita no catalogo ────────────────────────────────
	UZfCraftingSubsystem* Subsystem = GetCraftingSubsystem();
	if (!Subsystem)
	{
		UE_LOG(LogZfCraft, Error,
			TEXT("CraftingComponent: CraftingSubsystem indisponivel."));
		return;
	}

	UZfCraftRecipe* Recipe = Subsystem->GetRecipeByTag(RecipeTag);
	if (!Recipe)
	{
		UE_LOG(LogZfCraft, Warning,
			TEXT("CraftingComponent: Receita '%s' nao encontrada no catalogo."),
			*RecipeTag.ToString());

		FZfCraftResult Result;
		Result.ResultCode    = EZfCraftResult::Failed_InvalidRecipe;
		Result.FailureReason = NSLOCTEXT("ZfCraft", "InvalidRecipe", "Receita desconhecida.");
		BroadcastResultToInstigator(PC, Result);
		return;
	}

	// ─── Pipeline ─────────────────────────────────────────────────────
	FZfCraftResult Result = ExecuteCraftAttempt(PC, Recipe);

	// ─── Notifica o cliente e listeners server-side ───────────────────
	BroadcastResultToInstigator(PC, Result);

	UE_LOG(LogZfCraft, Log,
		TEXT("CraftingComponent: Receita '%s' | Player '%s' | Resultado: %s"),
		*Recipe->RecipeTag.ToString(),
		*PC->PlayerState->GetPlayerName(),
		*UEnum::GetValueAsString(Result.ResultCode));
}


// ============================================================
// ExecuteCraftAttempt — pipeline completo
// ============================================================

FZfCraftResult UZfCraftingComponent::ExecuteCraftAttempt(APlayerController* PC, UZfCraftRecipe* Recipe)
{
	FZfCraftResult Result;
	Result.Recipe = Recipe;

	// ─── Validacoes rapidas primeiro ──────────────────────────────────
	if (!Validate_NotForbidden(Recipe, Result))           return Result;
	if (!Validate_Range(PC, Result))                      return Result;
	if (!Validate_NPCAcceptsRecipe(Recipe, Result))       return Result;
	if (!Validate_PlayerKnowsRecipe(PC, Recipe, Result))  return Result;
	if (!Validate_WorldStateQuery(PC, Recipe, Result))    return Result;

	// ─── Requisitos polimorficos ──────────────────────────────────────
	const FZfCraftContext Context = BuildContext(PC, Recipe);
	if (!Validate_ExtraRequirements(Context, Result))     return Result;

	// ─── Inventario ───────────────────────────────────────────────────
	UZfInventoryComponent* Inventory = GetInstigatorInventory(PC);
	if (!Inventory)
	{
		Result.ResultCode    = EZfCraftResult::Failed_InvalidRecipe;
		Result.FailureReason = NSLOCTEXT("ZfCraft", "NoInventory",
			"Inventario do jogador nao encontrado.");
		return Result;
	}

	if (!Validate_Ingredients(Inventory, Recipe, Result))    return Result;
	if (!Validate_InventorySpace(Inventory, Recipe, Result)) return Result;

	// ─── Roll de falha (unica aleatoriedade do sistema) ──────────────
	if (Recipe->CanFail())
	{
		const float Roll = FMath::FRand();
		if (Roll < Recipe->FailureChance)
		{
			UE_LOG(LogZfCraft, Log,
				TEXT("CraftingComponent: Receita '%s' falhou no roll (%.3f < %.3f)."),
				*Recipe->RecipeTag.ToString(), Roll, Recipe->FailureChance);

			if (Recipe->FailurePolicy != EZfFailureIngredientPolicy::ConsumeNone)
			{
				ConsumeIngredients(Inventory, Recipe, /*bFailurePath=*/true);
			}

			FireCueOnCrafter(ZfCraftTags::Cue::Crafting_GameplayCue_Craft_Failure);

			Result.ResultCode    = EZfCraftResult::Failed_ChanceRoll;
			Result.FailureReason = NSLOCTEXT("ZfCraft", "FailureRoll", "O craft falhou pela sorte.");
			return Result;
		}
	}

	// ─── Execucao real ────────────────────────────────────────────────
	ConsumeIngredients(Inventory, Recipe, /*bFailurePath=*/false);
	SpawnOutputs(Inventory, Recipe);

	FireCueOnCrafter(ZfCraftTags::Cue::Crafting_GameplayCue_Craft_Success);

	Result.ResultCode = EZfCraftResult::Success;
	return Result;
}


// ============================================================
// BroadcastResultToInstigator
// ============================================================
// O componente vive num NPC sem NetConnection para um player
// especifico, entao Client RPC direto daqui nao funciona.
// Roteamos via AZfPlayerState::Client_ReceiveCraftResult.
// ============================================================

void UZfCraftingComponent::BroadcastResultToInstigator(APlayerController* PC, const FZfCraftResult& Result)
{
	// Broadcast local no servidor — para listeners server-side.
	OnCraftAttemptedOnServer.Broadcast(Result);

	// Roteia pro cliente via PlayerState.
	if (!PC) return;

	AZfPlayerState* ZfPS = PC->GetPlayerState<AZfPlayerState>();
	if (!ZfPS)
	{
		UE_LOG(LogZfCraft, Warning,
			TEXT("CraftingComponent: PlayerState nao encontrado ao rotear resultado."));
		return;
	}

	ZfPS->Client_ReceiveCraftResult(Result);
}


// ============================================================
// CanPlayerCraft — query otimista (cliente ou servidor)
// ============================================================

bool UZfCraftingComponent::CanPlayerCraft(APlayerController* PC, UZfCraftRecipe* Recipe, FText& OutReason) const
{
	if (!PC || !Recipe)
	{
		OutReason = NSLOCTEXT("ZfCraft", "InvalidParams", "Parametros invalidos.");
		return false;
	}

	FZfCraftResult Dummy;
	Dummy.Recipe = Recipe;

	if (!Validate_NotForbidden(Recipe, Dummy) ||
		!Validate_NPCAcceptsRecipe(Recipe, Dummy) ||
		!Validate_PlayerKnowsRecipe(PC, Recipe, Dummy) ||
		!Validate_WorldStateQuery(PC, Recipe, Dummy))
	{
		OutReason = Dummy.FailureReason;
		return false;
	}

	const FZfCraftContext Context = BuildContext(PC, Recipe);
	if (!Validate_ExtraRequirements(Context, Dummy))
	{
		OutReason = Dummy.FailureReason;
		return false;
	}

	return true;
}


// ============================================================
// VALIDADORES
// ============================================================

bool UZfCraftingComponent::Validate_NotForbidden(UZfCraftRecipe* Recipe, FZfCraftResult& OutResult) const
{
	if (Recipe->IsForbidden())
	{
		OutResult.ResultCode    = EZfCraftResult::Failed_Forbidden;
		OutResult.FailureReason = NSLOCTEXT("ZfCraft", "Forbidden", "Esta receita nao pode ser craftada.");
		return false;
	}
	return true;
}

bool UZfCraftingComponent::Validate_Range(APlayerController* PC, FZfCraftResult& OutResult) const
{
	AActor* Owner = GetOwner();
	APawn*  PlayerPawn = PC ? PC->GetPawn() : nullptr;
	if (!Owner || !PlayerPawn)
	{
		OutResult.ResultCode    = EZfCraftResult::Failed_OutOfRange;
		OutResult.FailureReason = NSLOCTEXT("ZfCraft", "OutOfRange", "Fora de alcance.");
		return false;
	}

	const float Distance = FVector::Dist(Owner->GetActorLocation(), PlayerPawn->GetActorLocation());
	if (Distance > MaxCraftRange)
	{
		UE_LOG(LogZfCraft, Verbose,
			TEXT("CraftingComponent: Range check falhou (%.0f > %.0f)."),
			Distance, MaxCraftRange);

		OutResult.ResultCode    = EZfCraftResult::Failed_OutOfRange;
		OutResult.FailureReason = NSLOCTEXT("ZfCraft", "OutOfRange", "Fora de alcance.");
		return false;
	}
	return true;
}

bool UZfCraftingComponent::Validate_NPCAcceptsRecipe(UZfCraftRecipe* Recipe, FZfCraftResult& OutResult) const
{
	if (AcceptedRecipesQuery.IsEmpty())
	{
		return true; // Query vazia = aceita tudo.
	}

	if (!AcceptedRecipesQuery.Matches(Recipe->CategoryTags))
	{
		OutResult.ResultCode    = EZfCraftResult::Failed_RecipeNotAccepted;
		OutResult.FailureReason = NSLOCTEXT("ZfCraft", "NotAccepted",
			"Este NPC nao pode fazer esta receita.");
		return false;
	}
	return true;
}

bool UZfCraftingComponent::Validate_PlayerKnowsRecipe(APlayerController* PC, UZfCraftRecipe* Recipe, FZfCraftResult& OutResult) const
{
	if (Recipe->bIsKnownByDefault)
	{
		return true;
	}

	AZfPlayerState* ZfPS = PC ? PC->GetPlayerState<AZfPlayerState>() : nullptr;
	if (!ZfPS || !ZfPS->HasLearnedRecipe(Recipe->RecipeTag))
	{
		OutResult.ResultCode    = EZfCraftResult::Failed_NotLearned;
		OutResult.FailureReason = NSLOCTEXT("ZfCraft", "NotLearned", "Receita desconhecida.");
		return false;
	}
	return true;
}

bool UZfCraftingComponent::Validate_WorldStateQuery(APlayerController* PC, UZfCraftRecipe* Recipe, FZfCraftResult& OutResult) const
{
	if (Recipe->WorldStateQuery.IsEmpty())
	{
		return true;
	}

	APawn* P = PC ? PC->GetPawn() : nullptr;
	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(P);
	UAbilitySystemComponent* ASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;

	if (!ASC)
	{
		OutResult.ResultCode    = EZfCraftResult::Failed_RequirementNotMet;
		OutResult.FailureReason = NSLOCTEXT("ZfCraft", "WorldStateFail", "Condicao de mundo nao atendida.");
		return false;
	}

	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	if (!Recipe->WorldStateQuery.Matches(OwnedTags))
	{
		OutResult.ResultCode    = EZfCraftResult::Failed_RequirementNotMet;
		OutResult.FailureReason = NSLOCTEXT("ZfCraft", "WorldStateFail", "Condicao de mundo nao atendida.");
		return false;
	}
	return true;
}

bool UZfCraftingComponent::Validate_ExtraRequirements(const FZfCraftContext& Context, FZfCraftResult& OutResult) const
{
	if (!Context.Recipe)
	{
		return true;
	}

	for (const TObjectPtr<UZfCraftRequirement>& Req : Context.Recipe->ExtraRequirements)
	{
		if (!Req) continue;

		if (!Req->CheckRequirement(Context))
		{
			OutResult.ResultCode    = EZfCraftResult::Failed_RequirementNotMet;
			OutResult.FailureReason = Req->GetFailureReason();
			return false;
		}
	}
	return true;
}

bool UZfCraftingComponent::Validate_Ingredients(UZfInventoryComponent* Inventory, UZfCraftRecipe* Recipe, FZfCraftResult& OutResult) const
{
	for (const FZfCraftIngredient& Ingredient : Recipe->Ingredients)
	{
		UZfItemDefinition* ItemDef = Ingredient.ItemDefinition.LoadSynchronous();
		if (!ItemDef)
		{
			UE_LOG(LogZfCraft, Warning,
				TEXT("CraftingComponent: Ingrediente com ItemDefinition invalida em '%s'."),
				*Recipe->RecipeTag.ToString());

			OutResult.ResultCode    = EZfCraftResult::Failed_InvalidRecipe;
			OutResult.FailureReason = NSLOCTEXT("ZfCraft", "InvalidIngredient",
				"Receita tem ingrediente invalido.");
			return false;
		}

		const int32 Available = CountItemsInInventory(Inventory, ItemDef);
		if (Available < Ingredient.Quantity)
		{
			OutResult.ResultCode    = EZfCraftResult::Failed_MissingIngredients;
			OutResult.FailureReason = FText::Format(
				NSLOCTEXT("ZfCraft", "MissingIngredient", "Falta {0}: precisa {1}, tem {2}."),
				ItemDef->ItemName,
				FText::AsNumber(Ingredient.Quantity),
				FText::AsNumber(Available));
			return false;
		}
	}
	return true;
}

bool UZfCraftingComponent::Validate_InventorySpace(UZfInventoryComponent* Inventory, UZfCraftRecipe* Recipe, FZfCraftResult& OutResult) const
{
	// Check conservador: assume 1 slot por output.
	// Se apos consumir ingredientes liberariam slots, considera isso
	// como estimativa para evitar falso negativo.
	const int32 AvailableSlots = Inventory->GetAvailableSlots();
	const int32 NeededSlots    = Recipe->Outputs.Num();

	if (AvailableSlots >= NeededSlots)
	{
		return true;
	}

	// Estima quantos slots serao liberados pelo consumo de ingredientes.
	int32 SlotsFreedByConsumption = 0;
	for (const FZfCraftIngredient& Ingredient : Recipe->Ingredients)
	{
		if (!Ingredient.bConsume) continue;

		UZfItemDefinition* ItemDef = Ingredient.ItemDefinition.LoadSynchronous();
		if (!ItemDef) continue;

		// Heuristica: se vai consumir todo o total em inventario, libera 1 slot.
		const int32 InInv = CountItemsInInventory(Inventory, ItemDef);
		if (InInv == Ingredient.Quantity)
		{
			SlotsFreedByConsumption++;
		}
	}

	if (AvailableSlots + SlotsFreedByConsumption < NeededSlots)
	{
		OutResult.ResultCode    = EZfCraftResult::Failed_InventoryFull;
		OutResult.FailureReason = NSLOCTEXT("ZfCraft", "InventoryFull", "Inventario cheio.");
		return false;
	}
	return true;
}


// ============================================================
// CONSUMO
// ============================================================

void UZfCraftingComponent::ConsumeIngredients(UZfInventoryComponent* Inventory, UZfCraftRecipe* Recipe, bool bFailurePath)
{
	for (const FZfCraftIngredient& Ingredient : Recipe->Ingredients)
	{
		if (!Ingredient.bConsume) continue;

		UZfItemDefinition* ItemDef = Ingredient.ItemDefinition.LoadSynchronous();
		if (!ItemDef) continue;

		int32 QuantityToRemove = Ingredient.Quantity;

		if (bFailurePath)
		{
			switch (Recipe->FailurePolicy)
			{
				case EZfFailureIngredientPolicy::ConsumeAll:
					// Mantem Quantity original.
					break;

				case EZfFailureIngredientPolicy::ConsumeHalf:
					// CeilToInt evita que "1 item" vire 0.
					QuantityToRemove = FMath::CeilToInt(Ingredient.Quantity * 0.5f);
					break;

				case EZfFailureIngredientPolicy::ConsumeNone:
					continue;
			}
		}

		RemoveItemsFromInventory(Inventory, ItemDef, QuantityToRemove);

		UE_LOG(LogZfCraft, Verbose,
			TEXT("CraftingComponent: Consumiu %d x '%s' (falha=%s)."),
			QuantityToRemove, *ItemDef->ItemName.ToString(),
			bFailurePath ? TEXT("SIM") : TEXT("NAO"));
	}
}


// ============================================================
// SPAWN DE OUTPUTS
// ============================================================

void UZfCraftingComponent::SpawnOutputs(UZfInventoryComponent* Inventory, UZfCraftRecipe* Recipe)
{
	if (!Inventory) return;

	for (const FZfCraftOutput& Output : Recipe->Outputs)
	{
		UZfItemDefinition* ItemDef = Output.ItemDefinition.LoadSynchronous();
		if (!ItemDef)
		{
			UE_LOG(LogZfCraft, Warning,
				TEXT("CraftingComponent: Output com ItemDefinition invalida, pulando."));
			continue;
		}

		const UZfFragment_Stackable* StackFrag = ItemDef->FindFragment<UZfFragment_Stackable>();
		const bool bStackable = (StackFrag != nullptr);

		if (bStackable)
		{
			// Cria 1 item com stack = Quantity.
			UZfItemInstance* NewItem = NewObject<UZfItemInstance>(Inventory->GetOwner());
			NewItem->InitializeItemInstance(ItemDef, /*Tier=*/0, EZfItemRarity::Common);

			if (Output.Quantity > 1)
			{
				NewItem->SetCurrentStack(Output.Quantity);
			}

			Inventory->ServerTryAddItemToInventory(NewItem);

			UE_LOG(LogZfCraft, Log,
				TEXT("CraftingComponent: Output '%s' x%d entregue (stackavel)."),
				*ItemDef->ItemName.ToString(), Output.Quantity);
		}
		else
		{
			// Nao stackavel: cria N itens separados.
			for (int32 i = 0; i < Output.Quantity; i++)
			{
				UZfItemInstance* NewItem = NewObject<UZfItemInstance>(Inventory->GetOwner());
				NewItem->InitializeItemInstance(ItemDef, 0, EZfItemRarity::Common);
				Inventory->ServerTryAddItemToInventory(NewItem);
			}

			UE_LOG(LogZfCraft, Log,
				TEXT("CraftingComponent: Output '%s' x%d entregue (%d itens separados)."),
				*ItemDef->ItemName.ToString(), Output.Quantity, Output.Quantity);
		}
	}
}


// ============================================================
// HELPERS
// ============================================================

int32 UZfCraftingComponent::CountItemsInInventory(UZfInventoryComponent* Inventory, UZfItemDefinition* ItemDef) const
{
	if (!Inventory || !ItemDef) return 0;

	int32 Total = 0;
	const TArray<UZfItemInstance*> AllItems = Inventory->GetAllItems();
	for (UZfItemInstance* Item : AllItems)
	{
		if (Item && Item->GetItemDefinition() == ItemDef)
		{
			// Stackavel: soma o stack. Nao-stackavel: conta como 1 instancia.
			const bool bStackable = Item->HasFragment<UZfFragment_Stackable>();
			Total += bStackable ? FMath::Max(1, Item->GetCurrentStack()) : 1;
		}
	}
	return Total;
}

void UZfCraftingComponent::RemoveItemsFromInventory(UZfInventoryComponent* Inventory, UZfItemDefinition* ItemDef, int32 Amount)
{
	if (!Inventory || !ItemDef || Amount <= 0) return;

	// Coleta instancias que batem com a ItemDefinition, junto dos slots.
	// Precisamos do slot para remover itens nao-stackaveis inteiros.
	struct FMatch
	{
		UZfItemInstance* Item;
		int32 SlotIndex;
	};

	TArray<FMatch> Matches;
	const int32 TotalSlots = Inventory->GetTotalSlots();
	for (int32 i = 0; i < TotalSlots; i++)
	{
		UZfItemInstance* Item = Inventory->GetItemAtSlot(i);
		if (Item && Item->GetItemDefinition() == ItemDef)
		{
			Matches.Add({Item, i});
		}
	}

	// Ordena do menor stack para o maior.
	Matches.Sort([](const FMatch& A, const FMatch& B)
	{
		return A.Item->GetCurrentStack() < B.Item->GetCurrentStack();
	});

	int32 Remaining = Amount;
	for (const FMatch& Match : Matches)
	{
		if (Remaining <= 0) break;

		UZfItemInstance* Item = Match.Item;
		const bool bStackable = Item->HasFragment<UZfFragment_Stackable>();

		if (bStackable)
		{
			// Stackavel: remove quantidade do stack.
			const int32 InStack  = FMath::Max(1, Item->GetCurrentStack());
			const int32 ToRemove = FMath::Min(Remaining, InStack);

			Inventory->ServerTryRemoveAmountFromStack(Item, ToRemove);
			Remaining -= ToRemove;
		}
		else
		{
			// Nao-stackavel: remove o item inteiro do slot.
			// Cada instancia conta como 1 unidade.
			Inventory->ServerTryRemoveItemFromInventory(Match.SlotIndex);
			Remaining -= 1;
		}
	}

	if (Remaining > 0)
	{
		UE_LOG(LogZfCraft, Error,
			TEXT("CraftingComponent: RemoveItems pediu %d mas sobrou %d — validacao de ingredientes falhou."),
			Amount, Remaining);
	}
}

FZfCraftContext UZfCraftingComponent::BuildContext(APlayerController* PC, UZfCraftRecipe* Recipe) const
{
	FZfCraftContext Context;
	Context.InstigatorController  = PC;
	Context.InstigatorPawn        = PC ? PC->GetPawn() : nullptr;
	Context.InstigatorPlayerState = PC ? PC->PlayerState : nullptr;
	Context.CrafterActor          = GetOwner();
	Context.Recipe                = Recipe;
	return Context;
}

UZfInventoryComponent* UZfCraftingComponent::GetInstigatorInventory(APlayerController* PC) const
{
	if (!PC) return nullptr;

	// Inventory vive no PlayerState no seu projeto.
	AZfPlayerState* ZfPS = PC->GetPlayerState<AZfPlayerState>();
	if (!ZfPS) return nullptr;

	return ZfPS->GetInventoryComponent();
}

UZfCraftingSubsystem* UZfCraftingComponent::GetCraftingSubsystem() const
{
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			return GI->GetSubsystem<UZfCraftingSubsystem>();
		}
	}
	return nullptr;
}

void UZfCraftingComponent::FireCueOnCrafter(const FGameplayTag& CueTag) const
{
	if (!CueTag.IsValid()) return;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);
	if (!ASC) return;

	FGameplayCueParameters Params;
	Params.Instigator = Owner;
	ASC->ExecuteGameplayCue(CueTag, Params);
}