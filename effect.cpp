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

vector<Creature*> Effect::summonCreatures(Position pos, vector<PCreature> creatures, TimeInterval delay) {
  vector<Creature*> ret;
  for (int i : All(creatures))
    if (auto v = pos.getLevel()->getClosestLanding({pos}, creatures[i].get())) {
      ret.push_back(creatures[i].get());
      v->addCreature(std::move(creatures[i]), delay);
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

static void summonFX(Creature* c) {
  auto color = Color(240, 146, 184);
  // TODO: color depending on creature type ?

  c->getGame()->addEvent(EventInfo::FX{c->getPosition(), {FXName::SPAWN, color}});
}

vector<Creature*> Effect::summon(Creature* c, CreatureId id, int num, optional<TimeInterval> ttl, TimeInterval delay) {
  vector<PCreature> creatures;
  for (int i : Range(num))
    creatures.push_back(c->getGame()->getContentFactory()->getCreatures().fromId(id, c->getTribeId(), MonsterAIFactory::summoned(c)));
  auto ret = summonCreatures(c->getPosition(), std::move(creatures), delay);
  for (auto c : ret) {
    if (ttl)
      c->addEffect(LastingEffect::SUMMONED, *ttl, false);
    summonFX(c);
  }
  return ret;
}

vector<Creature*> Effect::summon(Position pos, CreatureGroup& factory, int num, optional<TimeInterval> ttl, TimeInterval delay) {
  vector<PCreature> creatures;
  for (int i : Range(num))
    creatures.push_back(factory.random(&pos.getGame()->getContentFactory()->getCreatures(), MonsterAIFactory::monster()));
  auto ret = summonCreatures(pos, std::move(creatures), delay);
  for (auto c : ret) {
    if (ttl)
      c->addEffect(LastingEffect::SUMMONED, *ttl, false);
    summonFX(c);
  }
  return ret;
}

static void enhanceArmor(Creature* c, int mod, const string& msg) {
  for (EquipmentSlot slot : Random.permutation(getKeys(Equipment::slotTitles)))
    for (Item* item : c->getEquipment().getSlotItems(slot))
      if (item->getClass() == ItemClass::ARMOR) {
        c->you(MsgType::YOUR, item->getName() + " " + msg);
        if (item->getModifier(AttrType::DEFENSE) > 0 || mod > 0)
          item->addModifier(AttrType::DEFENSE, mod);
        return;
      }
}

static void enhanceWeapon(Creature* c, int mod, const string& msg) {
  if (auto item = c->getFirstWeapon()) {
    c->you(MsgType::YOUR, item->getName() + " " + msg);
    item->addModifier(item->getWeaponInfo().meleeAttackAttr, mod);
  }
}

static void summon(Creature* summoner, CreatureId id, Range count, bool hostile) {
  if (hostile) {
    CreatureGroup f = CreatureGroup::singleType(TribeId::getHostile(), id);
    Effect::summon(summoner->getPosition(), f, Random.get(count), 100_visible, 1_visible);
  } else
    Effect::summon(summoner, id, Random.get(count), 100_visible, 1_visible);
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
  return effect->visit(
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

void Effects::Escape::applyToCreature(Creature* c, Creature* attacker) const {
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
    return;
  }
  CHECK(!good.empty());
  c->you(MsgType::TELE_DISAPPEAR, "");
  c->getPosition().moveCreature(Random.choose(good), true);
  c->you(MsgType::TELE_APPEAR, "");
}

string Effects::Escape::getName() const {
  return "escape";
}

string Effects::Escape::getDescription() const {
  return "Teleports to a safer location close by.";
}

void Effects::Teleport::applyToCreature(Creature* c, Creature* attacker) const {
}

string Effects::Teleport::getName() const {
  return "escape";
}

string Effects::Teleport::getDescription() const {
  return "Teleport to any location that's close by.";
}

void Effects::Lasting::applyToCreature(Creature* c, Creature* attacker) const {
  if (c->addEffect(lastingEffect, LastingEffects::getDuration(c, lastingEffect)))
    if (auto fx = LastingEffects::getApplicationFX(lastingEffect))
      c->addFX(*fx);
}

string Effects::Lasting::getName() const {
  return LastingEffects::getName(lastingEffect);
}

string Effects::Lasting::getDescription() const {
  // Leave out the full stop.
  string desc = LastingEffects::getDescription(lastingEffect);
  return desc.substr(0, desc.size() - 1) + " for some turns.";
}

void Effects::RemoveLasting::applyToCreature(Creature* c, Creature* attacker) const {
  if (c->removeEffect(lastingEffect))
    c->addFX(FXInfo(FXName::CIRCULAR_SPELL, Color::WHITE));
}

string Effects::RemoveLasting::getName() const {
  return "remove " + LastingEffects::getName(lastingEffect);
}

string Effects::RemoveLasting::getDescription() const {
  return "Removes/cures from effect: " + LastingEffects::getName(lastingEffect);
}

void Effects::IncreaseAttr::applyToCreature(Creature* c, Creature*) const {
  c->you(MsgType::YOUR, ::getName(attr) + get(" improves", " wanes"));
  c->getAttributes().increaseBaseAttr(attr, amount);
}

string Effects::IncreaseAttr::getName() const {
  return ::getName(attr) + get(" boost", " loss");
}

string Effects::IncreaseAttr::getDescription() const {
  return get("Increases", "Decreases") + " your "_s + ::getName(attr) + " by " + toString(abs(amount));
}

const char* Effects::IncreaseAttr::get(const char* ifIncrease, const char* ifDecrease) const {
  if (amount > 0)
    return ifIncrease;
  else
    return ifDecrease;
}

void Effects::Permanent::applyToCreature(Creature* c, Creature* attacker) const {
  c->addPermanentEffect(lastingEffect);
}

string Effects::Permanent::getName() const {
  return "permanent " + LastingEffects::getName(lastingEffect);
}

string Effects::Permanent::getDescription() const {
  string desc = LastingEffects::getDescription(lastingEffect);
  return desc.substr(0, desc.size() - 1) + " permanently.";
}

void Effects::TeleEnemies::applyToCreature(Creature*, Creature* attacker) const {
}

string Effects::TeleEnemies::getName() const {
  return "surprise";
}

string Effects::TeleEnemies::getDescription() const {
  return "Surprise!";
}

void Effects::Alarm::applyToCreature(Creature* c, Creature* attacker) const {
  c->getGame()->addEvent(EventInfo::Alarm{c->getPosition(), silent});
}

string Effects::Alarm::getName() const {
  return "alarm";
}

string Effects::Alarm::getDescription() const {
  return "Alarm!";
}

void Effects::Acid::applyToCreature(Creature* c, Creature* attacker) const {
  c->affectByAcid();
  switch (Random.get(2)) {
    case 0 : enhanceArmor(c, -1, "corrodes"); break;
    case 1 : enhanceWeapon(c, -1, "corrodes"); break;
  }
}

string Effects::Acid::getName() const {
  return "acid";
}

string Effects::Acid::getDescription() const {
  return "Causes acid damage to skin and equipment.";
}

void Effects::Summon::applyToCreature(Creature* c, Creature* attacker) const {
  ::summon(c, creature, count, false);
}

/*static string getCreaturePluralName(CreatureId id) {
  static EnumMap<CreatureId, optional<string>> names;
  if (!names[id])
   names[id] = CreatureFactory::fromId(id, TribeId::getHuman())->getName().plural();
  return *names[id];
}*/

static string getCreatureName(CreatureId id) {
  string ret = toLower(id.data());
  std::replace(ret.begin(), ret.end(), '_', ' ');
  return ret;
  /*if (getSummonNumber(id).getEnd() > 2)
    return getCreaturePluralName(id);
  static EnumMap<CreatureId, optional<string>> names;
  if (!names[id])
    names[id] = CreatureFactory::fromId(id, TribeId::getHuman())->getName().bare();
  return *names[id];*/
}

/*static string getCreatureAName(CreatureId id) {
  static map<CreatureId, string> names;
  if (!names.count(id))
    names[id] = CreatureFactory::fromId(id, TribeId::getHuman())->getName().a();
  return names.at(id);
}*/

string Effects::Summon::getName() const {
  return "summon " + getCreatureName(creature);
}

string Effects::Summon::getDescription() const {
  if (count.getEnd() > 2)
    return "Summons " + toString(count.getStart()) + " to " + toString(count.getEnd() - 1)
        + " " + getCreatureName(creature);
  else
    return "Summons a " + getCreatureName(creature);
}

void Effects::SummonEnemy::applyToCreature(Creature* c, Creature* attacker) const {
}

string Effects::SummonEnemy::getName() const {
  return "summon hostile " + getCreatureName(creature);
}

string Effects::SummonEnemy::getDescription() const {
  if (count.getEnd() > 2)
    return "Summons " + toString(count.getStart()) + " to " + toString(count.getEnd() - 1)
        + " hostile " + getCreatureName(creature);
  else
    return "Summons a hostile " + getCreatureName(creature);
}

void Effects::SummonElement::applyToCreature(Creature* c, Creature* attacker) const {
  auto id = CreatureId("AIR_ELEMENTAL");
  for (Position p : c->getPosition().getRectangle(Rectangle::centered(3)))
    for (auto f : p.getFurniture())
      if (auto elem = f->getSummonedElement())
        id = *elem;
  ::summon(c, id, Range(1, 2), false);
}

string Effects::SummonElement::getName() const {
  return "summon element";
}

string Effects::SummonElement::getDescription() const {
  return "Summons an element or spirit from the surroundings.";
}

void Effects::Deception::applyToCreature(Creature* c, Creature* attacker) const {
  vector<PCreature> creatures;
  for (int i : Range(Random.get(3, 7)))
    creatures.push_back(CreatureFactory::getIllusion(c));
  Effect::summonCreatures(c->getPosition(), std::move(creatures));
}

string Effects::Deception::getName() const {
  return "deception";
}

string Effects::Deception::getDescription() const {
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
      trajectory = getSubsequence(trajectory, i + 1, maxDistance);
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

void Effects::CircularBlast::applyToCreature(Creature* c, Creature* attacker) const {
  for (Vec2 v : Vec2::directions8(Random))
    airBlast(attacker, c->getPosition(), c->getPosition().plus(v), c->getPosition().plus(v * 10));
  c->addFX({FXName::CIRCULAR_BLAST});
}

string Effects::CircularBlast::getName() const {
  return "air blast";
}

string Effects::CircularBlast::getDescription() const {
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

void Effects::Enhance::applyToCreature(Creature* c, Creature* attacker) const {
  switch (type) {
    case ItemUpgradeType::WEAPON:
      enhanceWeapon(c, amount, amountAs("is improved", "degrades"));
      break;
    case ItemUpgradeType::ARMOR:
      enhanceArmor(c, amount, amountAs("is improved", "degrades"));
      break;
  }
}

string Effects::Enhance::getName() const {
  return typeAsString() + " "_s + amountAs("enchantment", "degradation");
}

string Effects::Enhance::getDescription() const {
  return amountAs("Increases", "Decreases") + " "_s + typeAsString() + " capability"_s;
}

void Effects::DestroyEquipment::applyToCreature(Creature* c, Creature* attacker) const {
  auto equipped = c->getEquipment().getAllEquipped();
  if (!equipped.empty()) {
    Item* dest = Random.choose(equipped);
    c->you(MsgType::YOUR, dest->getName() + " crumbles to dust.");
    c->steal({dest});
  }
}

string Effects::DestroyEquipment::getName() const {
  return "equipment destruction";
}

string Effects::DestroyEquipment::getDescription() const {
  return "Destroys a random piece of equipment.";
}

void Effects::DestroyWalls::applyToCreature(Creature* c, Creature* attacker) const {
  PROFILE;
  for (auto pos : c->getPosition().neighbors8())
    for (auto furniture : pos.modFurniture())
      if (furniture->canDestroy(DestroyAction::Type::BOULDER))
        furniture->destroy(pos, DestroyAction::Type::BOULDER);
}

string Effects::DestroyWalls::getName() const {
  return "wall destruction";
}

string Effects::DestroyWalls::getDescription() const {
  return "Destroys walls in adjacent tiles.";
}

void Effects::Heal::applyToCreature(Creature* c, Creature* attacker) const {
  if (c->getBody().canHeal(healthType)) {
    c->heal(1);
    c->removeEffect(LastingEffect::BLEEDING);
    c->addFX(FXInfo(FXName::CIRCULAR_SPELL, Color::LIGHT_GREEN));
  } else
    c->message("Nothing happens.");
}

string Effects::Heal::getName() const {
  switch (healthType) {
    case HealthType::FLESH: return "healing";
    case HealthType::SPIRIT: return "materialization";
  }
}

string Effects::Heal::getDescription() const {
  switch (healthType) {
    case HealthType::FLESH: return "Fully restores health.";
    case HealthType::SPIRIT: return "Fully re-materializes a spirit.";
  }
}

void Effects::Fire::applyToCreature(Creature* c, Creature* attacker) const {
}

string Effects::Fire::getName() const {
  return "fire";
}

string Effects::Fire::getDescription() const {
  return "Burns!";
}

void Effects::Ice::applyToCreature(Creature* c, Creature* attacker) const {
}

string Effects::Ice::getName() const {
  return "ice";
}

string Effects::Ice::getDescription() const {
  return "Freezes water and causes cold damage";
}

void Effects::ReviveCorpse::applyToCreature(Creature* c, Creature* attacker) const {
}

string Effects::ReviveCorpse::getName() const {
  return "revive corpse";
}

string Effects::ReviveCorpse::getDescription() const {
  return "Brings a dead corpse back alive as a servant";
}

PCreature Effects::SummonGhost::getBestSpirit(const Model* model, TribeId tribe) const {
  auto& factory = model->getGame()->getContentFactory()->getCreatures();
  for (auto id : Random.permutation(factory.getAllCreatures())) {
    auto orig = factory.fromId(id, tribe);
    if (orig->getBody().hasBrain())
      return orig;
  }
  return nullptr;
}

void Effects::SummonGhost::applyToCreature(Creature* c, Creature* attacker) const {
  auto spirits = Effect::summon(c, CreatureId("SPIRIT"), Random.get(count), ttl);
  if (spirits.empty())
    attacker->message("The spell failed");
  for (auto spirit : spirits) {
    spirit->getAttributes().setBaseAttr(AttrType::DAMAGE, 0);
    spirit->getAttributes().setBaseAttr(AttrType::RANGED_DAMAGE, 0);
    spirit->getAttributes().setBaseAttr(AttrType::DEFENSE, ghostPower);
    spirit->getAttributes().setBaseAttr(AttrType::SPELL_DAMAGE, ghostPower);
    spirit->modViewObject().setModifier(ViewObject::Modifier::ILLUSION);
    auto orig = getBestSpirit(c->getPosition().getModel(), c->getTribeId());
    spirit->getAttributes().getName() = orig->getAttributes().getName();
    spirit->getAttributes().getName().setFirst(none);
    spirit->getAttributes().getName().useFullTitle(false);
    spirit->modViewObject().setId(orig->getViewObject().id());
    spirit->getName().addBareSuffix("spirit");
    spirit->updateViewObject();
    c->verb("have", "has", "summoned " + spirit->getName().a());
  }
}

string Effects::SummonGhost::getName() const {
  return "summon spirit";
}

string Effects::SummonGhost::getDescription() const {
  return "Summons a dead creature's spirit as a servant";
}

void Effects::EmitPoisonGas::applyToCreature(Creature* c, Creature* attacker) const {
  Effect::emitPoisonGas(c->getPosition(), amount, true);
}

string Effects::EmitPoisonGas::getName() const {
  return "poison gas";
}

string Effects::EmitPoisonGas::getDescription() const {
  return "Emits poison gas";
}

void Effects::SilverDamage::applyToCreature(Creature* c, Creature* attacker) const {
  c->affectBySilver();
}

string Effects::SilverDamage::getName() const {
  return "silver";
}

string Effects::SilverDamage::getDescription() const {
  return "Hurts the undead.";
}

void Effects::PlaceFurniture::applyToCreature(Creature* c, Creature* attacker) const {
}

string Effects::PlaceFurniture::getName() const {
  return furniture.data();
}

string Effects::PlaceFurniture::getDescription() const {
  return "Creates a " + getName() + ".";
}

void Effects::Damage::applyToCreature(Creature* c, Creature* attacker) const {
  CHECK(attacker) << "Unknown attacker";
  c->takeDamage(Attack(attacker, Random.choose<AttackLevel>(), attackType, attacker->getAttr(attr), attr));
  if (attr == AttrType::SPELL_DAMAGE)
    c->addFX({FXName::MAGIC_MISSILE_SPLASH});
}

string Effects::Damage::getName() const {
  return ::getName(attr);
}

string Effects::Damage::getDescription() const {
  return "Causes " + ::getName(attr);
}

void Effects::InjureBodyPart::applyToCreature(Creature* c, Creature* attacker) const {
  if (c->getBody().injureBodyPart(c, part, false)) {
    c->you(MsgType::DIE, "");
    c->dieWithAttacker(attacker);
  }
}

string Effects::InjureBodyPart::getName() const {
  return "injure "_s + ::getName(part);
}

string Effects::InjureBodyPart::getDescription() const {
  return "Injures "_s + ::getName(part);
}

void Effects::LooseBodyPart::applyToCreature(Creature* c, Creature* attacker) const {
  if (c->getBody().injureBodyPart(c, part, true)) {
    c->you(MsgType::DIE, "");
    c->dieWithAttacker(attacker);
  }
}

string Effects::LooseBodyPart::getName() const {
  return "lose "_s + ::getName(part);
}

string Effects::LooseBodyPart::getDescription() const {
  return "Causes you to lose a "_s + ::getName(part);
}

void Effects::RegrowBodyPart::applyToCreature(Creature* c, Creature* attacker) const {
  c->getBody().healBodyParts(c, true);
}

string Effects::RegrowBodyPart::getName() const {
  return "regrow lost body parts";
}

string Effects::RegrowBodyPart::getDescription() const {
  return "Causes lost body parts to regrow.";
}

void Effects::Area::applyToCreature(Creature* c, Creature* attacker) const {
}

string Effects::Area::getName() const {
  return "area " + effect->getName();
}

string Effects::Area::getDescription() const {
  return "Area effect of radius " + toString(radius) + ": " + noCapitalFirst(effect->getDescription());
}


void Effects::CustomArea::applyToCreature(Creature* c, Creature* attacker) const {
}

string Effects::CustomArea::getName() const {
  return "custom area " + effect->getName();
}

string Effects::CustomArea::getDescription() const {
  return "Custom area effect: " + noCapitalFirst(effect->getDescription());
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

void Effects::Suicide::applyToCreature(Creature* c, Creature* attacker) const {
  c->you(message, "");
  c->dieNoReason();
}

string Effects::Suicide::getName() const {
  return "suicide";
}

string Effects::Suicide::getDescription() const {
  return "Causes the *attacker* to die.";
}

void Effects::Wish::applyToCreature(Creature* c, Creature* attacker) const {
  c->getController()->grantWish((attacker ? attacker->getName().the() + " grants you a wish." : "You are granted a wish.") +
      " What do you wish for?");
}

string Effects::Wish::getName() const {
  return "wishing";
}

string Effects::Wish::getDescription() const {
  return "Gives you one wish.";
}

void Effects::Chain::applyToCreature(Creature* c, Creature* attacker) const {
}

string Effects::Chain::getName() const {
  return effects[0].getName();
}

string Effects::Chain::getDescription() const {
  return effects[0].getDescription();
}

void Effects::Message::applyToCreature(Creature* c, Creature* attacker) const {
}

string Effects::Message::getName() const {
  return "message";
}

string Effects::Message::getDescription() const {
  return text;
}

void Effects::IncreaseMorale::applyToCreature(Creature* c, Creature* attacker) const {
  if (amount > 0)
    c->you(MsgType::YOUR, "spirits are lifted");
  else
    c->you(MsgType::ARE, "disheartened");
  c->addMorale(amount);
}

string Effects::IncreaseMorale::getName() const {
  return amount > 0 ? "morale increase" : "morale decrease";
}

string Effects::IncreaseMorale::getDescription() const {
  return amount > 0 ? "Increases morale" : "Decreases morale";
}

void Effects::Caster::applyToCreature(Creature* c, Creature* attacker) const {
}

string Effects::Caster::getName() const {
  return effect->getName();
}

string Effects::Caster::getDescription() const {
  return effect->getDescription();
}

void Effects::Chance::applyToCreature(Creature* c, Creature* attacker) const {
}

string Effects::Chance::getName() const {
  return effect->getName();
}

string Effects::Chance::getDescription() const {
  return effect->getDescription() + " (" + toString(int(value * 100)) + "% chance)";
}

void Effects::DoubleTrouble::applyToCreature(Creature* c, Creature* attacker) const {
  auto attributes = c->getAttributes();
  c->getGame()->getContentFactory()->getCreatures().initializeAttributes(*c->getAttributes().getCreatureId(), attributes);
  PCreature copy = makeOwner<Creature>(c->getTribeId(), std::move(attributes), c->getSpellMap());
  copy->setController(Monster::getFactory(MonsterAIFactory::monster()).get(copy.get()));
  copy->modViewObject() = c->getViewObject();
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
  } else
    c->message(PlayerMessage("The spell failed!", MessagePriority::HIGH));
}

string Effects::DoubleTrouble::getName() const {
  return "double trouble";
}

string Effects::DoubleTrouble::getDescription() const {
  return "Creates a twin copy ally.";
}

void Effects::Blast::applyToCreature(Creature* c, Creature* attacker) const {
}

string Effects::Blast::getName() const {
  return "air blast";
}

string Effects::Blast::getDescription() const {
  return "Creates a directed blast of air that throws back creatures and items.";
}

static void pullCreature(Creature* victim, const vector<Position>& trajectory) {
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
  }
}

void Effects::Pull::applyToCreature(Creature* victim, Creature* attacker) const {
}

string Effects::Pull::getName() const {
  return "pull";
}

string Effects::Pull::getDescription() const {
  return "Pulls a creature towards the spellcaster.";
}

void Effects::Shove::applyToCreature(Creature* c, Creature* attacker) const {
  CHECK(attacker);
  auto origin = attacker->getPosition();
  auto dir = origin.getDir(c->getPosition());
  CHECK(dir.length8() == 1);
  if (c->getPosition().canMoveCreature(dir)) {
    c->displace(dir);
    c->you(MsgType::ARE, "thrown back");
    c->addEffect(LastingEffect::COLLAPSED, 2_visible);
  }
}

string Effects::Shove::getName() const {
  return "shove";
}

string Effects::Shove::getDescription() const {
  return "Push back a creature.";
}

void Effects::SwapPosition::applyToCreature(Creature* c, Creature* attacker) const {
  CHECK(attacker);
  auto origin = attacker->getPosition();
  auto dir = origin.getDir(c->getPosition());
  CHECK(dir.length8() == 1);
  if (attacker->canSwapPositionWithEnemy(c)) {
    attacker->swapPosition(dir, false);
    attacker->verb("swap", "swaps", "positions with " + c->getName().the());
  } else
    attacker->privateMessage(c->getName().the() + " resists");
}

string Effects::SwapPosition::getName() const {
  return "swap position";
}

string Effects::SwapPosition::getDescription() const {
  return "Swap positions with an enemy.";
}


bool Effects::Filter::applies(bool isEnemy) const {
  switch (filter) {
    case FilterType::ALLY:
      return !isEnemy;
    case FilterType::ENEMY:
      return isEnemy;
  }
}

void Effects::Filter::applyToCreature(Creature* c, Creature* attacker) const {
  if (applies(c->isEnemy(attacker)))
    effect->apply(c->getPosition(), attacker);
}

string Effects::Filter::getName() const {
  auto suffix = [&] {
    switch (filter) {
      case FilterType::ALLY:
        return " (ally only)";
      case FilterType::ENEMY:
        return " (enemy only)";
    }
  };
  return effect->getName() + suffix();
}

string Effects::Filter::getDescription() const {
  auto suffix = [&] {
    switch (filter) {
      case FilterType::ALLY:
        return " (applied only to allies)";
      case FilterType::ENEMY:
        return " (applied only to enemies)";
    }
  };
  return effect->getDescription() + suffix();
}

#define FORWARD_CALL(Var, Name, ...)\
Var->visit([&](const auto& e) { return e.Name(__VA_ARGS__); })

string Effect::getName() const {
  return FORWARD_CALL(effect, getName);
}

Effect::Effect(const EffectType& t) : effect(t) {
}

Effect::Effect(Effect&&) = default;

Effect::Effect(const Effect&) = default;

Effect::Effect() {
}

Effect::~Effect() {
}

Effect&Effect::operator =(Effect&&) = default;

Effect&Effect::operator =(const Effect&) = default;

bool Effect::operator ==(const Effect& o) const {
  return o.effect == effect;
}

bool Effect::operator !=(const Effect& o) const {
  return !(*this == o);
}

void Effect::apply(Position pos, Creature* attacker) const {
  if (auto c = pos.getCreature()) {
    FORWARD_CALL(effect, applyToCreature, c, attacker);
    if (isConsideredHostile() && attacker)
      c->onAttackedBy(attacker);
  }
  effect->visit(
      [&](const auto&) { },
      [&](const Effects::ReviveCorpse& effect) {
        for (auto& item : pos.getItems())
          if (auto info = item->getCorpseInfo())
            if (info->canBeRevived)
              for (auto& dead : pos.getModel()->getDeadCreatures())
                if (dead->getUniqueId() == info->victim) {
                  bool success = false;
                  for (auto id : effect.summoned) {
                    auto summoned = summon(attacker, id, 1, TimeInterval(effect.ttl));
                    if (!summoned.empty()) {
                      for (auto& c : summoned) {
                        c->getName().addBarePrefix(dead->getName().bare());
                        attacker->message("You have revived " + c->getName().a());
                      }
                      pos.removeItems({item});
                      success = true;
                      break;
                    }
                  }
                  if (!success)
                    attacker->message("The spell failed");
                  return;
                }
      },
      [&](Effects::Teleport) {
        if (attacker->getPosition().canMoveCreature(pos)) {
          attacker->you(MsgType::TELE_DISAPPEAR, "");
          attacker->getPosition().moveCreature(pos, true);
          attacker->you(MsgType::TELE_APPEAR, "");
        }
      },
      [&](Effects::Fire) {
        pos.getGame()->addEvent(EventInfo::FX{pos, {FXName::FIREBALL_SPLASH}});
        pos.fireDamage(1);
      },
      [&](Effects::Ice) {
        //pos.getGame()->addEvent(EventInfo::FX{pos, {FXName::FIREBALL_SPLASH}});
        pos.iceDamage();
      },
      [&](const Effects::Area& area) {
        for (auto v : pos.getRectangle(Rectangle::centered(area.radius)))
          area.effect->apply(v, attacker);
      },
      [&](const Effects::CustomArea& area) {
        for (auto& v : area.getTargetPos(attacker, pos))
          area.effect->apply(v, attacker);
      },
      [&](const Effects::Chain& chain) {
        for (auto& e : chain.effects)
          e.apply(pos, attacker);
      },
      [&](const Effects::Message& msg) {
        pos.globalMessage(msg.text);
      },
      [&](const Effects::Caster& e) {
        if (attacker)
          e.effect->apply(attacker->getPosition(), attacker);
      },
      [&](const Effects::Chance& e) {
        if (Random.chance(e.value))
          e.effect->apply(pos, attacker);
      },
      [&](const Effects::SummonEnemy& summon) {
        CreatureGroup f = CreatureGroup::singleType(TribeId::getMonster(), summon.creature);
        Effect::summon(pos, f, Random.get(summon.count), 100_visible, 1_visible);
      },
      [&](const Effects::PlaceFurniture& effect) {
        auto f = pos.getGame()->getContentFactory()->furniture.getFurniture(effect.furniture,
            attacker ? attacker->getTribeId() : TribeId::getMonster());
        auto ref = f.get()->getThis();
        pos.addFurniture(std::move(f));
        if (ref)
          ref->onConstructedBy(pos, attacker);
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
      },
      [&](Effects::Pull) {
        CHECK(attacker);
        vector<Position> trajectory = drawLine(attacker->getPosition(), pos);
        for (auto& pos : trajectory)
          if (auto c = pos.getCreature())
            pullCreature(c, trajectory);
      }
  );
}

string Effect::getDescription() const {
  return FORWARD_CALL(effect, getDescription);
}

static bool isConsideredInDanger(const Creature* c) {
  if (auto intent = c->getLastCombatIntent())
    return (intent->time > *c->getGlobalTime() - 5_visible);
  return false;
}

EffectAIIntent Effect::shouldAIApply(const Creature* victim, bool isEnemy) const {
  bool isFighting = isConsideredInDanger(victim);
  return effect->visit(
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
      [&] (const Effects::DoubleTrouble&) {
        return isFighting ? EffectAIIntent::WANTED : EffectAIIntent::NONE;
      },
      [&] (const Effects::SummonGhost& g) {
        return (isFighting && !!g.getBestSpirit(victim->getPosition().getModel(), victim->getTribeId()))
            ? EffectAIIntent::WANTED : EffectAIIntent::NONE;
      },
      [&] (const Effects::SummonElement&) {
        return isFighting ? EffectAIIntent::WANTED : EffectAIIntent::NONE;
      },
      [&] (const Effects::PlaceFurniture& f) {
        return victim->getGame()->getContentFactory()->furniture.getData(f.furniture).isHostileSpell() && isFighting
            ? EffectAIIntent::WANTED : EffectAIIntent::NONE;
      },
      [&] (const auto& e) {
        return EffectAIIntent::NONE;
      }
  );
}

/* Unimplemented: Teleport, EnhanceArmor, EnhanceWeapon, Suicide, IncreaseAttr,
      EmitPoisonGas, CircularBlast, Alarm, TeleEnemies, SilverDamage, DoubleTrouble,
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
  return effect->visit(
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
        if (victim && e.applies(isEnemy))
          return e.effect->shouldAIApply(caster, pos);
        return EffectAIIntent::NONE;
      },
      [&] (const Effects::Chance& e) {
        return e.effect->shouldAIApply(caster, pos);
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
  return effect->visit(
      [&](const auto&) -> optional<FXInfo> { return none; },
      [&](const Effects::Lasting& e) -> optional<FXInfo> { return ::getProjectileFX(e.lastingEffect); },
      [&](const Effects::Damage&) -> optional<FXInfo> { return {FXName::MAGIC_MISSILE}; },
      [&](const Effects::Blast&) -> optional<FXInfo> { return {FXName::AIR_BLAST}; },
      [&](const Effects::Pull&) -> optional<FXInfo> { return FXInfo{FXName::AIR_BLAST}.setReversed(); },
      [&](const Effects::Chance& e) -> optional<FXInfo> { return e.effect->getProjectileFX(); }
  );
}

optional<ViewId> Effect::getProjectile() const {
  return effect->visit(
      [&](const auto&) -> optional<ViewId> { return none; },
      [&](const Effects::Lasting& e) -> optional<ViewId> { return ::getProjectile(e.lastingEffect); },
      [&](const Effects::Damage&) -> optional<ViewId> { return ViewId("force_bolt"); },
      [&](const Effects::Fire&) -> optional<ViewId> { return ViewId("fireball"); },
      [&](const Effects::Blast&) -> optional<ViewId> { return ViewId("air_blast"); },
      [&](const Effects::Chance& e) -> optional<ViewId> { return e.effect->getProjectile(); }
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
       Effect(Effects::DestroyWalls{}),
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
  return effect->visit(
      [&](const Effects::IncreaseMorale&) -> optional<MinionEquipmentType> { return MinionEquipmentType::COMBAT_ITEM; },
      [&](const Effects::Chance& e) -> optional<MinionEquipmentType> { return e.effect->getMinionEquipmentType(); },
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
  return effect->visit(
      [&](const Effects::Suicide&) { return false; },
      [&](const Effects::Chance& e) { return e.effect->canAutoAssignMinionEquipment(); },
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

#include "pretty_archive.h"
template void Effect::serialize(PrettyInputArchive&, unsigned);
