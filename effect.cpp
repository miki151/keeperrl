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

Effect::Effect(EffectType t) : type(t) {}

EffectType Effect::getType() {
  return type;
}

PEffect Effect::getEffect(EffectType type) {
  switch (type) {
    case EffectType::HEAL: return PEffect(new HealingEffect());
    case EffectType::IDENTIFY: return PEffect(new IdentifyEffect());
    case EffectType::TELEPORT: return PEffect(new TeleportEffect());
    case EffectType::PORTAL: return PEffect(new PortalEffect());
    case EffectType::SLEEP: return PEffect(new SleepEffect());
    case EffectType::PANIC: return PEffect(new PanicEffect());
    case EffectType::RAGE: return PEffect(new RageEffect());
    case EffectType::ROLLING_BOULDER: return PEffect(new RollingBoulderEffect());
    case EffectType::FIRE: return PEffect(new FireEffect());
    case EffectType::SLOW: return PEffect(new SlowEffect());
    case EffectType::SPEED: return PEffect(new SpeedEffect());
    case EffectType::HALLU: return PEffect(new HalluEffect());
    case EffectType::BLINDNESS: return PEffect(new BlindnessEffect());
    case EffectType::INVISIBLE: return PEffect(new InvisibleEffect());
    case EffectType::STR_BONUS: return PEffect(new StrBonusEffect());
    case EffectType::DEX_BONUS: return PEffect(new DexBonusEffect());
    case EffectType::DESTROY_EQUIPMENT: return PEffect(new DestroyEquipmemntEffect());
    case EffectType::ENHANCE_ARMOR: return PEffect(new EnhanceArmorEffect());
    case EffectType::ENHANCE_WEAPON: return PEffect(new EnhanceWeaponEffect());
    case EffectType::FIRE_SPHERE_PET: return PEffect(new FireSpherePetEffect());
    case EffectType::GUARDING_BOULDER: return PEffect(new GuardingBuilderEffect());
    case EffectType::EMIT_POISON_GAS: return PEffect(new EmitPoisonGasEffect());
    default: Debug(FATAL) << "Can't construct effect " << (int)type;
  }
  return PEffect(nullptr);
}

PEffect Effect::giveItemEffect(ItemId id, int num) {
  return PEffect(new GiveItemEffect(id, num));
}

EmitPoisonGasEffect::EmitPoisonGasEffect() : Effect(EffectType::EMIT_POISON_GAS) {}

void EmitPoisonGasEffect::applyToCreature(Creature* c, EffectStrength strength) {
  for (Vec2 v : Vec2::directions8())
    c->getSquare(v)->addPoisonGas(1);
  c->getSquare()->addPoisonGas(2);
}

GuardingBuilderEffect::GuardingBuilderEffect() : Effect(EffectType::GUARDING_BOULDER) {}

void GuardingBuilderEffect::applyToCreature(Creature* c, EffectStrength strength) {
  Optional<Vec2> dest;
  for (Vec2 v : Vec2::directions8(true))
    if (c->canMove(v) && !c->getSquare(v)->getCreature()) {
      dest = v;
      break;
    }
  Vec2 pos = c->getPosition();
  if (dest)
    c->move(*dest);
  else {
    Effect::getEffect(EffectType::TELEPORT)->applyToCreature(c, EffectStrength::NORMAL);
  }
  if (c->getPosition() != pos) {
    PCreature boulder = CreatureFactory::getGuardingBoulder(c->getTribe());
    c->getLevel()->addCreature(pos, std::move(boulder));
  }
}

FireSpherePetEffect::FireSpherePetEffect() : Effect(EffectType::FIRE_SPHERE_PET) {}

void FireSpherePetEffect::applyToCreature(Creature* c, EffectStrength strength) {
  PCreature sphere = CreatureFactory::fromId(
      CreatureId::FIRE_SPHERE, c->getTribe(), MonsterAIFactory::follower(c, 1));
  for (Vec2 v : Vec2::directions8(true))
    if (c->getSquare(v)->canEnter(sphere.get())) {
      c->getLevel()->addCreature(c->getPosition() + v, std::move(sphere));
      c->globalMessage("A fire sphere appears.");
      break;
    }
}

EnhanceArmorEffect::EnhanceArmorEffect() : Effect(EffectType::ENHANCE_ARMOR) {}

void EnhanceArmorEffect::applyToCreature(Creature* c, EffectStrength strength) {
  for (EquipmentSlot slot : randomPermutation({ EquipmentSlot::BODY_ARMOR, EquipmentSlot::HELMET}))
    if (Item* item = c->getEquipment().getItem(slot)) {
      c->you(MsgType::YOUR, item->getName() + " seems improved");
      item->addModifier(AttrType::DEFENSE, 1);
      return;
    }
}

EnhanceWeaponEffect::EnhanceWeaponEffect() : Effect(EffectType::ENHANCE_WEAPON) {}

void EnhanceWeaponEffect::applyToCreature(Creature* c, EffectStrength strength) {
  if (Item* item = c->getEquipment().getItem(EquipmentSlot::WEAPON)) {
    c->you(MsgType::YOUR, item->getName() + " seems improved");
    item->addModifier(chooseRandom({AttrType::TO_HIT, AttrType::DAMAGE}), 1);
  }
}

DestroyEquipmemntEffect::DestroyEquipmemntEffect() : Effect(EffectType::DESTROY_EQUIPMENT) {}

void DestroyEquipmemntEffect::applyToCreature(Creature* c, EffectStrength strength) {
  vector<Item*> equiped;
  for (Item* item : c->getEquipment().getItems())
    if (c->getEquipment().isEquiped(item))
      equiped.push_back(item);
  Item* dest = chooseRandom(equiped);
  c->you(MsgType::YOUR, dest->getName() + " crumbles to dust.");
  c->steal({dest});
  return;
}

HealingEffect::HealingEffect() : Effect(EffectType::HEAL) {}

void HealingEffect::applyToCreature(Creature* c, EffectStrength strength) {
  if (c->getHealth() < 1 || (strength == EffectStrength::STRONG && c->lostLimbs()))
    c->heal(1, strength == EffectStrength::STRONG);
  else
    c->privateMessage("You feel refreshed.");
}

SleepEffect::SleepEffect() : Effect(EffectType::SLEEP) {}

void SleepEffect::applyToCreature(Creature* c, EffectStrength strength) {
  Square *square = c->getLevel()->getSquare(c->getPosition());
  c->you(MsgType::FALL_ASLEEP, square->getName());
  c->sleep(Random.getRandom(sleepTime[strength]));
}

GiveItemEffect::GiveItemEffect(ItemId i, int n) : Effect(EffectType::GIVE_ITEM), id(i), num(n) {}

void GiveItemEffect::applyToCreature(Creature* c, EffectStrength strength) {
  vector<PItem> item = ItemFactory::fromId(id, num);
  vector<Item*> ref(item.size());
  transform(item.begin(), item.end(), ref.begin(), [](PItem& it) { return it.get();});
  c->getLevel()->getSquare(c->getPosition())->dropItems(std::move(item));
  c->onItemsAppeared(ref);
}

IdentifyEffect::IdentifyEffect() : Effect(EffectType::IDENTIFY) {}

void IdentifyEffect::applyToCreature(Creature* c, EffectStrength strength) {
  c->grantIdentify(identifyNum[strength]);
}

PanicEffect::PanicEffect() : Effect(EffectType::PANIC) {}

void PanicEffect::applyToCreature(Creature* c, EffectStrength strength) {
  c->panic(panicTime[strength]);
}

RageEffect::RageEffect() : Effect(EffectType::RAGE) {}

void RageEffect::applyToCreature(Creature* c, EffectStrength strength) {
  c->rage(panicTime[strength]);
}

HalluEffect::HalluEffect() : Effect(EffectType::RAGE) {}

void HalluEffect::applyToCreature(Creature* c, EffectStrength strength) {
  c->hallucinate(halluTime[strength]);
}

StrBonusEffect::StrBonusEffect() : Effect(EffectType::STR_BONUS) {}

void StrBonusEffect::applyToCreature(Creature* c, EffectStrength strength) {
  c->giveStrBonus(attrBonusTime[strength]);
}

DexBonusEffect::DexBonusEffect() : Effect(EffectType::STR_BONUS) {}

void DexBonusEffect::applyToCreature(Creature* c, EffectStrength strength) {
  c->giveDexBonus(attrBonusTime[strength]);
}

SlowEffect::SlowEffect() : Effect(EffectType::SLOW) {}

void SlowEffect::applyToCreature(Creature* c, EffectStrength strength) {
  c->slowDown(panicTime[strength]);
}

SpeedEffect::SpeedEffect() : Effect(EffectType::SPEED) {}

void SpeedEffect::applyToCreature(Creature* c, EffectStrength strength) {
  c->speedUp(panicTime[strength]);
}

BlindnessEffect::BlindnessEffect() : Effect(EffectType::BLINDNESS) {}

void BlindnessEffect::applyToCreature(Creature* c, EffectStrength strength) {
  c->blind(blindTime[strength]);
}

InvisibleEffect::InvisibleEffect() : Effect(EffectType::INVISIBLE) {}

void InvisibleEffect::applyToCreature(Creature* c, EffectStrength strength) {
  c->makeInvisible(invisibleTime[strength]);
}

FireEffect::FireEffect() : Effect(EffectType::FIRE) {}

void FireEffect::applyToCreature(Creature* c, EffectStrength strength) {
  c->setOnFire(fireAmount[strength]);
}

PortalEffect::PortalEffect() : Effect(EffectType::PORTAL) {
}

void PortalEffect::applyToCreature(Creature* c, EffectStrength) {
  Level* l = c->getLevel();
  for (Vec2 v : c->getPosition().neighbors8(true))
    if (l->getSquare(v)->canEnter(c)) {
      l->globalMessage(v, "A magic portal appears.");
      l->getSquare(v)->addTrigger(Trigger::getPortal(
            ViewObject(ViewId::PORTAL, ViewLayer::LARGE_ITEM, "Portal"), l, v));
      return;
    }
}

TeleportEffect::TeleportEffect() : Effect(EffectType::TELEPORT) {
}

void TeleportEffect::applyToCreature(Creature* c, EffectStrength) {
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

RollingBoulderEffect::RollingBoulderEffect() : Effect(EffectType::ROLLING_BOULDER) {}

void RollingBoulderEffect::applyToCreature(Creature* c, EffectStrength strength) {
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

