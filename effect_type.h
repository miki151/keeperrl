#pragma once

#include "util.h"
#include "effect.h"
#include "creature_id.h"
#include "furniture_type.h"
#include "destroy_action.h"
#include "intrinsic_attack.h"
#include "spell_id.h"
#include "sound.h"

RICH_ENUM(FilterType, ALLY, ENEMY);

#define EFFECT_TYPE_INTERFACE \
  void applyToCreature(Creature*, Creature* attacker = nullptr) const;\
  string getName(const ContentFactory*) const;\
  string getDescription(const ContentFactory*) const


#define SIMPLE_EFFECT(Name) \
  struct Name : public EmptyStruct<Name> { \
    EFFECT_TYPE_INTERFACE;\
  }

namespace Effects {

SIMPLE_EFFECT(Escape);
SIMPLE_EFFECT(Teleport);
struct Heal {
  EFFECT_TYPE_INTERFACE;
  HealthType SERIAL(healthType);
  SERIALIZE_ALL(healthType)
};
SIMPLE_EFFECT(Fire);
SIMPLE_EFFECT(Ice);
SIMPLE_EFFECT(DestroyEquipment);
struct DestroyWalls {
  EFFECT_TYPE_INTERFACE;
  DestroyAction::Type SERIAL(action);
  SERIALIZE_ALL(action)
};
struct Enhance {
  EFFECT_TYPE_INTERFACE;
  ItemUpgradeType SERIAL(type);
  int SERIAL(amount);
  const char* typeAsString() const;
  const char* amountAs(const char* positive, const char* negative) const;
  SERIALIZE_ALL(type, amount)
};
struct EmitPoisonGas {
  EFFECT_TYPE_INTERFACE;
  double SERIAL(amount) = 0.8;
  SERIALIZE_ALL(amount)
};
SIMPLE_EFFECT(CircularBlast);
SIMPLE_EFFECT(Deception);
struct Summon {
  EFFECT_TYPE_INTERFACE;
  Summon(CreatureId id, Range c) : creature(id), count(c) {}
  Summon() {}
  CreatureId SERIAL(creature);
  Range SERIAL(count);
  optional<int> SERIAL(ttl);
  SERIALIZE_ALL(creature, count, ttl)
};
struct AssembledMinion {
  EFFECT_TYPE_INTERFACE;
  CreatureId SERIAL(creature);
  SERIALIZE_ALL(creature)
};
struct SummonEnemy {
  EFFECT_TYPE_INTERFACE;
  SummonEnemy(CreatureId id, Range c) : creature(id), count(c) {}
  SummonEnemy() {}
  CreatureId SERIAL(creature);
  Range SERIAL(count);
  optional<int> SERIAL(ttl);
  SERIALIZE_ALL(creature, count, ttl)
};
SIMPLE_EFFECT(SummonElement);
SIMPLE_EFFECT(Acid);
struct Alarm {
  EFFECT_TYPE_INTERFACE;
  bool SERIAL(silent) = false;
  SERIALIZE_ALL(silent)
};
SIMPLE_EFFECT(TeleEnemies);
SIMPLE_EFFECT(SilverDamage);

struct Lasting {
  EFFECT_TYPE_INTERFACE;
  LastingEffect SERIAL(lastingEffect);
  SERIALIZE_ALL(lastingEffect)
};

struct RemoveLasting {
  EFFECT_TYPE_INTERFACE;
  LastingEffect SERIAL(lastingEffect);
  SERIALIZE_ALL(lastingEffect)
};
struct Permanent {
  EFFECT_TYPE_INTERFACE;
  LastingEffect SERIAL(lastingEffect);
  SERIALIZE_ALL(lastingEffect)
};
struct RemovePermanent {
  EFFECT_TYPE_INTERFACE;
  LastingEffect SERIAL(lastingEffect);
  SERIALIZE_ALL(lastingEffect)
};
struct PlaceFurniture {
  EFFECT_TYPE_INTERFACE;
  FurnitureType SERIAL(furniture);
  SERIALIZE_ALL(furniture)
};
struct Damage {
  EFFECT_TYPE_INTERFACE;
  AttrType SERIAL(attr);
  AttackType SERIAL(attackType);
  SERIALIZE_ALL(attr, attackType)
};
struct IncreaseAttr {
  EFFECT_TYPE_INTERFACE;
  AttrType SERIAL(attr);
  int SERIAL(amount);
  const char* get(const char* ifIncrease, const char* ifDecrease) const;
  SERIALIZE_ALL(attr, amount)
};
struct InjureBodyPart {
  EFFECT_TYPE_INTERFACE;
  BodyPart SERIAL(part);
  SERIALIZE_ALL(part)
};
struct LoseBodyPart {
  EFFECT_TYPE_INTERFACE;
  BodyPart SERIAL(part);
  SERIALIZE_ALL(part)
};
struct AddBodyPart {
  EFFECT_TYPE_INTERFACE;
  BodyPart SERIAL(part);
  int SERIAL(count);
  optional<ItemType> SERIAL(attack);
  SERIALIZE_ALL(part, count, attack)
};
SIMPLE_EFFECT(MakeHumanoid);
struct Area {
  EFFECT_TYPE_INTERFACE;
  int SERIAL(radius);
  HeapAllocated<Effect> SERIAL(effect);
  SERIALIZE_ALL(radius, effect)
};
struct CustomArea {
  EFFECT_TYPE_INTERFACE;
  HeapAllocated<Effect> SERIAL(effect);
  vector<Vec2> SERIAL(positions);
  vector<Position> getTargetPos(const Creature* attacker, Position targetPos) const;
  SERIALIZE_ALL(effect, positions)
};
struct RegrowBodyPart {
  EFFECT_TYPE_INTERFACE;
  int SERIAL(maxCount);
  SERIALIZE_ALL(maxCount)
};
struct Suicide {
  EFFECT_TYPE_INTERFACE;
  MsgType SERIAL(message);
  SERIALIZE_ALL(message)
};
SIMPLE_EFFECT(DoubleTrouble);
SIMPLE_EFFECT(Blast);
SIMPLE_EFFECT(Pull);
SIMPLE_EFFECT(Shove);
SIMPLE_EFFECT(SwapPosition);
struct ReviveCorpse {
  EFFECT_TYPE_INTERFACE;
  vector<CreatureId> SERIAL(summoned);
  int SERIAL(ttl);
  SERIALIZE_ALL(summoned, ttl)
};
struct Filter {
  EFFECT_TYPE_INTERFACE;
  bool applies(bool isEnemy) const;
  FilterType SERIAL(filter);
  HeapAllocated<Effect> SERIAL(effect);
  SERIALIZE_ALL(filter, effect)
};
SIMPLE_EFFECT(Wish);
struct Caster {
  EFFECT_TYPE_INTERFACE;
  HeapAllocated<Effect> SERIAL(effect);
  SERIALIZE_ALL(effect)
};
struct Chain {
  EFFECT_TYPE_INTERFACE;
  vector<Effect> SERIAL(effects);
  SERIALIZE_ALL(effects)
};
struct ChooseRandom {
  EFFECT_TYPE_INTERFACE;
  vector<Effect> SERIAL(effects);
  SERIALIZE_ALL(effects)
};
struct Message {
  EFFECT_TYPE_INTERFACE;
  string SERIAL(text);
  SERIALIZE_ALL(text)
};
struct CreatureMessage {
  EFFECT_TYPE_INTERFACE;
  string SERIAL(secondPerson);
  string SERIAL(thirdPerson);
  SERIALIZE_ALL(secondPerson, thirdPerson)
};
struct GrantAbility {
  EFFECT_TYPE_INTERFACE;
  SpellId SERIAL(id);
  SERIALIZE_ALL(id)
};
struct IncreaseMorale {
  EFFECT_TYPE_INTERFACE;
  double SERIAL(amount);
  SERIALIZE_ALL(amount)
};
struct Chance {
  EFFECT_TYPE_INTERFACE;
  double SERIAL(value);
  HeapAllocated<Effect> SERIAL(effect);
  SERIALIZE_ALL(value, effect)
};
SIMPLE_EFFECT(TriggerTrap);
struct AnimateItems {
  EFFECT_TYPE_INTERFACE;
  int SERIAL(maxCount);
  int SERIAL(radius);
  Range SERIAL(time);
  SERIALIZE_ALL(maxCount, radius, time)
};
struct SoundEffect {
  EFFECT_TYPE_INTERFACE;
  Sound SERIAL(sound);
  SERIALIZE_ALL(sound)
};
MAKE_VARIANT2(EffectType, Escape, Teleport, Heal, Fire, Ice, DestroyEquipment, Enhance, Suicide, IncreaseAttr,
    EmitPoisonGas, CircularBlast, Deception, Summon, SummonElement, Acid, Alarm, TeleEnemies, SilverDamage, DoubleTrouble,
    Lasting, RemoveLasting, Permanent, RemovePermanent, PlaceFurniture, Damage, InjureBodyPart, LoseBodyPart, RegrowBodyPart,
    AddBodyPart, DestroyWalls, Area, CustomArea, ReviveCorpse, Blast, Pull, Shove, SwapPosition, Filter, SummonEnemy, Wish,
    Chain, ChooseRandom, Caster, IncreaseMorale, Message, Chance, AssembledMinion, TriggerTrap, AnimateItems, MakeHumanoid,
    GrantAbility, CreatureMessage, SoundEffect);
}

class EffectType : public Effects::EffectType {
  public:
  using Effects::EffectType::EffectType;
};
