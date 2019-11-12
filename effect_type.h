#pragma once

#include "util.h"
#include "effect.h"
#include "creature_id.h"
#include "furniture_type.h"

RICH_ENUM(FilterType, ALLY, ENEMY);

#define EFFECT_TYPE_INTERFACE \
  void applyToCreature(Creature*, Creature* attacker = nullptr) const;\
  string getName() const;\
  string getDescription() const


#define SIMPLE_EFFECT(Name) \
  struct Name { \
    EFFECT_TYPE_INTERFACE;\
    COMPARE_ALL()\
  }

namespace Effects {

SIMPLE_EFFECT(Escape);
SIMPLE_EFFECT(Teleport);
struct Heal {
  EFFECT_TYPE_INTERFACE;
  HealthType healthType;
  COMPARE_ALL(healthType)
};
SIMPLE_EFFECT(Fire);
SIMPLE_EFFECT(Ice);
SIMPLE_EFFECT(DestroyEquipment);
SIMPLE_EFFECT(DestroyWalls);
struct Enhance {
  EFFECT_TYPE_INTERFACE;
  ItemUpgradeType type;
  int amount;
  const char* typeAsString() const;
  const char* amountAs(const char* positive, const char* negative) const;
  COMPARE_ALL(type, amount)
};
struct EmitPoisonGas {
  EFFECT_TYPE_INTERFACE;
  double amount = 0.8;
  COMPARE_ALL(amount)
};
SIMPLE_EFFECT(CircularBlast);
SIMPLE_EFFECT(Deception);
struct Summon {
  EFFECT_TYPE_INTERFACE;
  Summon(CreatureId id, Range c) : creature(id), count(c) {}
  Summon() {}
  CreatureId creature;
  Range count;
  COMPARE_ALL(creature, count)
};
struct SummonEnemy {
  EFFECT_TYPE_INTERFACE;
  SummonEnemy(CreatureId id, Range c) : creature(id), count(c) {}
  SummonEnemy() {}
  CreatureId creature;
  Range count;
  COMPARE_ALL(creature, count)
};
SIMPLE_EFFECT(SummonElement);
SIMPLE_EFFECT(Acid);
struct Alarm {
  EFFECT_TYPE_INTERFACE;
  bool silent = false;
  COMPARE_ALL(silent)
};
SIMPLE_EFFECT(TeleEnemies);
SIMPLE_EFFECT(SilverDamage);

struct Lasting {
  EFFECT_TYPE_INTERFACE;
  LastingEffect lastingEffect;
  COMPARE_ALL(lastingEffect)
};

struct RemoveLasting {
  EFFECT_TYPE_INTERFACE;
  LastingEffect lastingEffect;
  COMPARE_ALL(lastingEffect)
};

struct Permanent {
  EFFECT_TYPE_INTERFACE;
  LastingEffect lastingEffect;
  COMPARE_ALL(lastingEffect)
};
struct PlaceFurniture {
  EFFECT_TYPE_INTERFACE;
  FurnitureType furniture;
  COMPARE_ALL(furniture)
};
struct Damage {
  EFFECT_TYPE_INTERFACE;
  AttrType attr;
  AttackType attackType;
  COMPARE_ALL(attr, attackType)
};
struct IncreaseAttr {
  EFFECT_TYPE_INTERFACE;
  AttrType attr;
  int amount;
  const char* get(const char* ifIncrease, const char* ifDecrease) const;
  COMPARE_ALL(attr, amount)
};
struct InjureBodyPart {
  EFFECT_TYPE_INTERFACE;
  BodyPart part;
  COMPARE_ALL(part)
};
struct LooseBodyPart {
  EFFECT_TYPE_INTERFACE;
  BodyPart part;
  COMPARE_ALL(part)
};
struct Area {
  EFFECT_TYPE_INTERFACE;
  int radius;
  HeapAllocated<Effect> effect;
  COMPARE_ALL(radius, effect)
};
struct CustomArea {
  EFFECT_TYPE_INTERFACE;
  HeapAllocated<Effect> effect;
  vector<Vec2> positions;
  vector<Position> getTargetPos(const Creature* attacker, Position targetPos) const;
  COMPARE_ALL(effect, positions)
};
SIMPLE_EFFECT(RegrowBodyPart);
struct Suicide {
  EFFECT_TYPE_INTERFACE;
  MsgType message;
  COMPARE_ALL(message)
};
SIMPLE_EFFECT(DoubleTrouble);
SIMPLE_EFFECT(Blast);
SIMPLE_EFFECT(Pull);
SIMPLE_EFFECT(Shove);
SIMPLE_EFFECT(SwapPosition);
struct ReviveCorpse {
  EFFECT_TYPE_INTERFACE;
  vector<CreatureId> summoned;
  int ttl;
  COMPARE_ALL(summoned, ttl)
};
struct Filter {
  EFFECT_TYPE_INTERFACE;
  bool applies(const Creature* victim, const Creature* attacker) const;
  FilterType filter;
  HeapAllocated<Effect> effect;
  COMPARE_ALL(filter, effect)
};
SIMPLE_EFFECT(Wish);
struct Caster {
  EFFECT_TYPE_INTERFACE;
  HeapAllocated<Effect> effect;
  COMPARE_ALL(effect)
};
struct Chain {
  EFFECT_TYPE_INTERFACE;
  vector<Effect> effects;
  COMPARE_ALL(effects)
};
struct Message {
  EFFECT_TYPE_INTERFACE;
  string text;
  COMPARE_ALL(text)
};
struct IncreaseMorale {
  EFFECT_TYPE_INTERFACE;
  double amount;
  COMPARE_ALL(amount)
};
struct Chance {
  EFFECT_TYPE_INTERFACE;
  double value;
  HeapAllocated<Effect> effect;
  COMPARE_ALL(value, effect)
};
MAKE_VARIANT2(EffectType, Escape, Teleport, Heal, Fire, Ice, DestroyEquipment, Enhance, Suicide, IncreaseAttr,
    EmitPoisonGas, CircularBlast, Deception, Summon, SummonElement, Acid, Alarm, TeleEnemies, SilverDamage, DoubleTrouble,
    Lasting, RemoveLasting, Permanent, PlaceFurniture, Damage, InjureBodyPart, LooseBodyPart, RegrowBodyPart, DestroyWalls,
    Area, CustomArea, ReviveCorpse, Blast, Pull, Shove, SwapPosition, Filter, SummonEnemy, Wish, Chain, Caster,
    IncreaseMorale, Message, Chance);
}

class EffectType : public Effects::EffectType {
  public:
  using Effects::EffectType::EffectType;
};
