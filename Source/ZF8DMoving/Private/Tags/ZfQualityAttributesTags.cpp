#include "Tags/ZfQualityAttributesTags.h"

namespace ZfQualityAttributesTags
{
	namespace AttributesWeapon
	{
		UE_DEFINE_GAMEPLAY_TAG(AttributesWeapon, "QualityAttributes.Weapon")
		UE_DEFINE_GAMEPLAY_TAG(AttributesWeapon_PhysicalDamage, "QualityAttributes.Weapon.PhysicalDamage")
		UE_DEFINE_GAMEPLAY_TAG(AttributesWeapon_MagicalDamage, "QualityAttributes.Weapon.MagicalDamage")
		UE_DEFINE_GAMEPLAY_TAG(AttributesWeapon_CriticalHitChance, "QualityAttributes.Weapon.CriticalHitChance")
		UE_DEFINE_GAMEPLAY_TAG(AttributesWeapon_CriticalDamage, "QualityAttributes.Weapon.CriticalDamage")
		UE_DEFINE_GAMEPLAY_TAG(AttributesWeapon_AttackSpeed, "QualityAttributes.Weapon.AttackSpeed")
	}
	
	namespace AttributesArmor
	{
		UE_DEFINE_GAMEPLAY_TAG(AttributesArmor, "QualityAttributes.Armor")
		UE_DEFINE_GAMEPLAY_TAG(AttributesArmor_PhysicalResistance, "QualityAttributes.Armor.PhysicalResistance")
		UE_DEFINE_GAMEPLAY_TAG(AttributesArmor_MagicalResistance, "QualityAttributes.Armor.MagicalResistance")
		UE_DEFINE_GAMEPLAY_TAG(AttributesArmor_StunResistance, "QualityAttributes.Armor.StunResistance")
	}
}