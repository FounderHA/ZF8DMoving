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
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Pickaxe_Bronze, "Gathering.Tool.Pickaxe.Bronze")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Pickaxe_Iron, "Gathering.Tool.Pickaxe.Iron")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Pickaxe_Silver, "Gathering.Tool.Pickaxe.Silver")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Pickaxe_Mithril, "Gathering.Tool.Pickaxe.Mithril")

        // Machados
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Axe,       "Gathering.Tool.Axe")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Axe_Bronze, "Gathering.Tool.Axe.Bronze")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Axe_Iron, "Gathering.Tool.Axe.Iron")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Axe_Silver, "Gathering.Tool.Axe.Silver")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Axe_Mithril, "Gathering.Tool.Axe.Mithril")

        // Pás
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Shovel,       "Gathering.Tool.Shovel")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Shovel_Bronze, "Gathering.Tool.Shovel.Bronze")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Shovel_Iron, "Gathering.Tool.Shovel.Iron")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Shovel_Silver, "Gathering.Tool.Shovel.Silver")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_Shovel_Mithril, "Gathering.Tool.Shovel.Mithril")

        // Varas de pesca
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_FishingRod,       "Gathering.Tool.FishingRod")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_FishingRod_Bronze, "Gathering.Tool.FishingRod.Bronze")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_FishingRod_Iron, "Gathering.Tool.FishingRod.Iron")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_FishingRod_Silver, "Gathering.Tool.FishingRod.Silver")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Tool_FishingRod_Mithril, "Gathering.Tool.FishingRod.Mithril")
    }

    namespace ResourceTags
    {
        // Minérios
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Resource_Ore,         "Gathering.Resource.Ore")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Resource_Ore_Bronze,    "Gathering.Resource.Ore.Bronze")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Resource_Ore_Iron,    "Gathering.Resource.Ore.Iron")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Resource_Ore_Silver, "Gathering.Resource.Ore.Silver")
        UE_DEFINE_GAMEPLAY_TAG(Gathering_Resource_Ore_Mithril, "Gathering.Resource.Ore.Mithril")

        
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

    namespace ToolProperties
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