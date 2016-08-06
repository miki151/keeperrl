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
#include "trigger.h"
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

vector<int> healingPoints { 5, 15, 40};
vector<int> sleepTime { 15, 80, 200};
vector<int> insanityTime { 5, 20, 50};
vector<int> panicTime { 5, 15, 40};
vector<int> halluTime { 30, 100, 250};
vector<int> blindTime { 5, 15, 45};
vector<int> invisibleTime { 5, 15, 45};
vector<double> fireAmount { 0.5, 1, 1};
vector<int> attrBonusTime { 10, 40, 150};
vector<int> identifyNum { 1, 1, 400};
vector<int> poisonTime { 20, 60, 200};
vector<int> stunTime { 1, 7, 20};
vector<int> resistantTime { 20, 60, 200};
vector<int> levitateTime { 20, 60, 200};
vector<int> magicShieldTime { 5, 20, 60};
vector<double> gasAmount { 0.3, 0.8, 3};
vector<double> wordOfPowerDist { 1, 3, 10};
vector<int> blastRange { 2, 5, 10};
vector<int> creatureEffectRange { 2, 5, 10};


static vector<Creature*> summonCreatures(Position pos, int radius, vector<PCreature> creatures, double delay = 0) {
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

static vector<Creature*> summonCreatures(Creature* c, int radius, vector<PCreature> creatures, double delay = 0) {
  return summonCreatures(c->getPosition(), radius, std::move(creatures), delay);
}

static void deception(Creature* creature) {
  vector<PCreature> creatures;
  for (int i : Range(Random.get(3, 7)))
    creatures.push_back(CreatureFactory::getIllusion(creature));
  summonCreatures(creature, 2, std::move(creatures));
}

static void creatureEffect(Creature* who, EffectType type, EffectStrength str, Vec2 direction, int range) {
  for (Vec2 v = direction * (range - 1); v.length4() >= 1; v -= direction)
    if (Creature* c = who->getPosition().plus(v).getCreature())
      Effect::applyToCreature(c, type, str);
}

static void blast(Creature* who, Position position, Vec2 direction, int maxDistance, bool damage) {
  if (Creature* c = position.getCreature())
    if (!c->getAttributes().isStationary()) {
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
      if (damage)
        c->takeDamage(Attack(who, AttackLevel::MIDDLE, AttackType::SPELL, 1000, 32, false));
    }
  for (auto elem : Item::stackItems(position.getItems())) {
    position.throwItem(
        position.removeItems(elem.second),
        Attack(who, Random.choose(AttackLevel::LOW, AttackLevel::MIDDLE, AttackLevel::HIGH),
          elem.second[0]->getAttackType(), 15, 15, false), maxDistance, direction, VisionId::NORMAL);
  }
  if (damage && position.isDestroyable())
    position.destroy();
}

static void blast(Creature* c, Vec2 direction, int range) {
  for (Vec2 v = direction * (range - 1); v.length4() >= 1; v -= direction)
    blast(c, c->getPosition().plus(v), direction, range, true);
}

static void wordOfPower(Creature* c, int strength) {
  c->getGame()->addEvent({EventId::EXPLOSION, c->getPosition()});
  for (Vec2 v : Vec2::directions8(Random))
    blast(c, c->getPosition().plus(v), v, wordOfPowerDist[strength], true);
}

static void airBlast(Creature* c, int strength) {
  for (Vec2 v : Vec2::directions8(Random))
    blast(c, c->getPosition().plus(v), v, wordOfPowerDist[strength], false);
}

static void emitPoisonGas(Position pos, int strength, bool msg) {
  for (Position v : pos.neighbors8())
    v.addPoisonGas(gasAmount[strength] / 2);
  pos.addPoisonGas(gasAmount[strength]);
  if (msg)
    pos.globalMessage("A cloud of gas is released", "You hear a hissing sound");
}

static void guardingBuilder(Creature* c) {
  optional<Vec2> dest;
  for (Position pos : c->getPosition().neighbors8(Random))
    if (c->move(pos) && !pos.getCreature()) {
      dest = c->getPosition().getDir(pos);
      break;
    }
  Position pos = c->getPosition();
  if (dest)
    c->displace(c->getLocalTime(), *dest);
  else {
    Effect::applyToCreature(c, EffectType(EffectId::TELEPORT), EffectStrength::NORMAL);
  }
  if (c->getPosition() != pos) {
    PCreature boulder = CreatureFactory::getGuardingBoulder(c->getTribeId());
    pos.addCreature(std::move(boulder));
  }
}

vector<Creature*> Effect::summon(Creature* c, CreatureId id, int num, int ttl, double delay) {
  vector<PCreature> creatures;
  for (int i : Range(num))
    creatures.push_back(CreatureFactory::fromId(id, c->getTribeId(), MonsterAIFactory::summoned(c, ttl)));
  return summonCreatures(c, 2, std::move(creatures), delay);
}

vector<Creature*> Effect::summon(Position pos, CreatureFactory& factory, int num, int ttl, double delay) {
  vector<PCreature> creatures;
  for (int i : Range(num))
    creatures.push_back(factory.random(MonsterAIFactory::dieTime(pos.getGame()->getGlobalTime() + ttl)));
  return summonCreatures(pos, 2, std::move(creatures), delay);
}

static void enhanceArmor(Creature* c, int mod = 1, const string msg = "is improved") {
  for (EquipmentSlot slot : Random.permutation(getKeys(Equipment::slotTitles)))
    for (Item* item : c->getEquipment().getItem(slot))
      if (item->getClass() == ItemClass::ARMOR) {
        c->you(MsgType::YOUR, item->getName() + " " + msg);
        if (item->getModifier(ModifierType::DEFENSE) > 0 || mod > 0)
          item->addModifier(ModifierType::DEFENSE, mod);
        return;
      }
}

static void enhanceWeapon(Creature* c, int mod = 1, const string msg = "is improved") {
  if (Item* item = c->getWeapon()) {
    c->you(MsgType::YOUR, item->getName() + " " + msg);
    item->addModifier(Random.choose(ModifierType::ACCURACY, ModifierType::DAMAGE), mod);
  }
}

static void destroyEquipment(Creature* c) {
  Item* dest = Random.choose(c->getEquipment().getAllEquipped());
  c->you(MsgType::YOUR, dest->getName() + " crumbles to dust.");
  c->steal({dest});
  return;
}

static void heal(Creature* c, int strength) {
  if (c->getBody().canHeal() || (strength == int(EffectStrength::STRONG) && c->getBody().lostOrInjuredBodyParts()))
    c->heal(1, strength == int(EffectStrength::STRONG));
  else
    c->playerMessage("You feel refreshed.");
}

static void portal(Creature* c) {
  for (Position pos : c->getPosition().neighbors8(Random))
    if (pos.canEnter(c)) {
      pos.globalMessage("A magic portal appears.");
      pos.addTrigger(Trigger::getPortal(ViewObject(ViewId::PORTAL, ViewLayer::LARGE_ITEM, "Portal"), pos));
      return;
    }
}

static void teleport(Creature* c) {
  Vec2 enemyRadius(12, 12);
  Vec2 teleRadius(6, 6);
  Rectangle area(-enemyRadius, enemyRadius + Vec2(1, 1));
  int infinity = 10000;
  PositionMap<int> weight(infinity);
  queue<Position> q;
  for (Position v : c->getPosition().getRectangle(area))
    if (Creature *other = v.getCreature())
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
    if (!v.canEnter(c) || v.isBurning() || v.getPoisonGasAmount() > 0)
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
    c->playerMessage("The spell didn't work.");
    return;
  }
  CHECK(!good.empty());
  c->you(MsgType::TELE_DISAPPEAR, "");
  c->getPosition().moveCreature(Random.choose(good));
  c->you(MsgType::TELE_APPEAR, "");
}

static void acid(Creature* c) {
  c->affectByAcid();
  switch (Random.get(2)) {
    case 0 : enhanceArmor(c, -1, "corrodes"); break;
    case 1 : enhanceWeapon(c, -1, "corrodes"); break;
  }
}

static void alarm(Creature* c) {
  c->getGame()->addEvent({EventId::ALARM, c->getPosition()});
}

static void teleEnemies(Creature* c) { // handled by Collective
}

double entangledTime(int strength) {
  return max(5, 30 - strength / 2);
}

double getDuration(const Creature* c, LastingEffect e, int strength) {
  switch (e) {
    case LastingEffect::PREGNANT: return 900;
    case LastingEffect::TIED_UP:
    case LastingEffect::ENTANGLED: return entangledTime(entangledTime(c->getAttr(AttrType::STRENGTH)));
    case LastingEffect::HALLU:
    case LastingEffect::SLOWED:
    case LastingEffect::SPEED:
    case LastingEffect::RAGE:
    case LastingEffect::DARKNESS_SOURCE:
    case LastingEffect::PANIC: return panicTime[strength];
    case LastingEffect::POISON: return poisonTime[strength];
    case LastingEffect::DEX_BONUS:
    case LastingEffect::STR_BONUS: return attrBonusTime[strength];
    case LastingEffect::BLIND: return blindTime[strength];
    case LastingEffect::INVISIBLE: return invisibleTime[strength];
    case LastingEffect::STUNNED: return stunTime[strength];
    case LastingEffect::FIRE_RESISTANT:
    case LastingEffect::POISON_RESISTANT: return resistantTime[strength];
    case LastingEffect::FLYING: return levitateTime[strength];
    case LastingEffect::SLEEP: return sleepTime[strength];
    case LastingEffect::INSANITY: return insanityTime[strength];
    case LastingEffect::MAGIC_SHIELD: return magicShieldTime[strength];
  }
  return 0;
}

static int getSummonTtl(CreatureId id) {
  switch (id) {
    case CreatureId::FIRE_SPHERE: return 30;
    case CreatureId::SPIRIT: return 100;
    case CreatureId::FLY: return 100;
    case CreatureId::AUTOMATON: return 100;
    default: FAIL << "Unsupported summon creature" << int(id);
             return 0;
  }
}

static Range getSummonNumber(CreatureId id) {
  switch (id) {
    case CreatureId::FIRE_SPHERE: return Range(1, 2);
    case CreatureId::SPIRIT: return Range(2, 5);
    case CreatureId::FLY: return Range(3, 7);
    case CreatureId::AUTOMATON: return Range(1, 2);
    default: FAIL << "Unsupported summon creature" << int(id);
             return 0;
  }
}

static double getSummonDelay(CreatureId id) {
  switch (id) {
    case CreatureId::AUTOMATON: return 5;
    default: return 0;
  }
}

static void summon(Creature* summoner, CreatureId id) {
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

void Effect::applyToCreature(Creature* c, const EffectType& type, EffectStrength strengthEnum) {
  int strength = int(strengthEnum);
  switch (type.getId()) {
    case EffectId::LEAVE_BODY: FAIL << "Implement"; break;
    case EffectId::LASTING:
        c->addEffect(type.get<LastingEffect>(), getDuration(c, type.get<LastingEffect>(), strength)); break;
    case EffectId::TELE_ENEMIES: teleEnemies(c); break;
    case EffectId::ALARM: alarm(c); break;
    case EffectId::ACID: acid(c); break;
    case EffectId::SUMMON: ::summon(c, type.get<CreatureId>()); break;
    case EffectId::DECEPTION: deception(c); break;
    case EffectId::WORD_OF_POWER: wordOfPower(c, strength); break;
    case EffectId::AIR_BLAST: airBlast(c, strength); break;
    case EffectId::GUARDING_BOULDER: guardingBuilder(c); break;
    case EffectId::ENHANCE_ARMOR: enhanceArmor(c); break;
    case EffectId::ENHANCE_WEAPON: enhanceWeapon(c); break;
    case EffectId::DESTROY_EQUIPMENT: destroyEquipment(c); break;
    case EffectId::HEAL: heal(c, strength); break;
    case EffectId::FIRE: c->getPosition().setOnFire(fireAmount[strength]); break;
    case EffectId::PORTAL: portal(c); break;
    case EffectId::TELEPORT: teleport(c); break;
    case EffectId::ROLLING_BOULDER: FAIL << "Not implemented"; break;
    case EffectId::EMIT_POISON_GAS: emitPoisonGas(c->getPosition(), strength, true); break;
    case EffectId::SILVER_DAMAGE: c->affectBySilver(); break;
    case EffectId::CURE_POISON: c->removeEffect(LastingEffect::POISON); break;
    case EffectId::METEOR_SHOWER: c->getPosition().addTrigger(Trigger::getMeteorShower(c, 15)); break;
  }
}

void Effect::applyToPosition(Position pos, const EffectType& type, EffectStrength strength) {
  switch (type.getId()) {
    case EffectId::EMIT_POISON_GAS: emitPoisonGas(pos, int(strength), false); break;
    default: FAIL << "Can't apply to position " << int(type.getId());
  }
}

void Effect::applyDirected(Creature* c, Vec2 direction, const DirEffectType& type, EffectStrength strength) {
  switch (type.getId()) {
    case DirEffectId::BLAST: blast(c, direction, blastRange[int(strength)]); break;
    case DirEffectId::CREATURE_EFFECT:
        creatureEffect(c, type.get<EffectType>(), strength, direction, creatureEffectRange[int(strength)]);
        break;
  }
}

static string getCreaturePluralName(CreatureId id) {
  static map<CreatureId, string> names;
  return CreatureFactory::fromId(id, TribeId::getHuman())->getName().plural();
}

static string getCreatureName(CreatureId id) {
  if (getSummonNumber(id).getEnd() > 2)
    return getCreaturePluralName(id);
  static map<CreatureId, string> names;
  if (!names.count(id))
    names[id] = CreatureFactory::fromId(id, TribeId::getHuman())->getName().bare();
  return names.at(id);
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
    case EffectId::PORTAL: return "magic portal";
    case EffectId::EMIT_POISON_GAS: return "poison gas";
    case EffectId::DESTROY_EQUIPMENT: return "destruction";
    case EffectId::ENHANCE_WEAPON: return "weapon enchantment";
    case EffectId::ENHANCE_ARMOR: return "armor enchantment";
    case EffectId::SUMMON: return getCreatureName(type.get<CreatureId>());
    case EffectId::WORD_OF_POWER: return "power";
    case EffectId::AIR_BLAST: return "air blast";
    case EffectId::DECEPTION: return "deception";
    case EffectId::LEAVE_BODY: return "possesion";
    case EffectId::FIRE: return "fire";
    case EffectId::ACID: return "acid";
    case EffectId::GUARDING_BOULDER: return "boulder";
    case EffectId::ALARM: return "alarm";
    case EffectId::TELE_ENEMIES: return "surprise";
    case EffectId::SILVER_DAMAGE: return "silver";
    case EffectId::CURE_POISON: return "cure poisoning";
    case EffectId::METEOR_SHOWER: return "meteor shower";
    case EffectId::LASTING: return getName(type.get<LastingEffect>());
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
    case EffectId::HEAL: return "Heals your wounds.";
    case EffectId::TELEPORT: return "Teleports to a safer location close by.";
    case EffectId::ROLLING_BOULDER: return "rolling boulder";
    case EffectId::PORTAL: return "Creates a magic portal. Two portals are needed for a connection.";
    case EffectId::EMIT_POISON_GAS: return "poison gas";
    case EffectId::DESTROY_EQUIPMENT: return "Destroys a random piece of equipment.";
    case EffectId::ENHANCE_WEAPON: return "Increases weapon damage or accuracy.";
    case EffectId::ENHANCE_ARMOR: return "Increases armor defense.";
    case EffectId::SUMMON: return getSummoningDescription(type.get<CreatureId>());
    case EffectId::WORD_OF_POWER: return "Causes an explosion around the spellcaster.";
    case EffectId::AIR_BLAST: return "Causes an explosion of air around the spellcaster.";
    case EffectId::DECEPTION: return "Creates multiple illusions of the spellcaster to confuse the enemy.";
    case EffectId::LEAVE_BODY: return "Lets the spellcaster leave his body and possess another one.";
    case EffectId::FIRE: return "fire";
    case EffectId::ACID: return "acid";
    case EffectId::GUARDING_BOULDER: return "boulder";
    case EffectId::ALARM: return "alarm";
    case EffectId::TELE_ENEMIES: return "surprise";
    case EffectId::SILVER_DAMAGE: return "silver";
    case EffectId::CURE_POISON: return "Cures poisoning.";
    case EffectId::METEOR_SHOWER: return "Initiates a deadly meteor shower at the site.";
    case EffectId::LASTING: return getLastingDescription(getDescription(type.get<LastingEffect>()));
  }
}

string Effect::getName(LastingEffect type) {
  switch (type) {
    case LastingEffect::PREGNANT: return "pregnant";
    case LastingEffect::SLOWED: return "slowness";
    case LastingEffect::SPEED: return "speed";
    case LastingEffect::BLIND: return "blindness";
    case LastingEffect::INVISIBLE: return "invisibility";
    case LastingEffect::POISON: return "poison";
    case LastingEffect::POISON_RESISTANT: return "poison resistance";
    case LastingEffect::FLYING: return "levitation";
    case LastingEffect::PANIC: return "panic";
    case LastingEffect::RAGE: return "rage";
    case LastingEffect::HALLU: return "magic";
    case LastingEffect::STR_BONUS: return "strength";
    case LastingEffect::DEX_BONUS: return "dexterity";
    case LastingEffect::SLEEP: return "sleep";
    case LastingEffect::TIED_UP:
    case LastingEffect::ENTANGLED: return "web";
    case LastingEffect::STUNNED: return "stunning";
    case LastingEffect::FIRE_RESISTANT: return "fire resistance";
    case LastingEffect::INSANITY: return "insanity";
    case LastingEffect::MAGIC_SHIELD: return "magic shield";
    case LastingEffect::DARKNESS_SOURCE: return "source of darkness";
  }
}

string Effect::getDescription(LastingEffect type) {
  switch (type) {
    case LastingEffect::PREGNANT: return "This is no dream! This is really happening!";
    case LastingEffect::SLOWED: return "Causes unnaturally slow movement.";
    case LastingEffect::SPEED: return "Causes unnaturally quick movement.";
    case LastingEffect::BLIND: return "Causes blindness";
    case LastingEffect::INVISIBLE: return "Causes invisibility.";
    case LastingEffect::POISON: return "Poisons.";
    case LastingEffect::POISON_RESISTANT: return "Gives poison resistance.";
    case LastingEffect::FLYING: return "Causes levitation.";
    case LastingEffect::PANIC: return "Increases defense and lowers damage.";
    case LastingEffect::RAGE: return "Increases damage and lowers defense.";
    case LastingEffect::HALLU: return "Causes hallucinations.";
    case LastingEffect::STR_BONUS: return "Gives a strength bonus.";
    case LastingEffect::DEX_BONUS: return "Gives a dexterity bonus.";
    case LastingEffect::SLEEP: return "Puts to sleep.";
    case LastingEffect::TIED_UP:;
    case LastingEffect::ENTANGLED: return "web";
    case LastingEffect::STUNNED: return "Causes stunning.";
    case LastingEffect::FIRE_RESISTANT: return "Gives fire resistance.";
    case LastingEffect::INSANITY: return "Confuses the target about who is friend and who is foe.";
    case LastingEffect::MAGIC_SHIELD: return "Gives protection from physical attacks.";
    case LastingEffect::DARKNESS_SOURCE: return "Causes the closest vicinity to become dark. Protects undead from sunlight.";
  }
}

string Effect::getDescription(const DirEffectType& type) {
  switch (type.getId()) {
    case DirEffectId::BLAST: return "Creates a directed blast that throws back creatures and items.";
    case DirEffectId::CREATURE_EFFECT:
        return "Creates a directed wave that " + noCapitalFirst(getDescription(type.get<EffectType>()));
        break;
  }
}


