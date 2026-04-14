// Copyright ZfGame Studio. All Rights Reserved.
// ZfGatheringTags.cpp

#include "Tags/ZfGatheringTags.h"

namespace ZfGatheringTags
{
    namespace QTE
    {
        UE_DEFINE_GAMEPLAY_TAG(Gathering_QTE_Hit,         "Gathering.QTE.Hit")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_QTE_Hit_Perfect, "Gathering.QTE.Hit.Perfect")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_QTE_Hit_Good,    "Gathering.QTE.Hit.Good")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_QTE_Hit_Bad,     "Gathering.QTE.Hit.Bad")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_QTE_Hit_Missed,  "Gathering.QTE.Hit.Missed")
    }

    namespace ToolTags
    {
        // Picaretas
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Pickaxe,       "Gathering.Tool.Pickaxe")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Pickaxe_Iron, "Gathering.Tool.Pickaxe.Iron")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Pickaxe_Tier2, "Gathering.Tool.Pickaxe.Tier2")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Pickaxe_Tier3, "Gathering.Tool.Pickaxe.Tier3")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Pickaxe_Tier4, "Gathering.Tool.Pickaxe.Tier4")

        // Machados
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Axe,       "Gathering.Tool.Axe")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Axe_Iron, "Gathering.Tool.Axe.Iron")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Axe_Tier2, "Gathering.Tool.Axe.Tier2")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Axe_Tier3, "Gathering.Tool.Axe.Tier3")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Axe_Tier4, "Gathering.Tool.Axe.Tier4")

        // Pás
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Shovel,       "Gathering.Tool.Shovel")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Shovel_Iron, "Gathering.Tool.Shovel.Iron")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Shovel_Tier2, "Gathering.Tool.Shovel.Tier2")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Shovel_Tier3, "Gathering.Tool.Shovel.Tier3")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Shovel_Tier4, "Gathering.Tool.Shovel.Tier4")

        // Varas de pesca
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_FishingRod,       "Gathering.Tool.FishingRod")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_FishingRod_Iron, "Gathering.Tool.FishingRod.Iron")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_FishingRod_Tier2, "Gathering.Tool.FishingRod.Tier2")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_FishingRod_Tier3, "Gathering.Tool.FishingRod.Tier3")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_FishingRod_Tier4, "Gathering.Tool.FishingRod.Tier4")
    }

    namespace ResourceTags
    {
        // Minérios
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Resource_Ore,         "Gathering.Resource.Ore")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Resource_Ore_Iron,    "Gathering.Resource.Ore.Iron")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Resource_Ore_Gold,    "Gathering.Resource.Ore.Gold")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Resource_Ore_Crystal, "Gathering.Resource.Ore.Crystal")

        // Madeiras
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Resource_Wood,      "Gathering.Resource.Wood")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Resource_Wood_Oak,  "Gathering.Resource.Wood.Oak")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Resource_Wood_Pine, "Gathering.Resource.Wood.Pine")

        // Escavação
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Resource_Earth,      "Gathering.Resource.Earth")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Resource_Earth_Dirt, "Gathering.Resource.Earth.Dirt")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Resource_Earth_Sand, "Gathering.Resource.Earth.Sand")

        // Pesca
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Resource_FishingSpot,        "Gathering.Resource.FishingSpot")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Resource_FishingSpot_River,  "Gathering.Resource.FishingSpot.River")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Resource_FishingSpot_Lake,   "Gathering.Resource.FishingSpot.Lake")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Resource_FishingSpot_Ocean,  "Gathering.Resource.FishingSpot.Ocean")
    }

    namespace ItemProperties
    {
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Property_DropMultiplier, "Gathering.Property.DropMultiplier")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Property_ScoreBonus,     "Gathering.Property.ScoreBonus")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Property_BonusDamage,  "Gathering.Property.BonusDamage")
    }

    namespace Activation
    {
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Activate, "Gathering.Activate")
    }
}