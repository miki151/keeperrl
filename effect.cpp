#include "stdafx.h"

#include "effect.h"
#include "controller.h"
#include "creature.h"
#include "square.h"
#include "level.h"
#include "creature_factory.h"
#include "message_buffer.h"

map<EffectStrength, int> healingPoints { 
    {EffectStrength::WEAK, 5},
    {EffectStrength::NORMAL, 15},
    {EffectStrength::STRONG, 40}};

map<EffectStrength, int> sleepTime { 
    {EffectStrength::WEAK, 15},
    {EffectStrength::NORMAL, 80},
    {EffectStrength::STRONG, 200}};

map<EffectStrength, int> panicTime { 
    {EffectStrength::WEAK, 5},
    {EffectStrength::NORMAL, 15},
    {EffectStrength::STRONG, 40}};

map<EffectStrength, int> halluTime { 
    {EffectStrength::WEAK, 30},
    {EffectStrength::NORMAL, 100},
    {EffectStrength::STRONG, 250}};

map<EffectStrength, int> blindTime { 
    {EffectStrength::WEAK, 5},
    {EffectStrength::NORMAL, 15},
    {EffectStrength::STRONG, 45}};

map<EffectStrength, int> invisibleTime { 
    {EffectStrength::WEAK, 5},
    {EffectStrength::NORMAL, 15},
    {EffectStrength::STRONG, 45}};

map<EffectStrength, int> fireAmount { 
    {EffectStrength::WEAK, 0.5},
    {EffectStrength::NORMAL, 1},
    {EffectStrength::STRONG, 1}};

map<EffectStrength, int> attrBonusTime { 
    {EffectStrength::WEAK, 10},
    {EffectStrength::NORMAL, 40},
    {EffectStrength::STRONG, 150}};

map<EffectStrength, int> identifyNum { 
    {EffectStrength::WEAK, 1},
    {EffectStrength::NORMAL, 1},
    {EffectStrength::STRONG, 400}};

map<EffectStrength, int> poisonTime { 
    {EffectStrength::WEAK, 200},
    {EffectStrength::NORMAL, 60},
    {EffectStrength::STRONG, 20}};

map<EffectStrength, double> gasAmount { 
    {EffectStrength::WEAK, 0.3},
    {EffectStrength::NORMAL, 0.8},
    {EffectStrength::STRONG, 3}};

map<EffectStrength, double> wordOfPowerDist { 
    {EffectStrength::WEAK, 1},
    {EffectStrength::NORMAL, 3},
    {EffectStrength::STRONG, 10}};

class IllusionController : public DoNothingController {
  public:
  IllusionController(Creature* c, double deathT) : DoNothingController(c), creature(c), deathTime(deathT) {}

  void kill() {
    creature->globalMessage("The illusion disappears.");
    creature->die();
  }

  virtual void onBump(Creature*) override {
    kill();
  }

  virtual void makeMove() override {
    if (creature->getTime() >= deathTime)
      kill();
    creature->wait();
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
    viewObject.setModifier(ViewObject::ILLUSION);
    creatures.push_back(PCreature(new Creature(viewObject, c->getTribe(), CATTR(
          c.viewId = ViewId::ROCK; //overriden anyway
          c.speed = 100;
          c.weight = 1;
          c.size = CreatureSize::LARGE;
          c.strength = 1;
          c.dexterity = 1;
          c.barehandedDamage = 20; // just so it's not ignored by creatures
          c.stationary = true;
          c.permanentlyBlind = true;
          c.noSleep = true;
          c.flyer = true;
          c.breathing = false;
          c.noBody = true;
          c.humanoid = true;
          c.name = "illusion";),
        ControllerFactory([c] (Creature* o) { return new IllusionController(o, c->getTime()
            + Random.getRandom(5, 10));}))));
  }
  summonCreatures(c, 2, std::move(creatures));
}

static void wordOfPower(Creature* c, EffectStrength strength) {
  Level* l = c->getLevel();
  EventListener::addExplosionEvent(c->getLevel(), c->getPosition());
  for (Vec2 v : Vec2::directions8(true)) {
    if (Creature* other = c->getSquare(v)->getCreature()) {
      if (other->isStationary())
        continue;
      if (!other->isDead()) {
        int dist = 0;
        for (int i : Range(1, wordOfPowerDist.at(strength)))
          if (l->canMoveCreature(other, v * i))
            dist = i;
          else
            break;
        if (dist > 0) {
          l->moveCreature(other, v * dist);
          other->you(MsgType::ARE, "pushed back");
        }
        other->addEffect(Creature::STUNNED, 2);
        other->takeDamage(Attack(c, AttackLevel::MIDDLE, AttackType::SPELL, 1000, 32, false));
      }
    }
    for (auto elem : Item::stackItems(c->getSquare(v)->getItems())) {
      l->throwItem(
        c->getSquare(v)->removeItems(elem.second),
        Attack(c, chooseRandom({AttackLevel::LOW, AttackLevel::MIDDLE, AttackLevel::HIGH}),
        elem.second[0]->getAttackType(), 15, 15, false), wordOfPowerDist.at(strength), c->getPosition(), v,
        VisionInfo::NORMAL);
    }
  }
}

static void emitPoisonGas(Creature* c, EffectStrength strength) {
  for (Vec2 v : Vec2::directions8())
    c->getSquare(v)->addPoisonGas(gasAmount.at(strength) / 2);
  c->getSquare()->addPoisonGas(gasAmount.at(strength));
  c->globalMessage("A cloud of gas is released", "You hear a hissing sound");
}

static void guardingBuilder(Creature* c) {
  Optional<Vec2> dest;
  for (Vec2 v : Vec2::directions8(true))
    if (c->canMove(v) && !c->getSquare(v)->getCreature()) {
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
  for (EquipmentSlot slot : randomPermutation({
        EquipmentSlot::BODY_ARMOR, EquipmentSlot::HELMET, EquipmentSlot::BOOTS}))
    if (Item* item = c->getEquipment().getItem(slot)) {
      c->you(MsgType::YOUR, item->getName() + " " + msg);
      if (item->getModifier(AttrType::DEFENSE) > 0 || mod > 0)
        item->addModifier(AttrType::DEFENSE, mod);
      return;
    }
}

static void enhanceWeapon(Creature* c, int mod = 1, const string msg = "is improved") {
  if (Item* item = c->getEquipment().getItem(EquipmentSlot::WEAPON)) {
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

static void heal(Creature* c, EffectStrength strength) {
  if (c->getHealth() < 1 || (strength == EffectStrength::STRONG && c->lostLimbs()))
    c->heal(1, strength == EffectStrength::STRONG);
  else
    c->privateMessage("You feel refreshed.");
}

static void sleep(Creature* c, EffectStrength strength) {
  Square *square = c->getLevel()->getSquare(c->getPosition());
  c->you(MsgType::FALL_ASLEEP, square->getName());
  c->addEffect(Creature::SLEEP, Random.getRandom(sleepTime[strength]));
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
    c->privateMessage("The spell didn't work.");
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

void Effect::applyToCreature(Creature* c, EffectType type, EffectStrength strength) {
  switch (type) {
    case EffectType::WEB: c->addEffect(Creature::ENTANGLED, entangledTime(c->getAttr(AttrType::STRENGTH))); break;
    case EffectType::TELE_ENEMIES: teleEnemies(c); break;
    case EffectType::ALARM: alarm(c); break;
    case EffectType::ACID: acid(c); break;
    case EffectType::SUMMON_INSECTS: insects(c); break;
    case EffectType::DECEPTION: deception(c); break;
    case EffectType::WORD_OF_POWER: wordOfPower(c, strength); break;
    case EffectType::POISON: c->addEffect(Creature::POISON, poisonTime.at(strength)); break;
    case EffectType::EMIT_POISON_GAS: emitPoisonGas(c, strength); break;
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
    case EffectType::PANIC: c->addEffect(Creature::PANIC, panicTime[strength]); break;
    case EffectType::RAGE: c->addEffect(Creature::RAGE, panicTime[strength]); break;
    case EffectType::HALLU: c->addEffect(Creature::HALLU, panicTime[strength]); break;
    case EffectType::STR_BONUS: c->addEffect(Creature::STR_BONUS, attrBonusTime[strength]); break;
    case EffectType::DEX_BONUS: c->addEffect(Creature::DEX_BONUS, attrBonusTime[strength]); break;
    case EffectType::SLOW: c->addEffect(Creature::SLOWED, panicTime[strength]); break;
    case EffectType::SPEED: c->addEffect(Creature::SPEED, panicTime[strength]); break;
    case EffectType::BLINDNESS: c->addEffect(Creature::BLIND, blindTime[strength]); break;
    case EffectType::INVISIBLE: c->addEffect(Creature::INVISIBLE, invisibleTime[strength]); break;
    case EffectType::FIRE: c->getSquare()->setOnFire(fireAmount[strength]); break;
    case EffectType::PORTAL: portal(c); break;
    case EffectType::TELEPORT: teleport(c); break;
    case EffectType::ROLLING_BOULDER: rollingBoulder(c); break;
    case EffectType::SUMMON_SPIRIT: summon(c, CreatureId::SPIRIT, Random.getRandom(2, 5), 100); break;
  }
}

