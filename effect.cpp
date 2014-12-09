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
#include "square.h"
#include "level.h"
#include "creature_factory.h"
#include "item.h"
#include "view_object.h"
#include "view_id.h"
#include "model.h"
#include "trigger.h"

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

class IllusionController : public DoNothingController {
  public:
  IllusionController(Creature* c, double deathT) : DoNothingController(c), creature(c), deathTime(deathT) {}

  void kill() {
    creature->monsterMessage("The illusion disappears.");
    creature->die();
  }

  virtual void onBump(Creature*) override {
    kill();
  }

  virtual void makeMove() override {
    if (creature->getTime() >= deathTime)
      kill();
    creature->wait().perform();
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(DoNothingController) 
      & SVAR(creature)
      & SVAR(deathTime);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(IllusionController);

  private:
  Creature* SERIAL(creature);
  double SERIAL(deathTime);
};

template <class Archive>
void Effect::registerTypes(Archive& ar) {
  REGISTER_TYPE(ar, IllusionController);
}

REGISTER_TYPES(Effect);

static int summonCreatures(Level* level, Vec2 pos, int radius, vector<PCreature> creatures) {
  int numCreated = 0;
  vector<Vec2> area = Rectangle(-radius, -radius, radius, radius).getAllSquares();
  for (int i : All(creatures))
    for (Vec2 v : randomPermutation(area))
      if (level->inBounds(v + pos) && level->getSquare(v + pos)->canEnter(creatures[i].get())) {
        ++numCreated;
        level->addCreature(pos + v, std::move(creatures[i]));
        break;
  }
  return numCreated;
}

static int summonCreatures(Creature* c, int radius, vector<PCreature> creatures) {
  return summonCreatures(c->getLevel(), c->getPosition(), radius, std::move(creatures));
}

static void deception(Creature* c) {
  vector<PCreature> creatures;
  for (int i : Range(Random.get(3, 7))) {
    ViewObject viewObject(c->getViewObject().id(), ViewLayer::CREATURE, "Illusion");
    viewObject.setModifier(ViewObject::Modifier::ILLUSION);
    creatures.push_back(PCreature(new Creature(viewObject, c->getTribe(), CATTR(
          c.viewId = ViewId::ROCK; //overriden anyway
          c.attr[AttrType::SPEED] = 100;
          c.weight = 1;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 1;
          c.attr[AttrType::DEXTERITY] = 1;
          c.barehandedDamage = 20; // just so it's not ignored by creatures
          c.stationary = true;
          c.permanentEffects[LastingEffect::BLIND] = 1;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.noSleep = true;
          c.breathing = false;
          c.uncorporal = true;
          c.humanoid = true;
          c.name = "illusion";),
        ControllerFactory([c] (Creature* o) { return new IllusionController(o, c->getTime()
            + Random.get(5, 10));}))));
  }
  summonCreatures(c, 2, std::move(creatures));
}

static void leaveBody(Creature* creature) {
  string spiritName = creature->getFirstName().getOr(creature->getName().bare()) + "'s spirit";
  ViewObject viewObject(creature->getViewObject().id(), ViewLayer::CREATURE, spiritName);
  viewObject.setModifier(ViewObject::Modifier::ILLUSION);
  PCreature spirit(new Creature(viewObject, creature->getTribe(), CATTR(
          c.viewId = ViewId::ROCK; //overriden anyway
          c.attr[AttrType::SPEED] = 100;
          c.weight = 1;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 1;
          c.barehandedDamage = 20; // just so it's not ignored by creatures
          c.attr[AttrType::DEXTERITY] = 1;
          c.noSleep = true;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.breathing = false;
          c.uncorporal = true;
          c.humanoid = false;
          c.name = spiritName;),
        ControllerFactory([creature] (Creature* o) { return creature->getController()->getPossessedController(o);})));
  summonCreatures(creature, 1, makeVec<PCreature>(std::move(spirit)));
}

static void creatureEffect(Creature* who, EffectType type, EffectStrength str, Vec2 direction, int range) {
  for (Vec2 v = direction * (range - 1); v.length4() >= 1; v -= direction)
    if (Creature* c = who->getSquare(v)->getCreature())
      Effect::applyToCreature(c, type, str);
}

static void blast(Creature* who, Square* square, Vec2 direction, int maxDistance) {
  Level* l = square->getLevel();
  Vec2 position = square->getPosition();
  if (Creature* c = square->getCreature())
    if (!c->isStationary()) {
      int dist = 0;
      for (int i : Range(1, maxDistance))
        if (l->canMoveCreature(c, direction * i))
          dist = i;
        else
          break;
      if (dist > 0) {
        l->moveCreature(c, direction * dist);
        c->you(MsgType::ARE, "thrown back");
      }
      c->takeDamage(Attack(who, AttackLevel::MIDDLE, AttackType::SPELL, 1000, 32, false));
    }
  for (auto elem : Item::stackItems(square->getItems())) {
    l->throwItem(
        square->removeItems(elem.second),
        Attack(who, chooseRandom({AttackLevel::LOW, AttackLevel::MIDDLE, AttackLevel::HIGH}),
          elem.second[0]->getAttackType(), 15, 15, false), maxDistance, position, direction,
        Vision::get(VisionId::NORMAL));
  }
  if (square->canDestroy())
    square->destroy();
}

static void blast(Creature* c, Vec2 direction, int range) {
  for (Vec2 v = direction * (range - 1); v.length4() >= 1; v -= direction)
    blast(c, c->getSquare(v), direction, range);
}

static void wordOfPower(Creature* c, int strength) {
  GlobalEvents.addExplosionEvent(c->getLevel(), c->getPosition());
  for (Vec2 v : Vec2::directions8(true))
    blast(c, c->getSquare(v), v, wordOfPowerDist[strength]);
}

static void emitPoisonGas(Level* level, Vec2 pos, int strength, bool msg) {
  for (Vec2 v : pos.neighbors8())
    level->getSquare(v)->addPoisonGas(gasAmount[strength] / 2);
  level->getSquare(pos)->addPoisonGas(gasAmount[strength]);
  if (msg)
    level->globalMessage(pos, "A cloud of gas is released", "You hear a hissing sound");
}

static void guardingBuilder(Creature* c) {
  Optional<Vec2> dest;
  for (Vec2 v : Vec2::directions8(true))
    if (c->move(v) && !c->getSquare(v)->getCreature()) {
      dest = v;
      break;
    }
  Vec2 pos = c->getPosition();
  if (dest)
    c->getLevel()->moveCreature(c, *dest);
  else {
    Effect::applyToCreature(c, EffectType(EffectId::TELEPORT), EffectStrength::NORMAL);
  }
  if (c->getPosition() != pos) {
    PCreature boulder = CreatureFactory::getGuardingBoulder(c->getTribe());
    c->getLevel()->addCreature(pos, std::move(boulder));
  }
}

void Effect::summon(Creature* c, CreatureId id, int num, int ttl) {
  vector<PCreature> creatures;
  for (int i : Range(num))
    creatures.push_back(CreatureFactory::fromId(id, c->getTribe(), MonsterAIFactory::summoned(c, ttl)));
  summonCreatures(c, 2, std::move(creatures));
}

void Effect::summon(Level* level, CreatureFactory factory, Vec2 pos, int num, int ttl) {
  vector<PCreature> creatures;
  for (int i : Range(num))
    creatures.push_back(factory.random(MonsterAIFactory::dieTime(level->getModel()->getTime() + ttl)));
  summonCreatures(level, pos, 2, std::move(creatures));
}

static void enhanceArmor(Creature* c, int mod = 1, const string msg = "is improved") {
  for (EquipmentSlot slot : randomPermutation(getKeys(Equipment::slotTitles)))
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
    item->addModifier(chooseRandom({ModifierType::ACCURACY, ModifierType::DAMAGE}), mod);
  }
}

static void destroyEquipment(Creature* c) {
  vector<Item*> equiped;
  for (Item* item : c->getEquipment().getItems())
    if (c->getEquipment().isEquiped(item))
      equiped.push_back(item);
  Item* dest = chooseRandom(equiped);
  c->you(MsgType::YOUR, dest->getName() + " crumbles to dust.");
  c->steal({dest});
  return;
}

static void heal(Creature* c, int strength) {
  if (c->getHealth() < 1 || (strength == int(EffectStrength::STRONG) && c->lostOrInjuredBodyParts()))
    c->heal(1, strength == int(EffectStrength::STRONG));
  else
    c->playerMessage("You feel refreshed.");
}

static void portal(Creature* c) {
  Level* l = c->getLevel();
  for (Vec2 v : c->getPosition().neighbors8(true))
    if (l->getSquare(v)->canEnter(c)) {
      l->globalMessage(v, "A magic portal appears.");
      l->getSquare(v)->addTrigger(Trigger::getPortal(
            ViewObject(ViewId::PORTAL, ViewLayer::LARGE_ITEM, "Portal"), l, v));
      return;
    }
}

static void teleport(Creature* c) {
  Vec2 pos = c->getPosition();
  int cnt = 1000;
  Vec2 enemyRadius(12, 12);
  Vec2 teleRadius(6, 6);
  Level* l = c->getLevel();
  Rectangle area = l->getBounds().intersection(Rectangle(pos - enemyRadius, pos + enemyRadius));
  int infinity = 10000;
  Table<int> weight(area, infinity);
  queue<Vec2> q;
  for (Vec2 v : area)
    if (Creature *other = l->getSquare(v)->getCreature())
      if (other->isEnemy(c)) {
        q.push(v);
        weight[v] = 0;
      }
  while (!q.empty()) {
    Vec2 v = q.front();
    q.pop();
    for (Vec2 w : v.neighbors8())
      if (w.inRectangle(area) && l->getSquare(w)->canEnterEmpty({MovementTrait::WALK}) && weight[w] == infinity) {
        weight[w] = weight[v] + 1;
        q.push(w);
      }
  }
  vector<Vec2> good;
  int maxW = 0;
  for (Vec2 v : l->getBounds().intersection(Rectangle(pos - teleRadius, pos + teleRadius))) {
    if (!l->canMoveCreature(c, v - pos) || l->getSquare(v)->isBurning() || l->getSquare(v)->getPoisonGasAmount() > 0)
      continue;
    if (weight[v] == maxW)
      good.push_back(v);
    else if (weight[v] > maxW) {
      good = {v};
      maxW = weight[v];
    }
  }
  if (maxW < 2)
    c->playerMessage("The spell didn't work.");
  CHECK(!good.empty());
  c->you(MsgType::TELE_DISAPPEAR, "");
  l->moveCreature(c, chooseRandom(good) - pos);
  c->you(MsgType::TELE_APPEAR, "");
}

static void rollingBoulder(Creature* c) {
  int maxDist = 7;
  int minDist = 5;
  Level* l = c->getLevel();
  for (int dist = maxDist; dist >= minDist; --dist) {
    Vec2 pos = c->getPosition();
    vector<Vec2> possibleDirs;
    for (Vec2 dir : Vec2::directions8()) {
      bool good = true;
      for (int i : Range(1, dist + 1))
        if (!l->getSquare(pos + dir * i)->canEnterEmpty({MovementTrait::WALK})) {
          good = false;
          break;
        }
      if (good)
        possibleDirs.push_back(dir);
    }
    if (possibleDirs.empty())
      continue;
    Vec2 dir = chooseRandom(possibleDirs);
    l->globalMessage(pos + dir * dist, 
        PlayerMessage("A huge rolling boulder appears!", PlayerMessage::CRITICAL),
        PlayerMessage("You hear a heavy boulder rolling.", PlayerMessage::CRITICAL));
    Square* target = l->getSquare(pos + dir * dist);
    if (target->canDestroy())
      target->destroy();
    if (Creature *c = target->getCreature()) {
      c->you(MsgType::ARE, "killed by the boulder");
      c->die();
    } 
    l->addCreature(pos + dir * dist, CreatureFactory::getRollingBoulder(dir * (-1)));
    return;
  }
}

static void acid(Creature* c) {
  c->you(MsgType::ARE, "hurt by the acid");
  c->bleed(0.2);
  switch (Random.get(2)) {
    case 0 : enhanceArmor(c, -1, "corrodes"); break;
    case 1 : enhanceWeapon(c, -1, "corrodes"); break;
  }
}

static void alarm(Creature* c) {
  GlobalEvents.addAlarmEvent(c->getLevel(), c->getPosition());
}

static void teleEnemies(Creature* c) { // handled by Collective
}

double entangledTime(int strength) {
  return max(5, 30 - strength / 2);
}

void silverDamage(Creature* c) {
  if (c->isUndead()) {
    c->you(MsgType::ARE, "hurt by the silver");
    c->bleed(Random.getDouble(0.0, 0.15));
  }
}

double getDuration(const Creature* c, LastingEffect e, int strength) {
  switch (e) {
    case LastingEffect::ENTANGLED: return entangledTime(entangledTime(c->getAttr(AttrType::STRENGTH)));
    case LastingEffect::HALLU:
    case LastingEffect::SLOWED:
    case LastingEffect::SPEED:
    case LastingEffect::RAGE:
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

void Effect::applyToCreature(Creature* c, EffectType type, EffectStrength strengthEnum) {
  int strength = int(strengthEnum);
  switch (type.getId()) {
    case EffectId::LEAVE_BODY: leaveBody(c); break;
    case EffectId::LASTING:
        c->addEffect(type.get<LastingEffect>(), getDuration(c, type.get<LastingEffect>(), strength)); break;
    case EffectId::TELE_ENEMIES: teleEnemies(c); break;
    case EffectId::ALARM: alarm(c); break;
    case EffectId::ACID: acid(c); break;
    case EffectId::SUMMON_INSECTS: summon(c, CreatureId::FLY, Random.get(3, 7), 100); break;
    case EffectId::DECEPTION: deception(c); break;
    case EffectId::WORD_OF_POWER: wordOfPower(c, strength); break;
    case EffectId::GUARDING_BOULDER: guardingBuilder(c); break;
    case EffectId::FIRE_SPHERE_PET: summon(c, CreatureId::FIRE_SPHERE, 1, 30); break;
    case EffectId::ENHANCE_ARMOR: enhanceArmor(c); break;
    case EffectId::ENHANCE_WEAPON: enhanceWeapon(c); break;
    case EffectId::DESTROY_EQUIPMENT: destroyEquipment(c); break;
    case EffectId::HEAL: heal(c, strength); break;
    case EffectId::IDENTIFY: c->grantIdentify(identifyNum[strength]); break;
    case EffectId::FIRE: c->getSquare()->setOnFire(fireAmount[strength]); break;
    case EffectId::PORTAL: portal(c); break;
    case EffectId::TELEPORT: teleport(c); break;
    case EffectId::ROLLING_BOULDER: rollingBoulder(c); break;
    case EffectId::SUMMON_SPIRIT: summon(c, CreatureId::SPIRIT, Random.get(2, 5), 100); break;
    case EffectId::EMIT_POISON_GAS: emitPoisonGas(c->getLevel(), c->getPosition(), strength, true); break;
    case EffectId::SILVER_DAMAGE: silverDamage(c); break;
    case EffectId::CURE_POISON: c->removeEffect(LastingEffect::POISON); break;
    case EffectId::METEOR_SHOWER: c->getSquare()->addTrigger(Trigger::getMeteorShower(c, 15)); break;
  }
}

void Effect::applyToPosition(Level* level, Vec2 pos, EffectType type, EffectStrength strength) {
  switch (type.getId()) {
    case EffectId::EMIT_POISON_GAS: emitPoisonGas(level, pos, int(strength), false); break;
    default: FAIL << "Can't apply to position " << int(type.getId());
  }
}

void Effect::applyDirected(Creature* c, Vec2 direction, DirEffectType type, EffectStrength strength) {
  switch (type.getId()) {
    case DirEffectId::BLAST: blast(c, direction, blastRange[int(strength)]); break;
    case DirEffectId::CREATURE_EFFECT:
        creatureEffect(c, type.get<EffectType>(), strength, direction, creatureEffectRange[int(strength)]);
        break;
    default: FAIL << "Can't apply directed " << int(type.getId());
  }
}

string Effect::getName(EffectType type) {
  switch (type.getId()) {
    case EffectId::HEAL: return "healing";
    case EffectId::TELEPORT: return "teleport";
    case EffectId::IDENTIFY: return "identify";
    case EffectId::ROLLING_BOULDER: return "rolling boulder";
    case EffectId::PORTAL: return "magic portal";
    case EffectId::EMIT_POISON_GAS: return "poison gas";
    case EffectId::DESTROY_EQUIPMENT: return "destruction";
    case EffectId::ENHANCE_WEAPON: return "weapon enchantement";
    case EffectId::ENHANCE_ARMOR: return "armor enchantement";
    case EffectId::FIRE_SPHERE_PET: return "fire sphere";
    case EffectId::WORD_OF_POWER: return "power";
    case EffectId::DECEPTION: return "deception";
    case EffectId::SUMMON_INSECTS: return "insect summoning";
    case EffectId::LEAVE_BODY: return "possesion";
    case EffectId::FIRE: return "fire";
    case EffectId::ACID: return "acid";
    case EffectId::GUARDING_BOULDER: return "boulder";
    case EffectId::ALARM: return "alarm";
    case EffectId::TELE_ENEMIES: return "surprise";
    case EffectId::SUMMON_SPIRIT: return "spirit summoning";
    case EffectId::SILVER_DAMAGE: return "silver";
    case EffectId::CURE_POISON: return "cure poisoning";
    case EffectId::METEOR_SHOWER: return "meteor shower";
    case EffectId::LASTING: return getName(type.get<LastingEffect>());
  }
  return "";
}

string Effect::getName(LastingEffect type) {
  switch (type) {
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
    case LastingEffect::ENTANGLED: return "web";
    case LastingEffect::STUNNED: return "stunning";
    case LastingEffect::FIRE_RESISTANT: return "fire resistance";
    case LastingEffect::INSANITY: return "insanity";
    case LastingEffect::MAGIC_SHIELD: return "magic shield";
  }
  return "";
}
