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
#include "event_listener.h"
#include "item_class.h"
#include "furniture_factory.h"
#include "furniture.h"
#include "movement_set.h"

vector<WCreature> Effect::summonCreatures(Position pos, int radius, vector<PCreature> creatures, double delay) {
  vector<Position> area = pos.getRectangle(Rectangle(-Vec2(radius, radius), Vec2(radius + 1, radius + 1)));
  vector<WCreature> ret;
  for (int i : All(creatures))
    for (Position v : Random.permutation(area))
      if (v.canEnter(creatures[i].get())) {
        ret.push_back(creatures[i].get());
        v.addCreature(std::move(creatures[i]), delay);
        break;
  }
  return ret;
}

vector<WCreature> Effect::summonCreatures(WCreature c, int radius, vector<PCreature> creatures, double delay) {
  return summonCreatures(c->getPosition(), radius, std::move(creatures), delay);
}

static void airBlast(WCreature who, Position position, Vec2 direction) {
  constexpr int maxDistance = 4;
  if (WCreature c = position.getCreature()) {
    int dist = 0;
    for (int i : Range(1, maxDistance))
      if (position.canMoveCreature(direction * i))
        dist = i;
      else
        break;
    if (dist > 0) {
      c->displace(who->getLocalTime(), direction * dist);
      c->you(MsgType::ARE, "thrown back");
    }
  }
  for (auto& stack : Item::stackItems(position.getItems())) {
    position.throwItem(
        position.removeItems(stack),
        Attack(who, Random.choose<AttackLevel>(),
          stack[0]->getAttackType(), 15, AttrType::DAMAGE), maxDistance, direction, VisionId::NORMAL);
  }
  for (auto furniture : position.modFurniture())
    if (furniture->canDestroy(DestroyAction::Type::BASH))
      furniture->destroy(position, DestroyAction::Type::BASH);
}

void Effect::emitPoisonGas(Position pos, double amount, bool msg) {
  for (Position v : pos.neighbors8())
    pos.addPoisonGas(amount / 2);
  pos.addPoisonGas(amount);
  if (msg) {
    pos.globalMessage("A cloud of gas is released");
    pos.unseenMessage("You hear a hissing sound");
  }
}

vector<WCreature> Effect::summon(WCreature c, CreatureId id, int num, int ttl, double delay) {
  vector<PCreature> creatures;
  for (int i : Range(num))
    creatures.push_back(CreatureFactory::fromId(id, c->getTribeId(), MonsterAIFactory::summoned(c, ttl)));
  return summonCreatures(c, 2, std::move(creatures), delay);
}

vector<WCreature> Effect::summon(Position pos, CreatureFactory& factory, int num, int ttl, double delay) {
  vector<PCreature> creatures;
  for (int i : Range(num))
    creatures.push_back(factory.random(MonsterAIFactory::dieTime(pos.getGame()->getGlobalTime() + ttl)));
  return summonCreatures(pos, 2, std::move(creatures), delay);
}

static void enhanceArmor(WCreature c, int mod, const string& msg) {
  for (EquipmentSlot slot : Random.permutation(getKeys(Equipment::slotTitles)))
    for (WItem item : c->getEquipment().getSlotItems(slot))
      if (item->getClass() == ItemClass::ARMOR) {
        c->you(MsgType::YOUR, item->getName() + " " + msg);
        if (item->getModifier(AttrType::DEFENSE) > 0 || mod > 0)
          item->addModifier(AttrType::DEFENSE, mod);
        return;
      }
}

static void enhanceWeapon(WCreature c, int mod, const string& msg) {
  if (WItem item = c->getWeapon()) {
    c->you(MsgType::YOUR, item->getName() + " " + msg);
    item->addModifier(item->getMeleeAttackAttr(), mod);
  }
}

static double entangledTime(int strength) {
  return max(5, 30 - strength / 2);
}

static double getDuration(WConstCreature c, LastingEffect e) {
  switch (e) {
    case LastingEffect::PREGNANT: return 900;
    case LastingEffect::NIGHT_VISION:
    case LastingEffect::ELF_VISION: return 60;
    case LastingEffect::TIED_UP:
    case LastingEffect::BLEEDING: return 50;
    case LastingEffect::ENTANGLED: return entangledTime(entangledTime(c->getAttr(AttrType::DAMAGE)));
    case LastingEffect::HALLU:
    case LastingEffect::SLOWED:
    case LastingEffect::SPEED:
    case LastingEffect::RAGE:
    case LastingEffect::DARKNESS_SOURCE:
    case LastingEffect::PANIC: return 15;
    case LastingEffect::POISON: return 60;
    case LastingEffect::DEF_BONUS:
    case LastingEffect::DAM_BONUS: return 40;
    case LastingEffect::BLIND: return 15;
    case LastingEffect::INVISIBLE: return 15;
    case LastingEffect::STUNNED: return 7;
    case LastingEffect::SLEEP_RESISTANT:
    case LastingEffect::FIRE_RESISTANT:
    case LastingEffect::POISON_RESISTANT: return 60;
    case LastingEffect::FLYING: return 60;
    case LastingEffect::COLLAPSED: return 2;
    case LastingEffect::SLEEP: return 80;
    case LastingEffect::INSANITY: return 20;
    case LastingEffect::MAGIC_VULNERABILITY:
    case LastingEffect::MELEE_VULNERABILITY:
    case LastingEffect::RANGED_VULNERABILITY:
    case LastingEffect::MAGIC_RESISTANCE:
    case LastingEffect::MELEE_RESISTANCE:
    case LastingEffect::RANGED_RESISTANCE:
      return 30;
  }
  return 0;
}

static int getSummonTtl(CreatureId id) {
  switch (id) {
    case CreatureId::FIRE_SPHERE:
      return 30;
    default:
      return 100;
  }
}

static Range getSummonNumber(CreatureId id) {
  switch (id) {
    case CreatureId::SPIRIT:
      return Range(2, 5);
    case CreatureId::FLY:
      return Range(3, 7);
    default:
      return Range(1, 2);
  }
}

static double getSummonDelay(CreatureId id) {
  switch (id) {
    case CreatureId::AUTOMATON: return 5;
    default: return 1;
  }
}

static void summon(WCreature summoner, CreatureId id) {
  switch (id) {
    case CreatureId::AUTOMATON: {
      CreatureFactory f = CreatureFactory::singleType(TribeId::getHostile(), id);
      Effect::summon(summoner->getPosition(), f, Random.get(getSummonNumber(id)), getSummonTtl(id),
          getSummonDelay(id));
      break;
    }
    default:
      Effect::summon(summoner, id, Random.get(getSummonNumber(id)), getSummonTtl(id), getSummonDelay(id));
      break;
  }
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

static bool isConsideredHostile(const EffectType& effect) {
  return effect.visit(
      [&](const EffectTypes::Lasting& e) {
        return isConsideredHostile(e.lastingEffect);
      },
      [&](const EffectTypes::Acid&) {
        return true;
      },
      [&](const EffectTypes::DestroyEquipment&) {
        return true;
      },
      [&](const EffectTypes::SilverDamage&) {
        return true;
      },
      [&](const EffectTypes::Fire&) {
        return true;
      },
      [&](const EffectTypes::Damage&) {
        return true;
      },
      [&](const auto&) {
        return false;
      }
  );
}

namespace EffectTypes {

void Teleport::applyToCreature(WCreature c, WCreature attacker) const {
  Rectangle area = Rectangle::centered(Vec2(0, 0), 12);
  int infinity = 10000;
  PositionMap<int> weight(infinity);
  queue<Position> q;
  for (Position v : c->getPosition().getRectangle(area))
    if (auto other = v.getCreature())
      if (other->isEnemy(c)) {
        q.push(v);
        weight.set(v, 0);
      }
  while (!q.empty()) {
    Position v = q.front();
    q.pop();
    for (Position w : v.neighbors8())
      if (w.canEnterEmpty({MovementTrait::WALK}) && weight.get(w) == infinity) {
        weight.set(w, weight.get(v) + 1);
        q.push(w);
      }
  }
  vector<Position> good;
  int maxW = 0;
  for (Position v : c->getPosition().getRectangle(area)) {
    if (!v.canEnter(c) || v.isBurning() || v.getPoisonGasAmount() > 0 || !c->isSameSector(v))
      continue;
    int weightV = weight.get(v);
    if (weightV == maxW)
      good.push_back(v);
    else if (weightV > maxW) {
      good = {v};
      maxW = weightV;
    }
  }
  if (maxW < 2) {
    c->message("The spell didn't work.");
    return;
  }
  CHECK(!good.empty());
  c->you(MsgType::TELE_DISAPPEAR, "");
  c->getPosition().moveCreature(Random.choose(good));
  c->you(MsgType::TELE_APPEAR, "");
}

string Teleport::getName() const {
  return "teleport";
}

string Teleport::getDescription() const {
  return "Teleports to a safer location close by.";
}

void Lasting::applyToCreature(WCreature c, WCreature attacker) const {
  c->addEffect(lastingEffect, getDuration(c, lastingEffect));
}

string Lasting::getName() const {
  return Effect::getName(lastingEffect);
}

string Lasting::getDescription() const {
  // Leave out the full stop.
  string desc = Effect::getDescription(lastingEffect);
  return desc.substr(0, desc.size() - 1) + " for some turns.";
}

void TeleEnemies::applyToCreature(WCreature, WCreature attacker) const {
}

string TeleEnemies::getName() const {
  return "surprise";
}

string TeleEnemies::getDescription() const {
  return "Surprise!";
}

void Alarm::applyToCreature(WCreature c, WCreature attacker) const {
  c->getGame()->addEvent(EventInfo::Alarm{c->getPosition()});
}

string Alarm::getName() const {
  return "alarm";
}

string Alarm::getDescription() const {
  return "Alarm!";
}

void Acid::applyToCreature(WCreature c, WCreature attacker) const {
  c->affectByAcid();
  switch (Random.get(2)) {
    case 0 : enhanceArmor(c, -1, "corrodes"); break;
    case 1 : enhanceWeapon(c, -1, "corrodes"); break;
  }
}

string Acid::getName() const {
  return "acid";
}

string Acid::getDescription() const {
  return "Causes acid damage to skin and equipment.";
}

void Summon::applyToCreature(WCreature c, WCreature attacker) const {
  ::summon(c, creature);
}

static string getCreaturePluralName(CreatureId id) {
  static EnumMap<CreatureId, optional<string>> names;
  if (!names[id])
   names[id] = CreatureFactory::fromId(id, TribeId::getHuman())->getName().plural();
  return *names[id];
}

static string getCreatureName(CreatureId id) {
  if (getSummonNumber(id).getEnd() > 2)
    return getCreaturePluralName(id);
  static EnumMap<CreatureId, optional<string>> names;
  if (!names[id])
    names[id] = CreatureFactory::fromId(id, TribeId::getHuman())->getName().bare();
  return *names[id];
}

static string getCreatureAName(CreatureId id) {
  static map<CreatureId, string> names;
  if (!names.count(id))
    names[id] = CreatureFactory::fromId(id, TribeId::getHuman())->getName().a();
  return names.at(id);
}

string Summon::getName() const {
  return getCreatureName(creature);
}

string Summon::getDescription() const {
  Range number = getSummonNumber(creature);
  if (number.getEnd() > 2)
    return "Summons " + toString(number.getStart()) + " to " + toString(number.getEnd() - 1)
        + getCreatureName(creature);
  else
    return "Summons " + getCreatureAName(creature);
}

void SummonElement::applyToCreature(WCreature c, WCreature attacker) const {
  auto id = CreatureId::AIR_ELEMENTAL;
  for (Position p : c->getPosition().getRectangle(Rectangle::centered(3)))
    for (auto f : p.getFurniture())
      if (auto elem = f->getSummonedElement())
        id = *elem;
  ::summon(c, id);
}

string SummonElement::getName() const {
  return "summon element";
}

string SummonElement::getDescription() const {
  return "Summons an element or spirit from the surroundings.";
}

void Deception::applyToCreature(WCreature c, WCreature attacker) const {
  vector<PCreature> creatures;
  for (int i : Range(Random.get(3, 7)))
    creatures.push_back(CreatureFactory::getIllusion(c));
  Effect::summonCreatures(c, 2, std::move(creatures));
}

string Deception::getName() const {
  return "deception";
}

string Deception::getDescription() const {
  return "Creates multiple illusions of the spellcaster to confuse the enemy.";
}

void CircularBlast::applyToCreature(WCreature c, WCreature attacker) const {
  for (Vec2 v : Vec2::directions8(Random))
    Effect::applyDirected(c, v, DirEffectType(1, DirEffectId::BLAST));
}

string CircularBlast::getName() const {
  return "air blast";
}

string CircularBlast::getDescription() const {
  return "Creates a circular blast of air that throws back creatures and items.";
}

void EnhanceArmor::applyToCreature(WCreature c, WCreature attacker) const {
  enhanceArmor(c, 1, "is improved");
}

string EnhanceArmor::getName() const {
  return "armor enchantment";
}

string EnhanceArmor::getDescription() const {
  return "Increases armor defense.";
}

void EnhanceWeapon::applyToCreature(WCreature c, WCreature attacker) const {
  enhanceWeapon(c, 1, "is improved");
}

string EnhanceWeapon::getName() const {
  return "weapon enchantment";
}

string EnhanceWeapon::getDescription() const {
  return "Increases weapon damage.";
}

void DestroyEquipment::applyToCreature(WCreature c, WCreature attacker) const {
  WItem dest = Random.choose(c->getEquipment().getAllEquipped());
  c->you(MsgType::YOUR, dest->getName() + " crumbles to dust.");
  c->steal({dest});
}

string DestroyEquipment::getName() const {
  return "equipment destruction";
}

string DestroyEquipment::getDescription() const {
  return "Destroys a random piece of equipment.";
}

void Heal::applyToCreature(WCreature c, WCreature attacker) const {
  if (c->getBody().canHeal()) {
    c->heal(1);
    c->removeEffect(LastingEffect::BLEEDING);
  } else
    c->message("Nothing happens.");
}

string Heal::getName() const {
  return "healing";
}

string Heal::getDescription() const {
  return "Fully restores your health";
}

void Fire::applyToCreature(WCreature c, WCreature attacker) const {
  c->getPosition().fireDamage(1);
}

string Fire::getName() const {
  return "fire";
}

string Fire::getDescription() const {
  return "Burns!";
}

void EmitPoisonGas::applyToCreature(WCreature c, WCreature attacker) const {
  Effect::emitPoisonGas(c->getPosition(), amount, true);
}

string EmitPoisonGas::getName() const {
  return "poison gas";
}

string EmitPoisonGas::getDescription() const {
  return "Emits poison gas";
}

void SilverDamage::applyToCreature(WCreature c, WCreature attacker) const {
  c->affectBySilver();
}

string SilverDamage::getName() const {
  return "silver";
}

string SilverDamage::getDescription() const {
  return "Hurts the undead";
}

void CurePoison::applyToCreature(WCreature c, WCreature attacker) const {
  c->removeEffect(LastingEffect::POISON);
}

string CurePoison::getName() const {
  return "cure poisioning";
}

string CurePoison::getDescription() const {
  return "Cures poisoning";
}

void PlaceFurniture::applyToCreature(WCreature c, WCreature attacker) const {
  Position pos = c->getPosition();
  auto f = FurnitureFactory::get(furniture, c->getTribeId());
  bool furnitureBlocks = !f->getMovementSet().canEnter(c->getMovementType());
  if (furnitureBlocks) {
    optional<Vec2> dest;
    for (Position pos2 : c->getPosition().neighbors8(Random))
      if (c->move(pos2) && !pos2.getCreature()) {
        dest = pos.getDir(pos2);
        break;
      }
    if (dest)
      c->displace(c->getLocalTime(), *dest);
    else
      Effect::applyToCreature(c, EffectTypes::Teleport{});
  }
  if (c->getPosition() != pos || !furnitureBlocks) {
    f->onConstructedBy(c);
    pos.addFurniture(std::move(f));
  }
}

string PlaceFurniture::getName() const {
  return Furniture::getName(furniture);
}

string PlaceFurniture::getDescription() const {
  return "Creates a " + Furniture::getName(furniture);
}

void Damage::applyToCreature(WCreature c, WCreature attacker) const {
  CHECK(attacker) << "Unknown attacker";
  c->takeDamage(Attack(attacker, Random.choose<AttackLevel>(), attackType, attacker->getAttr(attr), attr));
}

string Damage::getName() const {
  return ::getName(attr);
}

string Damage::getDescription() const {
  return "Causes " + ::getName(attr);
}

void InjureBodyPart::applyToCreature(WCreature c, WCreature attacker) const {
  c->getBody().injureBodyPart(c, part, false);
}

string InjureBodyPart::getName() const {
  return "injure "_s + ::getName(part);
}

string InjureBodyPart::getDescription() const {
  return "Injures "_s + ::getName(part);
}

void LooseBodyPart::applyToCreature(WCreature c, WCreature attacker) const {
  c->getBody().injureBodyPart(c, part, true);
}

string LooseBodyPart::getName() const {
  return "lose "_s + ::getName(part);
}

string LooseBodyPart::getDescription() const {
  return "Causes you to lose a "_s + ::getName(part);
}

void RegrowBodyPart::applyToCreature(WCreature c, WCreature attacker) const {
  c->getBody().healBodyParts(c, true);
}

string RegrowBodyPart::getName() const {
  return "regrow body lost parts";
}

string RegrowBodyPart::getDescription() const {
  return "Causes lost body parts to regrow";
}

}

#define FORWARD_CALL(Var, Name, ...)\
Var.visit([&](const auto& e) { return e.Name(__VA_ARGS__); })

string Effect::getName(const EffectType& effect) {
  return FORWARD_CALL(effect, getName);
}

void Effect::applyToCreature(WCreature c, const EffectType& effect, WCreature attacker) {
  FORWARD_CALL(effect, applyToCreature, c, attacker);
  if (isConsideredHostile(effect) && attacker)
    c->onAttackedBy(attacker);
}

string Effect::getDescription(const EffectType& effect) {
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

static optional<ViewId> getProjectile(const EffectType& effect) {
  return effect.visit(
      [&](const auto&) -> optional<ViewId> { return none; },
      [&](const EffectTypes::Lasting& e) -> optional<ViewId> { return getProjectile(e.lastingEffect); },
      [&](const EffectTypes::Damage&) -> optional<ViewId> { return ViewId::FORCE_BOLT; }
  );
}

static optional<ViewId> getProjectile(const DirEffectType& effect) {
  switch (effect.getId()) {
    case DirEffectId::BLAST:
      return ViewId::AIR_BLAST;
    case DirEffectId::CREATURE_EFFECT:
      return getProjectile(effect.get<EffectType>());
  }
}

void Effect::applyDirected(WCreature c, Vec2 direction, const DirEffectType& type) {
  auto begin = c->getPosition();
  int range = type.getRange();
  if (auto projectile = getProjectile(type))
    c->getGame()->addEvent(EventInfo::Projectile{*projectile, begin, begin.plus(direction * range)});
  switch (type.getId()) {
    case DirEffectId::BLAST:
      for (Vec2 v = direction * range; v.length4() >= 1; v -= direction)
        airBlast(c, c->getPosition().plus(v), direction);
      break;
    case DirEffectId::CREATURE_EFFECT:
      for (Vec2 v = direction * range; v.length4() >= 1; v -= direction)
        if (WCreature victim = c->getPosition().plus(v).getCreature())
          Effect::applyToCreature(victim, type.get<EffectType>(), c);
      break;
  }
}

const char* Effect::getName(LastingEffect type) {
  switch (type) {
    case LastingEffect::PREGNANT: return "pregnant";
    case LastingEffect::BLEEDING: return "bleeding";
    case LastingEffect::SLOWED: return "slowness";
    case LastingEffect::SPEED: return "speed";
    case LastingEffect::BLIND: return "blindness";
    case LastingEffect::INVISIBLE: return "invisibility";
    case LastingEffect::POISON: return "poison";
    case LastingEffect::POISON_RESISTANT: return "poison resistance";
    case LastingEffect::FLYING: return "levitation";
    case LastingEffect::COLLAPSED: return "collapse";
    case LastingEffect::PANIC: return "panic";
    case LastingEffect::RAGE: return "rage";
    case LastingEffect::HALLU: return "magic";
    case LastingEffect::SLEEP_RESISTANT: return "sleep resistance";
    case LastingEffect::DAM_BONUS: return "damage";
    case LastingEffect::DEF_BONUS: return "defense";
    case LastingEffect::SLEEP: return "sleep";
    case LastingEffect::TIED_UP:
    case LastingEffect::ENTANGLED: return "web";
    case LastingEffect::STUNNED: return "stunning";
    case LastingEffect::FIRE_RESISTANT: return "fire resistance";
    case LastingEffect::INSANITY: return "insanity";
    case LastingEffect::MAGIC_RESISTANCE: return "magic resistance";
    case LastingEffect::MELEE_RESISTANCE: return "melee resistance";
    case LastingEffect::RANGED_RESISTANCE: return "ranged resistance";
    case LastingEffect::MAGIC_VULNERABILITY: return "magic vulnerability";
    case LastingEffect::MELEE_VULNERABILITY: return "melee vulnerability";
    case LastingEffect::RANGED_VULNERABILITY: return "ranged vulnerability";
    case LastingEffect::DARKNESS_SOURCE: return "source of darkness";
    case LastingEffect::NIGHT_VISION: return "night vision";
    case LastingEffect::ELF_VISION: return "elf vision";
  }
}

const char* Effect::getDescription(LastingEffect type) {
  switch (type) {
    case LastingEffect::PREGNANT: return "This is no dream! This is really happening!";
    case LastingEffect::SLOWED: return "Causes unnaturally slow movement.";
    case LastingEffect::BLEEDING: return "Causes loss of health points over time.";
    case LastingEffect::SPEED: return "Causes unnaturally quick movement.";
    case LastingEffect::BLIND: return "Causes blindness";
    case LastingEffect::INVISIBLE: return "Makes you invisible to enemies.";
    case LastingEffect::POISON: return "Decreases health every turn by a little bit.";
    case LastingEffect::POISON_RESISTANT: return "Gives poison resistance.";
    case LastingEffect::FLYING: return "Causes levitation.";
    case LastingEffect::COLLAPSED: return "Moving across tiles takes three times longer.";
    case LastingEffect::PANIC: return "Increases defense and lowers damage.";
    case LastingEffect::RAGE: return "Increases damage and lowers defense.";
    case LastingEffect::HALLU: return "Causes hallucinations.";
    case LastingEffect::DAM_BONUS: return "Gives a damage bonus.";
    case LastingEffect::DEF_BONUS: return "Gives a defense bonus.";
    case LastingEffect::SLEEP_RESISTANT: return "Prevents being put to sleep.";
    case LastingEffect::SLEEP: return "Puts to sleep.";
    case LastingEffect::TIED_UP:
      FALLTHROUGH;
    case LastingEffect::ENTANGLED: return "web";
    case LastingEffect::STUNNED: return "Causes inability to make any action.";
    case LastingEffect::FIRE_RESISTANT: return "Gives fire resistance.";
    case LastingEffect::INSANITY: return "Confuses the target about who is friend and who is foe.";
    case LastingEffect::MAGIC_RESISTANCE: return "Increases defense against magical attacks by 30%.";
    case LastingEffect::MELEE_RESISTANCE: return "Increases defense against melee attacks by 30%.";
    case LastingEffect::RANGED_RESISTANCE: return "Increases defense against ranged attacks by 30%.";
    case LastingEffect::MAGIC_VULNERABILITY: return "Decreases defense against magical attacks by 23%.";
    case LastingEffect::MELEE_VULNERABILITY: return "Decreases defense against melee attacks by 23%.";
    case LastingEffect::RANGED_VULNERABILITY: return "Decreases defense against ranged attacks by 23%.";
    case LastingEffect::DARKNESS_SOURCE: return "Causes the closest vicinity to become dark. Protects undead from sunlight.";
    case LastingEffect::NIGHT_VISION: return "Gives vision in the dark at full distance.";
    case LastingEffect::ELF_VISION: return "Allows to see and shoot through trees.";
  }
}

string Effect::getDescription(const DirEffectType& type) {
  switch (type.getId()) {
    case DirEffectId::BLAST: return "Creates a directed blast of air that throws back creatures and items.";
    case DirEffectId::CREATURE_EFFECT:
        return "Creates a directed ray of range " + toString(type.getRange()) + " that " +
            noCapitalFirst(getDescription(type.get<EffectType>()));
        break;
  }
}


