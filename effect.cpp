#include "stdafx.h"

using namespace std;

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
    {EffectStrength::WEAK, 30},
    {EffectStrength::NORMAL, 100},
    {EffectStrength::STRONG, 450}};

map<EffectStrength, int> identifyNum { 
    {EffectStrength::WEAK, 1},
    {EffectStrength::NORMAL, 1},
    {EffectStrength::STRONG, 400}};

map<EffectStrength, int> poisonTime { 
    {EffectStrength::WEAK, 200},
    {EffectStrength::NORMAL, 60},
    {EffectStrength::STRONG, 20}};

static void emitPoisonGas(Creature* c, EffectStrength strength) {
  for (Vec2 v : Vec2::directions8())
    c->getSquare(v)->addPoisonGas(1);
  c->getSquare()->addPoisonGas(2);
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

static void fireSpherePet(Creature* c) {
  PCreature sphere = CreatureFactory::fromId(
      CreatureId::FIRE_SPHERE, c->getTribe(), MonsterAIFactory::follower(c, 1));
  for (Vec2 v : Vec2::directions8(true))
    if (c->getSquare(v)->canEnter(sphere.get())) {
      c->getLevel()->addCreature(c->getPosition() + v, std::move(sphere));
      c->globalMessage("A fire sphere appears.");
      break;
    }
}

static void enhanceArmor(Creature* c) {
  for (EquipmentSlot slot : randomPermutation({
        EquipmentSlot::BODY_ARMOR, EquipmentSlot::HELMET, EquipmentSlot::BOOTS}))
    if (Item* item = c->getEquipment().getItem(slot)) {
      c->you(MsgType::YOUR, item->getName() + " seems improved");
      item->addModifier(AttrType::DEFENSE, 1);
      return;
    }
}

static void enhanceWeapon(Creature* c) {
  if (Item* item = c->getEquipment().getItem(EquipmentSlot::WEAPON)) {
    c->you(MsgType::YOUR, item->getName() + " seems improved");
    item->addModifier(chooseRandom({AttrType::TO_HIT, AttrType::DAMAGE}), 1);
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
  c->sleep(Random.getRandom(sleepTime[strength]));
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
  Vec2 pos;
  int cnt = 1000;
  int maxRadius = 8;
  int minRadius = 4;
  Level* l = c->getLevel();
  vector<pair<int, Vec2>> dest;
  for (Vec2 v : Rectangle(-maxRadius, -maxRadius, maxRadius, maxRadius))
    if (int(v.lengthD()) <= maxRadius)
      dest.emplace_back(v.lengthD(), v);
  sort(dest.begin(), dest.end(),
      [] (const pair<int, Vec2>& e1, const pair<int, Vec2>& e2) { return e1.first > e2.first; });
  CHECK(dest.front().first > dest.back().first) << dest.front().first << " " << dest.front().second;
  vector<Vec2> goodDest;
  c->setHeld(nullptr);
  for (int i : All(dest)) {
    if (l->canMoveCreature(c, dest[i].second))
      goodDest.push_back(dest[i].second);
    if (i > 0 && dest[i].first < dest[i - 1].first && goodDest.size() > 0) {
      pos = chooseRandom(goodDest);
      break;
    }
    if (dest[i].first < minRadius)
      c->privateMessage("The spell didn't work.");
  }
  c->you(MsgType::TELE_DISAPPEAR, "");
  l->moveCreature(c, pos);
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
      target->destroy(1000);
    if (Creature *c = target->getCreature()) {
      c->you(MsgType::ARE, "killed by the boulder");
      c->die();
    } 
    l->addCreature(pos + dir * dist, CreatureFactory::getRollingBoulder(dir * (-1)));
    return;
  }
}

void Effect::applyToCreature(Creature* c, EffectType type, EffectStrength strength) {
  switch (type) {
    case EffectType::POISON: c->poison(poisonTime.at(strength)); break;
    case EffectType::EMIT_POISON_GAS: emitPoisonGas(c, strength); break;
    case EffectType::GUARDING_BOULDER: guardingBuilder(c); break;
    case EffectType::FIRE_SPHERE_PET: fireSpherePet(c); break;
    case EffectType::ENHANCE_ARMOR: enhanceArmor(c); break;
    case EffectType::ENHANCE_WEAPON: enhanceWeapon(c); break;
    case EffectType::DESTROY_EQUIPMENT: destroyEquipment(c); break;
    case EffectType::HEAL: heal(c, strength); break;
    case EffectType::SLEEP: sleep(c, strength); break;
    case EffectType::IDENTIFY: c->grantIdentify(identifyNum[strength]); break;
    case EffectType::PANIC: c->panic(panicTime[strength]); break;
    case EffectType::RAGE: c->rage(panicTime[strength]); break;
    case EffectType::HALLU: c->hallucinate(panicTime[strength]); break;
    case EffectType::STR_BONUS: c->giveStrBonus(attrBonusTime[strength]); break;
    case EffectType::DEX_BONUS: c->giveDexBonus(attrBonusTime[strength]); break;
    case EffectType::SLOW: c->slowDown(panicTime[strength]); break;
    case EffectType::SPEED: c->speedUp(panicTime[strength]); break;
    case EffectType::BLINDNESS: c->blind(blindTime[strength]); break;
    case EffectType::INVISIBLE: c->makeInvisible(invisibleTime[strength]); break;
    case EffectType::FIRE: c->setOnFire(fireAmount[strength]); break;
    case EffectType::PORTAL: portal(c); break;
    case EffectType::TELEPORT: teleport(c); break;
    case EffectType::ROLLING_BOULDER: rollingBoulder(c); break;
  }
}

