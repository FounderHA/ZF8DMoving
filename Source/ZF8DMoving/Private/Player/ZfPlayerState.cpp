// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ZfPlayerState.h"

#include "Player/LevelProgression/ZfAttributeSpendRequest.h"
#include "Player/LevelProgression/ZfAttributeRefundRequest.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Inventory/ZfInventoryComponent.h"
#include "AbilitySystem/ZfAbilitySystemComponent.h"
#include "Inventory/ZfEquipmentComponent.h"
#include "Tags/ZfGameplayTags.h"
#include "AbilitySystem/Attributes/ZfResourceAttributeSet.h"
#include "AbilitySystem/Attributes/ZfMainAttributeSet.h"
#include "AbilitySystem/Attributes/ZfProgressionAttributeSet.h"
#include "AbilitySystem/Attributes/ZfOffensiveAttributeSet.h"
#include "AbilitySystem/Attributes/ZfDefensiveAttributeSet.h"
#include "AbilitySystem/Attributes/ZfUtilityAttributeSet.h"
#include "AbilitySystem/Attributes/ZfMovementAttributeSet.h"
#include "CraftingSystem/ZfCraftingComponent.h"
#include "GameFramework/GameStateBase.h"
#include "CraftingSystem/ZfCraftTypes.h"
#include "CraftingSystem/ZfCraftRecipe.h"

AZfPlayerState::AZfPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UZfAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	InventoryComponent = CreateDefaultSubobject<UZfInventoryComponent>(TEXT("InventoryComponent"));
	InventoryComponent->SetIsReplicated(true);

	EquipmentComponent = CreateDefaultSubobject<UZfEquipmentComponent>(TEXT("EquipmentComponent"));
	EquipmentComponent->SetIsReplicated(true);
	
	AbilityTreeComponent = CreateDefaultSubobject<UZfAbilityTreeComponent>(TEXT("AbilityTreeComponent"));
	AbilityTreeComponent->SetIsReplicated(true);
	
	SetNetUpdateFrequency(100.0f);
	
	ResourceAttributeSet = CreateDefaultSubobject<UZfResourceAttributeSet>(TEXT("ResourceAttributeSet"));
	MainAttributeSet = CreateDefaultSubobject<UZfMainAttributeSet>(TEXT("MainAttributeSet"));
	ProgressionAttributeSet = CreateDefaultSubobject<UZfProgressionAttributeSet>(TEXT("ProgressionAttributeSet"));
	OffensiveAttributeSet = CreateDefaultSubobject<UZfOffensiveAttributeSet>(TEXT("DamageAttributeSet"));
	ResistanceAttributeSet = CreateDefaultSubobject<UZfDefensiveAttributeSet>(TEXT("ResistanceAttributeSet"));
	UtilityAttributeSet = CreateDefaultSubobject<UZfUtilityAttributeSet>(TEXT("UtilityAttributeSet"));
	MovementAttributeSet = CreateDefaultSubobject<UZfMovementAttributeSet>(TEXT("MovementAttributeSet"));
}

UAbilitySystemComponent* AZfPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AZfPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AZfPlayerState, CharacterClassData);
	DOREPLIFETIME(AZfPlayerState, KnownRecipeTags);
}

UZfResourceAttributeSet* AZfPlayerState::GetResourceAttributeSet() const
{
	return ResourceAttributeSet;
}

UZfMainAttributeSet* AZfPlayerState::GetMainAttributeSet() const
{
	return MainAttributeSet;
}

UZfProgressionAttributeSet* AZfPlayerState::GetProgressionAttributeSet() const
{
	return ProgressionAttributeSet;
}

UZfOffensiveAttributeSet* AZfPlayerState::GetOffensiveAttributeSet() const
{
	return OffensiveAttributeSet;
}

UZfDefensiveAttributeSet* AZfPlayerState::GetResistanceAttributeSet() const
{
	return ResistanceAttributeSet;
}

UZfUtilityAttributeSet* AZfPlayerState::GetUtilityAttributeSet() const
{
	return UtilityAttributeSet;
}

UZfMovementAttributeSet* AZfPlayerState::GetMovementAttributeSet() const
{
	return MovementAttributeSet;
}

//
//============================ Debug Attribut ============================
//



// =============================================================================
// Server RPCs — Progressão
// Valores passados como int32 — sem replicação de UObject pela rede.
// O Request é criado localmente no servidor após o RPC chegar.
// =============================================================================
 
void AZfPlayerState::Server_SpendAttributePoints_Implementation(
	int32 Strength,
	int32 Dexterity,
	int32 Intelligence,
	int32 Constitution,
	int32 Conviction)
{
	UZfAttributeSpendRequest* Request = NewObject<UZfAttributeSpendRequest>(this);
	Request->StrengthPointsToAdd     = Strength;
	Request->DexterityPointsToAdd    = Dexterity;
	Request->IntelligencePointsToAdd = Intelligence;
	Request->ConstitutionPointsToAdd = Constitution;
	Request->ConvictionPointsToAdd   = Conviction;
 
	if (!Request->IsValid()) return;
 
	APawn* Pawn = GetPawn();
	if (!Pawn) return;
 
	FGameplayEventData Payload;
	Payload.OptionalObject = static_cast<const UObject*>(Request);
 
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Pawn,
		ZfProgressionTags::LevelProgression_Event_Character_SpendAttributePoints,
		Payload
	);
}
 
void AZfPlayerState::Server_RefundAttributePoint_Implementation(
	int32 Strength,
	int32 Dexterity,
	int32 Intelligence,
	int32 Constitution,
	int32 Conviction)
{
	UZfAttributeRefundRequest* Request = NewObject<UZfAttributeRefundRequest>(this);
	Request->StrengthPointsToRemove     = Strength;
	Request->DexterityPointsToRemove    = Dexterity;
	Request->IntelligencePointsToRemove = Intelligence;
	Request->ConstitutionPointsToRemove = Constitution;
	Request->ConvictionPointsToRemove   = Conviction;
 
	if (!Request->IsValid()) return;
 
	APawn* Pawn = GetPawn();
	if (!Pawn) return;
 
	FGameplayEventData Payload;
	Payload.OptionalObject = static_cast<const UObject*>(Request);
 
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		Pawn,
		ZfProgressionTags::LevelProgression_Event_Character_RefundAttributePoint,
		Payload
	);
}
 
void AZfPlayerState::Server_ResetAttributePoints_Implementation()
{
	APawn* Pawn = GetPawn();
	if (!Pawn) return;
 
	FGameplayEventData Payload;
 
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Pawn,
		ZfProgressionTags::LevelProgression_Event_Character_ResetAttributePoints,
		Payload
	);
}

void AZfPlayerState::Server_UnlockAbilityNode_Implementation(const FName& NodeID)
{
	if (NodeID.IsNone()) return;
 
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;
 
	AbilityTreeComponent->UnlockNode(ASC, NodeID);
}
 
void AZfPlayerState::Server_UpgradeAbilityNode_Implementation(const FName& NodeID)
{
	if (NodeID.IsNone()) return;
 
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;
 
	AbilityTreeComponent->UpgradeNode(ASC, NodeID);
}
 
void AZfPlayerState::Server_UnlockSubEffect_Implementation(const FName& NodeID, int32 SubEffectIndex)
{
	if (NodeID.IsNone()) return;
 
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;
 
	AbilityTreeComponent->UnlockSubEffect(ASC, NodeID, SubEffectIndex);
}
 
void AZfPlayerState::Server_RespecAbilityTree_Implementation()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;
 
	AbilityTreeComponent->RespecTree(ASC);
}
 
void AZfPlayerState::Server_EquipAbilityInSlot_Implementation(const FName& NodeID, int32 SlotIndex)
{
	if (NodeID.IsNone()) return;
 
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;
 
	AbilityTreeComponent->EquipAbilityInSlot(ASC, NodeID, SlotIndex);
}
 
void AZfPlayerState::Server_UnequipAbilityFromSlot_Implementation(int32 SlotIndex)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;
 
	AbilityTreeComponent->UnequipAbilityFromSlot(SlotIndex);
}

// =============================================================================
// CRAFT — RECEITAS CONHECIDAS
// =============================================================================

bool AZfPlayerState::HasLearnedRecipe(const FGameplayTag& RecipeTag) const
{
	return KnownRecipeTags.HasTag(RecipeTag);
}

// ─────────────────────────────────────────────────────────────────────────────
// Server_LearnRecipe
// Chamado no PlayerState do jogador que descobriu.
// 1. Adiciona a tag localmente (LearnRecipe_Internal).
// 2. Propaga para TODOS os outros players online (LearnRecipe_Internal neles).
//
// O OnRep dispara automaticamente nos clientes quando o KnownRecipeTags muda
// via replicacao, atualizando a UI de quem recebeu.
// ─────────────────────────────────────────────────────────────────────────────

void AZfPlayerState::Server_LearnRecipe_Implementation(FGameplayTag RecipeTag)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!RecipeTag.IsValid())
	{
		UE_LOG(LogZfCraft, Warning,
			TEXT("AZfPlayerState::Server_LearnRecipe: RecipeTag invalida, ignorando."));
		return;
	}

	// Adiciona no proprio PlayerState do descobridor.
	const bool bLearnedHere = LearnRecipe_Internal(RecipeTag);

	// Propaga para todos os outros players online.
	AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GS)
	{
		UE_LOG(LogZfCraft, Warning,
			TEXT("AZfPlayerState::Server_LearnRecipe: GameState indisponivel, "
			     "impossivel propagar para os outros players."));
		return;
	}

	int32 PropagatedCount = 0;

	for (APlayerState* OtherPS : GS->PlayerArray)
	{
		if (!OtherPS || OtherPS == this)
		{
			continue;
		}

		AZfPlayerState* OtherZfPS = Cast<AZfPlayerState>(OtherPS);
		if (!OtherZfPS)
		{
			continue;
		}

		if (OtherZfPS->LearnRecipe_Internal(RecipeTag))
		{
			PropagatedCount++;
		}
	}

	UE_LOG(LogZfCraft, Log,
		TEXT("AZfPlayerState::Server_LearnRecipe: '%s' aprendida por '%s' "
		     "(novo aqui=%s, propagada para %d outros players)."),
		*RecipeTag.ToString(),
		*GetPlayerName(),
		bLearnedHere ? TEXT("SIM") : TEXT("JA_TINHA"),
		PropagatedCount);
}

// ─────────────────────────────────────────────────────────────────────────────
// LearnRecipe_Internal
// Adiciona a tag localmente no servidor. Nao propaga.
// Retorna true se foi adicionada (nao estava la antes).
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// Server_RequestCraft
// Roteador — recebe do cliente e delega ao CraftingComponent do NPC.
// ─────────────────────────────────────────────────────────────────────────────

void AZfPlayerState::Server_RequestCraft_Implementation(UZfCraftingComponent* CraftingComponent, FGameplayTag RecipeTag)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!CraftingComponent)
	{
		UE_LOG(LogZfCraft, Warning,
			TEXT("AZfPlayerState::Server_RequestCraft: CraftingComponent nulo."));
		return;
	}

	APlayerController* PC = GetPlayerController();
	if (!PC)
	{
		UE_LOG(LogZfCraft, Warning,
			TEXT("AZfPlayerState::Server_RequestCraft: PlayerController nulo."));
		return;
	}

	// Delega para o componente executar com o PC correto.
	CraftingComponent->RequestCraft(PC, RecipeTag);
}

bool AZfPlayerState::LearnRecipe_Internal(const FGameplayTag& RecipeTag)
{
	if (!HasAuthority())
	{
		return false;
	}

	if (KnownRecipeTags.HasTag(RecipeTag))
	{
		return false; // Ja tinha — nao conta como aprendido agora.
	}

	// Guarda snapshot ANTES de adicionar, para passar ao OnRep manual.
	// Sem isso, o diff no listen server daria vazio (ambos os containers
	// seriam iguais) e o OnRecipeLearned nunca dispararia no host.
	const FGameplayTagContainer PreviousTags = KnownRecipeTags;

	KnownRecipeTags.AddTag(RecipeTag);

	// No servidor, o OnRep NAO dispara automaticamente — invoca manualmente
	// passando o snapshot anterior para que o diff funcione.
	// Clientes remotos vao receber replicacao e disparar OnRep naturalmente.
	OnRep_KnownRecipeTags(PreviousTags);

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// OnRep_KnownRecipeTags
// Detecta quais tags sao novas e dispara OnRecipeLearned para cada uma.
// Importante: pode receber MULTIPLAS tags novas em um unico update se
// varias foram aprendidas em sequencia rapida antes de replicar.
// ─────────────────────────────────────────────────────────────────────────────

void AZfPlayerState::OnRep_KnownRecipeTags(const FGameplayTagContainer& OldTags)
{
	// Para cada tag do novo container que nao estava no antigo, e uma tag nova.
	for (const FGameplayTag& Tag : KnownRecipeTags)
	{
		if (!OldTags.HasTagExact(Tag))
		{
			UE_LOG(LogZfCraft, Verbose,
				TEXT("AZfPlayerState::OnRep_KnownRecipeTags: receita '%s' nova em '%s'."),
				*Tag.ToString(),
				*GetPlayerName());

			OnRecipeLearned.Broadcast(Tag);
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Client_ReceiveCraftResult
// Recebido no cliente do jogador originador de um craft.
// Faz broadcast local da delegate para a UI escutar.
// ─────────────────────────────────────────────────────────────────────────────

void AZfPlayerState::Client_ReceiveCraftResult_Implementation(const FZfCraftResult& Result)
{
	UE_LOG(LogZfCraft, Verbose,
		TEXT("AZfPlayerState::Client_ReceiveCraftResult: Receita '%s' | Resultado: %s"),
		Result.Recipe ? *Result.Recipe->RecipeTag.ToString() : TEXT("?"),
		*UEnum::GetValueAsString(Result.ResultCode));

	OnCraftResultReceived.Broadcast(Result);
}