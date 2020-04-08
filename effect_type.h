#pragma once

#include "util.h"
#include "effect.h"
#include "creature_id.h"
#include "furniture_type.h"
#include "destroy_action.h"
#include "intrinsic_attack.h"
#include "spell_id.h"
#include "sound.h"
#include "color.h"
#include "fx_info.h"
#include "workshop_type.h"
#include "enums.h"
#include "tech_id.h"

RICH_ENUM(FilterType, ALLY, ENEMY, AUTOMATON);

#define EFFECT_TYPE_INTERFACE \
  bool applyToCreature(Creature*, Creature* attacker = nullptr) const;\
  string getName(const ContentFactory*) const;\
  operator DefaultEffect() const { return DefaultEffect {}; }\
  string getDescription(const ContentFactory*) const


#define SIMPLE_EFFECT(Name) \
  struct Name : public EmptyStruct<Name> { \
    EFFECT_TYPE_INTERFACE;\
  }

namespace Effects {
struct DefaultEffect {};

struct GenericModifierEffect {
  EFFECT_TYPE_INTERFACE;
  HeapAllocated<Effect> SERIAL(effect);
  SERIALIZE_ALL(effect)
};
struct GenericFilterEffect : GenericModifierEffect {
  bool applyToCreature(Creature*, Creature* attacker = nullptr) const;
  bool applies(const Creature* c, const Creature* attacker) const;
};

SIMPLE_EFFECT(Escape);
SIMPLE_EFFECT(Teleport);
SIMPLE_EFFECT(Jump);
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
struct AddAutomatonParts {
  EFFECT_TYPE_INTERFACE;
  string getPartsNames(const ContentFactory*) const;
  vector<ItemType> SERIAL(partTypes);
  SERIALIZE_ALL(partTypes);
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
struct FixedDamage {
  EFFECT_TYPE_INTERFACE;
  AttrType SERIAL(attr);
  int SERIAL(value);
  AttackType SERIAL(attackType);
  SERIALIZE_ALL(attr, value, attackType)
};
struct IncreaseAttr {
  EFFECT_TYPE_INTERFACE;
  AttrType SERIAL(attr);
  int SERIAL(amount);
  const char* get(const char* ifIncrease, const char* ifDecrease) const;
  SERIALIZE_ALL(attr, amount)
};
struct IncreaseSkill {
  EFFECT_TYPE_INTERFACE;
  SkillId SERIAL(skillid);
  double SERIAL(amount);
  const char* get(const char* ifIncrease, const char* ifDecrease) const;
  SERIALIZE_ALL(skillid, amount)
};
struct IncreaseWorkshopSkill {
  EFFECT_TYPE_INTERFACE;
  WorkshopType SERIAL(workshoptype);
  double SERIAL(amount);
  const char* get(const char* ifIncrease, const char* ifDecrease) const;
  SERIALIZE_ALL(workshoptype, amount)
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
// Filter effects filters by FilterTypes.
struct Filter : GenericFilterEffect {
  string getName(const ContentFactory*) const;
  string getDescription(const ContentFactory*) const;
  bool applies(const Creature* c, const Creature* attacker) const;
  FilterType SERIAL(filter);
  SERIALIZE_ALL(filter, SUBCLASS(GenericFilterEffect));
};
struct FilterLasting : GenericFilterEffect {
  string getName(const ContentFactory*) const;
  string getDescription(const ContentFactory*) const;
  bool applies(const Creature* c, const Creature* attacker) const;
  LastingEffect SERIAL(filter);
  SERIALIZE_ALL(filter, SUBCLASS(GenericFilterEffect));
};
struct FilterSpell : GenericFilterEffect {
  string getName(const ContentFactory*) const;
  string getDescription(const ContentFactory*) const;
  bool applies(const Creature* c, const Creature* attacker) const;
  SpellId SERIAL(filter);
  SERIALIZE_ALL(filter, SUBCLASS(GenericFilterEffect));
};
// FilterTech is highly CPU unfriendly.
struct FilterTech : GenericFilterEffect {
  string getName(const ContentFactory*) const;
  string getDescription(const ContentFactory*) const;
  bool applies(const Creature* c, const Creature* attacker) const;
  TechId SERIAL(filter);
  SERIALIZE_ALL(filter, SUBCLASS(GenericFilterEffect));
};
// FilterTrait is highly CPU unfriendly.
struct FilterTrait : GenericFilterEffect {
  string getName(const ContentFactory*) const;
  string getDescription(const ContentFactory*) const;
  bool applies(const Creature* c, const Creature* attacker) const;
  string SERIAL(filter);
  SERIALIZE_ALL(filter, SUBCLASS(GenericFilterEffect));
};
struct FilterHasIngr : GenericFilterEffect {
  string getName(const ContentFactory*) const;
  string getDescription(const ContentFactory*) const;
  bool applies(const Creature* c, const Creature* attacker) const;
  string SERIAL(filter);
  SERIALIZE_ALL(filter, SUBCLASS(GenericFilterEffect));
};
struct FilterEquippedIngr : GenericFilterEffect {
  string getName(const ContentFactory*) const;
  string getDescription(const ContentFactory*) const;
  bool applies(const Creature* c, const Creature* attacker) const;
  string SERIAL(filter);
  SERIALIZE_ALL(filter, SUBCLASS(GenericFilterEffect));
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
struct Chance : GenericModifierEffect {
  string getDescription(const ContentFactory*) const;
  double SERIAL(value);
  SERIALIZE_ALL(value, SUBCLASS(GenericModifierEffect))
};
SIMPLE_EFFECT(TriggerTrap);
struct AnimateItems {
  EFFECT_TYPE_INTERFACE;
  int SERIAL(maxCount);
  int SERIAL(radius);
  Range SERIAL(time);
  SERIALIZE_ALL(maxCount, radius, time)
};
struct DropItems {
  EFFECT_TYPE_INTERFACE;
  ItemType SERIAL(type);
  Range SERIAL(count);
  SERIALIZE_ALL(type, count)
};
struct SoundEffect {
  EFFECT_TYPE_INTERFACE;
  Sound SERIAL(sound);
  SERIALIZE_ALL(sound)
};
struct Audience {
  EFFECT_TYPE_INTERFACE;
  optional<int> SERIAL(maxDistance);
  SERIALIZE_ALL(maxDistance)
};
struct FirstSuccessful {
  EFFECT_TYPE_INTERFACE;
  vector<Effect> SERIAL(effects);
  SERIALIZE_ALL(effects)
};
struct ChainFirstResult {
  EFFECT_TYPE_INTERFACE;
  vector<Effect> SERIAL(effects);
  SERIALIZE_ALL(effects)
};
struct ColorVariant {
  EFFECT_TYPE_INTERFACE;
  Color SERIAL(color);
  SERIALIZE_ALL(color)
};
struct Fx {
  EFFECT_TYPE_INTERFACE;
  FXInfo SERIAL(info);
  SERIALIZE_ALL(info)
};
struct Description : GenericModifierEffect {
  string getDescription(const ContentFactory*) const;
  string SERIAL(text);
  SERIALIZE_ALL(text, SUBCLASS(GenericModifierEffect))
};
struct Name : GenericModifierEffect {
  string getName(const ContentFactory*) const;
  string SERIAL(text);
  SERIALIZE_ALL(text, SUBCLASS(GenericModifierEffect))
};
struct AIBelowHealth : GenericModifierEffect {
  double SERIAL(value);
  SERIALIZE_ALL(value, SUBCLASS(GenericModifierEffect))
};
struct AITargetEnemy : GenericModifierEffect {
};

#define EFFECT_TYPES_LIST\
  X(Escape, 0)\
  X(Teleport, 1)\
  X(Heal, 2)\
  X(Fire, 3)\
  X(Ice, 4)\
  X(DestroyEquipment, 5)\
  X(Enhance, 6)\
  X(Suicide, 7)\
  X(IncreaseAttr, 8)\
  X(EmitPoisonGas, 9)\
  X(CircularBlast, 10)\
  X(Deception, 11)\
  X(Summon, 12)\
  X(SummonElement, 13)\
  X(Acid, 14)\
  X(Alarm, 15)\
  X(SilverDamage, 16)\
  X(DoubleTrouble, 17)\
  X(Lasting, 18)\
  X(RemoveLasting, 19)\
  X(Permanent, 20)\
  X(RemovePermanent, 21)\
  X(PlaceFurniture, 22)\
  X(Damage, 23)\
  X(InjureBodyPart, 24)\
  X(LoseBodyPart, 25)\
  X(RegrowBodyPart, 26)\
  X(AddBodyPart, 27)\
  X(DestroyWalls, 28)\
  X(Area, 29)\
  X(CustomArea, 30)\
  X(ReviveCorpse, 31)\
  X(Blast, 32)\
  X(Pull, 33)\
  X(Shove, 34)\
  X(SwapPosition, 35)\
  X(Filter, 36)\
  X(SummonEnemy, 37)\
  X(Audience, 38)\
  X(Wish, 39)\
  X(Chain, 40)\
  X(ChooseRandom, 41)\
  X(Caster, 42)\
  X(IncreaseMorale, 43)\
  X(Message, 44)\
  X(Chance, 45)\
  X(AssembledMinion, 46)\
  X(TriggerTrap, 47)\
  X(AnimateItems, 48)\
  X(MakeHumanoid, 49)\
  X(GrantAbility, 50)\
  X(CreatureMessage, 51)\
  X(SoundEffect, 52)\
  X(DropItems, 53)\
  X(FirstSuccessful, 54)\
  X(ChainFirstResult, 55)\
  X(ColorVariant, 56)\
  X(Jump, 57)\
  X(FixedDamage, 58)\
  X(Fx, 59)\
  X(Description, 60)\
  X(Name, 61)\
  X(AIBelowHealth, 62)\
  X(AITargetEnemy, 63)\
  X(IncreaseSkill, 64)\
  X(IncreaseWorkshopSkill, 65)\
  X(FilterLasting, 66)\
  X(AddAutomatonParts, 67)\
  X(FilterSpell, 68)\
  X(FilterTech, 69)\
  X(FilterTrait, 70)\
  X(FilterHasIngr, 71)\
  X(FilterEquippedIngr, 72)\

#define VARIANT_TYPES_LIST EFFECT_TYPES_LIST
#define VARIANT_NAME EffectType

#include "gen_variant.h"

#undef VARIANT_TYPES_LIST
#undef VARIANT_NAME

template <class Archive>
void serialize(Archive& ar1, EffectType& v);

}

class EffectType : public Effects::EffectType {
  public:
  using Effects::EffectType::EffectType;
};
