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
  
  MustInitialize<string> name;
  string description;
  MustInitialize<double> weight;
  MustInitialize<ItemType> type;
  Optional<string> realName;
  Optional<string> realPlural;
  Optional<string> plural;
  Optional<string> blindName;
  Optional<ItemId> firingWeapon;
  Optional<string> artifactName;
  Optional<TrapType> trapType;
  double flamability = 0;
  int price = 0;
  bool noArticle = false;
  int damage = 0;
  int toHit = 0;
  int thrownDamage = 0;
  int thrownToHit = 0;
  int rangedWeaponAccuracy = 0;
  int defense = 0;
  int strength = 0;
  int dexterity = 0;
  int speed = 0;
  bool twoHanded = false;
  AttackType attackType = AttackType::HIT;
  double attackTime = 1;
  Optional<ArmorType> armorType;
  double applyTime = 1;
  bool fragile = false;
  Optional<EffectType> effect;
  Optional<int> uses;
  bool usedUpMsg = false;
  bool displayUses = false;
  bool identifyOnApply = true;
  bool identifiable = false;
  bool identifyOnEquip = true;
};

#endif
