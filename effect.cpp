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

static vector<int> healingPoints { 5, 15, 40};
static vector<int> sleepTime { 15, 80, 200};
static vector<int> insanityTime { 5, 20, 50};
static vector<int> panicTime { 5, 15, 40};
static vector<int> halluTime { 30, 100, 250};
static vector<int> blindTime { 5, 15, 45};
static vector<int> invisibleTime { 5, 15, 45};
static vector<double> fireAmount { 0.5, 1, 1};
static vector<int> attrBonusTime { 10, 40, 150};
static vector<int> identifyNum { 1, 1, 400};
static vector<int> poisonTime { 20, 60, 200};
static vector<int> stunTime { 1, 7, 20};
static vector<int> resistantTime { 20, 60, 200};
static vector<int> levitateTime { 20, 60, 200};
static vector<int> magicShieldTime { 5, 20, 60};
static vector<double> gasAmount { 0.3, 0.8, 3};
static vector<int> wordOfPowerDist { 1, 3, 10};
static vector<int> directedEffectRange { 2, 5, 10};

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

static void deception(WCreature creature) {
  vector<PCreature> creatures;
  for (int i : Range(Random.get(3, 7)))
    creatures.push_back(CreatureFactory::getIllusion(creature));
  Effect::summonCreatures(creature, 2, std::move(creatures));
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

static void emitPoisonGas(Position pos, int strength, bool msg) {
  for (Position v : pos.neighbors8())
    pos.addPoisonGas(gasAmount[strength] / 2);
  pos.addPoisonGas(gasAmount[strength]);
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

static void enhanceArmor(WCreature c, int mod = 1, const string msg = "is improved") {
  for (EquipmentSlot slot : Random.permutation(getKeys(Equipment::slotTitles)))
    for (WItem item : c->getEquipment().getSlotItems(slot))
      if (item->getClass() == ItemClass::ARMOR) {
        c->you(MsgType::YOUR, item->getName() + " " + msg);
        if (item->getModifier(AttrType::DEFENSE) > 0 || mod > 0)
          item->addModifier(AttrType::DEFENSE, mod);
        return;
      }
}

static void enhanceWeapon(WCreature c, int mod = 1, const string msg = "is improved") {
  if (WItem item = c->getWeapon()) {
    c->you(MsgType::YOUR, item->getName() + " " + msg);
    item->addModifier(AttrType::DAMAGE, mod);
  }
}

static void destroyEquipment(WCreature c) {
  WItem dest = Random.choose(c->getEquipment().getAllEquipped());
  c->you(MsgType::YOUR, dest->getName() + " crumbles to dust.");
  c->steal({dest});
  return;
}

static void heal(WCreature c) {
  if (c->getBody().canHeal()) {
    c->heal(1);
    c->removeEffect(LastingEffect::BLEEDING);
  } else
    c->message("Nothing happens.");
}

static void teleport(WCreature c) {
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

static void acid(WCreature c) {
  c->affectByAcid();
  switch (Random.get(2)) {
    case 0 : enhanceArmor(c, -1, "corrodes"); break;
    case 1 : enhanceWeapon(c, -1, "corrodes"); break;
  }
}

static void alarm(WCreature c) {
  c->getGame()->addEvent({EventId::ALARM, c->getPosition()});
}

static void teleEnemies(WCreature c) { // handled by Collective
}

static double entangledTime(int strength) {
  return max(5, 30 - strength / 2);
}

static double getDuration(WConstCreature c, LastingEffect e, int strength) {
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
    case LastingEffect::PANIC: return panicTime[strength];
    case LastingEffect::POISON: return poisonTime[strength];
    case LastingEffect::DEF_BONUS:
    case LastingEffect::DAM_BONUS: return attrBonusTime[strength];
    case LastingEffect::BLIND: return blindTime[strength];
    case LastingEffect::INVISIBLE: return invisibleTime[strength];
    case LastingEffect::STUNNED: return stunTime[strength];
    case LastingEffect::FIRE_RESISTANT:
    case LastingEffect::POISON_RESISTANT: return resistantTime[strength];
    case LastingEffect::FLYING: return levitateTime[strength];
    case LastingEffect::COLLAPSED: return 2;
    case LastingEffect::SLEEP: return sleepTime[strength];
    case LastingEffect::INSANITY: return insanityTime[strength];
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

static void placeFurniture(WCreature c, FurnitureType type) {
  Position pos = c->getPosition();
  auto f = FurnitureFactory::get(type, c->getTribeId());
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
      Effect::applyToCreature(c, EffectType(EffectId::TELEPORT), EffectStrength::NORMAL);
  }
  if (c->getPosition() != pos || !furnitureBlocks) {
    f->onConstructedBy(c);
    pos.addFurniture(std::move(f));
  }
}

static CreatureId getSummonedElement(Position position) {
  for (Position p : position.getRectangle(Rectangle::centered(3)))
    for (auto f : p.getFurniture())
      if (auto elem = f->getSummonedElement())
        return *elem;
  return CreatureId::AIR_ELEMENTAL;
}

static void damage(WCreature victim, const DamageInfo& info, WCreature attacker) {
  CHECK(attacker) << "Unknown attacker";
  victim->takeDamage(Attack(attacker, Random.choose<AttackLevel>(), info.attackType, attacker->getAttr(info.attr),
      info.attr));
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
  switch (effect.getId()) {
    case EffectId::LASTING:
      return isConsideredHostile(effect.get<LastingEffect>());
    case EffectId::ACID:
    case EffectId::DESTROY_EQUIPMENT:
    case EffectId::SILVER_DAMAGE:
    case EffectId::FIRE:
    case EffectId::DAMAGE:
      return true;
    default:
      return false;
  }

}

void Effect::applyToCreature(WCreature c, const EffectType& effect, EffectStrength strengthEnum, WCreature attacker) {
  int strength = int(strengthEnum);
  switch (effect.getId()) {
    case EffectId::LASTING:
      c->addEffect(effect.get<LastingEffect>(), getDuration(c, effect.get<LastingEffect>(), strength));
      break;
    case EffectId::TELE_ENEMIES: teleEnemies(c); break;
    case EffectId::ALARM: alarm(c); break;
    case EffectId::ACID: acid(c); break;
    case EffectId::SUMMON: ::summon(c, effect.get<CreatureId>()); break;
    case EffectId::SUMMON_ELEMENT: ::summon(c, getSummonedElement(c->getPosition())); break;
    case EffectId::DECEPTION: deception(c); break;
    case EffectId::CIRCULAR_BLAST:
      for (Vec2 v : Vec2::directions8(Random))
        applyDirected(c, v, DirEffectType(1, DirEffectId::BLAST), strengthEnum);
      break;
    case EffectId::ENHANCE_ARMOR: enhanceArmor(c); break;
    case EffectId::ENHANCE_WEAPON: enhanceWeapon(c); break;
    case EffectId::DESTROY_EQUIPMENT: destroyEquipment(c); break;
    case EffectId::HEAL: heal(c); break;
    case EffectId::FIRE: c->getPosition().fireDamage(fireAmount[strength]); break;
    case EffectId::TELEPORT: teleport(c); break;
    case EffectId::ROLLING_BOULDER: FATAL << "Not implemented"; break;
    case EffectId::EMIT_POISON_GAS: emitPoisonGas(c->getPosition(), strength, true); break;
    case EffectId::SILVER_DAMAGE: c->affectBySilver(); break;
    case EffectId::CURE_POISON: c->removeEffect(LastingEffect::POISON); break;
    case EffectId::PLACE_FURNITURE: placeFurniture(c, effect.get<FurnitureType>()); break;
    case EffectId::DAMAGE: damage(c, effect.get<DamageInfo>(), attacker); break;
    case EffectId::INJURE_BODY_PART: c->getBody().injureBodyPart(c, effect.get<BodyPart>(), false); break;
    case EffectId::LOOSE_BODY_PART: c->getBody().injureBodyPart(c, effect.get<BodyPart>(), true); break;
    case EffectId::REGROW_BODY_PART: c->getBody().healBodyParts(c, true); break;
  }
  if (isConsideredHostile(effect) && attacker)
    c->onAttackedBy(attacker);
}

void Effect::applyToPosition(Position pos, const EffectType& type, EffectStrength strength) {
  switch (type.getId()) {
    case EffectId::EMIT_POISON_GAS: emitPoisonGas(pos, int(strength), false); break;
    default: FATAL << "Can't apply to position " << int(type.getId());
  }
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
  switch (effect.getId()) {
    case EffectId::LASTING:
      return getProjectile(effect.get<LastingEffect>());
    case EffectId::DAMAGE:
      return ViewId::FORCE_BOLT;
    default:
      return none;
  }
}

static optional<ViewId> getProjectile(const DirEffectType& effect) {
  switch (effect.getId()) {
    case DirEffectId::BLAST:
      return ViewId::AIR_BLAST;
    case DirEffectId::CREATURE_EFFECT:
      return getProjectile(effect.get<EffectType>());
  }
}

void Effect::applyDirected(WCreature c, Vec2 direction, const DirEffectType& type, EffectStrength strength) {
  auto begin = c->getPosition();
  int range = type.getRange();
  if (auto projectile = getProjectile(type))
    c->getGame()->addEvent({EventId::PROJECTILE,
        EventInfo::Projectile{*projectile, begin, begin.plus(direction * range)}});
  switch (type.getId()) {
    case DirEffectId::BLAST:
      for (Vec2 v = direction * range; v.length4() >= 1; v -= direction)
        airBlast(c, c->getPosition().plus(v), direction);
      break;
    case DirEffectId::CREATURE_EFFECT:
      for (Vec2 v = direction * range; v.length4() >= 1; v -= direction)
        if (WCreature victim = c->getPosition().plus(v).getCreature())
          Effect::applyToCreature(victim, type.get<EffectType>(), strength, c);
      break;
  }
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

string Effect::getName(const EffectType& type) {
  switch (type.getId()) {
    case EffectId::HEAL: return "healing";
    case EffectId::TELEPORT: return "teleport";
    case EffectId::ROLLING_BOULDER: return "rolling boulder";
    case EffectId::EMIT_POISON_GAS: return "poison gas";
    case EffectId::DESTROY_EQUIPMENT: return "destruction";
    case EffectId::ENHANCE_WEAPON: return "weapon enchantment";
    case EffectId::ENHANCE_ARMOR: return "armor enchantment";
    case EffectId::SUMMON: return getCreatureName(type.get<CreatureId>());
    case EffectId::SUMMON_ELEMENT: return "summon element";
    case EffectId::CIRCULAR_BLAST: return "air blast";
    case EffectId::DECEPTION: return "deception";
    case EffectId::FIRE: return "fire";
    case EffectId::ACID: return "acid";
    case EffectId::ALARM: return "alarm";
    case EffectId::DAMAGE: return "damage";
    case EffectId::TELE_ENEMIES: return "surprise";
    case EffectId::SILVER_DAMAGE: return "silver";
    case EffectId::CURE_POISON: return "cure poisoning";
    case EffectId::LASTING: return getName(type.get<LastingEffect>());
    case EffectId::PLACE_FURNITURE: return Furniture::getName(type.get<FurnitureType>());
    case EffectId::INJURE_BODY_PART: return "injure "_s + ::getName(type.get<BodyPart>());
    case EffectId::LOOSE_BODY_PART: return "amputate "_s + ::getName(type.get<BodyPart>());
    case EffectId::REGROW_BODY_PART: return "regrow lost body parts"_s;
  }
}

static string getLastingDescription(string desc) {
  // Leave out the full stop.
  return desc.substr(0, desc.size() - 1) + " for some turns.";
}

static string getSummoningDescription(CreatureId id) {
  Range number = getSummonNumber(id);
  if (number.getEnd() > 2)
    return "Summons " + toString(number.getStart()) + " to " + toString(number.getEnd() - 1)
        + getCreatureName(id);
  else
    return "Summons " + getCreatureAName(id);
}

string Effect::getDescription(const EffectType& type) {
  switch (type.getId()) {
    case EffectId::HEAL: return "Fully restores health.";
    case EffectId::TELEPORT: return "Teleports to a safer location close by.";
    case EffectId::ROLLING_BOULDER: return "rolling boulder";
    case EffectId::EMIT_POISON_GAS: return "poison gas";
    case EffectId::DESTROY_EQUIPMENT: return "Destroys a random piece of equipment.";
    case EffectId::ENHANCE_WEAPON: return "Increases weapon damage or accuracy.";
    case EffectId::ENHANCE_ARMOR: return "Increases armor defense.";
    case EffectId::SUMMON: return getSummoningDescription(type.get<CreatureId>());
    case EffectId::SUMMON_ELEMENT: return "Summons an element or spirit from the surroundings.";
    case EffectId::CIRCULAR_BLAST: return "Creates a circular blast of air that throws back creatures and items.";
    case EffectId::DECEPTION: return "Creates multiple illusions of the spellcaster to confuse the enemy.";
    case EffectId::FIRE: return "fire";
    case EffectId::ACID: return "acid";
    case EffectId::ALARM: return "alarm";
    case EffectId::DAMAGE: return "Causes "_s + getNameLowerCase(*getExperienceType(type.get<DamageInfo>().attr)) +
        " damage based on the \"" + ::getName(type.get<DamageInfo>().attr) + "\" attribute of the caster.";
    case EffectId::TELE_ENEMIES: return "surprise";
    case EffectId::SILVER_DAMAGE: return "silver";
    case EffectId::CURE_POISON: return "Cures poisoning.";
    case EffectId::LASTING: return getLastingDescription(getDescription(type.get<LastingEffect>()));
    case EffectId::PLACE_FURNITURE: return "Creates a " + Furniture::getName(type.get<FurnitureType>());
    case EffectId::INJURE_BODY_PART: return "Injures "_s + ::getName(type.get<BodyPart>());
    case EffectId::LOOSE_BODY_PART: return "Amputates "_s + ::getName(type.get<BodyPart>());
    case EffectId::REGROW_BODY_PART: return "Regrows lost body parts"_s;
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


