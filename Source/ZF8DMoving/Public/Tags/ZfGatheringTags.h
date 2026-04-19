// Copyright ZfGame Studio. All Rights Reserved.
// ZfGatherTags.h
// Tags nativas do sistema de coleta de recursos.
//
// SEPARAÇÃO DE RESPONSABILIDADES:
// - ZfInventoryTags → tipo do item (ItemType.Tools.Pickaxe)
// - ZfGatherTags    → capacidade e tier da ferramenta de coleta,
//                     resultado do QTE, tipo de recurso e
//                     propriedades de modifier de coleta
//
// USO EM C++:
// FGameplayTag Tag = ZfGatherTags::ToolTags::Pickaxe_Tier1;
//
// USO EM BLUEPRINT:
// FGameplayTag::RequestGameplayTag("Tool.Pickaxe.Tier1")

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace ZfGatheringTags
{
    // ----------------------------------------------------------
    // QTE
    // Enviadas pelo Blueprint Widget ao fim de cada golpe.
    // A GA aguarda qualquer filha de Gather.QTE.Hit.
    // ----------------------------------------------------------
    namespace QTE
    {
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_QTE_Hit)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_QTE_Hit_Perfect)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_QTE_Hit_Good)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_QTE_Hit_Bad)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_QTE_Hit_Missed)
    }

    // ----------------------------------------------------------
    // TOOL TAGS
    // Identidade da ferramenta no sistema de coleta.
    // Configurada no ZfGatheringToolData e referenciada no
    // AllowedTools[] do ZfGatheringResourceData.
    // Tier define eficiência: mais tier = menos golpes + zona maior.
    // ----------------------------------------------------------
    namespace ToolTags
    {
        // Picaretas — mineração e escavação
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Tool_Pickaxe)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Tool_Pickaxe_Iron)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Tool_Pickaxe_Tier2)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Tool_Pickaxe_Tier3)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Tool_Pickaxe_Tier4)

        // Machados — coleta de madeira
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Tool_Axe)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Tool_Axe_Iron)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Tool_Axe_Tier2)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Tool_Axe_Tier3)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Tool_Axe_Tier4)

        // Pás — escavação de terra
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Tool_Shovel)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Tool_Shovel_Iron)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Tool_Shovel_Tier2)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Tool_Shovel_Tier3)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Tool_Shovel_Tier4)

        // Varas de pesca
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Tool_FishingRod)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Tool_FishingRod_Iron)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Tool_FishingRod_Tier2)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Tool_FishingRod_Tier3)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Tool_FishingRod_Tier4)
    }

    // ----------------------------------------------------------
    // RESOURCE TAGS
    // Identidade do recurso no mundo.
    // Configurada no ZfGatheringResourceData de cada recurso.
    // ----------------------------------------------------------
    namespace ResourceTags
    {
        // Minérios
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Resource_Ore)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Resource_Ore_Iron)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Resource_Ore_Gold)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Resource_Ore_Crystal)

        // Madeiras
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Resource_Wood)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Resource_Wood_Oak)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Resource_Wood_Pine)

        // Escavação
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Resource_Earth)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Resource_Earth_Dirt)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Resource_Earth_Sand)

        // Pesca
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Resource_FishingSpot)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Resource_FishingSpot_River)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Resource_FishingSpot_Lake)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Resource_FishingSpot_Ocean)
    }

    // ----------------------------------------------------------
    // ITEM PROPERTIES
    // Usadas nos modifiers da ferramenta com TargetType = ItemProperty.
    // O ZfFragment_GatheringTool filtra por estas tags ao resolver stats.
    // ----------------------------------------------------------
    namespace ToolProperties
    {
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Property_DropMultiplier)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Property_ScoreBonus)
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Property_BonusDamage)
    }

    namespace Activation
    {
        ZF8DMOVING_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gathering_Activate)
    }
}