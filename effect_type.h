#pragma once

#include "enums.h"
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
#include "creature_predicate.h"
#include "view_id.h"
#include "scripted_ui_data.h"
#include "companion_info.h"
#include "minion_trait.h"
#include "special_attr.h"
#include "tech_id.h"

#define SIMPLE_EFFECT(Name) \
  struct Name { \
    SERIALIZE_EMPTY()\
  }

namespace Effects {
  enum class EnhanceType;
}

RICH_ENUM(Effects::EnhanceType, WEAPON, ARMOR);

namespace Effects {
struct Escape {
  int SERIAL(maxDist) = 10000;
  SERIALIZE_ALL(OPTION(maxDist))
};
SIMPLE_EFFECT(Teleport);
SIMPLE_EFFECT(Jump);
struct Heal {
  HealthType SERIAL(healthType);
  double amount = 1;
  SERIALIZE_ALL(NAMED(healthType), OPTION(amount))
};
SIMPLE_EFFECT(Fire);
SIMPLE_EFFECT(Ice);
SIMPLE_EFFECT(DestroyEquipment);
struct DestroyWalls {
  DestroyAction::Type SERIAL(action);
  SERIALIZE_ALL(action)
};
struct Enhance {
  EnhanceType SERIAL(type);
  int SERIAL(amount);
  const char* typeAsString() const;
  const char* amountAs(const char* positive, const char* negative) const;
  SERIALIZE_ALL(type, amount)
};
struct EmitPoisonGas {
  double SERIAL(amount) = 0.8;
  SERIALIZE_ALL(amount)
};
SIMPLE_EFFECT(CircularBlast);
SIMPLE_EFFECT(Deception);
struct Summon {
  Summon(CreatureId id, Range c) : creature(id), count(c) {}
  Summon() {}
  CreatureId SERIAL(creature);
  Range SERIAL(count);
  optional<int> SERIAL(ttl);
  SERIALIZE_ALL(creature, count, ttl)
};
struct AssembledMinion {
  CreatureId SERIAL(creature);
  EnumSet<MinionTrait> SERIAL(traits);
  vector<Effect> SERIAL(effects);
  SERIALIZE_ALL(NAMED(creature), NAMED(traits), OPTION(effects))
};
struct AddAutomatonParts {
  string getPartsNames(const ContentFactory*) const;
  vector<ItemType> SERIAL(partTypes);
  SERIALIZE_ALL(partTypes)
};
struct SummonEnemy {
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
  bool SERIAL(silent) = false;
  SERIALIZE_ALL(silent)
};

struct Lasting {
  LastingEffect SERIAL(lastingEffect);
  SERIALIZE_ALL(lastingEffect)
};

struct RemoveLasting {
  LastingEffect SERIAL(lastingEffect);
  SERIALIZE_ALL(lastingEffect)
};
struct Permanent {
  LastingEffect SERIAL(lastingEffect);
  SERIALIZE_ALL(lastingEffect)
};
struct RemovePermanent {
  LastingEffect SERIAL(lastingEffect);
  SERIALIZE_ALL(lastingEffect)
};
struct PlaceFurniture {
  FurnitureType SERIAL(furniture);
  SERIALIZE_ALL(furniture)
};
struct Damage {
  AttrType SERIAL(attr);
  AttackType SERIAL(attackType);
  SERIALIZE_ALL(attr, attackType)
};
struct FixedDamage {
  AttrType SERIAL(attr);
  int SERIAL(value);
  AttackType SERIAL(attackType);
  SERIALIZE_ALL(attr, value, attackType)
};
struct IncreaseAttr {
  AttrType SERIAL(attr);
  int SERIAL(amount);
  const char* get(const char* ifIncrease, const char* ifDecrease) const;
  SERIALIZE_ALL(attr, amount)
};
struct IncreaseSkill {
  SkillId SERIAL(skillid);
  double SERIAL(amount);
  const char* get(const char* ifIncrease, const char* ifDecrease) const;
  SERIALIZE_ALL(skillid, amount)
};
struct IncreaseWorkshopSkill {
  WorkshopType SERIAL(workshoptype);
  double SERIAL(amount);
  const char* get(const char* ifIncrease, const char* ifDecrease) const;
  SERIALIZE_ALL(workshoptype, amount)
};
struct InjureBodyPart {
  BodyPart SERIAL(part);
  SERIALIZE_ALL(part)
};
struct LoseBodyPart {
  BodyPart SERIAL(part);
  SERIALIZE_ALL(part)
};
struct AddBodyPart {
  BodyPart SERIAL(part);
  int SERIAL(count);
  optional<ItemType> SERIAL(attack);
  SERIALIZE_ALL(part, count, attack)
};
SIMPLE_EFFECT(MakeHumanoid);
struct GenericModifierEffect {
  HeapAllocated<Effect> SERIAL(effect);
  SERIALIZE_ALL(effect)
};
struct Area : GenericModifierEffect {
  Area() {}
  Area(int r, Effect e) : GenericModifierEffect{std::move(e)}, radius(r) {}
  int SERIAL(radius);
  SERIALIZE_ALL(radius, SUBCLASS(GenericModifierEffect))
};
struct CustomArea : GenericModifierEffect {
  vector<Vec2> SERIAL(positions);
  vector<Position> getTargetPos(const Creature* attacker, Position targetPos) const;
  SERIALIZE_ALL(SUBCLASS(GenericModifierEffect), positions)
};
struct RegrowBodyPart {
  int SERIAL(maxCount);
  SERIALIZE_ALL(maxCount)
};
struct Suicide {
  MsgType SERIAL(message);
  SERIALIZE_ALL(message)
};
SIMPLE_EFFECT(DoubleTrouble);
SIMPLE_EFFECT(Blast);
SIMPLE_EFFECT(Pull);
SIMPLE_EFFECT(Shove);
SIMPLE_EFFECT(SwapPosition);
SIMPLE_EFFECT(Stairs);
struct DirectedBlast {
  Vec2 dir;
  int length;
  SERIALIZE_ALL(dir, length)
};
struct ReviveCorpse {
  vector<CreatureId> SERIAL(summoned);
  int SERIAL(ttl);
  SERIALIZE_ALL(summoned, ttl)
};
struct Filter : GenericModifierEffect {
  Filter() {}
  Filter(CreaturePredicate p, Effect e) : GenericModifierEffect{std::move(e)}, predicate(std::move(p)) {}
  CreaturePredicate SERIAL(predicate);
  SERIALIZE_ALL(predicate, SUBCLASS(GenericModifierEffect))
};
SIMPLE_EFFECT(Wish);
struct Caster {
  HeapAllocated<Effect> SERIAL(effect);
  SERIALIZE_ALL(effect)
};
struct Chain {
  vector<Effect> SERIAL(effects);
  SERIALIZE_ALL(effects)
};
struct ChooseRandom : Chain {
  ChooseRandom() {}
  ChooseRandom(vector<Effect> e) : Chain{std::move(e)} {}
};
struct ChainUntilFail : Chain {
};

struct Message {
  string SERIAL(text);
  MessagePriority SERIAL(priority);
  SERIALIZE_ALL(priority, text)
};
struct UnseenMessage {
  string SERIAL(text);
  MessagePriority SERIAL(priority);
  SERIALIZE_ALL(priority, text)
};
struct CreatureMessage {
  string SERIAL(secondPerson);
  string SERIAL(thirdPerson);
  SERIALIZE_ALL(secondPerson, thirdPerson)
};
struct PlayerMessage {
  string SERIAL(text);
  MessagePriority SERIAL(priority);
  SERIALIZE_ALL(priority, text)
};
struct GrantAbility {
  SpellId SERIAL(id);
  SERIALIZE_ALL(id)
};
struct RemoveAbility {
  SpellId SERIAL(id);
  SERIALIZE_ALL(id)
};
struct IncreaseMorale {
  double SERIAL(amount);
  SERIALIZE_ALL(amount)
};
struct Chance : GenericModifierEffect {
  double SERIAL(value);
  SERIALIZE_ALL(value, SUBCLASS(GenericModifierEffect))
};
SIMPLE_EFFECT(TriggerTrap);
struct AnimateItems {
  int SERIAL(maxCount);
  int SERIAL(radius);
  Range SERIAL(time);
  SERIALIZE_ALL(maxCount, radius, time)
};
struct DropItems {
  ItemType SERIAL(type);
  Range SERIAL(count);
  SERIALIZE_ALL(type, count)
};
struct SoundEffect {
  Sound SERIAL(sound);
  SERIALIZE_ALL(sound)
};
struct Audience {
  optional<int> SERIAL(maxDistance);
  SERIALIZE_ALL(maxDistance)
};
struct FirstSuccessful : Chain {
};
struct ColorVariant {
  Color SERIAL(color);
  SERIALIZE_ALL(color)
};
struct Fx {
  FXInfo SERIAL(info);
  SERIALIZE_ALL(info)
};
struct Description : GenericModifierEffect {
  Description() {}
  Description(string text, Effect e) : GenericModifierEffect{std::move(e)}, text(std::move(text)) {}
  string SERIAL(text);
  SERIALIZE_ALL(text, SUBCLASS(GenericModifierEffect))
};
struct Name : GenericModifierEffect {
  Name() {}
  Name(string text, Effect e) : GenericModifierEffect{std::move(e)}, text(std::move(text)) {}
  string SERIAL(text);
  SERIALIZE_ALL(text, SUBCLASS(GenericModifierEffect))
};
struct AI : GenericModifierEffect {
  CreaturePredicate SERIAL(predicate);
  EffectAIIntent SERIAL(from);
  EffectAIIntent SERIAL(to);
  SERIALIZE_ALL(predicate, from, to, SUBCLASS(GenericModifierEffect))
};
struct ReturnFalse : GenericModifierEffect {
};
using SetMinionActivity = MinionActivity;
using AddMinionTrait = MinionTrait;
struct RemoveMinionTrait {
  MinionTrait SERIAL(trait);
  SERIALIZE_ALL(trait)
};
struct CollectiveMessage {
  string SERIAL(msg);
  SERIALIZE_ALL(msg)
};
struct RemoveFurniture {
  FurnitureType SERIAL(type);
  SERIALIZE_ALL(type)
};
struct SetFlag {
  string SERIAL(name);
  bool SERIAL(value) = true;
  SERIALIZE_ALL(name, value)
};
struct TakeItems {
  string SERIAL(ingredient);
  SERIALIZE_ALL(ingredient)
};
struct Unlock {
  UnlockId SERIAL(id);
  SERIALIZE_ALL(id)
};
struct Analytics {
  string SERIAL(name);
  string SERIAL(value);
  SERIALIZE_ALL(name, value)
};
struct Polymorph {
  optional<CreatureId> SERIAL(into);
  optional<TimeInterval> SERIAL(timeout);
  SERIALIZE_ALL(into, timeout)
};
struct SetCreatureName {
  string SERIAL(value);
  SERIALIZE_ALL(value)
};
struct SetViewId {
  ViewId SERIAL(value);
  SERIALIZE_ALL(value)
};
struct UI {
  ScriptedUIId SERIAL(id);
  ScriptedUIData SERIAL(data);
  SERIALIZE_ALL(id, data)
};
using AddCompanion = CompanionInfo;
SIMPLE_EFFECT(SetPhylactery);
using ::SpecialAttr;
using AddTechnology = TechId;
struct EquipmentType : GenericModifierEffect {
  MinionEquipmentType SERIAL(type);
  SERIALIZE_ALL(type, SUBCLASS(GenericModifierEffect))
};
struct NoAutoAssign : GenericModifierEffect {
};
struct Bleed {
  double amount;
  SERIALIZE_ALL(amount)
};
struct Price : GenericModifierEffect {
  int value;
  SERIALIZE_ALL(value, SUBCLASS(GenericModifierEffect))
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
  X(SpecialAttr, 9)\
  X(CircularBlast, 10)\
  X(Deception, 11)\
  X(Summon, 12)\
  X(SummonElement, 13)\
  X(Acid, 14)\
  X(Alarm, 15)\
  X(Stairs, 16)\
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
  X(DirectedBlast, 33)\
  X(Pull, 34)\
  X(Shove, 35)\
  X(SwapPosition, 36)\
  X(SummonEnemy, 37)\
  X(Audience, 38)\
  X(Wish, 39)\
  X(Chain, 40)\
  X(ChainUntilFail, 41)\
  X(ChooseRandom, 42)\
  X(IncreaseMorale, 43)\
  X(Message, 44)\
  X(UnseenMessage, 45)\
  X(AssembledMinion, 46)\
  X(TriggerTrap, 47)\
  X(AnimateItems, 48)\
  X(MakeHumanoid, 49)\
  X(GrantAbility, 50)\
  X(RemoveAbility, 51)\
  X(CreatureMessage, 52)\
  X(SoundEffect, 53)\
  X(DropItems, 54)\
  X(FirstSuccessful, 55)\
  X(PlayerMessage, 56)\
  X(ColorVariant, 57)\
  X(Jump, 58)\
  X(FixedDamage, 59)\
  X(Fx, 60)\
  X(Description, 61)\
  X(Name, 62)\
  X(AI, 63)\
  X(IncreaseSkill, 64)\
  X(IncreaseWorkshopSkill, 65)\
  X(AddAutomatonParts, 66)\
  X(AddMinionTrait, 67)\
  X(RemoveMinionTrait, 68)\
  X(SetMinionActivity, 69)\
  X(CollectiveMessage, 70)\
  X(TakeItems, 71)\
  X(Unlock, 72)\
  X(Analytics, 73)\
  X(ReturnFalse, 74)\
  X(Polymorph, 75)\
  X(SetCreatureName, 76)\
  X(SetViewId, 77)\
  X(UI, 78)\
  X(AddCompanion, 79)\
  X(SetPhylactery, 80)\
  X(EmitPoisonGas, 81)\
  X(AddTechnology, 82)\
  X(Caster, 83)\
  X(RemoveFurniture, 84)\
  X(SetFlag, 85)\
  X(EquipmentType, 86)\
  X(NoAutoAssign, 87)\
  X(Filter, 88)\
  X(Chance, 89)\
  X(Bleed, 90)\
  X(Price, 91)

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
