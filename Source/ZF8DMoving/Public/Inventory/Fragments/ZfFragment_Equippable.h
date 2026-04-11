// Copyright ZfGame Studio. All Rights Reserved.
// ZfFragment_Equippable.h
// Fragment que define em qual slot de equipamento o item pode ser equipado.
// Todo item equipável DEVE ter este fragment no seu ItemDefinition.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Fragments/ZfItemFragment.h"
#include "Inventory/ZfInventoryTypes.h"
#include "Tags/ZfGameplayTags.h"
#include "ZfFragment_Equippable.generated.h"

UENUM(BlueprintType)
enum class EZfHandedness : uint8
{
    // Não se aplica — item não ocupa slot de mão
    // Ex: Capacete, Botas, Anel, Colar
    NotApplicable   UMETA(DisplayName = "Not Applicable"),

    // Item de uma mão — ocupa apenas MainHand ou OffHand
    // Ex: Espada curta, Escudo, Adaga
    OneHanded       UMETA(DisplayName = "One Handed"),

    // Item de duas mãos — ocupa MainHand E bloqueia OffHand
    // Ex: Espada grande, Arco, Cajado de duas mãos
    TwoHanded       UMETA(DisplayName = "Two Handed"),
};

UCLASS(DisplayName = "Fragment: Equippable")
class ZF8DMOVING_API UZfFragment_Equippable : public UZfItemFragment
{
    GENERATED_BODY()

public:

    // Slot de equipamento onde este item será colocado.
    // Ex: Helmet → Head, Boots → Feet, Sword → MainHand
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fragment|Equippable", meta =(GameplayTagFilter = "EquipmentSlot"))
    FGameplayTag EquipmentSlotTag;
    
    // Define se o item ocupa uma ou duas mãos.
    // Só é relevante para itens nos slots MainHand e OffHand.
    // Para todos os outros slots, use NotApplicable.
    //
    // TwoHanded → ao equipar, o EquipmentComponent bloqueia
    // o slot OffHand completamente até o item ser desequipado.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|Equippable")
    EZfHandedness Handedness = EZfHandedness::NotApplicable;
    
    // Índice do slot quando há múltiplos do mesmo tipo.
    // Ex: Ring_0 ou Ring_1. Deixe 0 para slots únicos.
    // O EquipmentComponent encontrará o primeiro slot livre
    // do mesmo tipo automaticamente se bAutoFindSlot = true.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|Equippable")
    int32 PreferredSlotIndex = 0;

    // Se verdadeiro, ao equipar, o sistema busca automaticamente
    // o primeiro slot livre do mesmo tipo (ignora PreferredSlotIndex).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|Equippable")
    bool bAutoFindFreeSlot = true;

    // Tags de requisito para equipar — o ator precisa ter TODAS essas tags.
    // Ex: pode exigir "Character.Class.Warrior" para equipar uma espada pesada.
    // Deixe vazio para sem restrição.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fragment|Equippable")
    FGameplayTagContainer RequiredActorTags;

    // ----------------------------------------------------------
    // HELPERS
    // ----------------------------------------------------------

    // Retorna true se este item é de duas mãos.
    // Usado pelo EquipmentComponent para bloquear o OffHand.
    UFUNCTION(BlueprintCallable, Category = "Zf|Fragment|Equippable")
    bool IsTwoHanded() const
    {
        return Handedness == EZfHandedness::TwoHanded;
    }

    // Retorna true se este item é de uma mão.
    UFUNCTION(BlueprintCallable, Category = "Zf|Fragment|Equippable")
    bool IsOneHanded() const
    {
        return Handedness == EZfHandedness::OneHanded;
    }

    // Retorna true se a regra de mão se aplica a este item.
    // False para capacetes, botas, anéis, etc.
    UFUNCTION(BlueprintCallable, Category = "Zf|Fragment|Equippable")
    bool HasHandednessRule() const
    {
        return Handedness != EZfHandedness::NotApplicable;
    }


    // ----------------------------------------------------------
    // INTERAÇÕES
    // ----------------------------------------------------------

    virtual void OnItemEquipped(UZfItemInstance* OwningInstance, UZfEquipmentComponent* EquipmentComponent, AActor* EquippingActor) override;

    virtual void OnItemUnequipped(UZfItemInstance* OwningInstance, UZfEquipmentComponent* EquipmentComponent, AActor* UnequippingActor) override;

    // GE que aplica os atributos do item no ASC via MMC
    // Configure no editor com o GE que usa o MMC de atributos
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment|GAS")
    TSoftClassPtr<UGameplayEffect> AttributeGameplayEffect;
    
    // ----------------------------------------------------------
    // DEBUG
    // ----------------------------------------------------------
    
};
