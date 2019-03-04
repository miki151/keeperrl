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

vector<Creature*> Effect::summonCreatures(Position pos, int radius, vector<PCreature> creatures, TimeInterval delay) {
  vector<Position> area = pos.getRectangle(Rectangle(-Vec2(radius, radius), Vec2(radius + 1, radius + 1)));
  vector<Creature*> ret;
  for (int i : All(creatures))
    for (Position v : Random.permutation(area))
      if (v.canEnter(creatures[i].get())) {
        ret.push_back(creatures[i].get());
        v.addCreature(std::move(creatures[i]), delay);
        break;
  }
  return ret;
}

vector<Creature*> Effect::summonCreatures(Creature* c, int radius, vector<PCreature> creatures, TimeInterval delay) {
  return summonCreatures(c->getPosition(), radius, std::move(creatures), delay);
}

void Effect::emitPoisonGas(Position pos, double amount, bool msg) {
  PROFILE;
  for (Position v : pos.neighbors8())
    pos.addPoisonGas(amount / 2);
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
    creatures.push_back(c->getGame()->getCreatureFactory()->fromId(id, c->getTribeId(), MonsterAIFactory::summoned(c)));
  auto ret = summonCreatures(c, 2, std::move(creatures), delay);
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
    creatures.push_back(factory.random(pos.getGame()->getCreatureFactory(), MonsterAIFactory::monster()));
  auto ret = summonCreatures(pos, 2, std::move(creatures), delay);
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

static TimeInterval entangledTime(int strength) {
  return TimeInterval(max(5, 30 - strength / 2));
}

static TimeInterval getDuration(const Creature* c, LastingEffect e) {
  switch (e) {
    case LastingEffect::SUMMONED: return 900_visible;
    case LastingEffect::PREGNANT: return 900_visible;
    case LastingEffect::NIGHT_VISION:
    case LastingEffect::ELF_VISION: return  60_visible;
    case LastingEffect::TIED_UP:
    case LastingEffect::WARNING:
    case LastingEffect::REGENERATION:
    case LastingEffect::TELEPATHY:
    case LastingEffect::BLEEDING: return  50_visible;
    case LastingEffect::ENTANGLED: return entangledTime(c->getAttr(AttrType::DAMAGE));
    case LastingEffect::HALLU:
    case LastingEffect::SLOWED:
    case LastingEffect::SPEED:
    case LastingEffect::RAGE:
    case LastingEffect::LIGHT_SOURCE:
    case LastingEffect::DARKNESS_SOURCE:
    case LastingEffect::PANIC: return  15_visible;
    case LastingEffect::POISON: return  60_visible;
    case LastingEffect::DEF_BONUS:
    case LastingEffect::DAM_BONUS: return  40_visible;
    case LastingEffect::BLIND: return  15_visible;
    case LastingEffect::INVISIBLE: return  15_visible;
    case LastingEffect::STUNNED: return  7_visible;
    case LastingEffect::SLEEP_RESISTANT:
    case LastingEffect::FIRE_RESISTANT:
    case LastingEffect::POISON_RESISTANT: return  60_visible;
    case LastingEffect::FLYING: return  60_visible;
    case LastingEffect::COLLAPSED: return  2_visible;
    case LastingEffect::FAST_CRAFTING:
    case LastingEffect::FAST_TRAINING:
    case LastingEffect::SLOW_CRAFTING:
    case LastingEffect::SLOW_TRAINING:
    case LastingEffect::SLEEP: return  200_visible;
    case LastingEffect::PEACEFULNESS:
    case LastingEffect::INSANITY: return  20_visible;
    case LastingEffect::MAGIC_VULNERABILITY:
    case LastingEffect::MELEE_VULNERABILITY:
    case LastingEffect::RANGED_VULNERABILITY:
    case LastingEffect::MAGIC_RESISTANCE:
    case LastingEffect::MELEE_RESISTANCE:
    case LastingEffect::RANGED_RESISTANCE:
    case LastingEffect::SUNLIGHT_VULNERABLE:
      return  25_visible;
    case LastingEffect::SATIATED:
      return  500_visible;
    case LastingEffect::RESTED:
    case LastingEffect::HATE_UNDEAD:
    case LastingEffect::HATE_DWARVES:
    case LastingEffect::HATE_HUMANS:
    case LastingEffect::HATE_GREENSKINS:
    case LastingEffect::HATE_ELVES:
    default:
      return  1000_visible;
  }
}

static void summon(Creature* summoner, CreatureId id, Range count) {
  if (id == "AUTOMATON") {
    CreatureGroup f = CreatureGroup::singleType(TribeId::getHostile(), id);
    Effect::summon(summoner->getPosition(), f, Random.get(count), 100_visible,
        5_visible);
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

static bool isConsideredHostile(const Effect& effect) {
  return effect.visit(
      [&](const Effect::Lasting& e) {
        return isConsideredHostile(e.lastingEffect);
      },
      [&](const Effect::Acid&) {
        return true;
      },
      [&](const Effect::DestroyEquipment&) {
        return true;
      },
      [&](const Effect::SilverDamage&) {
        return true;
      },
      [&](const Effect::Fire&) {
        return true;
      },
      [&](const Effect::Damage&) {
        return true;
      },
      [&](const auto&) {
        return false;
      }
  );
}

void Effect::Teleport::applyToCreature(Creature* c, Creature* attacker) const {
  PROFILE_BLOCK("Teleport::applyToCreature");
  Rectangle area = Rectangle::centered(Vec2(0, 0), 12);
  int infinity = 10000;
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

string Effect::Teleport::getName() const {
  return "teleport";
}

string Effect::Teleport::getDescription() const {
  return "Teleports to a safer location close by.";
}

void Effect::Lasting::applyToCreature(Creature* c, Creature* attacker) const {
  if (c->addEffect(lastingEffect, getDuration(c, lastingEffect)))
    if (auto fx = LastingEffects::getApplicationFX(lastingEffect))
      c->addFX(*fx);
}

string Effect::Lasting::getName() const {
  return LastingEffects::getName(lastingEffect);
}

string Effect::Lasting::getDescription() const {
  // Leave out the full stop.
  string desc = LastingEffects::getDescription(lastingEffect);
  return desc.substr(0, desc.size() - 1) + " for some turns.";
}

void Effect::IncreaseAttr::applyToCreature(Creature* c, Creature*) const {
  c->you(MsgType::YOUR, ::getName(attr) + get(" improves", " wanes"));
  c->getAttributes().increaseBaseAttr(attr, amount);
}

string Effect::IncreaseAttr::getName() const {
  return ::getName(attr) + get(" boost", " loss");
}

string Effect::IncreaseAttr::getDescription() const {
  return get("Increases", "Decreases") + " your "_s + ::getName(attr) + " by " + toString(abs(amount));
}

const char* Effect::IncreaseAttr::get(const char* ifIncrease, const char* ifDecrease) const {
  if (amount > 0)
    return ifIncrease;
  else
    return ifDecrease;
}

void Effect::Permanent::applyToCreature(Creature* c, Creature* attacker) const {
  c->addPermanentEffect(lastingEffect);
}

string Effect::Permanent::getName() const {
  return "permanent " + LastingEffects::getName(lastingEffect);
}

string Effect::Permanent::getDescription() const {
  string desc = LastingEffects::getDescription(lastingEffect);
  return desc.substr(0, desc.size() - 1) + " permanently.";
}

void Effect::TeleEnemies::applyToCreature(Creature*, Creature* attacker) const {
}

string Effect::TeleEnemies::getName() const {
  return "surprise";
}

string Effect::TeleEnemies::getDescription() const {
  return "Surprise!";
}

void Effect::Alarm::applyToCreature(Creature* c, Creature* attacker) const {
  c->getGame()->addEvent(EventInfo::Alarm{c->getPosition(), silent});
}

string Effect::Alarm::getName() const {
  return "alarm";
}

string Effect::Alarm::getDescription() const {
  return "Alarm!";
}

void Effect::Acid::applyToCreature(Creature* c, Creature* attacker) const {
  c->affectByAcid();
  switch (Random.get(2)) {
    case 0 : enhanceArmor(c, -1, "corrodes"); break;
    case 1 : enhanceWeapon(c, -1, "corrodes"); break;
  }
}

string Effect::Acid::getName() const {
  return "acid";
}

string Effect::Acid::getDescription() const {
  return "Causes acid damage to skin and equipment.";
}

void Effect::Summon::applyToCreature(Creature* c, Creature* attacker) const {
  ::summon(c, creature, count);
}

/*static string getCreaturePluralName(CreatureId id) {
  static EnumMap<CreatureId, optional<string>> names;
  if (!names[id])
   names[id] = CreatureFactory::fromId(id, TribeId::getHuman())->getName().plural();
  return *names[id];
}*/

static string getCreatureName(CreatureId id) {
  id = toLower(id);
  std::replace(id.begin(), id.end(), '_', ' ');
  return id;
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

string Effect::Summon::getName() const {
  return "summon " + getCreatureName(creature);
}

string Effect::Summon::getDescription() const {
  if (count.getEnd() > 2)
    return "Summons " + toString(count.getStart()) + " to " + toString(count.getEnd() - 1)
        + getCreatureName(creature);
  else
    return "Summons a " + getCreatureName(creature);
}

void Effect::SummonElement::applyToCreature(Creature* c, Creature* attacker) const {
  auto id = "AIR_ELEMENTAL"_s;
  for (Position p : c->getPosition().getRectangle(Rectangle::centered(3)))
    for (auto f : p.getFurniture())
      if (auto elem = f->getSummonedElement())
        id = *elem;
  ::summon(c, id, Range(1, 2));
}

string Effect::SummonElement::getName() const {
  return "summon element";
}

string Effect::SummonElement::getDescription() const {
  return "Summons an element or spirit from the surroundings.";
}

void Effect::Deception::applyToCreature(Creature* c, Creature* attacker) const {
  vector<PCreature> creatures;
  for (int i : Range(Random.get(3, 7)))
    creatures.push_back(CreatureFactory::getIllusion(c));
  Effect::summonCreatures(c, 2, std::move(creatures));
}

string Effect::Deception::getName() const {
  return "deception";
}

string Effect::Deception::getDescription() const {
  return "Creates multiple illusions of the spellcaster to confuse the enemy.";
}

void Effect::CircularBlast::applyToCreature(Creature* c, Creature* attacker) const {
  for (Vec2 v : Vec2::directions8(Random))
    applyDirected(c, c->getPosition().plus(v * 10), DirEffectType(1, DirEffectId::BLAST), false);
  c->addFX({FXName::CIRCULAR_BLAST});
}

string Effect::CircularBlast::getName() const {
  return "air blast";
}

string Effect::CircularBlast::getDescription() const {
  return "Creates a circular blast of air that throws back creatures and items.";
}

void Effect::EnhanceArmor::applyToCreature(Creature* c, Creature* attacker) const {
  enhanceArmor(c, 1, "is improved");
}

string Effect::EnhanceArmor::getName() const {
  return "armor enchantment";
}

string Effect::EnhanceArmor::getDescription() const {
  return "Increases armor defense.";
}

void Effect::EnhanceWeapon::applyToCreature(Creature* c, Creature* attacker) const {
  enhanceWeapon(c, 1, "is improved");
}

string Effect::EnhanceWeapon::getName() const {
  return "weapon enchantment";
}

string Effect::EnhanceWeapon::getDescription() const {
  return "Increases weapon damage.";
}

void Effect::DestroyEquipment::applyToCreature(Creature* c, Creature* attacker) const {
  auto equipped = c->getEquipment().getAllEquipped();
  if (!equipped.empty()) {
    Item* dest = Random.choose(equipped);
    c->you(MsgType::YOUR, dest->getName() + " crumbles to dust.");
    c->steal({dest});
  }
}

string Effect::DestroyEquipment::getName() const {
  return "equipment destruction";
}

string Effect::DestroyEquipment::getDescription() const {
  return "Destroys a random piece of equipment.";
}

void Effect::DestroyWalls::applyToCreature(Creature* c, Creature* attacker) const {
  PROFILE;
  for (auto pos : c->getPosition().neighbors8())
    for (auto furniture : pos.modFurniture())
      if (furniture->canDestroy(DestroyAction::Type::BOULDER))
        furniture->destroy(pos, DestroyAction::Type::BOULDER);
}

string Effect::DestroyWalls::getName() const {
  return "wall destruction";
}

string Effect::DestroyWalls::getDescription() const {
  return "Destroys walls in adjacent tiles.";
}

void Effect::Heal::applyToCreature(Creature* c, Creature* attacker) const {
  if (c->getBody().canHeal()) {
    c->heal(1);
    c->removeEffect(LastingEffect::BLEEDING);
    c->addFX(FXInfo(FXName::CIRCULAR_SPELL, Color::LIGHT_GREEN));
  } else
    c->message("Nothing happens.");
}

string Effect::Heal::getName() const {
  return "healing";
}

string Effect::Heal::getDescription() const {
  return "Fully restores your health.";
}

void Effect::Fire::applyToCreature(Creature* c, Creature* attacker) const {
  c->getPosition().fireDamage(1);
}

string Effect::Fire::getName() const {
  return "fire";
}

string Effect::Fire::getDescription() const {
  return "Burns!";
}

void Effect::EmitPoisonGas::applyToCreature(Creature* c, Creature* attacker) const {
  Effect::emitPoisonGas(c->getPosition(), amount, true);
}

string Effect::EmitPoisonGas::getName() const {
  return "poison gas";
}

string Effect::EmitPoisonGas::getDescription() const {
  return "Emits poison gas";
}

void Effect::SilverDamage::applyToCreature(Creature* c, Creature* attacker) const {
  c->affectBySilver();
}

string Effect::SilverDamage::getName() const {
  return "silver";
}

string Effect::SilverDamage::getDescription() const {
  return "Hurts the undead.";
}

void Effect::CurePoison::applyToCreature(Creature* c, Creature* attacker) const {
  if (c->removeEffect(LastingEffect::POISON))
    c->addFX(FXInfo(FXName::CIRCULAR_SPELL, Color::LIGHT_GREEN));
}

string Effect::CurePoison::getName() const {
  return "cure poisioning";
}

string Effect::CurePoison::getDescription() const {
  return "Cures poisoning.";
}

void Effect::PlaceFurniture::applyToCreature(Creature* c, Creature* attacker) const {
  PROFILE;
  Position pos = c->getPosition();
  auto f = FurnitureFactory::get(furniture, c->getTribeId());
  f->onConstructedBy(pos, c);
  pos.addFurniture(std::move(f));
}

string Effect::PlaceFurniture::getName() const {
  return Furniture::getName(furniture);
}

string Effect::PlaceFurniture::getDescription() const {
  return "Creates a " + Furniture::getName(furniture) + ".";
}

void Effect::Damage::applyToCreature(Creature* c, Creature* attacker) const {
  CHECK(attacker) << "Unknown attacker";
  c->takeDamage(Attack(attacker, Random.choose<AttackLevel>(), attackType, attacker->getAttr(attr), attr));
  if (attr == AttrType::SPELL_DAMAGE)
    c->addFX({FXName::MAGIC_MISSILE_SPLASH});
}

string Effect::Damage::getName() const {
  return ::getName(attr);
}

string Effect::Damage::getDescription() const {
  return "Causes " + ::getName(attr);
}

void Effect::InjureBodyPart::applyToCreature(Creature* c, Creature* attacker) const {
  if (c->getBody().injureBodyPart(c, part, false)) {
    c->you(MsgType::DIE, "");
    c->dieWithAttacker(attacker);
  }
}

string Effect::InjureBodyPart::getName() const {
  return "injure "_s + ::getName(part);
}

string Effect::InjureBodyPart::getDescription() const {
  return "Injures "_s + ::getName(part);
}

void Effect::LooseBodyPart::applyToCreature(Creature* c, Creature* attacker) const {
  if (c->getBody().injureBodyPart(c, part, true)) {
    c->you(MsgType::DIE, "");
    c->dieWithAttacker(attacker);
  }
}

string Effect::LooseBodyPart::getName() const {
  return "lose "_s + ::getName(part);
}

string Effect::LooseBodyPart::getDescription() const {
  return "Causes you to lose a "_s + ::getName(part);
}

void Effect::RegrowBodyPart::applyToCreature(Creature* c, Creature* attacker) const {
  c->getBody().healBodyParts(c, true);
}

string Effect::RegrowBodyPart::getName() const {
  return "regrow body lost parts";
}

string Effect::RegrowBodyPart::getDescription() const {
  return "Causes lost body parts to regrow.";
}

/*void Effect::Chain::applyToCreature(Creature* c, Creature* attacker) const {
  for (auto& elem : effects)
    elem.applyToCreature(c, attacker);
}

string Effect::Chain::getName() const {
  string ret;
  for (auto& elem : effects) {
    if (!ret.empty())
      ret += " and ";
    ret += elem.getName();
  }
  return ret;
}

string Effect::Chain::getDescription() const {
  string ret;
  for (auto& elem : effects) {
    if (!ret.empty()) {
      if (ret.back() != '.')
        ret += '.';
      ret += ' ';
    }
    ret += elem.getDescription();
  }
  return ret;
}*/

void Effect::Suicide::applyToCreature(Creature* c, Creature* attacker) const {
  c->you(MsgType::DIE, "");
  c->dieNoReason();
}

string Effect::Suicide::getName() const {
  return "suicide";
}

string Effect::Suicide::getDescription() const {
  return "Causes the *attacker* to die.";
}

void Effect::DoubleTrouble::applyToCreature(Creature* c, Creature* attacker) const {
  PCreature copy = makeOwner<Creature>(c->getTribeId(), c->getAttributes());
  copy->setController(Monster::getFactory(MonsterAIFactory::monster()).get(copy.get()));
  auto ttl = 50_visible;
  for (auto& item : c->getEquipment().getItems())
    if (!item->getResourceId()) {
      auto itemCopy = item->getCopy();
      itemCopy->setTimeout(c->getGame()->getGlobalTime() + ttl + 10_visible);
      copy->take(std::move(itemCopy));
    }
  auto cRef = summonCreatures(c, 2, makeVec(std::move(copy))).getOnlyElement();
  cRef->addEffect(LastingEffect::SUMMONED, ttl, false);
}

string Effect::DoubleTrouble::getName() const {
  return "double trouble";
}

string Effect::DoubleTrouble::getDescription() const {
  return "Creates a twin copy ally.";
}

#define FORWARD_CALL(Var, Name, ...)\
Var.visit([&](const auto& e) { return e.Name(__VA_ARGS__); })

string Effect::getName() const {
  return FORWARD_CALL(effect, getName);
}

Effect::Effect() {
}

bool Effect::operator ==(const Effect& o) const {
  return o.effect == effect;
}

bool Effect::operator !=(const Effect& o) const {
  return !(*this == o);
}

void Effect::applyToCreature(Creature* c, Creature* attacker) const {
  FORWARD_CALL(effect, applyToCreature, c, attacker);
  if (isConsideredHostile(effect) && attacker)
    c->onAttackedBy(attacker);
}

string Effect::getDescription() const {
  return FORWARD_CALL(effect, getDescription);
}

static optional<ViewId> getProjectile(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::STUNNED:
      return ViewId::STUN_RAY;
    default:
      return none;
  }
}

static optional<ViewId> getProjectile(const Effect& effect) {
  return effect.visit(
      [&](const auto&) -> optional<ViewId> { return none; },
      [&](const Effect::Lasting& e) -> optional<ViewId> { return getProjectile(e.lastingEffect); },
      [&](const Effect::Damage&) -> optional<ViewId> { return ViewId::FORCE_BOLT; }
  );
}

static optional<ViewId> getProjectile(const DirEffectType& effect) {
  switch (effect.getId()) {
    case DirEffectId::BLAST:
      return ViewId::AIR_BLAST;
    case DirEffectId::FIREBREATH:
    case DirEffectId::FIREBALL:
      return ViewId::FIREBALL;
    case DirEffectId::CREATURE_EFFECT:
      return getProjectile(effect.get<Effect>());
  }
}

static optional<FXInfo> getProjectileFX(LastingEffect effect) {
  switch (effect) {
    default:
      return none;
  }
}

static optional<FXInfo> getProjectileFX(const Effect& effect) {
  return effect.visit(
      [&](const auto&) -> optional<FXInfo> { return none; },
      [&](const Effect::Lasting& e) -> optional<FXInfo> { return getProjectileFX(e.lastingEffect); },
      [&](const Effect::Damage&) -> optional<FXInfo> { return {FXName::MAGIC_MISSILE}; }
  );
}

static optional<FXInfo> getProjectileFX(const DirEffectType& effect) {
  switch (effect.getId()) {
    case DirEffectId::BLAST:
      return {FXName::AIR_BLAST};
    case DirEffectId::FIREBALL:
      return {FXName::FIREBALL};
    case DirEffectId::FIREBREATH:
      return {FXName::FLAMETHROWER};
    case DirEffectId::CREATURE_EFFECT:
      return getProjectileFX(effect.get<Effect>());
  }
}

static void airBlast(Creature* who, Position position, Position target) {
  CHECK(target != who->getPosition());
  Vec2 direction = who->getPosition().getDir(target);
  constexpr int maxDistance = 4;
  while (direction.length8() < maxDistance * 3)
    direction += who->getPosition().getDir(target);
  auto trajectory = drawLine(Vec2(0, 0), direction);
  for (int i : All(trajectory))
    if (trajectory[i] == who->getPosition().getDir(position)) {
      trajectory = getSubsequence(trajectory, i + 1, maxDistance);
      for (auto& v : trajectory)
        v = v - who->getPosition().getDir(position);
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
  }
  for (auto& stack : Item::stackItems(position.getItems())) {
    position.throwItem(
        position.removeItems(stack),
        Attack(who, Random.choose<AttackLevel>(),
          stack[0]->getWeaponInfo().attackType, 15, AttrType::DAMAGE), maxDistance,
          position.plus(trajectory.back()), VisionId::NORMAL);
  }
  for (auto furniture : position.modFurniture())
    if (furniture->canDestroy(DestroyAction::Type::BASH))
      furniture->destroy(position, DestroyAction::Type::BASH);
}

void applyDirected(Creature* c, Position target, const DirEffectType& type, bool withProjectileFX) {
  if (target == c->getPosition())
    return;
  vector<Position> trajectory;
  auto origin = c->getPosition().getCoord();
  for (auto& v : drawLine(origin, target.getCoord()))
    if (v != origin && v.dist8(origin) <= type.getRange()) {
      trajectory.push_back(Position(v, target.getLevel()));
      if (Position(v, target.getLevel()).isDirEffectBlocked())
        break;
    }
  if (withProjectileFX)
    c->getGame()->addEvent(
        EventInfo::Projectile{getProjectileFX(type), getProjectile(type), c->getPosition(), trajectory.back()});
  switch (type.getId()) {
    case DirEffectId::BLAST:
      for (int i : All(trajectory).reverse())
        airBlast(c, trajectory[i], target);
      break;
    case DirEffectId::FIREBREATH:
    case DirEffectId::FIREBALL:
      for (auto& pos : trajectory) {
        pos.fireDamage(1);
        if (Creature* victim = pos.getCreature())
          victim->addFX({FXName::FIREBALL_SPLASH});
      }
      break;
    case DirEffectId::CREATURE_EFFECT:
      for (auto& pos : trajectory)
        if (auto victim = pos.getCreature())
          type.get<Effect>().applyToCreature(victim, c);
      break;
  }
}

string getDescription(const DirEffectType& type) {
  switch (type.getId()) {
    case DirEffectId::BLAST:
      return "Creates a directed blast of air that throws back creatures and items.";
    case DirEffectId::FIREBREATH:
      return "Creates a ray of fire.";
    case DirEffectId::FIREBALL:
      return "Creates a directed fireball.";
    case DirEffectId::CREATURE_EFFECT:
      return "Creates a directed ray of range " + toString(type.getRange()) + " that " +
          noCapitalFirst(type.get<Effect>().getDescription());
  }
}

SERIALIZE_DEF(Effect, effect)

#include "pretty_archive.h"
template void Effect::serialize(PrettyInputArchive&, unsigned);
