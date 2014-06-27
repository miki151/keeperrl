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
#include "message_buffer.h"

vector<int> healingPoints { 5, 15, 40};
vector<int> sleepTime { 15, 80, 200};
vector<int> panicTime { 5, 15, 40};
vector<int> halluTime { 30, 100, 250};
vector<int> blindTime { 5, 15, 45};
vector<int> invisibleTime { 5, 15, 45};
vector<double> fireAmount { 0.5, 1, 1};
vector<int> attrBonusTime { 10, 40, 150};
vector<int> identifyNum { 1, 1, 400};
vector<int> poisonTime { 200, 60, 20};
vector<int> stunTime { 200, 60, 20};
vector<int> resistantTime { 200, 60, 20};
vector<int> levitateTime { 200, 60, 20};
vector<double> gasAmount { 0.3, 0.8, 3};
vector<double> wordOfPowerDist { 1, 3, 10};

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

static int summonCreatures(Creature* c, int radius, vector<PCreature> creatures) {
  int numCreated = 0;
  vector<Vec2> area = Rectangle(-radius, -radius, radius, radius).getAllSquares();
  for (int i : All(creatures))
    for (Vec2 v : randomPermutation(area))
      if (c->getLevel()->inBounds(v + c->getPosition()) && c->getSquare(v)->canEnter(creatures[i].get())) {
        ++numCreated;
        c->getLevel()->addCreature(c->getPosition() + v, std::move(creatures[i]));
        break;
  }
  return numCreated;
}

static void insects(Creature* c) {
  vector<PCreature> creatures;
  for (int i : Range(Random.getRandom(3, 7)))
    creatures.push_back(CreatureFactory::fromId(CreatureId::FLY, c->getTribe()));
  summonCreatures(c, 1, std::move(creatures));
}

static void deception(Creature* c) {
  vector<PCreature> creatures;
  for (int i : Range(Random.getRandom(3, 7))) {
    ViewObject viewObject(c->getViewObject().id(), ViewLayer::CREATURE, "Illusion");
    viewObject.setModifier(ViewObject::Modifier::ILLUSION);
    creatures.push_back(PCreature(new Creature(viewObject, c->getTribe(), CATTR(
          c.viewId = ViewId::ROCK; //overriden anyway
          c.speed = 100;
          c.weight = 1;
          c.size = CreatureSize::LARGE;
          c.strength = 1;
          c.dexterity = 1;
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
            + Random.getRandom(5, 10));}))));
  }
  summonCreatures(c, 2, std::move(creatures));
}

static void leaveBody(Creature* creature) {
  string spiritName = creature->getFirstName().getOr(creature->getName()) + "'s spirit";
  ViewObject viewObject(creature->getViewObject().id(), ViewLayer::CREATURE, spiritName);
  viewObject.setModifier(ViewObject::Modifier::ILLUSION);
  PCreature spirit(new Creature(viewObject, creature->getTribe(), CATTR(
          c.viewId = ViewId::ROCK; //overriden anyway
          c.speed = 100;
          c.weight = 1;
          c.size = CreatureSize::LARGE;
          c.strength = 1;
          c.barehandedDamage = 20; // just so it's not ignored by creatures
          c.dexterity = 1;
          c.noSleep = true;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.breathing = false;
          c.uncorporal = true;
          c.humanoid = false;
          c.name = spiritName;),
        ControllerFactory([creature] (Creature* o) { return creature->getController()->getPossessedController(o);})));
  summonCreatures(creature, 1, makeVec<PCreature>(std::move(spirit)));
}

static void wordOfPower(Creature* c, int strength) {
  Level* l = c->getLevel();
  EventListener::addExplosionEvent(c->getLevel(), c->getPosition());
  for (Vec2 v : Vec2::directions8(true)) {
    if (Creature* other = c->getSquare(v)->getCreature()) {
      if (other->isStationary())
        continue;
      if (!other->isDead()) {
        int dist = 0;
        for (int i : Range(1, wordOfPowerDist[strength]))
          if (l->canMoveCreature(other, v * i))
            dist = i;
          else
            break;
        if (dist > 0) {
          l->moveCreature(other, v * dist);
          other->you(MsgType::ARE, "pushed back");
        }
        other->addEffect(LastingEffect::STUNNED, 2);
        other->takeDamage(Attack(c, AttackLevel::MIDDLE, AttackType::SPELL, 1000, 32, false));
      }
    }
    for (auto elem : Item::stackItems(c->getSquare(v)->getItems())) {
      l->throwItem(
        c->getSquare(v)->removeItems(elem.second),
        Attack(c, chooseRandom({AttackLevel::LOW, AttackLevel::MIDDLE, AttackLevel::HIGH}),
        elem.second[0]->getAttackType(), 15, 15, false), wordOfPowerDist[strength], c->getPosition(), v,
        Vision::get(VisionId::NORMAL));
    }
  }
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
    Effect::applyToCreature(c, EffectType::TELEPORT, EffectStrength::NORMAL);
  }
  if (c->getPosition() != pos) {
    PCreature boulder = CreatureFactory::getGuardingBoulder(c->getTribe());
    c->getLevel()->addCreature(pos, std::move(boulder));
  }
}

static void summon(Creature* c, CreatureId id, int num, int ttl) {
  vector<PCreature> creatures;
  for (int i : Range(num))
    creatures.push_back(CreatureFactory::fromId(id, c->getTribe(), MonsterAIFactory::summoned(c, ttl)));
  summonCreatures(c, 2, std::move(creatures));
}

static void enhanceArmor(Creature* c, int mod = 1, const string msg = "is improved") {
  for (EquipmentSlot slot : randomPermutation(getKeys(Equipment::slotTitles)))
    for (Item* item : c->getEquipment().getItem(slot))
      if (item->getType() == ItemType::ARMOR) {
        c->you(MsgType::YOUR, item->getName() + " " + msg);
        if (item->getModifier(AttrType::DEFENSE) > 0 || mod > 0)
          item->addModifier(AttrType::DEFENSE, mod);
        return;
      }
}

static void enhanceWeapon(Creature* c, int mod = 1, const string msg = "is improved") {
  if (Item* item = c->getWeapon()) {
    c->you(MsgType::YOUR, item->getName() + " " + msg);
    item->addModifier(chooseRandom({AttrType::TO_HIT, AttrType::DAMAGE}), mod);
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

static void sleep(Creature* c, int strength) {
  Square *square = c->getLevel()->getSquare(c->getPosition());
  c->you(MsgType::FALL_ASLEEP, square->getName());
  c->addEffect(LastingEffect::SLEEP, Random.getRandom(sleepTime[strength]));
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
      if (w.inRectangle(area) && l->getSquare(w)->canEnterEmpty(Creature::getDefault()) && weight[w] == infinity) {
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
        if (!l->getSquare(pos + dir * i)->canEnterEmpty(Creature::getDefault())) {
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
        MessageBuffer::important("A huge rolling boulder appears!"),
        MessageBuffer::important("You hear a heavy boulder rolling."));
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
  switch (Random.getRandom(2)) {
    case 0 : enhanceArmor(c, -1, "corrodes"); break;
    case 1 : enhanceWeapon(c, -1, "corrodes"); break;
  }
}

static void alarm(Creature* c) {
  EventListener::addAlarmEvent(c->getLevel(), c->getPosition());
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

void Effect::applyToCreature(Creature* c, EffectType type, EffectStrength strengthEnum) {
  int strength = int(strengthEnum);
  switch (type) {
    case EffectType::LEAVE_BODY: leaveBody(c); break;
    case EffectType::WEB:
      c->addEffect(LastingEffect::ENTANGLED, entangledTime(c->getAttr(AttrType::STRENGTH))); break;
    case EffectType::TELE_ENEMIES: teleEnemies(c); break;
    case EffectType::ALARM: alarm(c); break;
    case EffectType::ACID: acid(c); break;
    case EffectType::SUMMON_INSECTS: insects(c); break;
    case EffectType::DECEPTION: deception(c); break;
    case EffectType::WORD_OF_POWER: wordOfPower(c, strength); break;
    case EffectType::POISON: c->addEffect(LastingEffect::POISON, poisonTime[strength]); break;
    case EffectType::GUARDING_BOULDER: guardingBuilder(c); break;
    case EffectType::FIRE_SPHERE_PET: summon(c, CreatureId::FIRE_SPHERE, 1, 30); break;
    case EffectType::ENHANCE_ARMOR: enhanceArmor(c); break;
    case EffectType::ENHANCE_WEAPON: enhanceWeapon(c); break;
    case EffectType::DESTROY_EQUIPMENT: destroyEquipment(c); break;
    case EffectType::HEAL: heal(c, strength); break;
    case EffectType::SLEEP: sleep(c, strength); break;
    case EffectType::IDENTIFY: c->grantIdentify(identifyNum[strength]); break;
    case EffectType::TERROR: c->globalMessage("You hear maniacal laughter close by",
                                 "You hear maniacal laughter in the distance");
    case EffectType::PANIC: c->addEffect(LastingEffect::PANIC, panicTime[strength]); break;
    case EffectType::RAGE: c->addEffect(LastingEffect::RAGE, panicTime[strength]); break;
    case EffectType::HALLU: c->addEffect(LastingEffect::HALLU, panicTime[strength]); break;
    case EffectType::STR_BONUS: c->addEffect(LastingEffect::STR_BONUS, attrBonusTime[strength]); break;
    case EffectType::DEX_BONUS: c->addEffect(LastingEffect::DEX_BONUS, attrBonusTime[strength]); break;
    case EffectType::SLOW: c->addEffect(LastingEffect::SLOWED, panicTime[strength]); break;
    case EffectType::SPEED: c->addEffect(LastingEffect::SPEED, panicTime[strength]); break;
    case EffectType::BLINDNESS: c->addEffect(LastingEffect::BLIND, blindTime[strength]); break;
    case EffectType::INVISIBLE: c->addEffect(LastingEffect::INVISIBLE, invisibleTime[strength]); break;
    case EffectType::FIRE: c->getSquare()->setOnFire(fireAmount[strength]); break;
    case EffectType::PORTAL: portal(c); break;
    case EffectType::TELEPORT: teleport(c); break;
    case EffectType::ROLLING_BOULDER: rollingBoulder(c); break;
    case EffectType::SUMMON_SPIRIT: summon(c, CreatureId::SPIRIT, Random.getRandom(2, 5), 100); break;
    case EffectType::EMIT_POISON_GAS: emitPoisonGas(c->getLevel(), c->getPosition(), strength, true); break;
    case EffectType::STUN: c->addEffect(LastingEffect::STUNNED, stunTime[strength]); break;
    case EffectType::POISON_RESISTANCE: c->addEffect(LastingEffect::POISON_RESISTANT, resistantTime[strength]); break;
    case EffectType::FIRE_RESISTANCE: c->addEffect(LastingEffect::FIRE_RESISTANT, resistantTime[strength]); break;
    case EffectType::LEVITATION: c->addEffect(LastingEffect::FLYING, levitateTime[strength]); break;
    case EffectType::SILVER_DAMAGE: silverDamage(c); break;
  }
}

void Effect::applyToPosition(Level* level, Vec2 pos, EffectType type, EffectStrength strength) {
  switch (type) {
    case EffectType::EMIT_POISON_GAS: emitPoisonGas(level, pos, int(strength), false); break;
    default: FAIL << "Can't apply to position " << int(type);
  }
}

