// Copyright ZfGame Studio. All Rights Reserved.
// ZfFragment_Consumable.h
// Fragment que define o comportamento ao consumir o item.
//
// CONCEITO:
// Qualquer item com este fragment pode ser "usado" pelo player.
// O uso e processado pela ZfGA_UseItem (GameplayAbility), que:
//   1. Checa cooldowns (global e por item)
//   2. Checa single use (bIsSingleUsePerGame)
//   3. Aplica o ConsumptionGameplayEffect no ASC do player
//   4. Verifica UZfFragment_RecipeScroll (aprende receita se for scroll)
//   5. Consome o item (reduz stack ou remove) se bConsumeOnUse = true
//
// COOLDOWNS:
// - Global: configurado no GE_Cooldown_ItemGlobal (asset Blueprint).
//           Duração fixa, bloqueia qualquer consumivel por X segundos.
// - Por item: configurado via ItemCooldownSeconds aqui no fragment.
//             A GA cria um GE dinamico com a duracao deste campo.
//
// SINGLE USE:
// Se bIsSingleUsePerGame = true, o item so pode ser usado uma vez
// por personagem, para sempre. A tag UniqueItemTag e salva em
// AZfPlayerState::UsedUniqueItemTags ao usar.
// Na proxima tentativa, a GA checa essa tag e bloqueia.
// Persiste via Profile Save junto com KnownRecipeTags.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Inventory/Fragments/ZfItemFragment.h"
#include "ZfFragment_Consumable.generated.h"

class UGameplayEffect;


UCLASS(DisplayName = "Fragment: Consumable")
class ZF8DMOVING_API UZfFragment_Consumable : public UZfItemFragment
{
	GENERATED_BODY()

public:

	// ----------------------------------------------------------
	// EFEITO
	// ----------------------------------------------------------

	// GameplayEffect aplicado ao consumir o item.
	// Ex: Pocao de vida → GE_RestoreHealth
	// Soft reference para carregamento assincrono.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|Consumable|Effect")
	TSoftClassPtr<UGameplayEffect> ConsumptionGameplayEffect;

	// Nivel do GameplayEffect ao aplicar.
	// Permite escalamento de efeitos por nivel.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|Consumable|Effect",
		meta = (ClampMin = "1"))
	int32 EffectLevel = 1;

	// ----------------------------------------------------------
	// CONSUMO
	// ----------------------------------------------------------

	// Se verdadeiro, o item e destruido ao consumir
	// (reduz stack em 1, remove o item se chegar a 0).
	// False = item e "usado" mas nao desaparece (ex: item de quest reutilizavel).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|Consumable|Consume")
	bool bConsumeOnUse = true;

	// ----------------------------------------------------------
	// SINGLE USE
	// ----------------------------------------------------------

	// Se verdadeiro, so pode ser usado uma unica vez no jogo inteiro.
	// Requer UniqueItemTag configurada abaixo.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|Consumable|SingleUse")
	bool bIsSingleUsePerGame = false;

	// Tag unica que identifica este item para fins de single use.
	// Deve ser uma tag do namespace Item.Unique.* (declarada em ZfInventoryTags.h).
	// Quando o player usa o item, essa tag e salva em PlayerState::UsedUniqueItemTags.
	// Deixe vazia se bIsSingleUsePerGame = false.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|Consumable|SingleUse",
		meta = (EditCondition = "bIsSingleUsePerGame", GameplayTagFilter = "Item.Unique"))
	FGameplayTag UniqueItemTag;

	// ----------------------------------------------------------
	// DEBUG
	// ----------------------------------------------------------

	virtual FString GetDebugString() const override
	{
		return FString::Printf(
			TEXT("[Fragment_Consumable] Effect: %s | ConsumeOnUse: %s | SingleUse: %s"),
			*ConsumptionGameplayEffect.ToString(),
			bConsumeOnUse ? TEXT("Yes") : TEXT("No"),
			bIsSingleUsePerGame ? TEXT("Yes") : TEXT("No"));
	}
};