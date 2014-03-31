#ifndef _ITEM_ATTRIBUTES_H
#define _ITEM_ATTRIBUTES_H

#include <string>
#include <functional>

#include "util.h"
#include "effect.h"
#include "enums.h"

#define ITATTR(X) ItemAttributes([&](ItemAttributes& i) { X })

class EnemyCheck;

class ItemAttributes {
  public:
  ItemAttributes(function<void(ItemAttributes&)> fun) {
    fun(*this);
  }

  SERIALIZATION_DECL(ItemAttributes);

  MustInitialize<string> SERIAL(name);
  string SERIAL(description);
  MustInitialize<double> SERIAL(weight);
  MustInitialize<ItemType> SERIAL(type);
  Optional<string> SERIAL(realName);
  Optional<string> SERIAL(realPlural);
  Optional<string> SERIAL(plural);
  Optional<string> SERIAL(blindName);
  Optional<ItemId> SERIAL(firingWeapon);
  Optional<string> SERIAL(artifactName);
  Optional<TrapType> SERIAL(trapType);
  double SERIAL2(flamability, 0);
  int SERIAL2(price, 0);
  bool SERIAL2(noArticle, false);
  int SERIAL2(damage, 0);
  int SERIAL2(toHit, 0);
  int SERIAL2(thrownDamage, 0);
  int SERIAL2(thrownToHit, 0);
  int SERIAL2(rangedWeaponAccuracy, 0);
  int SERIAL2(defense, 0);
  int SERIAL2(strength, 0);
  int SERIAL2(dexterity, 0);
  int SERIAL2(speed, 0);
  bool SERIAL2(twoHanded, false);
  AttackType SERIAL2(attackType, AttackType::HIT);
  double SERIAL2(attackTime, 1);
  Optional<ArmorType> SERIAL(armorType);
  double SERIAL2(applyTime, 1);
  bool SERIAL2(fragile, false);
  Optional<EffectType> SERIAL(effect);
  int SERIAL2(uses, -1);
  bool SERIAL2(usedUpMsg, false);
  bool SERIAL2(displayUses, false);
  bool SERIAL2(identifyOnApply, true);
  bool SERIAL2(identifiable, false);
  bool SERIAL2(identifyOnEquip, true);

};

#endif
