#include "stdafx.h"
#include "body.h"
#include "attack.h"
#include "attack_type.h"
#include "attack_level.h"
#include "creature.h"
#include "lasting_effect.h"
#include "game.h"
#include "statistics.h"
#include "creature_attributes.h"
#include "item_factory.h"
#include "position.h"
#include "player_message.h"
#include "view_object.h"
#include "item_type.h"
#include "effect.h"
#include "item.h"

static double getDefaultWeight(Body::Size size) {
  switch (size) {
    case Body::Size::HUGE: return 1000;
    case Body::Size::LARGE: return 90;
    case Body::Size::MEDIUM: return 45;
    case Body::Size::SMALL: return 20;
  }
}

SERIALIZE_DEF(Body, xhumanoid, size, weight, bodyParts, injuredBodyParts, lostBodyParts, material, health, minionFood, deathSound, carryLimit, doesntEat);

SERIALIZATION_CONSTRUCTOR_IMPL(Body);

static double getDefaultCarryLimit(Body::Size size) {
  switch (size) {
    case Body::Size::HUGE: return 200;
    case Body::Size::LARGE: return 60;
    case Body::Size::MEDIUM: return 20;
    case Body::Size::SMALL: return 4;
  }
}

Body::Body(bool humanoid, Material m, Size s) : xhumanoid(humanoid), size(s),
    weight(getDefaultWeight(size)), material(m),
    deathSound(humanoid ? SoundId::HUMANOID_DEATH : SoundId::BEAST_DEATH), carryLimit(getDefaultCarryLimit(size)) {
  if (humanoid)
    setHumanoidBodyParts();
}

Body Body::humanoid(Material m, Size s) {
  return Body(true, m, s);
}

Body Body::humanoid(Size s) {
  return humanoid(Material::FLESH, s);
}

Body Body::nonHumanoid(Material m, Size s) {
  return Body(false, m, s);
}

Body Body::nonHumanoid(Size s) {
  return nonHumanoid(Material::FLESH, s);
}

Body Body::humanoidSpirit(Size s) {
  return Body(true, Material::SPIRIT, s);
}

Body Body::nonHumanoidSpirit(Size s) {
  return Body(false, Material::SPIRIT, s);
}

Body& Body::addWings() {
  bodyParts[BodyPart::WING] = 2;
  return *this;
}

Body& Body::setWeight(double w) {
  weight = w;
  return *this;
}

Body& Body::setBodyParts(const EnumMap<BodyPart, int>& p) {
  bodyParts = p;
  return *this;
}

Body& Body::setHumanoidBodyParts() {
  setBodyParts({{BodyPart::LEG, 2}, {BodyPart::ARM, 2}, {BodyPart::HEAD, 1}, {BodyPart::BACK, 1},
      {BodyPart::TORSO, 1}});
  return *this;
}

Body& Body::setHorseBodyParts() {
  setBodyParts({{BodyPart::LEG, 4}, {BodyPart::HEAD, 1}, {BodyPart::BACK, 1},
      {BodyPart::TORSO, 1}});
  return *this;
}

Body& Body::setBirdBodyParts() {
  setBodyParts({{BodyPart::LEG, 2}, {BodyPart::WING, 2}, {BodyPart::HEAD, 1}, {BodyPart::BACK, 1},
      {BodyPart::TORSO, 1}});
  return *this;
}

Body& Body::setMinionFood() {
  minionFood = true;
  return *this;
}

Body& Body::setDeathSound(optional<SoundId> s) {
  deathSound = s;
  return *this;
}

Body& Body::setNoCarryLimit() {
  carryLimit = none;
  return *this;
}

Body& Body::setDoesntEat() {
  doesntEat = true;
  return *this;
}

bool Body::canHeal() const {
  return health < 1;
}

bool Body::hasHealth() const {
  switch (material) {
    case Material::FLESH:
    case Material::SPIRIT:
    case Material::FIRE: return true;
    default: return false;
  }
}

double Body::getMinDamage(BodyPart part) const {
  double ret;
  switch (part) {
    case BodyPart::WING: ret = 0.3; break;
    case BodyPart::ARM: ret = 0.6; break;
    case BodyPart::LEG:
    case BodyPart::HEAD: ret = 0.8; break;
    case BodyPart::BACK:
    case BodyPart::TORSO: ret = 1.5; break;
  }
  if (material == Material::FLESH)
    return ret;
  else
  if (material == Material::SPIRIT)
    return 10000;
  else
    return ret / 2;
}

BodyPart Body::armOrWing() const {
  if (numGood(BodyPart::ARM) == 0)
    return BodyPart::WING;
  if (numGood(BodyPart::WING) == 0)
    return BodyPart::ARM;
  return Random.choose({ BodyPart::WING, BodyPart::ARM }, {1, 1});
}

bool Body::isCritical(BodyPart part) const {
  return contains({BodyPart::TORSO}, part)
    || (part == BodyPart::HEAD && numGood(part) == 0 && material == Material::FLESH);
}


int Body::numBodyParts(BodyPart part) const {
  return bodyParts[part];
}

int Body::numLost(BodyPart part) const {
  return lostBodyParts[part];
}

int Body::lostOrInjuredBodyParts() const {
  int ret = 0;
  for (BodyPart part : ENUM_ALL(BodyPart))
    ret += injuredBodyParts[part];
  for (BodyPart part : ENUM_ALL(BodyPart))
    ret += lostBodyParts[part];
  return ret;
}

int Body::numInjured(BodyPart part) const {
  return injuredBodyParts[part];
}

int Body::numGood(BodyPart part) const {
  return numBodyParts(part) - numInjured(part);
}

void Body::clearInjured(BodyPart part) {
  injuredBodyParts[part] = 0;
}

void Body::clearLost(BodyPart part) {
  bodyParts[part] += lostBodyParts[part];
  lostBodyParts[part] = 0;
}


BodyPart Body::getBodyPart(AttackLevel attack, bool flying, bool collapsed) const {
  if (flying)
    return Random.choose({BodyPart::TORSO, BodyPart::HEAD, BodyPart::LEG, BodyPart::WING, BodyPart::ARM},
        {1, 1, 1, 2, 1});
  switch (attack) {
    case AttackLevel::HIGH: 
       return BodyPart::HEAD;
    case AttackLevel::MIDDLE:
       if (size == Size::SMALL || size == Size::MEDIUM || collapsed)
         return BodyPart::HEAD;
       else
         return Random.choose({BodyPart::TORSO, armOrWing()}, {1, 1});
    case AttackLevel::LOW:
       if (size == Size::SMALL || collapsed)
         return Random.choose({BodyPart::TORSO, armOrWing(), BodyPart::HEAD, BodyPart::LEG}, {1, 1, 1, 1});
       if (size == Size::MEDIUM)
         return Random.choose({BodyPart::TORSO, armOrWing(), BodyPart::LEG}, {1, 1, 3});
       else
         return BodyPart::LEG;
  }
  return BodyPart::ARM;
}

void Body::healBodyParts(WCreature creature, bool regrow) {
  auto updateEffects = [&] (BodyPart part, int count) {
    switch (part) {
      case BodyPart::LEG:
        creature->removePermanentEffect(LastingEffect::COLLAPSED, count);
        break;
      case BodyPart::WING:
        creature->addPermanentEffect(LastingEffect::FLYING, count);
        break;
      case BodyPart::HEAD:
        if (numGood(BodyPart::HEAD) == 1)
          creature->removePermanentEffect(LastingEffect::BLIND, count);
        break;
      default:
        break;
    }
  };
  for (BodyPart part : ENUM_ALL(BodyPart))
    if (int count = numInjured(part)) {
      creature->you(MsgType::YOUR,
          string(getName(part)) + (count > 1 ? "s are" : " is") + " in better shape");
      clearInjured(part);
      updateEffects(part, count);
    }
  if (regrow)
    for (BodyPart part : ENUM_ALL(BodyPart))
      if (int count = numLost(part)) {
        creature->you(MsgType::YOUR, string(getName(part)) + (count > 1 ? "s grow back!" : " grows back!"));
        clearLost(part);
        updateEffects(part, count);
      }
}

void Body::injureBodyPart(WCreature creature, BodyPart part, bool drop) {
  if (bodyParts[part] == 0 || (!drop && injuredBodyParts[part] == bodyParts[part]))
    return;
  if (drop) {
    if (contains({BodyPart::LEG, BodyPart::ARM, BodyPart::WING}, part))
      creature->getGame()->getStatistics().add(StatId::CHOPPED_LIMB);
    else if (part == BodyPart::HEAD)
      creature->getGame()->getStatistics().add(StatId::CHOPPED_HEAD);
    if (PItem item = getBodyPartItem(creature->getAttributes().getName().bare(), part))
      creature->getPosition().dropItem(std::move(item));
    looseBodyPart(part);
  } else
    injureBodyPart(part);
  switch (part) {
    case BodyPart::HEAD:
      if (numGood(BodyPart::HEAD) == 0)
        creature->addPermanentEffect(LastingEffect::BLIND);
      break;
    case BodyPart::LEG:
      creature->addPermanentEffect(LastingEffect::COLLAPSED);
      break;
    case BodyPart::ARM:
      if (creature->getWeapon())
        creature->dropWeapon();
      break;
    case BodyPart::WING:
      creature->removePermanentEffect(LastingEffect::FLYING);
      break;
    default: break;
  }
}

template <typename T>
void consumeAttr(T& mine, const T& his, vector<string>& adjectives, const string& adj) {
  if (mine < his) {
    mine = his;
    if (!adj.empty())
      adjectives.push_back(adj);
  }
}


void Body::consumeBodyParts(WCreature c, const Body& other, vector<string>& adjectives) {
  for (BodyPart part : ENUM_ALL(BodyPart))
    if (other.bodyParts[part] > bodyParts[part]) {
      if (bodyParts[part] + 1 == other.bodyParts[part])
        c->you(MsgType::GROW, string("a ") + getName(part));
      else
        c->you(MsgType::GROW, toString(other.bodyParts[part] - bodyParts[part]) + " " + getName(part) + "s");
      bodyParts[part] = other.bodyParts[part];
    }
  if (other.isHumanoid() && !isHumanoid() && numBodyParts(BodyPart::ARM) >= 2 &&
      numBodyParts(BodyPart::LEG) >= 2 && numBodyParts(BodyPart::HEAD) >= 1) {
    c->you(MsgType::BECOME, "a humanoid");
    c->addPersonalEvent(c->getName().the() + " turns into a humanoid");
    xhumanoid = true;
  }
  consumeAttr(size, other.size, adjectives, "larger");
}


void Body::looseBodyPart(BodyPart part) {
  if (bodyParts[part] > 0) {
    --bodyParts[part];
    ++lostBodyParts[part];
    if (injuredBodyParts[part] > bodyParts[part])
      --injuredBodyParts[part];
  }
}

void Body::injureBodyPart(BodyPart part) {
  if (injuredBodyParts[part] < bodyParts[part])
    ++injuredBodyParts[part];
}


const char* getName(BodyPart part) {
  switch (part) {
    case BodyPart::LEG: return "leg";
    case BodyPart::ARM: return "arm";
    case BodyPart::WING: return "wing";
    case BodyPart::HEAD: return "head";
    case BodyPart::TORSO: return "torso";
    case BodyPart::BACK: return "back";
  }
}

string sizeStr(Body::Size s) {
  switch (s) {
    case Body::Size::SMALL: return "small";
    case Body::Size::MEDIUM: return "medium";
    case Body::Size::LARGE: return "large";
    case Body::Size::HUGE: return "huge";
  }
}

static string getMaterialName(Body::Material material) {
  switch (material) {
    case Body::Material::FLESH: return "flesh";
    case Body::Material::BONE: return "bone";
    case Body::Material::WATER: return "water";
    case Body::Material::UNDEAD_FLESH: return "rotting flesh";
    case Body::Material::LAVA: return "lava";
    case Body::Material::ROCK: return "rock";
    case Body::Material::FIRE: return "fire";
    case Body::Material::WOOD: return "wood";
    case Body::Material::SPIRIT: return "ectoplasm";
    case Body::Material::CLAY: return "clay";
    case Body::Material::IRON: return "iron";
  }
}

string Body::getMaterialAndSizeAdjectives() const {
  vector<string> ret {sizeStr(size)};
  switch (material) {
    case Material::FLESH: break;
    case Material::UNDEAD_FLESH: ret.push_back("undead"); break;
    default: ret.push_back("made of " + getMaterialName(material));
  }
  return combine(ret);
}

string Body::getDescription() const {
  vector<string> ret;
  bool anyLimbs = false;
  vector<BodyPart> listParts = {BodyPart::ARM, BodyPart::LEG, BodyPart::WING};
  if (xhumanoid)
    listParts = {BodyPart::WING};
  for (BodyPart part : listParts)
    if (int num = numBodyParts(part)) {
      ret.push_back(getPluralText(getName(part), num));
      anyLimbs = true;
    }
  if (xhumanoid) {
    bool noArms = numBodyParts(BodyPart::ARM) == 0;
    bool noLegs = numBodyParts(BodyPart::LEG) == 0;
    if (noArms && noLegs)
      ret.push_back("no limbs");
    else if (noArms)
      ret.push_back("no arms");
    else if (noLegs)
      ret.push_back("no legs");
  } else
  if (!anyLimbs)
    ret.push_back("no limbs");
  if (numBodyParts(BodyPart::HEAD) == 0)
    ret.push_back("no head");
  string limbDescription = ret.size() > 0 ? " with " + combine(ret) : "";
  return getMaterialAndSizeAdjectives() + limbDescription;
}

bool Body::isHumanoid() const {
  return xhumanoid;
}

static string getBodyPartBone(BodyPart part) {
  switch (part) {
    case BodyPart::HEAD: return "skull";
    default: return "bone";
  }
}

static int numCorpseItems(Body::Size size) {
  switch (size) {
    case Body::Size::LARGE: return Random.get(30, 50);
    case Body::Size::HUGE: return Random.get(80, 120);
    case Body::Size::MEDIUM: return Random.get(15, 30);
    case Body::Size::SMALL: return Random.get(1, 8);
  }
}

PItem Body::getBodyPartItem(const string& name, BodyPart part) {
  switch (material) {
    case Material::FLESH:
    case Material::UNDEAD_FLESH:
      return ItemFactory::corpse(name + " " + getName(part), name + " " + getBodyPartBone(part),
        weight / 8, isMinionFood() ? ItemClass::FOOD : ItemClass::CORPSE);
    case Material::CLAY:
    case Material::ROCK:
      return ItemFactory::fromId(ItemId::ROCK);
    case Material::BONE:
      return ItemFactory::fromId(ItemId::BONE);
    case Material::IRON:
      return ItemFactory::fromId(ItemId::IRON_ORE);
    case Material::WOOD:
      return ItemFactory::fromId(ItemId::WOOD_PLANK);
    default: return nullptr;
  }
}

vector<PItem> Body::getCorpseItem(const string& name, Creature::Id id) {
  switch (material) {
    case Material::FLESH:
    case Material::UNDEAD_FLESH:
      return makeVec(
          ItemFactory::corpse(name + " corpse", name + " skeleton", weight,
            minionFood ? ItemClass::FOOD : ItemClass::CORPSE,
            {id, true, numBodyParts(BodyPart::HEAD) > 0, false}));
    case Material::CLAY:
    case Material::ROCK:
      return ItemFactory::fromId(ItemId::ROCK, numCorpseItems(size));
    case Material::BONE:
      return ItemFactory::fromId(ItemId::BONE, numCorpseItems(size));
    case Material::IRON:
      return ItemFactory::fromId(ItemId::IRON_ORE, numCorpseItems(size));
    case Material::WOOD:
      return ItemFactory::fromId(ItemId::WOOD_PLANK, numCorpseItems(size));
    default: return {};
  }
}

void Body::affectPosition(Position position) {
  if (material == Material::FIRE && Random.roll(5))
    position.fireDamage(1);
}

bool Body::takeDamage(const Attack& attack, WCreature creature, double damage) {
  bleed(creature, damage);
  BodyPart part = getBodyPart(attack.level, creature->isAffected(LastingEffect::FLYING),
      creature->isAffected(LastingEffect::COLLAPSED));
  if (damage >= getMinDamage(part) && numGood(part) > 0) {
    creature->youHit(part, attack.type);
    injureBodyPart(creature, part, contains({AttackType::CUT, AttackType::BITE}, attack.type));
    if (isCritical(part)) {
      creature->you(MsgType::DIE, "");
      creature->dieWithAttacker(attack.attacker);
      return true;
    }
    if (health <= 0)
      health = 0.1;
    return false;
  }
  if (health <= 0) {
    creature->you(MsgType::ARE, "critically wounded");
    creature->you(MsgType::DIE, "");
    creature->dieWithAttacker(attack.attacker);
    return true;
  } else
  if (health < 0.5)
    creature->you(MsgType::ARE, "critically wounded");
  else {
    if (hasHealth())
      creature->you(MsgType::ARE, "wounded");
    else if (!attack.effect)
      creature->you(MsgType::ARE, "not hurt");
  }
  return false;
}

void Body::getBadAdjectives(vector<AdjectiveInfo>& ret) const {
  if (health < 1)
    ret.push_back({"Wounded", ""});
  for (BodyPart part : ENUM_ALL(BodyPart))
    if (int num = numInjured(part))
      ret.push_back({getPlural(string("Injured ") + getName(part), num), ""});
  for (BodyPart part : ENUM_ALL(BodyPart))
    if (int num = numLost(part))
      ret.push_back({getPlural(string("Lost ") + getName(part), num), ""});
}

const static map<BodyPart, int> defensePenalty {
  {BodyPart::ARM, 2},
  {BodyPart::LEG, 10},
  {BodyPart::WING, 3},
  {BodyPart::HEAD, 3}};

const static map<BodyPart, int> damagePenalty {
  {BodyPart::ARM, 2},
  {BodyPart::LEG, 5},
  {BodyPart::WING, 2},
  {BodyPart::HEAD, 3}};

double Body::modifyAttr(AttrType type, double def) const {
  switch (type) {
    case AttrType::DAMAGE:
        if (health < 1)
          def *= 0.666 + health / 3;
        for (auto elem : damagePenalty)
          def -= elem.second * (numInjured(elem.first) + numLost(elem.first));
        break;
    case AttrType::DEFENSE:
        if (health < 1)
          def *= 0.666 + health / 3;
        for (auto elem : defensePenalty)
          def -= elem.second * (numInjured(elem.first) + numLost(elem.first));
        break;
    default: break;
  }
  return def;
}

bool Body::tick(WConstCreature c) {
  if (fallsApartFromDamage() && lostOrInjuredBodyParts() >= 4) {
    c->you(MsgType::FALL, "apart");
    return true;
  }
  if (health <= 0) {
    c->you(MsgType::DIE_OF, c->isAffected(LastingEffect::POISON) ? "poisoning" : "bleeding");
    return true;
  }
  if (c->getPosition().sunlightBurns() && isUndead()) {
    c->you(MsgType::ARE, "burnt by the sun");
    if (Random.roll(10)) {
      c->you(MsgType::YOUR, "body crumbles to dust");
      return true;
    }
  }
  return false;
}

void Body::updateViewObject(ViewObject& obj) const {
  obj.setAttribute(ViewObject::Attribute::WOUNDED, 1 - health);
  switch (material) {
    case Material::SPIRIT:
    case Material::FIRE:
      obj.setModifier(ViewObject::Modifier::SPIRIT_DAMAGE);
      break;
    default:
      break;
  }
}

bool Body::heal(WCreature c, double amount, bool replaceLimbs) {
  INFO << c->getName().the() << " heal";
  if (replaceLimbs)
    healBodyParts(c, true);
  if (health < 1) {
    health = min(1., health + amount);
    if (health >= 0.5)
      healBodyParts(c, replaceLimbs);
    if (health == 1) {
      c->you(MsgType::BLEEDING_STOPS, "");
      health = 1;
      c->updateViewObject();
      return true;
    }
  }
  return false;
}

bool Body::isIntrinsicallyAffected(LastingEffect effect) const {
  switch (effect) {
    case LastingEffect::FIRE_RESISTANT:
      switch (material) {
        case Material::FLESH:
        case Material::SPIRIT:
        case Material::UNDEAD_FLESH:
        case Material::WOOD: return false;
        default: return true;
      }
    case LastingEffect::POISON_RESISTANT:
      return material != Material::FLESH;
    case LastingEffect::FLYING:
      return numGood(BodyPart::WING) >= 2;
    default:
      return false;
  }
}

void Body::fireDamage(WCreature c, double amount) {
  c->you(MsgType::ARE, "burnt by the fire");
  bleed(c, 6. * amount / double(1 + c->getAttr(AttrType::DEFENSE)));
}

bool Body::affectByPoison(WCreature c, double amount) {
  if (material == Material::FLESH) {
    bleed(c, amount);
    c->playerMessage("You feel poison flowing in your veins.");
    if (health <= 0) {
      c->you(MsgType::DIE_OF, "poisoning");
      return true;
    }
  }
  return false;
}

bool Body::affectByPoisonGas(WCreature c, double amount) {
  if (!c->isAffected(LastingEffect::POISON_RESISTANT) && material == Material::FLESH) {
    bleed(c, amount / 20);
    c->you(MsgType::ARE, "poisoned by the gas");
    if (health <= 0) {
      c->you(MsgType::DIE_OF, "poisoning");
      return true;
    }
  }
  return false;
}

void Body::affectByTorture(WCreature c) {
  bleed(c, 0.1);
  if (health < 0.3) {
    if (!Random.roll(8))
      c->heal();
    else
      c->dieWithReason("killed by torture");
  }
}

bool Body::affectBySilver(WCreature c) {
  if (isUndead()) {
    c->you(MsgType::ARE, "hurt by the silver");
    bleed(c, Random.getDouble(0.0, 0.15));
  }
  return health <= 0;
}

bool Body::affectByAcid(WCreature c) {
  switch (material) {
    case Material::FIRE:
    case Material::SPIRIT:
      return false;
    default: break;
  }
  c->you(MsgType::ARE, "hurt by the acid");
  bleed(c, 0.2);
  return health <= 0;
}

void Body::bleed(WCreature c, double amount) {
  if (hasHealth()) {
    health -= amount;
    c->updateViewObject();
  }
}

bool Body::canWade() const {
  switch (size) {
    case Size::LARGE:
    case Size::HUGE: return true;
    default: return false;
  }
}

bool Body::isKilledByBoulder() const {
  switch (material) {
    case Material::FIRE:
    case Material::SPIRIT:
      return false;
    default: return true;
  }
}

bool Body::isMinionFood() const {
  return minionFood;
}

bool Body::canCopulateWith() const {
  switch (material) {
    case Material::FLESH:
    case Material::UNDEAD_FLESH: return isHumanoid();
    default: return false;
  }
}

bool Body::canConsume() const {
  switch (material) {
    case Material::WATER:
    case Material::FIRE:
    case Material::SPIRIT: return false;
    default: return true;
  }
}

bool Body::isSunlightVulnerable() const {
  return material == Material::UNDEAD_FLESH;
}

bool Body::isWounded() const {
  return health < 1;
}

bool Body::isSeriouslyWounded() const {
  return health < 0.5;
}

bool Body::canEntangle() const {
  switch (material) {
    case Material::WATER:
    case Material::FIRE:
    case Material::SPIRIT: return false;
    default: return true;
  }
}

double Body::getHealth() const {
  return health;
}

bool Body::hasBrain() const {
  return material == Material::FLESH;
}

bool Body::needsToEat() const {
  switch (material) {
    case Material::FLESH:
    case Material::BONE:
    case Material::UNDEAD_FLESH:
      return !doesntEat;
    default:
      return false;
  }
}

bool Body::fallsApartFromDamage() const {
  switch (material) {
    case Material::FLESH:
    case Material::FIRE:
    case Material::SPIRIT:
    case Material::WATER: return false;
    default: return true;

  }
}

bool Body::isUndead() const {
  return material == Material::UNDEAD_FLESH || material == Material::BONE;
}

vector<AttackLevel> Body::getAttackLevels() const {
  if (isHumanoid() && !numGood(BodyPart::ARM))
    return {AttackLevel::LOW};
  switch (size) {
    case Body::Size::SMALL: return {AttackLevel::LOW};
    case Body::Size::MEDIUM: return {AttackLevel::LOW, AttackLevel::MIDDLE};
    case Body::Size::LARGE: return {AttackLevel::LOW, AttackLevel::MIDDLE, AttackLevel::HIGH};
    case Body::Size::HUGE: return {AttackLevel::MIDDLE, AttackLevel::HIGH};
  }
}

static double getDeathSoundPitch(Body::Size size) {
  switch (size) {
    case Body::Size::HUGE: return 0.6;
    case Body::Size::LARGE: return 0.9;
    case Body::Size::MEDIUM: return 1.5;
    case Body::Size::SMALL: return 3.3;
  }
}

optional<Sound> Body::getDeathSound() const {
  if (!deathSound)
    return none;
  else
    return Sound(*deathSound).setPitch(getDeathSoundPitch(size));
}

double Body::getBoulderDamage() const {
  switch (size) {
    case Body::Size::HUGE: return 1;
    case Body::Size::LARGE: return 0.3;
    case Body::Size::MEDIUM: return 0.15;
    case Body::Size::SMALL: return 0;
  }
}

const optional<double>& Body::getCarryLimit() const {
  return carryLimit;
}
