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

vector<Creature*> Effect::summon(Creature* c, CreatureId id, int num, optional<TimeInterval> ttl, TimeInterval delay) {
  vector<PCreature> creatures;
  for (int i : Range(num))
    creatures.push_back(c->getGame()->getContentFactory()->getCreatures().fromId(id, c->getTribeId(), MonsterAIFactory::summoned(c)));
  auto ret = summonCreatures(c->getPosition(), std::move(creatures), delay);
  for (auto c : ret)
    if (ttl)
      c->addEffect(LastingEffect::SUMMONED, *ttl, false);
  return ret;
}

vector<Creature*> Effect::summon(Position pos, CreatureGroup& factory, int num, optional<TimeInterval> ttl, TimeInterval delay) {
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

bool Effect::isConsideredHostile() const {
  return effect->visit<bool>(
      [&](const Effects::Lasting& e) {
        return ::isConsideredHostile(e.lastingEffect);
      },
      [&](const Effects::Acid&) {
        return true;
      },
      [&](const Effects::DestroyEquipment&) {
        return true;
      },
      [&](const Effects::SilverDamage&) {
        return true;
      },
      [&](const Effects::Fire&) {
        return true;
      },
      [&](const Effects::Ice&) {
        return true;
      },
      [&](const Effects::Damage&) {
        return true;
      },
      [&](const auto&) {
        return false;
      }
  );
}

bool Effects::Escape::applyToCreature(Creature* c, Creature* attacker) const {
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
        !v.isConnectedTo(c->getPosition(), movementType))
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

string Effects::Escape::getName(const ContentFactory*) const {
  return "escape";
}

string Effects::Escape::getDescription(const ContentFactory*) const {
  return "Teleports to a safer location close by.";
}

bool Effects::Teleport::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::Teleport::getName(const ContentFactory*) const {
  return "teleport";
}

string Effects::Teleport::getDescription(const ContentFactory*) const {
  return "Teleport to any location that's close by.";
}

bool Effects::Jump::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::Jump::getName(const ContentFactory*) const {
  return "jumping";
}

string Effects::Jump::getDescription(const ContentFactory*) const {
  return "Jump!";
}

bool Effects::Lasting::applyToCreature(Creature* c, Creature* attacker) const {
  return c->addEffect(lastingEffect, LastingEffects::getDuration(c, lastingEffect));
}

string Effects::Lasting::getName(const ContentFactory*) const {
  return LastingEffects::getName(lastingEffect);
}

string Effects::Lasting::getDescription(const ContentFactory*) const {
  // Leave out the full stop.
  string desc = LastingEffects::getDescription(lastingEffect);
  return desc.substr(0, desc.size() - 1) + " for some turns.";
}

bool Effects::RemoveLasting::applyToCreature(Creature* c, Creature* attacker) const {
  return c->removeEffect(lastingEffect);
}

string Effects::RemoveLasting::getName(const ContentFactory*) const {
  return "remove " + LastingEffects::getName(lastingEffect);
}

string Effects::RemoveLasting::getDescription(const ContentFactory*) const {
  return "Removes/cures from effect: " + LastingEffects::getName(lastingEffect);
}

bool Effects::IncreaseAttr::applyToCreature(Creature* c, Creature*) const {
  c->you(MsgType::YOUR, ::getName(attr) + get(" improves", " wanes"));
  c->getAttributes().increaseBaseAttr(attr, amount);
  return true;
}

string Effects::IncreaseAttr::getName(const ContentFactory*) const {
  return ::getName(attr) + get(" boost", " loss");
}

string Effects::IncreaseAttr::getDescription(const ContentFactory*) const {
  return get("Increases", "Decreases") + " "_s + ::getName(attr) + " by " + toString(abs(amount));
}

const char* Effects::IncreaseAttr::get(const char* ifIncrease, const char* ifDecrease) const {
  if (amount > 0)
    return ifIncrease;
  else
    return ifDecrease;
}

bool Effects::Permanent::applyToCreature(Creature* c, Creature* attacker) const {
  return c->addPermanentEffect(lastingEffect);
}

string Effects::Permanent::getName(const ContentFactory*) const {
  return "permanent " + LastingEffects::getName(lastingEffect);
}

string Effects::Permanent::getDescription(const ContentFactory*) const {
  string desc = LastingEffects::getDescription(lastingEffect);
  return desc.substr(0, desc.size() - 1) + " permanently.";
}

bool Effects::RemovePermanent::applyToCreature(Creature* c, Creature* attacker) const {
  return c->removePermanentEffect(lastingEffect);
}

string Effects::RemovePermanent::getName(const ContentFactory*) const {
  return "removes/cures from permanent effect: " + LastingEffects::getName(lastingEffect);
}

string Effects::RemovePermanent::getDescription(const ContentFactory*) const {
  string desc = LastingEffects::getDescription(lastingEffect);
  return "Removes " + desc.substr(0, desc.size() - 1) + " permanently.";
}

bool Effects::Alarm::applyToCreature(Creature* c, Creature* attacker) const {
  c->getGame()->addEvent(EventInfo::Alarm{c->getPosition(), silent});
  return true;
}

string Effects::Alarm::getName(const ContentFactory*) const {
  return "alarm";
}

string Effects::Alarm::getDescription(const ContentFactory*) const {
  return "Alarm!";
}

bool Effects::Acid::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::Acid::getName(const ContentFactory*) const {
  return "acid";
}

string Effects::Acid::getDescription(const ContentFactory*) const {
  return "Causes acid damage to skin and equipment.";
}

bool Effects::Summon::applyToCreature(Creature* c, Creature* attacker) const {
  return ::summon(c, creature, count, false, ttl.map([](int v) { return TimeInterval(v); }));
}

string Effects::Summon::getName(const ContentFactory* f) const {
  return "summon " + f->getCreatures().getName(creature);
}

string Effects::Summon::getDescription(const ContentFactory* f) const {
  if (count.getEnd() > 2)
    return "Summons " + toString(count.getStart()) + " to " + toString(count.getEnd() - 1) + " " + getName(f);
  else
    return "Summons a " + getName(f);
}

bool Effects::AssembledMinion::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::AssembledMinion::getName(const ContentFactory* f) const {
  return f->getCreatures().getName(creature);
}

string Effects::AssembledMinion::getDescription(const ContentFactory* f) const {
  return "Can be assembled to a " + getName(f);
}

bool Effects::SummonEnemy::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::SummonEnemy::getName(const ContentFactory* f) const {
  return "summon hostile " + f->getCreatures().getName(creature);
}

string Effects::SummonEnemy::getDescription(const ContentFactory* f) const {
  if (count.getEnd() > 2)
    return "Summons " + toString(count.getStart()) + " to " + toString(count.getEnd() - 1) + " hostile " + getName(f);
  else
    return "Summons a hostile " + getName(f);
}

bool Effects::SummonElement::applyToCreature(Creature* c, Creature* attacker) const {
  auto id = CreatureId("AIR_ELEMENTAL");
  for (Position p : c->getPosition().getRectangle(Rectangle::centered(3)))
    for (auto f : p.getFurniture())
      if (auto elem = f->getSummonedElement())
        id = *elem;
  return ::summon(c, id, Range(1, 2), false, 100_visible);
}

string Effects::SummonElement::getName(const ContentFactory*) const {
  return "summon element";
}

string Effects::SummonElement::getDescription(const ContentFactory*) const {
  return "Summons an element or spirit from the surroundings.";
}

bool Effects::Deception::applyToCreature(Creature* c, Creature* attacker) const {
  vector<PCreature> creatures;
  for (int i : Range(Random.get(3, 7)))
    creatures.push_back(CreatureFactory::getIllusion(c));
  return !Effect::summonCreatures(c->getPosition(), std::move(creatures)).empty();
}

string Effects::Deception::getName(const ContentFactory*) const {
  return "deception";
}

string Effects::Deception::getDescription(const ContentFactory*) const {
  return "Creates multiple illusions of the spellcaster to confuse the enemy.";
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

bool Effects::CircularBlast::applyToCreature(Creature* c, Creature* attacker) const {
  for (Vec2 v : Vec2::directions8(Random))
    airBlast(attacker, c->getPosition(), c->getPosition().plus(v), c->getPosition().plus(v * 10));
  c->addFX({FXName::CIRCULAR_BLAST});
  return true;
}

string Effects::CircularBlast::getName(const ContentFactory*) const {
  return "air blast";
}

string Effects::CircularBlast::getDescription(const ContentFactory*) const {
  return "Creates a circular blast of air that throws back creatures and items.";
}

const char* Effects::Enhance::typeAsString() const {
  switch (type) {
    case ItemUpgradeType::WEAPON:
      return "weapon";
    case ItemUpgradeType::ARMOR:
      return "armor";
  }
}

const char* Effects::Enhance::amountAs(const char* positive, const char* negative) const {
  return amount > 0 ? positive : negative;
}

bool Effects::Enhance::applyToCreature(Creature* c, Creature* attacker) const {
  switch (type) {
    case ItemUpgradeType::WEAPON:
      return enhanceWeapon(c, amount, amountAs("is improved", "degrades"));
    case ItemUpgradeType::ARMOR:
      return enhanceArmor(c, amount, amountAs("is improved", "degrades"));
  }
}

string Effects::Enhance::getName(const ContentFactory*) const {
  return typeAsString() + " "_s + amountAs("enchantment", "degradation");
}

string Effects::Enhance::getDescription(const ContentFactory*) const {
  return amountAs("Increases", "Decreases") + " "_s + typeAsString() + " capability"_s;
}

bool Effects::DestroyEquipment::applyToCreature(Creature* c, Creature* attacker) const {
  auto equipped = c->getEquipment().getAllEquipped();
  if (!equipped.empty()) {
    Item* dest = Random.choose(equipped);
    c->you(MsgType::YOUR, dest->getName() + " crumbles to dust.");
    c->steal({dest});
    return true;
  }
  return false;
}

string Effects::DestroyEquipment::getName(const ContentFactory*) const {
  return "equipment destruction";
}

string Effects::DestroyEquipment::getDescription(const ContentFactory*) const {
  return "Destroys a random piece of equipment.";
}

bool Effects::DestroyWalls::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::DestroyWalls::getName(const ContentFactory*) const {
  return "destruction";
}

string Effects::DestroyWalls::getDescription(const ContentFactory*) const {
  return "Destroys terrain and objects.";
}

bool Effects::Heal::applyToCreature(Creature* c, Creature* attacker) const {
  if (c->getBody().canHeal(healthType)) {
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

string Effects::Heal::getName(const ContentFactory*) const {
  switch (healthType) {
    case HealthType::FLESH: return "healing";
    case HealthType::SPIRIT: return "materialization";
  }
}

string Effects::Heal::getDescription(const ContentFactory*) const {
  switch (healthType) {
    case HealthType::FLESH: return "Fully restores health.";
    case HealthType::SPIRIT: return "Fully re-materializes a spirit.";
  }
}

bool Effects::Fire::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::Fire::getName(const ContentFactory*) const {
  return "fire";
}

string Effects::Fire::getDescription(const ContentFactory*) const {
  return "Burns!";
}

bool Effects::Ice::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::Ice::getName(const ContentFactory*) const {
  return "ice";
}

string Effects::Ice::getDescription(const ContentFactory*) const {
  return "Freezes water and causes cold damage";
}

bool Effects::ReviveCorpse::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::ReviveCorpse::getName(const ContentFactory*) const {
  return "revive corpse";
}

string Effects::ReviveCorpse::getDescription(const ContentFactory*) const {
  return "Brings a dead corpse back alive as a servant";
}

static PCreature getBestSpirit(const Model* model, TribeId tribe) {
  auto& factory = model->getGame()->getContentFactory()->getCreatures();
  for (auto id : Random.permutation(factory.getAllCreatures())) {
    auto orig = factory.fromId(id, tribe);
    if (orig->getBody().hasBrain())
      return orig;
  }
  return nullptr;
}

bool Effects::EmitPoisonGas::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::EmitPoisonGas::getName(const ContentFactory*) const {
  return "poison gas";
}

string Effects::EmitPoisonGas::getDescription(const ContentFactory*) const {
  return "Emits poison gas";
}

bool Effects::SilverDamage::applyToCreature(Creature* c, Creature* attacker) const {
  c->affectBySilver();
  return c->getBody().isUndead();
}

string Effects::SilverDamage::getName(const ContentFactory*) const {
  return "silver";
}

string Effects::SilverDamage::getDescription(const ContentFactory*) const {
  return "Hurts the undead.";
}

bool Effects::PlaceFurniture::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::PlaceFurniture::getName(const ContentFactory* c) const {
  return c->furniture.getData(furniture).getName();
}

string Effects::PlaceFurniture::getDescription(const ContentFactory* c) const {
  return "Creates a " + getName(c);
}

bool Effects::DropItems::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::DropItems::getName(const ContentFactory* c) const {
  return "create items";
}

string Effects::DropItems::getDescription(const ContentFactory* c) const {
  return "Creates items";
}

bool Effects::Damage::applyToCreature(Creature* c, Creature* attacker) const {
  CHECK(attacker) << "Unknown attacker";
  bool result = c->takeDamage(Attack(attacker, Random.choose<AttackLevel>(), attackType, attacker->getAttr(attr), attr), true);
  if (attr == AttrType::SPELL_DAMAGE)
    c->addFX({FXName::MAGIC_MISSILE_SPLASH});
  return result;
}

string Effects::Damage::getName(const ContentFactory*) const {
  return ::getName(attr);
}

string Effects::Damage::getDescription(const ContentFactory*) const {
  return "Causes " + ::getName(attr);
}

bool Effects::FixedDamage::applyToCreature(Creature* c, Creature*) const {
  bool result = c->takeDamage(Attack(nullptr, Random.choose<AttackLevel>(), attackType, value, attr), true);
  if (attr == AttrType::SPELL_DAMAGE)
    c->addFX({FXName::MAGIC_MISSILE_SPLASH});
  return result;
}

string Effects::FixedDamage::getName(const ContentFactory*) const {
  return toString(value) + " " + ::getName(attr);
}

string Effects::FixedDamage::getDescription(const ContentFactory*) const {
  return "Causes " + toString(value) + " " + ::getName(attr);
}

bool Effects::InjureBodyPart::applyToCreature(Creature* c, Creature* attacker) const {
  if (c->getBody().injureBodyPart(c, part, false)) {
    c->you(MsgType::DIE, "");
    c->dieWithAttacker(attacker);
  }
  return false;
}

string Effects::InjureBodyPart::getName(const ContentFactory*) const {
  return "injure "_s + ::getName(part);
}

string Effects::InjureBodyPart::getDescription(const ContentFactory*) const {
  return "Injures "_s + ::getName(part);
}

bool Effects::LoseBodyPart::applyToCreature(Creature* c, Creature* attacker) const {
  if (c->getBody().injureBodyPart(c, part, true)) {
    c->you(MsgType::DIE, "");
    c->dieWithAttacker(attacker);
  }
  return false;
}

string Effects::LoseBodyPart::getName(const ContentFactory*) const {
  return "lose "_s + ::getName(part);
}

string Effects::LoseBodyPart::getDescription(const ContentFactory*) const {
  return "Causes you to lose a "_s + ::getName(part);
}

bool Effects::AddBodyPart::applyToCreature(Creature* c, Creature* attacker) const {
  c->getBody().addBodyPart(part, count);
  if (attack) {
    c->getBody().addIntrinsicAttack(part, IntrinsicAttack{*attack, true});
    c->getBody().initializeIntrinsicAttack(c->getGame()->getContentFactory());
  }
  return true;
}

string Effects::AddBodyPart::getName(const ContentFactory*) const {
  return "add "_s + getPlural(::getName(part), count);
}

string Effects::AddBodyPart::getDescription(const ContentFactory*) const {
  return "Adds "_s + getPlural(::getName(part), count);
}

bool Effects::MakeHumanoid::applyToCreature(Creature* c, Creature* attacker) const {
  bool ret = !c->getBody().isHumanoid();
  c->getBody().setHumanoid(true);
  return ret;
}

string Effects::MakeHumanoid::getName(const ContentFactory*) const {
  return "turn into a humanoid";
}

string Effects::MakeHumanoid::getDescription(const ContentFactory*) const {
  return "Turns creature into a humanoid";
}

bool Effects::RegrowBodyPart::applyToCreature(Creature* c, Creature* attacker) const {
  return c->getBody().healBodyParts(c, maxCount);
}

string Effects::RegrowBodyPart::getName(const ContentFactory*) const {
  return "regrow lost body parts";
}

string Effects::RegrowBodyPart::getDescription(const ContentFactory*) const {
  return "Causes lost body parts to regrow.";
}

bool Effects::Area::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::Area::getName(const ContentFactory* f) const {
  return effect->getName(f);
}

string Effects::Area::getDescription(const ContentFactory* factory) const {
  return "Area effect of radius " + toString(radius) + ": " + noCapitalFirst(effect->getDescription(factory));
}


bool Effects::CustomArea::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::CustomArea::getName(const ContentFactory* f) const {
  return "custom area " + effect->getName(f);
}

string Effects::CustomArea::getDescription(const ContentFactory* factory) const {
  return "Custom area effect: " + noCapitalFirst(effect->getDescription(factory));
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

bool Effects::Suicide::applyToCreature(Creature* c, Creature* attacker) const {
  c->you(message, "");
  c->dieNoReason();
  return true;
}

string Effects::Suicide::getName(const ContentFactory*) const {
  return "suicide";
}

string Effects::Suicide::getDescription(const ContentFactory*) const {
  return "Causes the *attacker* to die.";
}

bool Effects::Wish::applyToCreature(Creature* c, Creature* attacker) const {
  c->getController()->grantWish((attacker ? attacker->getName().the() + " grants you a wish." : "You are granted a wish.") +
      " What do you wish for?");
  return true;
}

string Effects::Wish::getName(const ContentFactory*) const {
  return "wishing";
}

string Effects::Wish::getDescription(const ContentFactory*) const {
  return "Gives you one wish.";
}

static string combineNames(const ContentFactory* f, const vector<Effect>& effects) {
  string ret;
  for (auto& e : effects)
    ret += e.getName(f) + ", ";
  ret.pop_back();
  ret.pop_back();
  return ret;
}

static string combineDescriptions(const ContentFactory* f, const vector<Effect>& effects) {
  string ret;
  for (auto& e : effects)
    ret += e.getDescription(f) + ", ";
  ret.pop_back();
  ret.pop_back();
  return ret;
}

bool Effects::Chain::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::Chain::getName(const ContentFactory* f) const {
  return combineNames(f, effects);
}

string Effects::Chain::getDescription(const ContentFactory* f) const {
  return combineDescriptions(f, effects);
}

bool Effects::ChainFirstResult::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::ChainFirstResult::getName(const ContentFactory* f) const {
  return combineNames(f, effects);
}

string Effects::ChainFirstResult::getDescription(const ContentFactory* f) const {
  return combineDescriptions(f, effects);
}

bool Effects::FirstSuccessful::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::FirstSuccessful::getName(const ContentFactory* f) const {
  return "try: " + combineNames(f, effects);
}

string Effects::FirstSuccessful::getDescription(const ContentFactory* f) const {
  return "First successful: " + combineDescriptions(f, effects);
}

bool Effects::ChooseRandom::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::ChooseRandom::getName(const ContentFactory* f) const {
  return effects[0].getName(f);
}

string Effects::ChooseRandom::getDescription(const ContentFactory* f) const {
  return effects[0].getDescription(f);
}

bool Effects::Message::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::Message::getName(const ContentFactory*) const {
  return "message";
}

string Effects::Message::getDescription(const ContentFactory*) const {
  return text;
}

bool Effects::CreatureMessage::applyToCreature(Creature* c, Creature* attacker) const {
  c->verb(secondPerson, thirdPerson);
  return true;
}

string Effects::CreatureMessage::getName(const ContentFactory*) const {
  return "message";
}

string Effects::CreatureMessage::getDescription(const ContentFactory*) const {
  return "Custom message";
}

bool Effects::GrantAbility::applyToCreature(Creature* c, Creature* attacker) const {
  bool ret = !c->getSpellMap().contains(id);
  c->getSpellMap().add(*c->getGame()->getContentFactory()->getCreatures().getSpell(id), ExperienceType::MELEE, 0);
  return ret;
}

string Effects::GrantAbility::getName(const ContentFactory* f) const {
  return "grant "_s + f->getCreatures().getSpell(id)->getName();
}

string Effects::GrantAbility::getDescription(const ContentFactory* f) const {
  return "Grants ability: "_s + f->getCreatures().getSpell(id)->getName();
}

bool Effects::IncreaseMorale::applyToCreature(Creature* c, Creature* attacker) const {
  if (amount > 0)
    c->you(MsgType::YOUR, "spirits are lifted");
  else
    c->you(MsgType::ARE, "disheartened");
  double before = c->getMorale();
  c->addMorale(amount);
  return c->getMorale() != before;
}

string Effects::IncreaseMorale::getName(const ContentFactory*) const {
  return amount > 0 ? "morale increase" : "morale decrease";
}

string Effects::IncreaseMorale::getDescription(const ContentFactory*) const {
  return amount > 0 ? "Increases morale" : "Decreases morale";
}

bool Effects::Caster::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::Caster::getName(const ContentFactory* f) const {
  return effect->getName(f);
}

string Effects::Caster::getDescription(const ContentFactory* f) const {
  return effect->getDescription(f);
}

bool Effects::Chance::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::Chance::getName(const ContentFactory* f) const {
  return effect->getName(f);
}

string Effects::Chance::getDescription(const ContentFactory* f) const {
  return effect->getDescription(f) + " (" + toString(int(value * 100)) + "% chance)";
}

bool Effects::DoubleTrouble::applyToCreature(Creature* c, Creature* attacker) const {
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
    c->message(PlayerMessage("Double trouble!", MessagePriority::HIGH));
    return true;
  } else {
    c->message(PlayerMessage("The spell failed!", MessagePriority::HIGH));
    return false;
  }
}

string Effects::DoubleTrouble::getName(const ContentFactory*) const {
  return "double trouble";
}

string Effects::DoubleTrouble::getDescription(const ContentFactory*) const {
  return "Creates a twin copy ally.";
}

bool Effects::Blast::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::Blast::getName(const ContentFactory*) const {
  return "air blast";
}

string Effects::Blast::getDescription(const ContentFactory*) const {
  return "Creates a directed blast of air that throws back creatures and items.";
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

bool Effects::Pull::applyToCreature(Creature* victim, Creature* attacker) const {
  return false;
}

string Effects::Pull::getName(const ContentFactory*) const {
  return "pull";
}

string Effects::Pull::getDescription(const ContentFactory*) const {
  return "Pulls a creature towards the spellcaster.";
}

bool Effects::Shove::applyToCreature(Creature* c, Creature* attacker) const {
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

string Effects::Shove::getName(const ContentFactory*) const {
  return "shove";
}

string Effects::Shove::getDescription(const ContentFactory*) const {
  return "Push back a creature.";
}

bool Effects::SwapPosition::applyToCreature(Creature* c, Creature* attacker) const {
  CHECK(attacker);
  auto origin = attacker->getPosition();
  auto dir = origin.getDir(c->getPosition());
  CHECK(dir.length8() == 1);
  if (attacker->canSwapPositionWithEnemy(c)) {
    attacker->swapPosition(dir, false);
    attacker->verb("swap", "swaps", "positions with " + c->getName().the());
    return true;
  } else {
    attacker->privateMessage(c->getName().the() + " resists");
    return false;
  }
}

string Effects::SwapPosition::getName(const ContentFactory*) const {
  return "swap position";
}

string Effects::SwapPosition::getDescription(const ContentFactory*) const {
  return "Swap positions with an enemy.";
}

bool Effects::TriggerTrap::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::TriggerTrap::getName(const ContentFactory*) const {
  return "trigger trap";
}

string Effects::TriggerTrap::getDescription(const ContentFactory*) const {
  return "Triggers a trap if present.";
}

bool Effects::AnimateItems::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::AnimateItems::getName(const ContentFactory*) const {
  return "animate weapons";
}

string Effects::AnimateItems::getDescription(const ContentFactory*) const {
  return "Animates up to " + toString(maxCount) + " weapons from the surroundings";
}

bool Effects::Audience::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::Audience::getName(const ContentFactory*) const {
  return "audience";
}

string Effects::Audience::getDescription(const ContentFactory*) const {
  return "Summons all fighters defending the territory that the creature is in";
}

bool Effects::SoundEffect::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::SoundEffect::getName(const ContentFactory*) const {
  return "sound effect";
}

string Effects::SoundEffect::getDescription(const ContentFactory*) const {
  return "Makes a real sound";
}

bool Effects::ColorVariant::applyToCreature(Creature* c, Creature* attacker) const {
  auto& obj = c->modViewObject();
  obj.setId(obj.id().withColor(color));
  return true;
}

string Effects::ColorVariant::getName(const ContentFactory*) const {
  return "color change";
}

string Effects::ColorVariant::getDescription(const ContentFactory*) const {
  return "Changes the color variant of a creature";
}

bool Effects::Fx::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::Fx::getName(const ContentFactory*) const {
  return "visual effect";
}

string Effects::Fx::getDescription(const ContentFactory*) const {
  return "Just a visual effect";
}

bool Effects::Filter::applies(const Creature* c, const Creature* attacker) const {
  switch (filter) {
    case FilterType::ALLY:
      return !!c && !!attacker && !c->isEnemy(attacker);
    case FilterType::ENEMY:
      return !!c && !!attacker && c->isEnemy(attacker);
    case FilterType::AUTOMATON:
      return !!c && !c->automatonParts.empty();
  }
}

bool Effects::Filter::applyToCreature(Creature* c, Creature* attacker) const {
  return applies(c, attacker) && effect->apply(c->getPosition(), attacker);
}

string Effects::Filter::getName(const ContentFactory* f) const {
  auto suffix = [&] {
    switch (filter) {
      case FilterType::ALLY:
        return " (ally only)";
      case FilterType::ENEMY:
        return " (enemy only)";
      case FilterType::AUTOMATON:
        return " (automaton only)";
    }
  };
  return effect->getName(f) + suffix();
}

string Effects::Filter::getDescription(const ContentFactory* f) const {
  auto suffix = [&] {
    switch (filter) {
      case FilterType::ALLY:
        return " (applied only to allies)";
      case FilterType::ENEMY:
        return " (applied only to enemies)";
      case FilterType::AUTOMATON:
        return " (applied only to automatons)";
    }
  };
  return effect->getDescription(f) + suffix();
}

bool Effects::Description::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::Description::getName(const ContentFactory* f) const {
  return effect->getName(f);
}

string Effects::Description::getDescription(const ContentFactory* f) const {
  return text;
}

bool Effects::Name::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::Name::getName(const ContentFactory* f) const {
  return text;
}

string Effects::Name::getDescription(const ContentFactory* f) const {
  return effect->getDescription(f);
}

bool Effects::AIBelowHealth::applyToCreature(Creature* c, Creature* attacker) const {
  return false;
}

string Effects::AIBelowHealth::getName(const ContentFactory* f) const {
  return effect->getName(f);
}

string Effects::AIBelowHealth::getDescription(const ContentFactory* f) const {
  return effect->getDescription(f);
}

#define FORWARD_CALL(RetType, Var, Name, ...)\
Var->visit<RetType>([&](const auto& e) { return e.Name(__VA_ARGS__); })

string Effect::getName(const ContentFactory* f) const {
  return FORWARD_CALL(string, effect, getName, f);
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

bool Effect::apply(Position pos, Creature* attacker) const {
  if (auto c = pos.getCreature()) {
    bool res = FORWARD_CALL(bool, effect, applyToCreature, c, attacker);
    if (isConsideredHostile() && attacker)
      c->onAttackedBy(attacker);
    if (res)
      return true;
  }
  return effect->visit<bool>(
      [&](const auto&) { return false; },
      [&](const Effects::ReviveCorpse& effect) {
        for (auto& item : pos.getItems())
          if (auto info = item->getCorpseInfo())
            if (info->canBeRevived)
              for (auto& dead : pos.getModel()->getDeadCreatures())
                if (dead->getUniqueId() == info->victim) {
                  for (auto id : effect.summoned) {
                    auto summoned = summon(attacker, id, 1, TimeInterval(effect.ttl));
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
      },
      [&](Effects::Teleport) {
        if (attacker->getPosition().canMoveCreature(pos)) {
          attacker->you(MsgType::TELE_DISAPPEAR, "");
          attacker->getPosition().moveCreature(pos, true);
          attacker->you(MsgType::TELE_APPEAR, "");
          return true;
        }
        return false;
      },
      [&](Effects::Jump) {
        auto from = attacker->getPosition();
        if (pos.canEnter(attacker)) {
          for (auto v : drawLine(from, pos))
            if (v != from && !v.canEnter(MovementType({MovementTrait::WALK, MovementTrait::FLY})))
              return false;
          attacker->displace(attacker->getPosition().getDir(pos));
          return true;
        }
        return false;
      },
      [&](Effects::Fire) {
        pos.getGame()->addEvent(EventInfo::FX{pos, {FXName::FIREBALL_SPLASH}});
        return pos.fireDamage(1);
      },
      [&](Effects::Ice) {
        //pos.getGame()->addEvent(EventInfo::FX{pos, {FXName::FIREBALL_SPLASH}});
        return pos.iceDamage();
      },
      [&](Effects::Acid) {
        return pos.acidDamage();
      },
      [&](const Effects::Area& area) {
        bool res = false;
        for (auto v : pos.getRectangle(Rectangle::centered(area.radius)))
          res |= area.effect->apply(v, attacker);
        return res;
      },
      [&](const Effects::CustomArea& area) {
        bool res = false;
        for (auto& v : area.getTargetPos(attacker, pos))
          res |= area.effect->apply(v, attacker);
        return res;
      },
      [&](const Effects::Chain& chain) {
        bool res = false;
        for (auto& e : chain.effects)
          res |= e.apply(pos, attacker);
        return res;
      },
      [&](const Effects::ChainFirstResult& chain) {
        bool res = false;
        for (int i : All(chain.effects)) {
          auto value = chain.effects[i].apply(pos, attacker);
          if (i == 0)
            res = value;
        }
        return res;
      },
      [&](const Effects::FirstSuccessful& chain) {
        for (auto& e : chain.effects)
          if (e.apply(pos, attacker))
            return true;
        return false;
      },
      [&](const Effects::ChooseRandom& r) {
        return r.effects[Random.get(r.effects.size())].apply(pos, attacker);
      },
      [&](const Effects::Message& msg) {
        pos.globalMessage(msg.text);
        return true;
      },
      [&](const Effects::Caster& e) {
        if (attacker)
          return e.effect->apply(attacker->getPosition(), attacker);
        return false;
      },
      [&](const Effects::Chance& e) {
        if (Random.chance(e.value))
          return e.effect->apply(pos, attacker);
        return false;
      },
      [&](const Effects::Description& e) {
        return e.effect->apply(pos, attacker);
      },
      [&](const Effects::Name& e) {
        return e.effect->apply(pos, attacker);
      },
      [&](const Effects::AIBelowHealth& e) {
        return e.effect->apply(pos, attacker);
      },
      [&](const Effects::SummonEnemy& summon) {
        CreatureGroup f = CreatureGroup::singleType(TribeId::getMonster(), summon.creature);
        return !Effect::summon(pos, f, Random.get(summon.count), summon.ttl.map([](int v) { return TimeInterval(v); }), 1_visible).empty();
      },
      [&](const Effects::PlaceFurniture& effect) {
        auto f = pos.getGame()->getContentFactory()->furniture.getFurniture(effect.furniture,
            attacker ? attacker->getTribeId() : TribeId::getMonster());
        auto layer = f->getLayer();
        auto ref = f.get();
        pos.addFurniture(std::move(f));
        if (pos.getFurniture(layer) == ref)
          ref->onConstructedBy(pos, attacker);
        return true;
      },
      [&](const Effects::DropItems& effect) {
        pos.dropItems(effect.type.get(Random.get(effect.count), pos.getGame()->getContentFactory()));
        return true;
      },
      [&](Effects::Blast) {
        constexpr int range = 4;
        CHECK(attacker);
        vector<Position> trajectory;
        auto origin = attacker->getPosition().getCoord();
        for (auto& v : drawLine(origin, pos.getCoord()))
          if (v != origin && v.dist8(origin) <= range) {
            trajectory.push_back(Position(v, pos.getLevel()));
            if (trajectory.back().isDirEffectBlocked())
              break;
          }
        for (int i : All(trajectory).reverse())
          airBlast(attacker, attacker->getPosition(), trajectory[i], pos);
        return true;
      },
      [&](Effects::Pull) {
        CHECK(attacker);
        vector<Position> trajectory = drawLine(attacker->getPosition(), pos);
        for (auto& pos : trajectory)
          if (auto c = pos.getCreature())
            return pullCreature(c, trajectory);
        return false;
      },
      [&](const Effects::AssembledMinion& m) {
        auto group = CreatureGroup::singleType(attacker->getTribeId(), m.creature);
        auto c = Effect::summon(pos, group, 1, none);
        if (!c.empty()) {
          for (auto col : pos.getGame()->getCollectives())
            if (col->getCreatures().contains(attacker)) {
              for (auto& elem : col->getImmigration().getImmigrants())
                if (elem.getId(0) == m.creature) {
                  col->addCreature(c[0], elem.getTraits());
                  return true;
                }
            }
        }
        return false;
      },
      [&](const Effects::DestroyWalls& m) {
        bool res = false;
        for (auto furniture : pos.modFurniture())
          if (furniture->canDestroy(m.action)) {
            furniture->destroy(pos, m.action);
            res = true;
          }
        return res;
      },
      [&](const Effects::EmitPoisonGas& m) {
        Effect::emitPoisonGas(pos, m.amount, true);
        return true;
      },
      [&](const Effects::Fx& fx) {
        if (auto game = pos.getGame())
          game->addEvent(EventInfo::FX{pos, fx.info});
        return true;
      },
      [&](const Effects::TriggerTrap&) {
        for (auto furniture : pos.getFurniture())
          if (auto& entry = furniture->getEntryType())
            if (auto trapInfo = entry->entryData.getReferenceMaybe<FurnitureEntry::Trap>()) {
              pos.globalMessage("A " + trapInfo->effect.getName(pos.getGame()->getContentFactory()) + " trap is triggered");
              auto effect = trapInfo->effect;
              pos.removeFurniture(furniture);
              effect.apply(pos, attacker);
              return true;
            }
        return false;
      },
      [&](const Effects::AnimateItems& m) {
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
      },
      [&](const Effects::SoundEffect& e) {
        pos.addSound(e.sound);
        return true;
      },
      [&](const Effects::Audience& a) {
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
          auto distance = enemy->getPosition().dist8(pos);
          if ((!a.maxDistance || *a.maxDistance >= distance.value_or(10000)) &&
              (distance.value_or(4) > 3 || !pos.canSee(enemy->getPosition(), Vision())))
            if (auto landing = pos.getLevel()->getClosestLanding({pos}, enemy)) {
              enemy->getPosition().moveCreature(*landing, true);
              wasTeleported = true;
              enemy->removeEffect(LastingEffect::SLEEP);
            }
        };
        if (collective) {
          for (auto enemy : collective->getCreatures(MinionTrait::FIGHTER))
            tryTeleporting(enemy);
          for (auto l : collective->getLeaders())
            tryTeleporting(l);
        }
        for (auto enemy : pos.getLevel()->getAllCreatures())
          if (!collective || !collective->getCreatures().contains(enemy))
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
  );
}

string Effect::getDescription(const ContentFactory* f) const {
  return FORWARD_CALL(string, effect, getDescription, f);
}

static bool isConsideredInDanger(const Creature* c) {
  if (auto intent = c->getLastCombatIntent())
    return (intent->time > *c->getGlobalTime() - 5_visible);
  return false;
}

EffectAIIntent Effect::shouldAIApply(const Creature* victim, bool isEnemy) const {
  bool isFighting = isConsideredInDanger(victim);
  return effect->visit<EffectAIIntent>(
      [&] (const Effects::Permanent& e) {
        if (victim->getAttributes().isAffectedPermanently(e.lastingEffect))
          return EffectAIIntent::NONE;
        return LastingEffects::shouldAIApply(victim, e.lastingEffect, isEnemy);
      },
      [&] (const Effects::Lasting& e) {
        if (victim->isAffected(e.lastingEffect))
          return EffectAIIntent::NONE;
        return LastingEffects::shouldAIApply(victim, e.lastingEffect, isEnemy);
      },
      [&] (const Effects::RemoveLasting& e) {
        if (!victim->isAffected(e.lastingEffect))
          return EffectAIIntent::NONE;
        return reverse(LastingEffects::shouldAIApply(victim, e.lastingEffect, isEnemy));
      },
      [&] (const Effects::Heal& e) {
        if (victim->getBody().canHeal(e.healthType))
          return isEnemy ? EffectAIIntent::UNWANTED : EffectAIIntent::WANTED;
        return EffectAIIntent::NONE;
      },
      [&] (const Effects::IncreaseMorale& e) {
        return isEnemy == (e.amount < 0) ? EffectAIIntent::WANTED : EffectAIIntent::UNWANTED;
      },
      [&] (const Effects::Fire&) {
        if (!victim->isAffected(LastingEffect::FIRE_RESISTANT))
          return isEnemy ? EffectAIIntent::WANTED : EffectAIIntent::UNWANTED;
        return EffectAIIntent::NONE;
      },
      [&] (const Effects::Ice&) {
        if (!victim->isAffected(LastingEffect::COLD_RESISTANT))
          return isEnemy ? EffectAIIntent::WANTED : EffectAIIntent::UNWANTED;
        return EffectAIIntent::NONE;
      },
      [&] (const Effects::Suicide&) {
        return isEnemy ? EffectAIIntent::WANTED : EffectAIIntent::UNWANTED;
      },
      [&] (const Effects::Damage&) {
        return isEnemy ? EffectAIIntent::WANTED : EffectAIIntent::UNWANTED;
      },
      [&] (const Effects::DestroyEquipment&) {
        return isEnemy ? EffectAIIntent::WANTED : EffectAIIntent::UNWANTED;
      },
      [&] (const Effects::Acid&) {
        return isEnemy ? EffectAIIntent::WANTED : EffectAIIntent::UNWANTED;
      },
      [&] (const Effects::Deception&) {
        return isFighting ? EffectAIIntent::WANTED : EffectAIIntent::NONE;
      },
      [&] (const Effects::Summon&) {
        return isFighting ? EffectAIIntent::WANTED : EffectAIIntent::NONE;
      },
      [&] (const Effects::Audience&) {
        return isFighting ? EffectAIIntent::WANTED : EffectAIIntent::NONE;
      },
      [&] (const Effects::DoubleTrouble&) {
        return isFighting ? EffectAIIntent::WANTED : EffectAIIntent::NONE;
      },
      [&] (const Effects::SummonElement&) {
        return isFighting ? EffectAIIntent::WANTED : EffectAIIntent::NONE;
      },
      [&] (const Effects::PlaceFurniture& f) {
        return victim->getGame()->getContentFactory()->furniture.getData(f.furniture).isHostileSpell() && isFighting
            ? EffectAIIntent::WANTED : EffectAIIntent::NONE;
      },
      [&] (const Effects::RegrowBodyPart&) {
        for (auto part : ENUM_ALL(BodyPart))
          if (victim->getBody().numLost(part) + victim->getBody().numInjured(part) > 0)
            return EffectAIIntent::WANTED;
        return EffectAIIntent::NONE;
      },
      [&] (const auto&) {
        return EffectAIIntent::NONE;
      }
  );
}

/* Unimplemented: Teleport, EnhanceArmor, EnhanceWeapon, Suicide, IncreaseAttr,
      EmitPoisonGas, CircularBlast, Alarm, SilverDamage, DoubleTrouble,
      PlaceFurniture, InjureBodyPart, LooseBodyPart, RegrowBodyPart, DestroyWalls,
      ReviveCorpse, Blast, Shove, SwapPosition*/

EffectAIIntent Effect::shouldAIApply(const Creature* caster, Position pos) const {
  PROFILE_BLOCK("Effect::shouldAIApply");
  auto victim = pos.getCreature();
  if (victim && !caster->canSee(victim))
    victim = nullptr;
  bool isEnemy = false;
  if (victim && !victim->isAffected(LastingEffect::STUNNED)) {
    isEnemy = caster->isEnemy(victim);
    auto res = shouldAIApply(victim, isEnemy);
    if (res != EffectAIIntent::NONE)
      return res;
  }
  auto considerArea = [&](const auto& range, const Effect& effect) {
    auto allRes = EffectAIIntent::UNWANTED;
    for (auto v : range) {
      auto res = effect.shouldAIApply(caster, v);
      if (res == EffectAIIntent::UNWANTED)
        return EffectAIIntent::UNWANTED;
      if (res == EffectAIIntent::WANTED)
        allRes = res;
    }
    return allRes;
  };
  return effect->visit<EffectAIIntent>(
      [&] (const Effects::Wish&){
        if (victim && victim->isPlayer() && !isEnemy)
          return EffectAIIntent::WANTED;
        else
          return EffectAIIntent::UNWANTED;
      },
      [&] (const Effects::Chain& chain){
        auto allRes = EffectAIIntent::UNWANTED;
        for (auto& e : chain.effects) {
          auto res = e.shouldAIApply(caster, pos);
          if (res == EffectAIIntent::UNWANTED)
            return EffectAIIntent::UNWANTED;
          if (res == EffectAIIntent::WANTED)
            allRes = res;
        }
        return allRes;
      },
      [&] (const Effects::Area& a) {
        return considerArea(pos.getRectangle(Rectangle::centered(a.radius)), *a.effect);
      },
      [&] (const Effects::CustomArea& a) {
        return considerArea(a.getTargetPos(caster, pos), *a.effect);
      },
      [&] (const Effects::Filter& e) {
        if (victim && e.applies(victim, caster))
          return e.effect->shouldAIApply(caster, pos);
        return EffectAIIntent::NONE;
      },
      [&] (const Effects::Chance& e) {
        return e.effect->shouldAIApply(caster, pos);
      },
      [&] (const Effects::Description& e) {
        return e.effect->shouldAIApply(caster, pos);
      },
      [&] (const Effects::Name& e) {
        return e.effect->shouldAIApply(caster, pos);
      },
      [&] (const Effects::AIBelowHealth& e) {
        if (caster->getBody().getHealth() <= e.value)
          return e.effect->shouldAIApply(caster, pos);
        return EffectAIIntent::UNWANTED;
      },
      [&](const Effects::AnimateItems& m) {
        if (caster && isConsideredInDanger(caster)) {
          int totalWeapons = 0;
          for (auto v : pos.getRectangle(Rectangle::centered(m.radius))) {
            totalWeapons += v.getItems(ItemIndex::WEAPON).size();
            if (totalWeapons >= m.maxCount / 2)
              return EffectAIIntent::WANTED;
          }
        }
        return EffectAIIntent::NONE;
      },
      [&] (const auto&) { return EffectAIIntent::NONE; }
  );
}

static optional<FXInfo> getProjectileFX(LastingEffect effect) {
  switch (effect) {
    default:
      return none;
  }
}

static optional<ViewId> getProjectile(LastingEffect effect) {
  switch (effect) {
    default:
      return none;
  }
}
optional<FXInfo> Effect::getProjectileFX() const {
  return effect->visit<optional<FXInfo>>(
      [&](const auto&) { return none; },
      [&](const Effects::Lasting& e) -> optional<FXInfo> { return ::getProjectileFX(e.lastingEffect); },
      [&](const Effects::Damage&) -> optional<FXInfo> { return {FXName::MAGIC_MISSILE}; },
      [&](const Effects::Blast&) -> optional<FXInfo> { return {FXName::AIR_BLAST}; },
      [&](const Effects::Pull&) -> optional<FXInfo> { return FXInfo{FXName::AIR_BLAST}.setReversed(); },
      [&](const Effects::Chance& e) -> optional<FXInfo> { return e.effect->getProjectileFX(); },
      [&](const Effects::Description& e) -> optional<FXInfo> { return e.effect->getProjectileFX(); },
      [&](const Effects::Name& e) -> optional<FXInfo> { return e.effect->getProjectileFX(); },
      [&](const Effects::AIBelowHealth& e) -> optional<FXInfo> { return e.effect->getProjectileFX(); }
  );
}

optional<ViewId> Effect::getProjectile() const {
  return effect->visit<optional<ViewId>>(
      [&](const auto&) -> optional<ViewId> { return none; },
      [&](const Effects::Lasting& e) -> optional<ViewId> { return ::getProjectile(e.lastingEffect); },
      [&](const Effects::Damage&) -> optional<ViewId> { return ViewId("force_bolt"); },
      [&](const Effects::Fire&) -> optional<ViewId> { return ViewId("fireball"); },
      [&](const Effects::Blast&) -> optional<ViewId> { return ViewId("air_blast"); },
      [&](const Effects::Chance& e) -> optional<ViewId> { return e.effect->getProjectile(); },
      [&](const Effects::Description& e) -> optional<ViewId> { return e.effect->getProjectile(); },
      [&](const Effects::Name& e) -> optional<ViewId> { return e.effect->getProjectile(); },
      [&](const Effects::AIBelowHealth& e) -> optional<ViewId> { return e.effect->getProjectile(); }
  );
}

vector<Effect> Effect::getWishedForEffects() {
  vector<Effect> allEffects {
       Effect(Effects::Escape{}),
       Effect(Effects::Heal{HealthType::FLESH}),
       Effect(Effects::Heal{HealthType::SPIRIT}),
       Effect(Effects::Ice{}),
       Effect(Effects::Fire{}),
       Effect(Effects::DestroyEquipment{}),
       Effect(Effects::Area{2,  Effects::DestroyWalls{DestroyAction::Type::BOULDER}}),
       Effect(Effects::Enhance{ItemUpgradeType::WEAPON, 2}),
       Effect(Effects::Enhance{ItemUpgradeType::ARMOR, 2}),
       Effect(Effects::Enhance{ItemUpgradeType::WEAPON, -2}),
       Effect(Effects::Enhance{ItemUpgradeType::ARMOR, -2}),
       Effect(Effects::CircularBlast{}),
       Effect(Effects::Deception{}),
       Effect(Effects::SummonElement{}),
       Effect(Effects::Acid{}),
       Effect(Effects::Suicide{MsgType::DIE}),
       Effect(Effects::DoubleTrouble{})
  };
  for (auto effect : ENUM_ALL(LastingEffect)) {
    allEffects.push_back(EffectType(Effects::Lasting{effect}));
    allEffects.push_back(EffectType(Effects::Permanent{effect}));
    allEffects.push_back(EffectType(Effects::RemoveLasting{effect}));
  }
  for (auto attr : ENUM_ALL(AttrType))
    allEffects.push_back(EffectType(Effects::IncreaseAttr{attr, (attr == AttrType::PARRY ? 2 : 5)}));
  return allEffects;
}

optional<MinionEquipmentType> Effect::getMinionEquipmentType() const {
  return effect->visit<optional<MinionEquipmentType>>(
      [&](const Effects::IncreaseMorale&) -> optional<MinionEquipmentType> { return MinionEquipmentType::COMBAT_ITEM; },
      [&](const Effects::Chance& e) -> optional<MinionEquipmentType> { return e.effect->getMinionEquipmentType(); },
      [&](const Effects::Description& e) -> optional<MinionEquipmentType> { return e.effect->getMinionEquipmentType(); },
      [&](const Effects::Name& e) -> optional<MinionEquipmentType> { return e.effect->getMinionEquipmentType(); },
      [&](const Effects::AIBelowHealth& e) -> optional<MinionEquipmentType> { return e.effect->getMinionEquipmentType(); },
      [&](const Effects::Area& a) -> optional<MinionEquipmentType> { return a.effect->getMinionEquipmentType(); },
      [&](const Effects::Filter& f) -> optional<MinionEquipmentType> { return f.effect->getMinionEquipmentType(); },
      [&](const Effects::Escape&) -> optional<MinionEquipmentType> { return MinionEquipmentType::COMBAT_ITEM; },
      [&](const Effects::Chain& c) -> optional<MinionEquipmentType> {
        for (auto& e : c.effects)
          if (auto t = e.getMinionEquipmentType())
            return t;
        return none;
      },
      [&](const Effects::Heal& e) -> optional<MinionEquipmentType> {
        if (e.healthType == HealthType::FLESH)
          return MinionEquipmentType::HEALING;
        else
          return MinionEquipmentType::MATERIALIZATION;
      },
      [&](const Effects::Lasting& e) -> optional<MinionEquipmentType> {
        return MinionEquipmentType::COMBAT_ITEM;
      },
      [&](const auto&) -> optional<MinionEquipmentType> { return none; }
  );
}

bool Effect::canAutoAssignMinionEquipment() const {
  return effect->visit<bool>(
      [&](const Effects::Suicide&) { return false; },
      [&](const Effects::Chance& e) { return e.effect->canAutoAssignMinionEquipment(); },
      [&](const Effects::Description& e) { return e.effect->canAutoAssignMinionEquipment(); },
      [&](const Effects::Name& e) { return e.effect->canAutoAssignMinionEquipment(); },
      [&](const Effects::AIBelowHealth& e) { return e.effect->canAutoAssignMinionEquipment(); },
      [&](const Effects::Area& a) { return a.effect->canAutoAssignMinionEquipment(); },
      [&](const Effects::Filter& f) { return f.effect->canAutoAssignMinionEquipment(); },
      [&](const Effects::Chain& c) {
        for (auto& e : c.effects)
          if (!e.canAutoAssignMinionEquipment())
            return false;
        return true;
      },
      [&](const auto&) { return true; }
  );
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
template<>
#include "gen_variant_serialize_pretty.h"
}
#undef VARIANT_TYPES_LIST
#undef VARIANT_NAME
