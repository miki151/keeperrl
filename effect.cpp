/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "stdafx.h"
#include "effect.h"
#include "controller.h"
#include "creature.h"
#include "level.h"
#include "creature_factory.h"
#include "creature_group.h"
#include "item.h"
#include "view_object.h"
#include "view_id.h"
#include "game.h"
#include "model.h"
#include "monster_ai.h"
#include "attack.h"
#include "player_message.h"
#include "equipment.h"
#include "creature_attributes.h"
#include "creature_name.h"
#include "position_map.h"
#include "sound.h"
#include "attack_level.h"
#include "attack_type.h"
#include "body.h"
#include "game_event.h"
#include "item_class.h"
#include "furniture_factory.h"
#include "furniture.h"
#include "movement_set.h"
#include "weapon_info.h"
#include "fx_name.h"
#include "draw_line.h"
#include "monster.h"
#include "spell_map.h"
#include "content_factory.h"
#include "effect_type.h"
#include "minion_equipment_type.h"
#include "health_type.h"
#include "collective.h"
#include "immigration.h"
#include "immigrant_info.h"
#include "furniture_entry.h"
#include "territory.h"
#include "vision.h"
#include "workshop_type.h"
#include "automaton_part.h"
#include "minion_trait.h"
#include "unlocks.h"
#include "view.h"
#include "player.h"

namespace {
struct DefaultType {
  template <typename T>
  DefaultType(const T&) {}
};
}

static bool isConsideredInDanger(const Creature* c) {
  if (auto intent = c->getLastCombatIntent())
    return (intent->time > *c->getGlobalTime() - 5_visible);
  return false;
}

static void summonFX(Position pos) {
  auto color = Color(240, 146, 184);
  pos.getGame()->addEvent(EventInfo::FX{pos, {FXName::SPAWN, color}});
}

vector<Creature*> Effect::summonCreatures(Position pos, vector<PCreature> creatures, TimeInterval delay) {
  vector<Creature*> ret;
  for (int i : All(creatures))
    if (auto v = pos.getLevel()->getClosestLanding({pos}, creatures[i].get())) {
      ret.push_back(creatures[i].get());
      v->addCreature(std::move(creatures[i]), delay);
      summonFX(*v);
    }
  return ret;
}

void Effect::emitPoisonGas(Position pos, double amount, bool msg) {
  PROFILE;
  for (Position v : pos.neighbors8())
    v.addPoisonGas(amount / 2);
  pos.addPoisonGas(amount);
  if (msg) {
    pos.globalMessage("A cloud of gas is released");
    pos.unseenMessage("You hear a hissing sound");
  }
}

vector<Creature*> Effect::summon(Creature* c, CreatureId id, int num, optional<TimeInterval> ttl, TimeInterval delay,
    optional<Position> position) {
  vector<PCreature> creatures;
  for (int i : Range(num))
    creatures.push_back(c->getGame()->getContentFactory()->getCreatures().fromId(id, c->getTribeId(),
        MonsterAIFactory::summoned(c)));
  auto ret = summonCreatures(position.value_or(c->getPosition()), std::move(creatures), delay);
  for (auto c : ret)
    if (ttl)
      c->addEffect(LastingEffect::SUMMONED, *ttl, false);
  return ret;
}

vector<Creature*> Effect::summon(Position pos, CreatureGroup& factory, int num, optional<TimeInterval> ttl,
    TimeInterval delay) {
  vector<PCreature> creatures;
  for (int i : Range(num))
    creatures.push_back(factory.random(&pos.getGame()->getContentFactory()->getCreatures(), MonsterAIFactory::monster()));
  auto ret = summonCreatures(pos, std::move(creatures), delay);
  for (auto c : ret)
    if (ttl)
      c->addEffect(LastingEffect::SUMMONED, *ttl, false);
  return ret;
}

static bool enhanceArmor(Creature* c, int mod, const string& msg) {
  for (EquipmentSlot slot : Random.permutation(getKeys(Equipment::slotTitles)))
    for (Item* item : c->getEquipment().getSlotItems(slot))
      if (item->getClass() == ItemClass::ARMOR) {
        c->you(MsgType::YOUR, item->getName() + " " + msg);
        if (item->getModifier(AttrType::DEFENSE) > 0 || mod > 0)
          item->addModifier(AttrType::DEFENSE, mod);
        return true;
      }
  return false;
}

static bool enhanceWeapon(Creature* c, int mod, const string& msg) {
  if (auto item = c->getFirstWeapon()) {
    c->you(MsgType::YOUR, item->getName() + " " + msg);
    item->addModifier(item->getWeaponInfo().meleeAttackAttr, mod);
    return true;
  }
  return false;
}

static bool summon(Creature* summoner, CreatureId id, Range count, bool hostile, optional<TimeInterval> ttl) {
  if (hostile) {
    CreatureGroup f = CreatureGroup::singleType(TribeId::getHostile(), id);
    return !Effect::summon(summoner->getPosition(), f, Random.get(count), ttl, 1_visible).empty();
  } else
    return !Effect::summon(summoner, id, Random.get(count), ttl, 1_visible).empty();
}

static bool applyToCreature(const Effects::Escape& e, Creature* c, Creature*) {
  PROFILE_BLOCK("Escape::applyToCreature");
  Rectangle area = Rectangle::centered(Vec2(0, 0), 12);
  PositionMap<int> weight;
  queue<Position> q;
  auto addDanger = [&] (Position pos) {
    q.push(pos);
    weight.set(pos, 0);
  };
  for (Position v : c->getPosition().getRectangle(area)) {
    if (v.isBurning())
      addDanger(v);
    else if (auto other = v.getCreature())
      if (other->isEnemy(c))
        addDanger(v);
  }
  while (!q.empty()) {
    Position v = q.front();
    q.pop();
    for (Position w : v.neighbors8())
      if (w.canEnterEmpty({MovementTrait::WALK}) && !weight.contains(w)) {
        weight.set(w, weight.getOrFail(v) + 1);
        q.push(w);
      }
  }
  vector<Position> good;
  int maxW = 0;
  auto movementType = c->getMovementType();
  for (Position v : c->getPosition().getRectangle(area)) {
    if (!v.canEnter(c) || v.isBurning() || v.getPoisonGasAmount() > 0 ||
        !v.isConnectedTo(c->getPosition(), movementType) || *v.dist8(c->getPosition()) > e.maxDist)
      continue;
    if (auto weightV = weight.getValueMaybe(v)) {
      if (*weightV == maxW)
        good.push_back(v);
      else if (*weightV > maxW) {
        good = {v};
        maxW = *weightV;
      }
    }
  }
  if (maxW < 2) {
    c->message("The spell didn't work.");
    return false;
  }
  CHECK(!good.empty());
  c->you(MsgType::TELE_DISAPPEAR, "");
  c->getPosition().moveCreature(Random.choose(good), true);
  c->you(MsgType::TELE_APPEAR, "");
  return true;
}

static optional<MinionEquipmentType> getMinionEquipmentType(const Effects::Escape&) {
  return MinionEquipmentType::COMBAT_ITEM;
}

static string getName(const Effects::Escape&, const ContentFactory*) {
  return "escape";
}

static string getDescription(const Effects::Escape&, const ContentFactory*) {
  return "Teleports to a safer location close by.";
}

static string getName(const Effects::Teleport&, const ContentFactory*) {
  return "teleport";
}

static string getDescription(const Effects::Teleport&, const ContentFactory*) {
  return "Teleport to any location that's close by.";
}

static bool apply(const Effects::Teleport&, Position pos, Creature* attacker) {
  if (attacker->getPosition().canMoveCreature(pos)) {
    attacker->you(MsgType::TELE_DISAPPEAR, "");
    attacker->getPosition().moveCreature(pos, true);
    attacker->you(MsgType::TELE_APPEAR, "");
    return true;
  }
  return false;
}

static string getName(const Effects::SetPhylactery&, const ContentFactory*) {
  return "phylactery";
}

static string getDescription(const Effects::SetPhylactery&, const ContentFactory*) {
  return "";
}

static bool apply(const Effects::SetPhylactery&, Position pos, Creature* attacker) {
  if (attacker)
    if (auto f = pos.getFurniture(FurnitureLayer::MIDDLE)) {
      attacker->setPhylactery(pos, f->getType());
      return true;
    }
  return false;
}

static string getName(const Effects::AddTechnology& t, const ContentFactory* f) {
  return t.data();
}

static string getDescription(const Effects::AddTechnology& t, const ContentFactory*) {
  return "Provides "_s + t.data();
}

static bool apply(const Effects::AddTechnology& t, Position pos, Creature* attacker) {
  pos.getGame()->addEvent(EventInfo::TechbookRead{t});
  return true;
}

static string getName(const Effects::Jump&, const ContentFactory*) {
  return "jumping";
}

static string getDescription(const Effects::Jump&, const ContentFactory*) {
  return "Jump!";
}

static bool apply(const Effects::Jump&, Position pos, Creature* attacker) {
  auto from = attacker->getPosition();
  if (pos.canEnter(attacker)) {
    for (auto v : drawLine(from, pos))
      if (v != from && !v.canEnter(MovementType({MovementTrait::WALK, MovementTrait::FLY})))
        return false;
    attacker->displace(attacker->getPosition().getDir(pos));
    return true;
  }
  return false;
}

static bool applyToCreature(const Effects::Lasting& e, Creature* c, Creature*) {
  return c->addEffect(e.lastingEffect, LastingEffects::getDuration(c, e.lastingEffect));
}

static bool isConsideredHostile(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::BLIND:
    case LastingEffect::ENTANGLED:
    case LastingEffect::HALLU:
    case LastingEffect::INSANITY:
    case LastingEffect::PANIC:
    case LastingEffect::POISON:
    case LastingEffect::SLOWED:
    case LastingEffect::STUNNED:
      return true;
    default:
      return false;
  }
}

static bool isConsideredHostile(const Effects::Lasting& e, const Creature* victim) {
  return LastingEffects::affects(victim, e.lastingEffect) && ::isConsideredHostile(e.lastingEffect);
}

static optional<MinionEquipmentType> getMinionEquipmentType(const Effects::Lasting& e) {
  return MinionEquipmentType::COMBAT_ITEM;
}

static EffectAIIntent shouldAIApplyToCreature(const Effects::Lasting& e, const Creature* victim, bool isEnemy) {
  if (victim->isAffected(e.lastingEffect))
    return 0;
  return LastingEffects::shouldAIApply(victim, e.lastingEffect, isEnemy);
}

static optional<FXInfo> getProjectileFX(LastingEffect effect) {
  switch (effect) {
    default:
      return none;
  }
}

static optional<FXInfo> getProjectileFX(const Effects::Lasting& e) {
  return ::getProjectileFX(e.lastingEffect);
}

static optional<ViewId> getProjectile(LastingEffect effect) {
  switch (effect) {
    default:
      return none;
  }
}

static optional<ViewId> getProjectile(const Effects::Lasting& e) {
  return ::getProjectile(e.lastingEffect);
}

static Color getColor(const Effects::Lasting& e, const ContentFactory* f) {
  return LastingEffects::getColor(e.lastingEffect);
}

static string getName(const Effects::Lasting& e, const ContentFactory*) {
  return LastingEffects::getName(e.lastingEffect);
}

static string getDescription(const Effects::Lasting& e, const ContentFactory*) {
  // Leave out the full stop.
  string desc = LastingEffects::getDescription(e.lastingEffect);
  return desc.substr(0, desc.size() - 1) + " for some turns.";
}

static bool applyToCreature(const Effects::RemoveLasting& e, Creature* c, Creature*) {
  return c->removeEffect(e.lastingEffect);
}

static string getName(const Effects::RemoveLasting& e, const ContentFactory*) {
  return "remove " + LastingEffects::getName(e.lastingEffect);
}

static string getDescription(const Effects::RemoveLasting& e, const ContentFactory*) {
  return "Removes/cures from effect: " + LastingEffects::getName(e.lastingEffect);
}

static EffectAIIntent shouldAIApplyToCreature(const Effects::RemoveLasting& e, const Creature* victim, bool isEnemy) {
  if (!victim->isAffected(e.lastingEffect))
    return 0;
  return -LastingEffects::shouldAIApply(victim, e.lastingEffect, isEnemy);
}

static bool applyToCreature(const Effects::IncreaseAttr& e, Creature* c, Creature*) {
  c->you(MsgType::YOUR, ::getName(e.attr) + e.get(" improves", " wanes"));
  c->getAttributes().increaseBaseAttr(e.attr, e.amount);
  return true;
}

static string getName(const Effects::IncreaseAttr& e, const ContentFactory*) {
  return ::getName(e.attr) + e.get(" boost", " loss");
}

static string getDescription(const Effects::IncreaseAttr& e, const ContentFactory*) {
  return e.get("Increases", "Decreases") + " "_s + ::getName(e.attr) + " by " + toString(abs(e.amount));
}

const char* Effects::IncreaseAttr::get(const char* ifIncrease, const char* ifDecrease) const {
  if (amount > 0)
    return ifIncrease;
  else
    return ifDecrease;
}

static const char* get(const Effects::SpecialAttr& a, const char* ifIncrease, const char* ifDecrease) {
  if (a.value > 0)
    return ifIncrease;
  else
    return ifDecrease;
}

static bool applyToCreature(const Effects::SpecialAttr& e, Creature* c, Creature*) {
  c->you(MsgType::YOUR, ::getName(e.attr) + " " + e.predicate.getName() + get(e, " improves", " wanes"));
  c->getAttributes().specialAttr[e.attr].push_back(make_pair(e.value, e.predicate));
  return true;
}

static string getName(const Effects::SpecialAttr& e, const ContentFactory*) {
  return ::getName(e.attr) + get(e, " boost", " loss") + " " + e.predicate.getName();
}

static string getDescription(const Effects::SpecialAttr& e, const ContentFactory*) {
  return get(e, "Increases", "Decreases") + " "_s + ::getName(e.attr)
      + " " + e.predicate.getName() + " by " + toString(abs(e.value));
}

static bool applyToCreature(const Effects::IncreaseSkill& e, Creature* c, Creature*) {
  c->you(MsgType::YOUR, ::getName(e.skillid) + e.get(" skill improves", " skill wanes"));
  c->getAttributes().getSkills().increaseValue(e.skillid, e.amount);
  return true;
}

static string getName(const Effects::IncreaseSkill& e, const ContentFactory*) {
  return ::getName(e.skillid) + e.get(" proficency boost", " proficency loss");
}

static string getDescription(const Effects::IncreaseSkill& e, const ContentFactory*) {
  return e.get("Increases", "Decreases") + " "_s + ::getName(e.skillid) + " by " + toString(fabs(e.amount));
}

const char* Effects::IncreaseSkill::get(const char* ifIncrease, const char* ifDecrease) const {
  if (amount > 0)
    return ifIncrease;
  else
    return ifDecrease;
}

static bool applyToCreature(const Effects::IncreaseWorkshopSkill& e, Creature* c, Creature*) {
  c->you(MsgType::YOUR, c->getGame()->getContentFactory()->workshopInfo.at(e.workshoptype).name +
      e.get(" proficiency improves", " proficiency wanes"));
  c->getAttributes().getSkills().increaseValue(e.workshoptype, e.amount);
  return true;
}

static string getName(const Effects::IncreaseWorkshopSkill& e, const ContentFactory* content_factory) {
  return content_factory->workshopInfo.at(e.workshoptype).name + e.get(" proficency boost", " proficency loss");
}

static string getDescription(const Effects::IncreaseWorkshopSkill& e, const ContentFactory* content_factory) {
  return e.get("Increases", "Decreases") + " "_s + content_factory->workshopInfo.at(e.workshoptype).name +
      " proficiency by " + toString(fabs(e.amount));
}

const char* Effects::IncreaseWorkshopSkill::get(const char* ifIncrease, const char* ifDecrease) const {
  if (amount > 0)
    return ifIncrease;
  else
    return ifDecrease;
}

static bool applyToCreature(const Effects::AddCompanion& e, Creature* c, Creature*) {
  c->getAttributes().companions.push_back(e);
  return true;
}

static string getName(const Effects::AddCompanion& e, const ContentFactory*) {
  return "companion";
}

static string getDescription(const Effects::AddCompanion& e, const ContentFactory* f) {
  return "Grants a permanent " + f->getCreatures().getName(e.creatures[0]) + " companion";
}

static bool applyToCreature(const Effects::Permanent& e, Creature* c, Creature*) {
  return c->addPermanentEffect(e.lastingEffect);
}

static EffectAIIntent shouldAIApplyToCreature(const Effects::Permanent& e, const Creature* victim, bool isEnemy) {
  if (victim->getAttributes().isAffectedPermanently(e.lastingEffect))
    return 0;
  return LastingEffects::shouldAIApply(victim, e.lastingEffect, isEnemy);
}

static string getName(const Effects::Permanent& e, const ContentFactory*) {
  return "permanent " + LastingEffects::getName(e.lastingEffect);
}

static string getDescription(const Effects::Permanent& e, const ContentFactory*) {
  string desc = LastingEffects::getDescription(e.lastingEffect);
  return desc.substr(0, desc.size() - 1) + " permanently.";
}

static Color getColor(const Effects::Permanent& e, const ContentFactory* f) {
  return LastingEffects::getColor(e.lastingEffect);
}

static bool applyToCreature(const Effects::RemovePermanent& e, Creature* c, Creature*) {
  return c->removePermanentEffect(e.lastingEffect);
}

static string getName(const Effects::RemovePermanent& e, const ContentFactory*) {
  return "remove permanent " + LastingEffects::getName(e.lastingEffect);
}

static string getDescription(const Effects::RemovePermanent& e, const ContentFactory*) {
  string desc = LastingEffects::getDescription(e.lastingEffect);
  return "Removes/cures from " + desc.substr(0, desc.size() - 1) + " permanently.";
}

static bool applyToCreature(const Effects::Alarm& e, Creature* c, Creature*) {
  c->getGame()->addEvent(EventInfo::Alarm{c->getPosition(), e.silent});
  return true;
}

static string getName(const Effects::Alarm&, const ContentFactory*) {
  return "alarm";
}

static string getDescription(const Effects::Alarm&, const ContentFactory*) {
  return "Alarm!";
}

static string getName(const Effects::Acid&, const ContentFactory*) {
  return "acid";
}

static string getDescription(const Effects::Acid&, const ContentFactory*) {
  return "Causes acid damage to skin and equipment.";
}

static Color getColor(const Effects::Acid&, const ContentFactory* f) {
  return Color::YELLOW;
}

static bool apply(const Effects::Acid&, Position pos, Creature*) {
  return pos.acidDamage();
}

static EffectAIIntent shouldAIApplyToCreature(const Effects::Acid&, const Creature* victim, bool isEnemy) {
  return isEnemy ? 1 : -1;
}

static bool isConsideredHostile(const Effects::Acid&, const Creature* victim) {
  return !victim->isAffected(LastingEffect::ACID_RESISTANT);
}

static bool applyToCreature(const Effects::Summon& e, Creature* c, Creature*) {
  return ::summon(c, e.creature, e.count, false, e.ttl.map([](int v) { return TimeInterval(v); }));
}

static EffectAIIntent shouldAIApplyToCreature(const Effects::Summon&, const Creature* victim, bool isEnemy) {
  return isConsideredInDanger(victim) ? 1 : 0;
}

static string getName(const Effects::Summon& e, const ContentFactory* f) {
  return "summon " + f->getCreatures().getName(e.creature);
}

static string getDescription(const Effects::Summon& e, const ContentFactory* f) {
  if (e.count.getEnd() > 2)
    return "Summons " + toString(e.count.getStart()) + " to " + toString(e.count.getEnd() - 1) + " "
        + f->getCreatures().getNamePlural(e.creature);
  else
    return "Summons a " + f->getCreatures().getName(e.creature);
}

static string getName(const Effects::AssembledMinion& e, const ContentFactory* f) {
  return f->getCreatures().getName(e.creature);
}

static string getDescription(const Effects::AssembledMinion& e, const ContentFactory* f) {
  return "";// "Can be assembled to a " + getName(e, f);
}

static bool apply(const Effects::AssembledMinion& m, Position pos, Creature* attacker) {
  auto group = CreatureGroup::singleType(attacker->getTribeId(), m.creature);
  auto c = Effect::summon(pos, group, 1, none).getFirstElement();
  if (c) {
    for (auto& e : m.effects)
      e.apply((*c)->getPosition(), attacker);
    for (auto col : pos.getGame()->getCollectives())
      if (col->getCreatures().contains(attacker)) {
        col->addCreature(*c, m.traits);
        for (auto& part : (*c)->getAttributes().automatonParts)
          (*c)->addAutomatonPart(*part.get((*c)->getGame()->getContentFactory())->getAutomatonPart());
        return true;
      }
  }
  return false;
}

string Effects::AddAutomatonParts::getPartsNames(const ContentFactory* f) const {
  string ret;
  for (auto& item : partTypes) {
    if (!ret.empty())
      ret += ", ";
    ret += item.get(f)->getName();
  }
  return ret;
}

static bool applyToCreature(const Effects::AddAutomatonParts& e, Creature* c, Creature*) {
  CHECK(e.partTypes.size() > 0);
  if (c->getSpareAutomatonSlots() < e.partTypes.size())
    return false;
  for (auto& item : e.partTypes)
    c->addAutomatonPart(*item.get(c->getGame()->getContentFactory())->getAutomatonPart());
  return true;
}

static string getName(const Effects::AddAutomatonParts& e, const ContentFactory* f) {
  return "attach " + e.getPartsNames(f);
}

static string getDescription(const Effects::AddAutomatonParts& e, const ContentFactory* f) {
  return "Attaches " + e.getPartsNames(f) + " to the creature.";
}

static string getName(const Effects::SummonEnemy& e, const ContentFactory* f) {
  return "summon hostile " + f->getCreatures().getName(e.creature);
}

static string getDescription(const Effects::SummonEnemy& e, const ContentFactory* f) {
  if (e.count.getEnd() > 2)
    return "Summons " + toString(e.count.getStart()) + " to " + toString(e.count.getEnd() - 1) + " hostile " +
        getName(e, f);
  else
    return "Summons a hostile " + getName(e, f);
}

static bool apply(const Effects::SummonEnemy& summon, Position pos, Creature*) {
  CreatureGroup f = CreatureGroup::singleType(TribeId::getMonster(), summon.creature);
  return !Effect::summon(pos, f, Random.get(summon.count), summon.ttl.map([](int v) { return TimeInterval(v); }), 1_visible).empty();
}

static bool applyToCreature(const Effects::SummonElement&, Creature* c, Creature*) {
  auto id = CreatureId("AIR_ELEMENTAL");
  for (Position p : c->getPosition().getRectangle(Rectangle::centered(3)))
    for (auto f : p.getFurniture())
      if (auto elem = f->getSummonedElement())
        id = *elem;
  return ::summon(c, id, Range(1, 2), false, 100_visible);
}

static string getName(const Effects::SummonElement&, const ContentFactory*) {
  return "summon element";
}

static string getDescription(const Effects::SummonElement&, const ContentFactory*) {
  return "Summons an element or spirit from the surroundings.";
}

static EffectAIIntent shouldAIApplyToCreature(const Effects::SummonElement&, const Creature* victim, bool isEnemy) {
  return isConsideredInDanger(victim) ? 1 : 0;
}

static bool applyToCreature(const Effects::Deception&, Creature* c, Creature*) {
  vector<PCreature> creatures;
  for (int i : Range(Random.get(3, 7)))
    creatures.push_back(CreatureFactory::getIllusion(c));
  return !Effect::summonCreatures(c->getPosition(), std::move(creatures)).empty();
}

static string getName(const Effects::Deception&, const ContentFactory*) {
  return "deception";
}

static string getDescription(const Effects::Deception&, const ContentFactory*) {
  return "Creates multiple illusions of the spellcaster to confuse the enemy.";
}

static EffectAIIntent shouldAIApplyToCreature(const Effects::Deception&, const Creature* victim, bool isEnemy) {
  return isConsideredInDanger(victim) ? 1 : 0;
}

static void airBlast(Creature* attacker, Position origin, Position position, Position target) {
  CHECK(target != origin);
  Vec2 direction = origin.getDir(target);
  constexpr int maxDistance = 4;
  while (direction.length8() < maxDistance * 3)
    direction += origin.getDir(target);
  auto trajectory = drawLine(Vec2(0, 0), direction);
  for (int i : All(trajectory))
    if (trajectory[i] == origin.getDir(position)) {
      trajectory = trajectory.getSubsequence(i + 1, maxDistance);
      for (auto& v : trajectory)
        v = v - origin.getDir(position);
      break;
    }
  CHECK(trajectory.size() == maxDistance);
  if (Creature* c = position.getCreature()) {
    optional<Position> target;
    for (auto& v : trajectory)
      if (position.canMoveCreature(v))
        target = position.plus(v);
      else
        break;
    if (target) {
      c->displace(c->getPosition().getDir(*target));
      c->you(MsgType::ARE, "thrown back");
    }
    c->addEffect(LastingEffect::COLLAPSED, 2_visible);
  }
  for (auto& stack : Item::stackItems(position.getItems())) {
    position.throwItem(
        position.removeItems(stack),
        Attack(attacker, Random.choose<AttackLevel>(),
          stack[0]->getWeaponInfo().attackType, 15, AttrType::DAMAGE), maxDistance,
          position.plus(trajectory.back()), VisionId::NORMAL);
  }
  for (auto furniture : position.modFurniture())
    if (furniture->canDestroy(DestroyAction::Type::BASH))
      furniture->destroy(position, DestroyAction::Type::BASH);
}

static bool applyToCreature(const Effects::CircularBlast&, Creature* c, Creature* attacker) {
  for (Vec2 v : Vec2::directions8(Random))
    airBlast(attacker, c->getPosition(), c->getPosition().plus(v), c->getPosition().plus(v * 10));
  c->addFX({FXName::CIRCULAR_BLAST});
  return true;
}

static string getName(const Effects::CircularBlast&, const ContentFactory*) {
  return "air blast";
}

static string getDescription(const Effects::CircularBlast&, const ContentFactory*) {
  return "Creates a circular blast of air that throws back creatures and items.";
}

const char* Effects::Enhance::typeAsString() const {
  switch (type) {
    case EnhanceType::WEAPON:
      return "weapon";
    case EnhanceType::ARMOR:
      return "armor";
  }
}

const char* Effects::Enhance::amountAs(const char* positive, const char* negative) const {
  return amount > 0 ? positive : negative;
}

static bool applyToCreature(const Effects::Enhance& e, Creature* c, Creature*) {
  switch (e.type) {
    case Effects::EnhanceType::WEAPON:
      return enhanceWeapon(c, e.amount, e.amountAs("is improved", "degrades"));
    case Effects::EnhanceType::ARMOR:
      return enhanceArmor(c, e.amount, e.amountAs("is improved", "degrades"));
  }
}

static string getName(const Effects::Enhance& e, const ContentFactory*) {
  return e.typeAsString() + " "_s + e.amountAs("enchantment", "degradation");
}

static string getDescription(const Effects::Enhance& e, const ContentFactory*) {
  return e.amountAs("Increases", "Decreases") + " "_s + e.typeAsString() + " capability"_s;
}

static bool applyToCreature(const Effects::DestroyEquipment&, Creature* c, Creature*) {
  auto equipped = c->getEquipment().getAllEquipped();
  if (!equipped.empty()) {
    Item* dest = Random.choose(equipped);
    c->you(MsgType::YOUR, dest->getName() + " crumbles to dust.");
    c->steal({dest});
    return true;
  }
  return false;
}

static EffectAIIntent shouldAIApplyToCreature(const Effects::DestroyEquipment&, const Creature* victim, bool isEnemy) {
  return isEnemy ? 1 : -1;
}

static bool isConsideredHostile(const Effects::DestroyEquipment&, const Creature*) {
  return true;
}

static string getName(const Effects::DestroyEquipment&, const ContentFactory*) {
  return "equipment destruction";
}

static string getDescription(const Effects::DestroyEquipment&, const ContentFactory*) {
  return "Destroys a random piece of equipment.";
}

static string getName(const Effects::DestroyWalls&, const ContentFactory*) {
  return "destruction";
}

static string getDescription(const Effects::DestroyWalls&, const ContentFactory*) {
  return "Destroys terrain and objects.";
}

static bool apply(const Effects::DestroyWalls& m, Position pos, Creature* attacker) {
  bool res = false;
  for (auto furniture : pos.modFurniture())
    if (furniture->canDestroy(m.action)) {
      furniture->destroy(pos, m.action, attacker);
      res = true;
    }
  return res;
}

static string getName(const Effects::RemoveFurniture& e, const ContentFactory* c) {
  return "remove " + c->furniture.getData(e.type).getName();
}

static string getDescription(const Effects::RemoveFurniture& e, const ContentFactory* c) {
  return "Removes " + c->furniture.getData(e.type).getName();
}

static bool apply(const Effects::RemoveFurniture& e, Position pos, Creature*) {
  if (auto f = pos.getFurniture(e.type)) {
    pos.removeFurniture(f->getLayer());
    return true;
  }
  return false;
}

static bool applyToCreature(const Effects::Heal& e, Creature* c, Creature*) {
  if (c->getBody().canHeal(e.healthType)) {
    bool res = false;
    res |= c->heal(1);
    res |= c->removeEffect(LastingEffect::BLEEDING);
    c->addFX(FXInfo(FXName::CIRCULAR_SPELL, Color::LIGHT_GREEN));
    return res;
  } else {
    c->message("Nothing happens.");
    return false;
  }
}

static optional<MinionEquipmentType> getMinionEquipmentType(const Effects::Heal& e) {
  if (e.healthType == HealthType::FLESH)
    return MinionEquipmentType::HEALING;
  else
    return MinionEquipmentType::MATERIALIZATION;
}

static EffectAIIntent shouldAIApplyToCreature(const Effects::Heal& e, const Creature* victim, bool isEnemy) {
  if (victim->getBody().canHeal(e.healthType))
    return isEnemy ? -1 : 1;
  return 0;
}

static string getName(const Effects::Heal& e, const ContentFactory*) {
  switch (e.healthType) {
    case HealthType::FLESH: return "healing";
    case HealthType::SPIRIT: return "materialization";
  }
}

static string getDescription(const Effects::Heal& e, const ContentFactory*) {
  switch (e.healthType) {
    case HealthType::FLESH: return "Fully restores health.";
    case HealthType::SPIRIT: return "Fully re-materializes a spirit.";
  }
}

static Color getColor(const Effects::Heal& e, const ContentFactory* f) {
  switch (e.healthType) {
    case HealthType::FLESH: return Color::RED;
    case HealthType::SPIRIT: return Color::PURPLE;
  }
}

static string getName(const Effects::Fire&, const ContentFactory*) {
  return "fire";
}

static string getDescription(const Effects::Fire&, const ContentFactory*) {
  return "Burns!";
}

static bool apply(const Effects::Fire&, Position pos, Creature*) {
  pos.getGame()->addEvent(EventInfo::FX{pos, {FXName::FIREBALL_SPLASH}});
  return pos.fireDamage(1);
}

static optional<ViewId> getProjectile(const Effects::Fire&) {
  return ViewId("fireball");
}

static EffectAIIntent shouldAIApplyToCreature(const Effects::Fire&, const Creature* victim, bool isEnemy) {
  if (!victim->isAffected(LastingEffect::FIRE_RESISTANT))
    return isEnemy ? 1 : -1;
  return 0;
}

static bool isConsideredHostile(const Effects::Fire&, const Creature* victim) {
  return !victim->isAffected(LastingEffect::FIRE_RESISTANT);
}

static string getName(const Effects::Ice&, const ContentFactory*) {
  return "ice";
}

static string getDescription(const Effects::Ice&, const ContentFactory*) {
  return "Freezes water and causes cold damage";
}

static bool apply(const Effects::Ice&, Position pos, Creature*) {
  return pos.iceDamage();
}

static EffectAIIntent shouldAIApplyToCreature(const Effects::Ice&, const Creature* victim, bool isEnemy) {
  if (!victim->isAffected(LastingEffect::COLD_RESISTANT))
    return isEnemy ? 1 : -1;
  return 0;
}

static bool isConsideredHostile(const Effects::Ice&, const Creature* victim) {
  return !victim->isAffected(LastingEffect::COLD_RESISTANT);
}

static string getName(const Effects::ReviveCorpse&, const ContentFactory*) {
  return "revive corpse";
}

static string getDescription(const Effects::ReviveCorpse&, const ContentFactory*) {
  return "Brings a dead corpse back alive as a servant";
}

static bool apply(const Effects::ReviveCorpse& effect, Position pos, Creature* attacker) {
  for (auto& item : pos.getItems())
    if (auto info = item->getCorpseInfo())
      if (info->canBeRevived)
        for (auto& dead : pos.getModel()->getDeadCreatures())
          if (dead->getUniqueId() == info->victim) {
            for (auto id : effect.summoned) {
              auto summoned = Effect::summon(attacker, id, 1, TimeInterval(effect.ttl));
              if (!summoned.empty()) {
                for (auto& c : summoned) {
                  c->getName().addBarePrefix(dead->getName().bare());
                  attacker->message("You have revived " + c->getName().a());
                }
                pos.removeItems({item});
                return true;
              }
            }
            attacker->message("The spell failed");
            return false;
          }
  return false;
}

static string getName(const Effects::EmitPoisonGas&, const ContentFactory*) {
  return "poison gas";
}

static string getDescription(const Effects::EmitPoisonGas&, const ContentFactory*) {
  return "Emits poison gas";
}

static bool apply(const Effects::EmitPoisonGas& m, Position pos, Creature*) {
  Effect::emitPoisonGas(pos, m.amount, true);
  return true;
}

static EffectAIIntent shouldAIApplyToCreature(const Effects::EmitPoisonGas&, const Creature* victim, bool isEnemy) {
  return isEnemy ? 1 : -1;
}

static string getName(const Effects::PlaceFurniture& e, const ContentFactory* c) {
  return c->furniture.getData(e.furniture).getName();
}

static string getDescription(const Effects::PlaceFurniture& e, const ContentFactory* c) {
  return "Creates a " + getName(e, c);
}

static bool apply(const Effects::PlaceFurniture& summon, Position pos, Creature* attacker) {
  auto f = pos.getGame()->getContentFactory()->furniture.getFurniture(summon.furniture,
      attacker ? attacker->getTribeId() : TribeId::getMonster());
  auto layer = f->getLayer();
  auto ref = f.get();
  pos.addFurniture(std::move(f));
  if (pos.getFurniture(layer) == ref)
    ref->onConstructedBy(pos, attacker);
  return true;
}

static EffectAIIntent shouldAIApplyToCreature(const Effects::PlaceFurniture& f, const Creature* victim, bool isEnemy) {
  return victim->getGame()->getContentFactory()->furniture.getData(f.furniture).isHostileSpell() &&
      isConsideredInDanger(victim) ? 1 : 0;
}

static string getName(const Effects::DropItems&, const ContentFactory* c) {
  return "create items";
}

static string getDescription(const Effects::DropItems&, const ContentFactory* c) {
  return "Creates items";
}

static bool apply(const Effects::DropItems& effect, Position pos, Creature*) {
  pos.dropItems(effect.type.get(Random.get(effect.count), pos.getGame()->getContentFactory()));
  return true;
}

static bool applyToCreature(const Effects::Damage& e, Creature* c, Creature* attacker) {
  CHECK(attacker) << "Unknown attacker";
  int value = attacker->getAttr(e.attr) + attacker->getSpecialAttr(e.attr, c);
  bool result = c->takeDamage(Attack(attacker, Random.choose<AttackLevel>(), e.attackType, value, e.attr), true);
  if (e.attr == AttrType::SPELL_DAMAGE)
    c->addFX({FXName::MAGIC_MISSILE_SPLASH});
  return result;
}

static EffectAIIntent shouldAIApplyToCreature(const Effects::Damage&, const Creature* victim, bool isEnemy) {
  return isEnemy ? 1 : -1;
}

static optional<FXInfo> getProjectileFX(const Effects::Damage&) {
  return {FXName::MAGIC_MISSILE};
}

static optional<ViewId> getProjectile(const Effects::Damage&) {
  return ViewId("force_bolt");
}

static bool isConsideredHostile(const Effects::Damage&, const Creature*) {
  return true;
}

static string getName(const Effects::Damage& e, const ContentFactory*) {
  return ::getName(e.attr);
}

static string getDescription(const Effects::Damage& e, const ContentFactory*) {
  return "Causes " + ::getName(e.attr);
}

static bool applyToCreature(const Effects::FixedDamage& e, Creature* c, Creature*) {
  bool result = c->takeDamage(Attack(nullptr, Random.choose<AttackLevel>(), e.attackType, e.value, e.attr), true);
  if (e.attr == AttrType::SPELL_DAMAGE)
    c->addFX({FXName::MAGIC_MISSILE_SPLASH});
  return result;
}

static string getName(const Effects::FixedDamage& e, const ContentFactory*) {
  return toString(e.value) + " " + ::getName(e.attr);
}

static string getDescription(const Effects::FixedDamage& e, const ContentFactory*) {
  return "Causes " + toString(e.value) + " " + ::getName(e.attr);
}

static bool applyToCreature(const Effects::InjureBodyPart& e, Creature* c, Creature* attacker) {
  if (c->getBody().injureBodyPart(c, e.part, false)) {
    c->you(MsgType::DIE, "");
    c->dieWithAttacker(attacker);
  }
  return false;
}

static string getName(const Effects::InjureBodyPart& e, const ContentFactory*) {
  return "injure "_s + ::getName(e.part);
}

static string getDescription(const Effects::InjureBodyPart& e, const ContentFactory*) {
  return "Injures "_s + ::getName(e.part);
}

static bool applyToCreature(const Effects::LoseBodyPart& e, Creature* c, Creature* attacker) {
  if (c->getBody().injureBodyPart(c, e.part, true)) {
    c->you(MsgType::DIE, "");
    c->dieWithAttacker(attacker);
  }
  return false;
}

static string getName(const Effects::LoseBodyPart& e, const ContentFactory*) {
  return "lose "_s + ::getName(e.part);
}

static string getDescription(const Effects::LoseBodyPart& e, const ContentFactory*) {
  return "Causes you to lose a "_s + ::getName(e.part);
}

static bool applyToCreature(const Effects::AddBodyPart& p, Creature* c, Creature* attacker) {
  c->getBody().addBodyPart(p.part, p.count);
  if (p.attack) {
    c->getBody().addIntrinsicAttack(p.part, IntrinsicAttack{*p.attack, true});
    c->getBody().initializeIntrinsicAttack(c->getGame()->getContentFactory());
  }
  return true;
}

static string getName(const Effects::AddBodyPart& e, const ContentFactory*) {
  return "extra "_s + (e.count > 1 ? makePlural(::getName(e.part)) : string(::getName(e.part)));
}

static string getDescription(const Effects::AddBodyPart& e, const ContentFactory*) {
  return "Adds "_s + getPlural(::getName(e.part), e.count);
}

static bool applyToCreature(const Effects::MakeHumanoid&, Creature* c, Creature*) {
  bool ret = !c->getBody().isHumanoid();
  c->getBody().setHumanoid(true);
  return ret;
}

static string getName(const Effects::MakeHumanoid&, const ContentFactory*) {
  return "turn into a humanoid";
}

static string getDescription(const Effects::MakeHumanoid&, const ContentFactory*) {
  return "Turns creature into a humanoid";
}

static bool applyToCreature(const Effects::RegrowBodyPart& e, Creature* c, Creature*) {
  return c->getBody().healBodyParts(c, e.maxCount);
}

static string getName(const Effects::RegrowBodyPart&, const ContentFactory*) {
  return "regrow lost body parts";
}

static string getDescription(const Effects::RegrowBodyPart&, const ContentFactory*) {
  return "Causes lost body parts to regrow.";
}

static EffectAIIntent shouldAIApplyToCreature(const Effects::RegrowBodyPart&, const Creature* victim, bool isEnemy) {
  for (auto part : ENUM_ALL(BodyPart))
    if (victim->getBody().numLost(part) + victim->getBody().numInjured(part) > 0)
      return isEnemy ? -1 : 1;
  return 0;
}

static string getDescription(const Effects::Area& e, const ContentFactory* factory) {
  return "Area effect of radius " + toString(e.radius) + ": " + noCapitalFirst(e.effect->getDescription(factory));
}

static bool apply(const Effects::Area& area, Position pos, Creature* attacker) {
  bool res = false;
  for (auto v : pos.getRectangle(Rectangle::centered(area.radius)))
    res |= area.effect->apply(v, attacker);
  return res;
}

template <typename Range>
EffectAIIntent considerArea(const Creature* caster, const Range& range, const Effect& effect) {
  auto allRes = 0;
  auto badRes = 0;
  for (auto v : range) {
    auto res = effect.shouldAIApply(caster, v);
    if (res < 0)
      badRes += res;
    else
      allRes += res;
  }
  return badRes < 0 ? badRes : allRes;
};

static EffectAIIntent shouldAIApply(const Effects::Area& a, const Creature* caster, Position pos) {
  return considerArea(caster, pos.getRectangle(Rectangle::centered(a.radius)), *a.effect);
}

static string getName(const Effects::CustomArea& e, const ContentFactory* f) {
  return "custom area " + e.effect->getName(f);
}

static string getDescription(const Effects::CustomArea& e, const ContentFactory* factory) {
  return "Custom area effect: " + noCapitalFirst(e.effect->getDescription(factory));
}

static Vec2 rotate(Vec2 v, Vec2 r) {
  return Vec2(v.x * r.x - v.y * r.y, v.x * r.y + v.y * r.x);
}

vector<Position> Effects::CustomArea::getTargetPos(const Creature* attacker, Position targetPos) const {
  Vec2 orientation = Vec2(1, 0);
  if (attacker)
    orientation = attacker->getPosition().getDir(targetPos).getBearing();
  vector<Position> ret;
  for (auto v : positions)
    ret.push_back(targetPos.plus(rotate(v, orientation)));
  return ret;
}

static EffectAIIntent shouldAIApply(const Effects::CustomArea& a, const Creature* caster, Position pos) {
  return considerArea(caster, a.getTargetPos(caster, pos), *a.effect);
}

static bool apply(const Effects::CustomArea& area, Position pos, Creature* attacker) {
  bool res = false;
  for (auto& v : area.getTargetPos(attacker, pos))
    res |= area.effect->apply(v, attacker);
  return res;
}

static bool applyToCreature(const Effects::Suicide& e, Creature* c, Creature*) {
  c->you(e.message, "");
  c->dieNoReason();
  return true;
}

static bool canAutoAssignMinionEquipment(const Effects::Suicide&) {
  return false;
}

static EffectAIIntent shouldAIApplyToCreature(const Effects::Suicide&, const Creature* victim, bool isEnemy) {
  return isEnemy ? 1 : -1;
}

static string getName(const Effects::Suicide&, const ContentFactory*) {
  return "suicide";
}

static string getDescription(const Effects::Suicide&, const ContentFactory*) {
  return "Causes the *attacker* to die.";
}

static bool applyToCreature(const Effects::Wish&, Creature* c, Creature* attacker) {
  c->getController()->grantWish(
      (attacker ? attacker->getName().the() + " grants you a wish." : "You are granted a wish.") +
      " What do you wish for?");
  return true;
}

static EffectAIIntent shouldAIApply(const Effects::Wish&, const Creature* caster, Position pos) {
  auto victim = pos.getCreature();
  if (victim && victim->isPlayer() && !caster->isEnemy(victim))
    return 1;
  else
    return -1;
}

static string getName(const Effects::Wish&, const ContentFactory*) {
  return "wishing";
}

static string getDescription(const Effects::Wish&, const ContentFactory*) {
  return "Gives you one wish.";
}

static string combineNames(const ContentFactory* f, const vector<Effect>& effects) {
  if (effects.empty())
    return "";
  string ret;
  for (auto& e : effects)
    ret += e.getName(f) + ", ";
  ret.pop_back();
  ret.pop_back();
  return ret;
}

static string combineDescriptions(const ContentFactory* f, const vector<Effect>& effects) {
  if (effects.empty())
    return "";
  string ret;
  for (auto& e : effects) {
    auto desc = e.getDescription(f);
    if (!ret.empty())
      desc = noCapitalFirst(std::move(desc));
    ret += std::move(desc);
    ret += ", ";
  }
  ret.pop_back();
  ret.pop_back();
  return ret;
}

static string getName(const Effects::Chain& e, const ContentFactory* f) {
  return combineNames(f, e.effects);
}

static string getDescription(const Effects::Chain& e, const ContentFactory* f) {
  return combineDescriptions(f, e.effects);
}

static bool apply(const Effects::Chain& chain, Position pos, Creature* attacker) {
  bool res = false;
  for (auto& e : chain.effects)
    res |= e.apply(pos, attacker);
  return res;
}

static optional<MinionEquipmentType> getMinionEquipmentType(const Effects::Chain& c) {
  for (auto& e : c.effects)
    if (auto t = e.getMinionEquipmentType())
      return t;
  return none;
}

static bool canAutoAssignMinionEquipment(const Effects::Chain& c) {
  for (auto& e : c.effects)
    if (!e.canAutoAssignMinionEquipment())
      return false;
  return true;
}

static EffectAIIntent shouldAIApply(const Effects::Chain& chain, const Creature* caster, Position pos) {
  auto allRes = 0;
  auto badRes = 0;
  for (auto& e : chain.effects) {
    auto res = e.shouldAIApply(caster, pos);
    if (res < 0)
      badRes += res;
    else
      allRes += res;
  }
  return badRes < 0 ? badRes : allRes;
}

static bool apply(const Effects::ChainUntilFail& chain, Position pos, Creature* attacker) {
  bool res = false;
  for (auto& e : chain.effects) {
    auto b = e.apply(pos, attacker);
    res |= b;
    if (!b)
      break;
  }
  return res;
}

static string getName(const Effects::FirstSuccessful& e, const ContentFactory* f) {
  return "try: " + combineNames(f, e.effects);
}

static string getDescription(const Effects::FirstSuccessful& e, const ContentFactory* f) {
  return "First successful: " + combineDescriptions(f, e.effects);
}

static bool apply(const Effects::FirstSuccessful& chain, Position pos, Creature* attacker) {
  for (auto& e : chain.effects)
    if (e.apply(pos, attacker))
      return true;
  return false;
}

static string getName(const Effects::ChooseRandom& e, const ContentFactory* f) {
  return e.effects[0].getName(f);
}

static string getDescription(const Effects::ChooseRandom& e, const ContentFactory* f) {
  return e.effects[0].getDescription(f);
}

static bool apply(const Effects::ChooseRandom& r, Position pos, Creature* attacker) {
  return r.effects[Random.get(r.effects.size())].apply(pos, attacker);
}

static string getName(const Effects::Message&, const ContentFactory*) {
  return "message";
}

static string getDescription(const Effects::Message& e, const ContentFactory*) {
  return e.text;
}

static bool apply(const Effects::Message& msg, Position pos, Creature*) {
  pos.globalMessage(msg.text);
  return true;
}

static string getName(const Effects::CreatureMessage&, const ContentFactory*) {
  return "message";
}

static string getDescription(const Effects::CreatureMessage&, const ContentFactory*) {
  return "Custom message";
}

static bool applyToCreature(const Effects::CreatureMessage& e, Creature* c, Creature*) {
  c->verb(e.secondPerson, e.thirdPerson);
  return true;
}

static string getName(const Effects::PlayerMessage&, const ContentFactory*) {
  return "message";
}

static string getDescription(const Effects::PlayerMessage&, const ContentFactory*) {
  return "Custom message";
}

static bool applyToCreature(const Effects::PlayerMessage& e, Creature* c, Creature*) {
  c->privateMessage(::PlayerMessage(e.text, e.priority));
  return true;
}

static bool applyToCreature(const Effects::GrantAbility& e, Creature* c, Creature*) {
  bool ret = !c->getSpellMap().contains(e.id);
  c->getSpellMap().add(*c->getGame()->getContentFactory()->getCreatures().getSpell(e.id), ExperienceType::MELEE, 0);
  return ret;
}

static string getName(const Effects::GrantAbility& e, const ContentFactory* f) {
  return "grant "_s + f->getCreatures().getSpell(e.id)->getName(f);
}

static string getDescription(const Effects::GrantAbility& e, const ContentFactory* f) {
  return "Grants ability: "_s + f->getCreatures().getSpell(e.id)->getName(f);
}

static bool applyToCreature(const Effects::Polymorph& e, Creature* c, Creature*) {
  auto& factory = c->getGame()->getContentFactory()->getCreatures();
  auto attributes = [&] {
    if (e.into)
      return factory.getAttributesFromId(*e.into);
    for (auto id : Random.permutation(factory.getAllCreatures())) {
      auto attr = factory.getAttributesFromId(id);
      if (attr.getBody().getMaterial() == BodyMaterial::FLESH && !attr.isAffectedPermanently(LastingEffect::PLAGUE))
        return attr;
    }
    fail();
  }();
  auto spells = factory.getSpellMap(attributes);
  auto origName = c->getName().the();
  if (e.timeout) {
    c->pushAttributes(std::move(attributes), std::move(spells));
    c->addEffect(LastingEffect::POLYMORPHED, *e.timeout);
  } else
    c->setAttributes(std::move(attributes), std::move(spells));
  c->secondPerson("You turn into " + c->getName().a() + "!");
  c->thirdPerson(origName + " turns into " + c->getName().a() + "!");
  summonFX(c->getPosition());
  return true;
}

static string getName(const Effects::Polymorph& e, const ContentFactory* f) {
  return !!e.timeout ? "temporary polymorph" : "permanent polymorph";
}

static string getDescription(const Effects::Polymorph& e, const ContentFactory* f) {
  return "Polymorphs into a " + (e.into ? f->getCreatures().getName(*e.into) : "random creature"_s);
}

static bool applyToCreature(const Effects::SetCreatureName& e, Creature* c, Creature*) {
  c->getName().setBare(e.value);
  return true;
}

static string getName(const Effects::SetCreatureName& e, const ContentFactory* f) {
  return "change name";
}

static string getDescription(const Effects::SetCreatureName& e, const ContentFactory* f) {
  return "Changes creature's name to \"" + e.value + "\"";
}

static bool applyToCreature(const Effects::SetViewId& e, Creature* c, Creature*) {
  c->modViewObject().setId(e.value);
  return true;
}

static string getName(const Effects::SetViewId& e, const ContentFactory* f) {
  return "set sprite";
}

static string getDescription(const Effects::SetViewId& e, const ContentFactory* f) {
  return "Changes creature's sprite";
}

static bool apply(const Effects::UI& e, Position pos, Creature*) {
  auto view = pos.getGame()->getView();
  if (auto c = pos.getCreature())
    if (c->isPlayer())
      view->updateView(dynamic_cast<Player*>(c->getController()), true);
  ScriptedUIState state;
  view->scriptedUI(e.id, e.data, state);
  return true;
}

static string getName(const Effects::UI& e, const ContentFactory* f) {
  return "UI";
}

static string getDescription(const Effects::UI& e, const ContentFactory* f) {
  return "Displays a UI";
}

static bool applyToCreature(const Effects::RemoveAbility& e, Creature* c, Creature*) {
  bool ret = c->getSpellMap().contains(e.id);
  c->getSpellMap().remove(e.id);
  return ret;
}

static string getName(const Effects::RemoveAbility& e, const ContentFactory* f) {
  return "remove "_s + f->getCreatures().getSpell(e.id)->getName(f);
}

static string getDescription(const Effects::RemoveAbility& e, const ContentFactory* f) {
  return "Removes ability: "_s + f->getCreatures().getSpell(e.id)->getName(f);
}

static bool applyToCreature(const Effects::IncreaseMorale& e, Creature* c, Creature*) {
  if (e.amount > 0)
    c->you(MsgType::YOUR, "spirits are lifted");
  else
    c->you(MsgType::ARE, "disheartened");
  if (auto before = c->getMorale()) {
    c->addMorale(e.amount);
    return c->getMorale() != *before;
  }
  return false;
}

static optional<MinionEquipmentType> getMinionEquipmentType(const Effects::IncreaseMorale&) {
  return MinionEquipmentType::COMBAT_ITEM;
}

static EffectAIIntent shouldAIApplyToCreature(const Effects::IncreaseMorale& e, const Creature* victim, bool isEnemy) {
  return isEnemy == (e.amount < 0) ? 1 : -1;
}

static string getName(const Effects::IncreaseMorale& e, const ContentFactory*) {
  return e.amount > 0 ? "morale increase" : "morale decrease";
}

static string getDescription(const Effects::IncreaseMorale& e, const ContentFactory*) {
  return e.amount > 0 ? "Increases morale" : "Decreases morale";
}

static string getName(const Effects::Caster& e, const ContentFactory* f) {
  return e.effect->getName(f);
}

static string getDescription(const Effects::Caster& e, const ContentFactory* f) {
  return e.effect->getDescription(f);
}

static bool apply(const Effects::Caster& e, Position, Creature* attacker) {
  if (attacker)
    return e.effect->apply(attacker->getPosition(), attacker);
  return false;
}

static string getName(const Effects::GenericModifierEffect& e, const ContentFactory* f) {
  return e.effect->getName(f);
}

static string getDescription(const Effects::GenericModifierEffect& e, const ContentFactory* f) {
  return e.effect->getDescription(f);
}

static bool apply(const Effects::GenericModifierEffect& e, Position pos, Creature* attacker) {
  return e.effect->apply(pos, attacker);
}

static EffectAIIntent shouldAIApply(const Effects::GenericModifierEffect& e, const Creature* caster, Position pos) {
  return e.effect->shouldAIApply(caster, pos);
}

static optional<FXInfo> getProjectileFX(const Effects::GenericModifierEffect& e) {
  return e.effect->getProjectileFX();
}

static optional<ViewId> getProjectile(const Effects::GenericModifierEffect& e) {
  return e.effect->getProjectile();
}

static bool canAutoAssignMinionEquipment(const Effects::GenericModifierEffect& e) {
  return e.effect->canAutoAssignMinionEquipment();
}

static optional<MinionEquipmentType> getMinionEquipmentType(const Effects::GenericModifierEffect& e) {
  return e.effect->getMinionEquipmentType();
}

static Color getColor(const Effects::GenericModifierEffect& e, const ContentFactory* f) {
  return e.effect->getColor(f);
}

static string getDescription(const Effects::Chance& e, const ContentFactory* f) {
  return e.effect->getDescription(f) + " (" + toString(int(e.value * 100)) + "% chance)";
}

static bool apply(const Effects::Chance& e, Position pos, Creature* attacker) {
  if (Random.chance(e.value))
    return e.effect->apply(pos, attacker);
  return false;
}

static bool applyToCreature(const Effects::DoubleTrouble&, Creature* c, Creature*) {
  PCreature copy = c->getGame()->getContentFactory()->getCreatures().makeCopy(c);
  auto ttl = 50_visible;
  for (auto& item : c->getEquipment().getItems())
    if (!item->getResourceId() && !item->isDiscarded()) {
      auto itemCopy = item->getCopy(c->getGame()->getContentFactory());
      itemCopy->setTimeout(c->getGame()->getGlobalTime() + ttl + 10_visible);
      copy->take(std::move(itemCopy));
    }
  auto cRef = Effect::summonCreatures(c->getPosition(), makeVec(std::move(copy)));
  if (!cRef.empty()) {
    cRef[0]->addEffect(LastingEffect::SUMMONED, ttl, false);
    c->message(::PlayerMessage("Double trouble!", MessagePriority::HIGH));
    return true;
  } else {
    c->message(::PlayerMessage("The spell failed!", MessagePriority::HIGH));
    return false;
  }
}

static string getName(const Effects::DoubleTrouble&, const ContentFactory*) {
  return "double trouble";
}

static string getDescription(const Effects::DoubleTrouble&, const ContentFactory*) {
  return "Creates a twin copy ally.";
}

static EffectAIIntent shouldAIApplyToCreature(const Effects::DoubleTrouble&, const Creature* victim, bool isEnemy) {
  return isConsideredInDanger(victim) ? 1 : 0;
}

static string getName(const Effects::Blast&, const ContentFactory*) {
  return "air blast";
}

static string getDescription(const Effects::Blast&, const ContentFactory*) {
  return "Creates a directed blast of air that throws back creatures and items.";
}

static optional<FXInfo> getProjectileFX(const Effects::Blast&) {
  return {FXName::AIR_BLAST};
}

static optional<ViewId> getProjectile(const Effects::Blast&) {
  return ViewId("air_blast");
}

static bool apply(const Effects::Blast&, Position pos, Creature* attacker) {
  constexpr int range = 4;
  CHECK(attacker);
  vector<Position> trajectory;
  auto origin = attacker->getPosition().getCoord();
  for (auto& v : drawLine(origin, pos.getCoord()))
    if (v != origin && v.dist8(origin) <= range) {
      trajectory.push_back(Position(v, pos.getLevel()));
      if (trajectory.back().isDirEffectBlocked(attacker))
        break;
    }
  for (int i : All(trajectory).reverse())
    airBlast(attacker, attacker->getPosition(), trajectory[i], pos);
  return true;
}

static bool pullCreature(Creature* victim, const vector<Position>& trajectory) {
  auto victimPos = victim->getPosition();
  optional<Position> target;
  for (auto& v : trajectory) {
    if (v == victimPos)
      break;
    if (v.canEnter(victim)) {
      if (!target)
        target = v;
    } else
      target = none;
  }
  if (target) {
    victim->addEffect(LastingEffect::COLLAPSED, 2_visible);
    victim->displace(victim->getPosition().getDir(*target));
    return true;
  }
  return false;
}

static string getName(const Effects::Pull&, const ContentFactory*) {
  return "pull";
}

static string getDescription(const Effects::Pull&, const ContentFactory*) {
  return "Pulls a creature towards the spellcaster.";
}

static bool apply(const Effects::Pull&, Position pos, Creature* attacker) {
  CHECK(attacker);
  vector<Position> trajectory = drawLine(attacker->getPosition(), pos);
  for (auto& pos : trajectory)
    if (auto c = pos.getCreature())
      return pullCreature(c, trajectory);
  return false;
}

static optional<FXInfo> getProjectileFX(const Effects::Pull&) {
  return FXInfo{FXName::AIR_BLAST}.setReversed();
}

static bool applyToCreature(const Effects::Shove&, Creature* c, Creature* attacker) {
  CHECK(attacker);
  auto origin = attacker->getPosition();
  auto dir = origin.getDir(c->getPosition());
  CHECK(dir.length8() == 1);
  if (c->getPosition().canMoveCreature(dir)) {
    c->displace(dir);
    c->you(MsgType::ARE, "thrown back");
    c->addEffect(LastingEffect::COLLAPSED, 2_visible);
    return true;
  }
  return false;
}

static string getName(const Effects::Shove&, const ContentFactory*) {
  return "shove";
}

static string getDescription(const Effects::Shove&, const ContentFactory*) {
  return "Push back a creature.";
}

static bool applyToCreature(const Effects::SwapPosition&, Creature* c, Creature* attacker) {
  CHECK(attacker);
  auto origin = attacker->getPosition();
  auto dir = origin.getDir(c->getPosition());
  if (dir.length8() != 1) {
    attacker->privateMessage("You can't swap position with " + c->getName().the());
    return false;
  } else if (attacker->canSwapPositionWithEnemy(c)) {
    attacker->swapPosition(dir, false);
    attacker->verb("swap", "swaps", "positions with " + c->getName().the());
    return true;
  } else {
    attacker->privateMessage(c->getName().the() + " resists");
    return false;
  }
}

static string getName(const Effects::SwapPosition&, const ContentFactory*) {
  return "swap position";
}

static string getDescription(const Effects::SwapPosition&, const ContentFactory*) {
  return "Swap positions with an enemy.";
}

static string getName(const Effects::TriggerTrap&, const ContentFactory*) {
  return "trigger trap";
}

static string getDescription(const Effects::TriggerTrap&, const ContentFactory*) {
  return "Triggers a trap if present.";
}

static bool apply(const Effects::TriggerTrap&, Position pos, Creature* attacker) {
  for (auto furniture : pos.getFurniture())
    if (auto& entry = furniture->getEntryType())
      if (auto trapInfo = entry->entryData.getReferenceMaybe<FurnitureEntry::Trap>()) {
        pos.globalMessage("A " + trapInfo->effect.getName(pos.getGame()->getContentFactory()) + " trap is triggered");
        auto effect = trapInfo->effect;
        pos.removeFurniture(furniture);
        effect.apply(pos, attacker);
        pos.getGame()->addEvent(EventInfo::TrapTriggered{pos});
        return true;
      }
  return false;
}

static string getName(const Effects::AnimateItems&, const ContentFactory*) {
  return "animate weapons";
}

static string getDescription(const Effects::AnimateItems& e, const ContentFactory*) {
  return "Animates up to " + toString(e.maxCount) + " weapons from the surroundings";
}

static bool apply(const Effects::AnimateItems& m, Position pos, Creature* attacker) {
  vector<pair<Position, Item*>> candidates;
  for (auto v : pos.getRectangle(Rectangle::centered(m.radius)))
    for (auto item : v.getItems(ItemIndex::WEAPON))
      candidates.push_back(make_pair(v, item));
  candidates = Random.permutation(candidates);
  bool res = false;
  for (int i : Range(min(m.maxCount, candidates.size()))) {
    auto v = candidates[i].first;
    auto creature = pos.getGame()->getContentFactory()->getCreatures().
        getAnimatedItem(v.removeItem(candidates[i].second), attacker->getTribeId(),
            attacker->getAttr(AttrType::SPELL_DAMAGE));
    for (auto c : Effect::summonCreatures(v, makeVec(std::move(creature)))) {
      c->addEffect(LastingEffect::SUMMONED, TimeInterval{Random.get(m.time)}, false);
      res = true;
    }
  }
  return res;
}

static EffectAIIntent shouldAIApply(const Effects::AnimateItems& m, const Creature* caster, Position pos) {
  if (caster && isConsideredInDanger(caster)) {
    int totalWeapons = 0;
    for (auto v : pos.getRectangle(Rectangle::centered(m.radius))) {
      totalWeapons += v.getItems(ItemIndex::WEAPON).size();
      if (totalWeapons >= m.maxCount / 2)
        return 1;
    }
  }
  return 0;
}

static string getName(const Effects::Audience&, const ContentFactory*) {
  return "audience";
}

static string getDescription(const Effects::Audience&, const ContentFactory*) {
  return "Summons all fighters defending the territory that the creature is in";
}

static EffectAIIntent shouldAIApplyToCreature(const Effects::Audience&, const Creature* victim, bool isEnemy) {
  return isConsideredInDanger(victim) ? 1 : 0;
}

static bool apply(const Effects::Audience& a, Position pos, Creature* attacker) {
  auto collective = [&]() -> Collective* {
    for (auto col : pos.getGame()->getCollectives())
      if (col->getTerritory().contains(pos))
        return col;
    for (auto col : pos.getGame()->getCollectives())
      if (col->getTerritory().getStandardExtended().contains(pos))
        return col;
    return nullptr;
  }();
  bool wasTeleported = false;
  auto tryTeleporting = [&] (Creature* enemy) {
    if (!enemy->getStatus().contains(CreatureStatus::CIVILIAN) && !enemy->getStatus().contains(CreatureStatus::PRISONER)) {
      auto distance = enemy->getPosition().dist8(pos);
      if ((!a.maxDistance || *a.maxDistance >= distance.value_or(10000)) &&
          (distance.value_or(4) > 3 || !pos.canSee(enemy->getPosition(), Vision())))
        if (auto landing = pos.getLevel()->getClosestLanding({pos}, enemy)) {
          enemy->getPosition().moveCreature(*landing, true);
          wasTeleported = true;
          enemy->removeEffect(LastingEffect::SLEEP);
        }
    }
  };
  if (collective) {
    for (auto enemy : copyOf(collective->getCreatures(MinionTrait::FIGHTER)))
      tryTeleporting(enemy);
    if (collective != collective->getGame()->getPlayerCollective())
      for (auto l : collective->getLeaders())
        tryTeleporting(l);
  } else
    for (auto enemy : pos.getLevel()->getAllCreatures())
      tryTeleporting(enemy);
  if (wasTeleported) {
    if (attacker)
      attacker->privateMessage(PlayerMessage("Thy audience hath been summoned"_s +
          get(attacker->getAttributes().getGender(), ", Sire", ", Dame", ""), MessagePriority::HIGH));
    else
      pos.globalMessage(PlayerMessage("The audience has been summoned"_s, MessagePriority::HIGH));
    return true;
  }
  pos.globalMessage("Nothing happens");
  return false;
}

static string getName(const Effects::SoundEffect&, const ContentFactory*) {
  return "sound effect";
}

static string getDescription(const Effects::SoundEffect&, const ContentFactory*) {
  return "Makes a real sound";
}

static bool apply(const Effects::SoundEffect& e, Position pos, Creature*) {
  pos.addSound(e.sound);
  return true;
}

static bool applyToCreature(const Effects::ColorVariant& e, Creature* c, Creature*) {
  auto& obj = c->modViewObject();
  obj.setId(obj.id().withColor(e.color));
  return true;
}

static string getName(const Effects::ColorVariant&, const ContentFactory*) {
  return "color change";
}

static string getDescription(const Effects::ColorVariant&, const ContentFactory*) {
  return "Changes the color variant of a creature";
}

static string getName(const Effects::Fx&, const ContentFactory*) {
  return "visual effect";
}

static string getDescription(const Effects::Fx&, const ContentFactory*) {
  return "Just a visual effect";
}

static bool apply(const Effects::Fx& fx, Position pos, Creature*) {
  if (auto game = pos.getGame())
    game->addEvent(EventInfo::FX{pos, fx.info});
  return true;
}

static string getName(const Effects::SetFlag& e, const ContentFactory*) {
  return e.name;
}

static string getDescription(const Effects::SetFlag& e, const ContentFactory*) {
  return "Sets " + e.name + " to " + (e.value ? "true" : "false");
}

static bool apply(const Effects::SetFlag& e, Position pos, Creature*) {
  if (auto game = pos.getGame()) {
    if (e.value) {
      if (!game->effectFlags.count(e.name)) {
        game->effectFlags.insert(e.name);
        return true;
      }
    } else
    if (game->effectFlags.count(e.name)) {
      game->effectFlags.erase(e.name);
      return true;
    }
  }
  return false;
}

static string getName(const Effects::Unlock& e, const ContentFactory*) {
  return e.id;
}

static string getDescription(const Effects::Unlock& e, const ContentFactory*) {
  return "Unlocks " + e.id;
}

static bool apply(const Effects::Unlock& e, Position pos, Creature*) {
  if (auto game = pos.getGame())
    game->getUnlocks()->unlock(e.id);
  return true;
}

static string getName(const Effects::Analytics& e, const ContentFactory*) {
  return "";
}

static string getDescription(const Effects::Analytics& e, const ContentFactory*) {
  return "";
}

static bool apply(const Effects::Analytics& e, Position pos, Creature*) {
  if (auto game = pos.getGame())
    game->addAnalytics(e.name, e.value);
  return false;
}

static string getName(const Effects::Stairs&, const ContentFactory*) {
  return "stairs";
}

static string getDescription(const Effects::Stairs&, const ContentFactory*) {
  return "Use the stairs";
}

static bool applyToCreature(const Effects::Stairs&, Creature* c, Creature* attacker) {
  if (auto link = c->getPosition().getLandingLink()) {
    c->getLevel()->changeLevel(*link, c);
    return true;
  } else {
    c->message("These stairs don't lead anywhere.");
    return false;
  }
}

static string getName(const Effects::AddMinionTrait& e, const ContentFactory*) {
  return "make " + toLower(EnumInfo<MinionTrait>::getString(e.trait));
}

static string getDescription(const Effects::AddMinionTrait& e, const ContentFactory*) {
  return "Makes the creature a " + toLower(EnumInfo<MinionTrait>::getString(e.trait));
}

static bool applyToCreature(const Effects::AddMinionTrait& e, Creature* c, Creature* attacker) {
  CHECK(c->getGame());
  for (auto col : c->getGame()->getCollectives())
    if (col->getCreatures().contains(c)) {
      col->setTrait(c, e.trait);
      return true;
    }
  FATAL << "Couldn't find collective for " << c->identify();
  fail();
}

static string getName(const Effects::TakeItems& e, const ContentFactory*) {
  return "take " + e.ingredient;
}

static string getDescription(const Effects::TakeItems& e, const ContentFactory*) {
  return "Takes " + e.ingredient;
}

static bool applyToCreature(const Effects::TakeItems& e, Creature* c, Creature* attacker) {
  auto items = c->getEquipment().removeItems(
      c->getEquipment().getItems(ItemIndex::RUNE).filter(
      [&] (auto& it){ return it->getIngredientType() == e.ingredient; }),
      c);
  if (items.empty())
    return false;
  if (attacker)
    attacker->takeItems(std::move(items), c);
  return true;
}

static bool canAutoAssignMinionEquipment(const Effects::Filter& f) {
  return f.effect->canAutoAssignMinionEquipment();
}

static optional<MinionEquipmentType> getMinionEquipmentType(const Effects::Filter& f) {
  return f.effect->getMinionEquipmentType();
}

static bool apply(const Effects::Filter& e, Position pos, Creature* attacker) {
  return e.predicate.apply(pos, attacker) && e.effect->apply(pos, attacker);
}

static optional<FXInfo> getProjectileFX(const Effects::Filter& e) {
  return e.effect->getProjectileFX();
}

static optional<ViewId> getProjectile(const Effects::Filter& e) {
  return e.effect->getProjectile();
}

static EffectAIIntent shouldAIApply(const Effects::Filter& e, const Creature* caster, Position pos) {
  if (e.predicate.apply(pos, caster))
    return e.effect->shouldAIApply(caster, pos);
  return 0;
}

static string getName(const Effects::Filter& e, const ContentFactory* f) {
  return e.effect->getName(f) + " (" + e.predicate.getName() + ")";
}

static string getDescription(const Effects::Filter& e, const ContentFactory* f) {
  return e.effect->getDescription(f) + " (applied only to " + e.predicate.getName() + ")";
}

static bool apply(const Effects::ReturnFalse& e, Position pos, Creature* attacker) {
  e.effect->apply(pos, attacker);
  return false;
}

static string getDescription(const Effects::Description& e, const ContentFactory*) {
  return e.text;
}

static string getName(const Effects::Name& e, const ContentFactory*) {
  return e.text;
}

static bool sameAIIntent(EffectAIIntent a, EffectAIIntent b) {
  return (a > 0 && b > 0) || (a < 0 && b < 0) || (a == 0 && b == 0);
}

static EffectAIIntent shouldAIApply(const Effects::AI& e, const Creature* caster, Position pos) {
  auto origRes = e.effect->shouldAIApply(caster, pos);
  if (!caster->isPlayer() && sameAIIntent(origRes, e.from) && e.predicate.apply(pos, caster))
    return e.to;
  return origRes;
}

Effect::Effect(const EffectType& t) noexcept : effect(t) {
}

Effect::Effect(Effect&&) noexcept = default;

Effect::Effect(const Effect&) noexcept = default;

Effect::Effect() noexcept {
}

Effect::~Effect() {
}

Effect& Effect::operator =(Effect&&) = default;

Effect& Effect::operator =(const Effect&) = default;

template <typename T>
static bool isConsideredHostile(const T&, const Creature*) {
  return false;
}

template <typename T, REQUIRE(applyToCreature(TVALUE(const T&), TVALUE(Creature*), TVALUE(Creature*)))>
static bool apply(const T& t, Position pos, Creature* attacker) {
  if (auto c = pos.getCreature())
    return applyToCreature(t, c, attacker);
  return false;
}

bool Effect::apply(Position pos, Creature* attacker) const {
  if (auto c = pos.getCreature()) {
    if (attacker)
      effect->visit([&](const auto& e) {
        if (isConsideredHostile(e, c))
          c->onAttackedBy(attacker);
      });
  }
  return effect->visit<bool>([&](const auto& e) { return ::apply(e, pos, attacker); });
}

string Effect::getName(const ContentFactory* f) const {
  return effect->visit<string>([&](const auto& elem) { return ::getName(elem, f); });
}

string Effect::getDescription(const ContentFactory* f) const {
  return effect->visit<string>([&](const auto& elem) { return ::getDescription(elem, f); });
}

/* Unimplemented: Teleport, EnhanceArmor, EnhanceWeapon, Suicide, IncreaseAttr, IncreaseSkill, IncreaseWorkshopSkill
      EmitPoisonGas, CircularBlast, Alarm, DoubleTrouble,
      PlaceFurniture, InjureBodyPart, LooseBodyPart, RegrowBodyPart, DestroyWalls,
      ReviveCorpse, Blast, Shove, SwapPosition, AddAutomatonParts, AddBodyPart, MakeHumanoid */

static EffectAIIntent shouldAIApply(const DefaultType& m, const Creature* caster, Position pos) {
  return 0;
}

template <typename T, REQUIRE(shouldAIApplyToCreature(TVALUE(const T&), TVALUE(const Creature*), TVALUE(bool)))>
static EffectAIIntent shouldAIApply(const T& elem, const Creature* caster, Position pos) {
  auto victim = pos.getCreature();
  if (victim && !caster->canSee(victim))
    victim = nullptr;
  if (victim && !victim->isAffected(LastingEffect::STUNNED))
    return shouldAIApplyToCreature(elem, victim, caster->isEnemy(victim));
  return 0;
}

EffectAIIntent Effect::shouldAIApply(const Creature* caster, Position pos) const {
  PROFILE_BLOCK("Effect::shouldAIApply");
  return effect->visit<EffectAIIntent>([&](const auto& e) { return ::shouldAIApply(e, caster, pos); });
}

static optional<FXInfo> getProjectileFX(const DefaultType&) {
  return none;
}

optional<FXInfo> Effect::getProjectileFX() const {
  return effect->visit<optional<FXInfo>>([&](const auto& e) { return ::getProjectileFX(e); });
}

static optional<ViewId> getProjectile(const DefaultType&) {
  return none;
}

optional<ViewId> Effect::getProjectile() const {
  return effect->visit<optional<ViewId>>([&](const auto& elem) { return ::getProjectile(elem); } );
}

vector<Effect> Effect::getWishedForEffects() {
  vector<Effect> allEffects {
       Effect(Effects::Escape{}),
       Effect(Effects::Heal{HealthType::FLESH}),
       Effect(Effects::Heal{HealthType::SPIRIT}),
       Effect(Effects::Ice{}),
       Effect(Effects::Fire{}),
       Effect(Effects::DestroyEquipment{}),
       Effect(Effects::Area{2, Effect(Effects::DestroyWalls{DestroyAction::Type::BOULDER})}),
       Effect(Effects::Enhance{Effects::EnhanceType::WEAPON, 2}),
       Effect(Effects::Enhance{Effects::EnhanceType::ARMOR, 2}),
       Effect(Effects::Enhance{Effects::EnhanceType::WEAPON, -2}),
       Effect(Effects::Enhance{Effects::EnhanceType::ARMOR, -2}),
       Effect(Effects::CircularBlast{}),
       Effect(Effects::Deception{}),
       Effect(Effects::SummonElement{}),
       Effect(Effects::Acid{}),
       Effect(Effects::DoubleTrouble{})
  };
  for (auto effect : ENUM_ALL(LastingEffect))
    if (LastingEffects::canWishFor(effect)) {
      allEffects.push_back(EffectType(Effects::Lasting{effect}));
      allEffects.push_back(EffectType(Effects::Permanent{effect}));
      allEffects.push_back(EffectType(Effects::RemoveLasting{effect}));
    }
  for (auto attr : ENUM_ALL(AttrType))
    allEffects.push_back(EffectType(Effects::IncreaseAttr{attr, (attr == AttrType::PARRY ? 2 : 5)}));
  return allEffects;
}

static optional<MinionEquipmentType> getMinionEquipmentType(const DefaultType&) {
  return none;
}

optional<MinionEquipmentType> Effect::getMinionEquipmentType() const {
  return effect->visit<optional<MinionEquipmentType>>([](const auto& elem) { return ::getMinionEquipmentType(elem); } );
}

static bool canAutoAssignMinionEquipment(const DefaultType&) { return true; }

bool Effect::canAutoAssignMinionEquipment() const {
  return effect->visit<bool>([](const auto& elem) { return ::canAutoAssignMinionEquipment(elem); });
}

template <typename T, REQUIRE(getColor(TVALUE(const T&), TVALUE(const ContentFactory*)))>
static Color getColor(const T& t, const Effect& effect, const ContentFactory* f) {
  return getColor(t, f);
}

static Color getColor(const DefaultType& e, const Effect& effect, const ContentFactory* f) {
  int h = int(combineHash(effect.getName(f)));
  return Color(h % 256, ((h / 256) % 256), ((h / 256 / 256) % 256));
}

Color Effect::getColor(const ContentFactory* f) const {
  return effect->visit<Color>([f, this](const auto& elem) { return ::getColor(elem, *this, f); });
}

SERIALIZE_DEF(Effect, effect)

#define VARIANT_TYPES_LIST EFFECT_TYPES_LIST
#define VARIANT_NAME EffectType
namespace Effects {
#include "gen_variant_serialize.h"
}

#include "pretty_archive.h"
template void Effect::serialize(PrettyInputArchive&, unsigned);

namespace Effects {
#define DEFAULT_ELEM "Chain"
template<>
#include "gen_variant_serialize_pretty.h"
}
#undef DEFAULT_ELEM
#undef VARIANT_TYPES_LIST
#undef VARIANT_NAME
