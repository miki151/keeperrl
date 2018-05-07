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
#include "game_time.h"

static double getDefaultWeight(Body::Size size) {
  switch (size) {
    case Body::Size::HUGE: return 1000;
    case Body::Size::LARGE: return 90;
    case Body::Size::MEDIUM: return 45;
    case Body::Size::SMALL: return 20;
  }
}

SERIALIZE_DEF(Body, xhumanoid, size, weight, bodyParts, injuredBodyParts, lostBodyParts, material, health, minionFood, deathSound, carryLimit, intrinsicAttacks, canAlwaysPush)

SERIALIZATION_CONSTRUCTOR_IMPL(Body)

static double getDefaultCarryLimit(Body::Size size) {
  switch (size) {
    case Body::Size::HUGE: return 200;
    case Body::Size::LARGE: return 80;
    case Body::Size::MEDIUM: return 60;
    case Body::Size::SMALL: return 6;
  }
}

static int getDefaultIntrinsicDamage(Body::Size size) {
  switch (size) {
    case Body::Size::HUGE: return 8;
    case Body::Size::LARGE: return 5;
    case Body::Size::MEDIUM: return 3;
    case Body::Size::SMALL: return 1;
  }
}

Body::Body(bool humanoid, Material m, Size s) : xhumanoid(humanoid), size(s),
    weight(getDefaultWeight(size)), material(m),
    deathSound(humanoid ? SoundId::HUMANOID_DEATH : SoundId::BEAST_DEATH), carryLimit(getDefaultCarryLimit(size)) {
  if (humanoid)
    setHumanoidBodyParts(getDefaultIntrinsicDamage(size));
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

void Body::addWings() {
  bodyParts[BodyPart::WING] = 2;
}

void Body::setWeight(double w) {
  weight = w;
}

void Body::setBodyParts(const EnumMap<BodyPart, int>& p) {
  bodyParts = p;
  bodyParts[BodyPart::TORSO] = 1;
  bodyParts[BodyPart::BACK] = 1;
}

void Body::setIntrinsicAttack(BodyPart part, IntrinsicAttack attack) {
  if (!numGood(part)) {
    part = BodyPart::TORSO;
    CHECK(numGood(part));
  }
  intrinsicAttacks[part] = std::move(attack);
}

void Body::setCanAlwaysPush() {
  canAlwaysPush = true;
}

WItem Body::chooseWeapon(WItem weapon) const {
  // choose one of the available weapons with equal probability
  bool hasRealWeapon = !!weapon;
  double numOptions = !!weapon ? 1 : 0;
  for (auto part : ENUM_ALL(BodyPart)) {
    auto& attack = intrinsicAttacks[part];
    if (numGood(part) > 0 && attack &&
        (attack->active == attack->ALWAYS || (attack->active == attack->NO_WEAPON && !hasRealWeapon))) {
      ++numOptions;
      if (!weapon || Random.chance(1.0 / numOptions))
        weapon = intrinsicAttacks[part]->item.get();
    }
  }
  return weapon;
}

EnumMap<BodyPart, optional<IntrinsicAttack>>& Body::getIntrinsicAttacks() {
  return intrinsicAttacks;
}

const EnumMap<BodyPart, optional<IntrinsicAttack>>& Body::getIntrinsicAttacks() const {
  return intrinsicAttacks;
}

void Body::setHumanoidBodyParts(int intrinsicDamage) {
  setBodyParts({{BodyPart::LEG, 2}, {BodyPart::ARM, 2}, {BodyPart::HEAD, 1}, {BodyPart::BACK, 1},
      {BodyPart::TORSO, 1}});
  setIntrinsicAttack(BodyPart::ARM, IntrinsicAttack(ItemType::fists(intrinsicDamage), IntrinsicAttack::NO_WEAPON));
  setIntrinsicAttack(BodyPart::LEG, IntrinsicAttack(ItemType::legs(intrinsicDamage), IntrinsicAttack::NO_WEAPON));
}

void Body::setHorseBodyParts(int intrinsicDamage) {
  setBodyParts({{BodyPart::LEG, 4}, {BodyPart::HEAD, 1}, {BodyPart::BACK, 1},
      {BodyPart::TORSO, 1}});
  setIntrinsicAttack(BodyPart::HEAD, IntrinsicAttack(ItemType::fangs(intrinsicDamage), IntrinsicAttack::NO_WEAPON));
  setIntrinsicAttack(BodyPart::LEG, IntrinsicAttack(ItemType::legs(intrinsicDamage), IntrinsicAttack::NO_WEAPON));
}

void Body::setBirdBodyParts(int intrinsicDamage) {
  setBodyParts({{BodyPart::LEG, 2}, {BodyPart::WING, 2}, {BodyPart::HEAD, 1}, {BodyPart::BACK, 1},
      {BodyPart::TORSO, 1}});
  setIntrinsicAttack(BodyPart::HEAD, IntrinsicAttack(ItemType::beak(intrinsicDamage), IntrinsicAttack::NO_WEAPON));
}

void Body::setMinionFood() {
  minionFood = true;
}

void Body::setDeathSound(optional<SoundId> s) {
  deathSound = s;
}

void Body::setNoCarryLimit() {
  carryLimit = none;
}

bool Body::canHeal() const {
  return health < 1;
}

bool Body::hasHealth() const {
  switch (material) {
    case Material::FLESH:
    case Material::SPIRIT:
    case Material::FIRE:
      return true;
    default: return false;
  }
}

bool Body::isPartDamaged(BodyPart part, double damage) const {
  double strength = [&] {
    switch (part) {
      case BodyPart::WING: return 0.3;
      case BodyPart::ARM: return 0.6;
      case BodyPart::LEG:
      case BodyPart::HEAD: return 0.8;
      case BodyPart::BACK:
      case BodyPart::TORSO: return 1.5;
    }
  }();
  if (material == Material::FLESH)
    return damage >= strength;
  if (material == Material::SPIRIT)
    return false;
  else
    return Random.chance(damage / strength);
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

optional<BodyPart> Body::getAnyGoodBodyPart() const {
  vector<BodyPart> good;
  for (auto part : ENUM_ALL(BodyPart))
    if (numGood(part) > 0)
      good.push_back(part);
  return Random.choose(good);
}

optional<BodyPart> Body::getBodyPart(AttackLevel attack, bool flying, bool collapsed) const {
  auto best = [&] {
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
  }();
  if (numGood(best) > 0)
    return best;
  else
    return getAnyGoodBodyPart();
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
      creature->dropWeapon();
      break;
    case BodyPart::WING:
      creature->removePermanentEffect(LastingEffect::FLYING);
      break;
    default: break;
  }
}

template <typename T>
void consumeBodyAttr(T& mine, const T& his, vector<string>& adjectives, const string& adj) {
  if (mine < his) {
    mine = his;
    if (!adj.empty())
      adjectives.push_back(adj);
  }
}

void Body::consumeBodyParts(WCreature c, Body& other, vector<string>& adjectives) {
  for (BodyPart part : ENUM_ALL(BodyPart)) {
    int cnt = other.bodyParts[part] - bodyParts[part];
    if (cnt > 0) {
      string what = getPlural(getName(part), cnt);
      c->verb("grow", "grows", what);
      c->addPersonalEvent(c->getName().the() + " grows "_s + what);
      bodyParts[part] = other.bodyParts[part];
    }
    if (auto& attack = other.intrinsicAttacks[part]) {
      c->verb("develop", "develops",  "a " + attack->item->getNameAndModifiers() + " attack");
      c->addPersonalEvent(c->getName().the() + " develops a " + attack->item->getNameAndModifiers() + " attack");
      intrinsicAttacks[part] = std::move(*attack);
    }
  }
  if (other.isHumanoid() && !isHumanoid() && numBodyParts(BodyPart::ARM) >= 2 &&
      numBodyParts(BodyPart::LEG) >= 2 && numBodyParts(BodyPart::HEAD) >= 1) {
    c->you(MsgType::BECOME, "a humanoid");
    c->addPersonalEvent(c->getName().the() + " turns into a humanoid");
    xhumanoid = true;
  }
  consumeBodyAttr(size, other.size, adjectives, "larger");
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
    case Body::Material::ADA: return "adamantium";
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
        weight / 8, false, isMinionFood() ? ItemClass::FOOD : ItemClass::CORPSE);
    case Material::CLAY:
    case Material::ROCK:
      return ItemType(ItemType::Rock{}).get();
    case Material::BONE:
      return ItemType(ItemType::Bone{}).get();
    case Material::IRON:
      return ItemType(ItemType::IronOre{}).get();
    case Material::WOOD:
      return ItemType(ItemType::WoodPlank{}).get();
    case Material::ADA:
      return ItemType(ItemType::AdaOre{}).get();
    default: return nullptr;
  }
}

vector<PItem> Body::getCorpseItems(const string& name, Creature::Id id, bool instantlyRotten) const {
  switch (material) {
    case Material::FLESH:
    case Material::UNDEAD_FLESH:
      return makeVec(
          ItemFactory::corpse(name + " corpse", name + " skeleton", weight, instantlyRotten,
            minionFood ? ItemClass::FOOD : ItemClass::CORPSE,
            {id, true, numBodyParts(BodyPart::HEAD) > 0, false}));
    case Material::CLAY:
    case Material::ROCK:
      return ItemType(ItemType::Rock{}).get(numCorpseItems(size));
    case Material::BONE:
      return ItemType(ItemType::Bone{}).get(numCorpseItems(size));
    case Material::IRON:
      return ItemType(ItemType::IronOre{}).get(numCorpseItems(size));
    case Material::WOOD:
      return ItemType(ItemType::WoodPlank{}).get(numCorpseItems(size));
    case Material::ADA:
      return ItemType(ItemType::AdaOre{}).get(numCorpseItems(size));
    default: return {};
  }
}

void Body::affectPosition(Position position) {
  if (material == Material::FIRE && Random.roll(5))
    position.fireDamage(1);
}

static void youHit(WConstCreature c, BodyPart part, AttackType type) {
  switch (part) {
    case BodyPart::BACK:
        switch (type) {
          case AttackType::SHOOT: c->you(MsgType::ARE, "shot in the back!"); break;
          case AttackType::BITE: c->you(MsgType::ARE, "bitten in the neck!"); break;
          case AttackType::CUT: c->you(MsgType::YOUR, "throat is cut!"); break;
          case AttackType::CRUSH: c->you(MsgType::YOUR, "spine is crushed!"); break;
          case AttackType::HIT: c->you(MsgType::YOUR, "neck is broken!"); break;
          case AttackType::STAB: c->you(MsgType::ARE, "stabbed in the "_s +
                                     Random.choose("back"_s, "neck"_s)); break;
          case AttackType::SPELL: c->you(MsgType::ARE, "ripped to pieces!"); break;
        }
        break;
    case BodyPart::HEAD:
        switch (type) {
          case AttackType::SHOOT: c->you(MsgType::ARE, "shot in the " +
                                      Random.choose("eye"_s, "neck"_s, "forehead"_s) + "!"); break;
          case AttackType::BITE: c->you(MsgType::YOUR, "head is bitten off!"); break;
          case AttackType::CUT: c->you(MsgType::YOUR, "head is chopped off!"); break;
          case AttackType::CRUSH: c->you(MsgType::YOUR, "skull is shattered!"); break;
          case AttackType::HIT: c->you(MsgType::YOUR, "neck is broken!"); break;
          case AttackType::STAB: c->you(MsgType::ARE, "stabbed in the eye!"); break;
          case AttackType::SPELL: c->you(MsgType::YOUR, "head is ripped to pieces!"); break;
        }
        break;
    case BodyPart::TORSO:
        switch (type) {
          case AttackType::SHOOT: c->you(MsgType::YOUR, "shot in the heart!"); break;
          case AttackType::BITE: c->you(MsgType::YOUR, "internal organs are ripped out!"); break;
          case AttackType::CUT: c->you(MsgType::ARE, "cut in half!"); break;
          case AttackType::STAB: c->you(MsgType::ARE, "stabbed in the " +
                                     Random.choose("stomach"_s, "heart"_s) + "!"); break;
          case AttackType::CRUSH: c->you(MsgType::YOUR, "ribs and internal organs are crushed!"); break;
          case AttackType::HIT: c->you(MsgType::YOUR, "stomach receives a deadly blow!"); break;
          case AttackType::SPELL: c->you(MsgType::ARE, "ripped to pieces!"); break;
        }
        break;
    case BodyPart::ARM:
        switch (type) {
          case AttackType::SHOOT: c->you(MsgType::YOUR, "shot in the arm!"); break;
          case AttackType::BITE: c->you(MsgType::YOUR, "arm is bitten off!"); break;
          case AttackType::CUT: c->you(MsgType::YOUR, "arm is chopped off!"); break;
          case AttackType::STAB: c->you(MsgType::ARE, "stabbed in the arm!"); break;
          case AttackType::CRUSH: c->you(MsgType::YOUR, "arm is smashed!"); break;
          case AttackType::HIT: c->you(MsgType::YOUR, "arm is broken!"); break;
          case AttackType::SPELL: c->you(MsgType::YOUR, "arm is ripped to pieces!"); break;
        }
        break;
    case BodyPart::WING:
        switch (type) {
          case AttackType::SHOOT: c->you(MsgType::YOUR, "shot in the wing!"); break;
          case AttackType::BITE: c->you(MsgType::YOUR, "wing is bitten off!"); break;
          case AttackType::CUT: c->you(MsgType::YOUR, "wing is chopped off!"); break;
          case AttackType::STAB: c->you(MsgType::ARE, "stabbed in the wing!"); break;
          case AttackType::CRUSH: c->you(MsgType::YOUR, "wing is smashed!"); break;
          case AttackType::HIT: c->you(MsgType::YOUR, "wing is broken!"); break;
          case AttackType::SPELL: c->you(MsgType::YOUR, "wing is ripped to pieces!"); break;
        }
        break;
    case BodyPart::LEG:
        switch (type) {
          case AttackType::SHOOT: c->you(MsgType::YOUR, "shot in the leg!"); break;
          case AttackType::BITE: c->you(MsgType::YOUR, "leg is bitten off!"); break;
          case AttackType::CUT: c->you(MsgType::YOUR, "leg is cut off!"); break;
          case AttackType::STAB: c->you(MsgType::YOUR, "stabbed in the leg!"); break;
          case AttackType::CRUSH: c->you(MsgType::YOUR, "knee is crushed!"); break;
          case AttackType::HIT: c->you(MsgType::YOUR, "leg is broken!"); break;
          case AttackType::SPELL: c->you(MsgType::YOUR, "leg is ripped to pieces!"); break;
        }
        break;
  }
}

Body::DamageResult Body::takeDamage(const Attack& attack, WCreature creature, double damage) {
  PROFILE;
  bleed(creature, damage);
  if (auto part = getBodyPart(attack.level, creature->isAffected(LastingEffect::FLYING),
      creature->isAffected(LastingEffect::COLLAPSED)))
    if (isPartDamaged(*part, damage)) {
      youHit(creature, *part, attack.type);
      injureBodyPart(creature, *part, contains({AttackType::CUT, AttackType::BITE}, attack.type));
      if (isCritical(*part)) {
        creature->you(MsgType::DIE, "");
        creature->dieWithAttacker(attack.attacker);
        return Body::KILLED;
      }
      creature->addEffect(LastingEffect::BLEEDING, 50_visible);
      if (health <= 0)
        health = 0.1;
      creature->updateViewObject();
      return Body::HURT;
    }
  if (health <= 0) {
    creature->you(MsgType::ARE, "critically wounded");
    creature->you(MsgType::DIE, "");
    creature->dieWithAttacker(attack.attacker);
    return Body::KILLED;
  } else
  if (health < 0.5) {
    creature->you(MsgType::ARE, "critically wounded");
    return Body::HURT;
  } else {
    if (hasHealth())
      creature->you(MsgType::ARE, "wounded");
    else if (!attack.effect) {
      creature->you(MsgType::ARE, "not hurt");
      return Body::NOT_HURT;
    }
    return Body::HURT;
  }
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

int Body::getAttrBonus(AttrType type) const {
  int ret = 0;
  switch (type) {
    case AttrType::DAMAGE:
      for (auto elem : damagePenalty)
        ret -= elem.second * (numInjured(elem.first) + numLost(elem.first));
      break;
    case AttrType::DEFENSE:
      for (auto elem : defensePenalty)
        ret -= elem.second * (numInjured(elem.first) + numLost(elem.first));
      break;
    default: break;
  }
  return ret;
}

bool Body::tick(WConstCreature c) {
  if (fallsApartFromDamage() && lostOrInjuredBodyParts() >= 4) {
    c->you(MsgType::FALL, "apart");
    return true;
  }
  return false;
}

double Body::getBodyPartHealth() const {
  int gone = 0;
  int total = 0;
  for (auto part : ENUM_ALL(BodyPart)) {
    gone += injuredBodyParts[part] + lostBodyParts[part];
    total += bodyParts[part] + lostBodyParts[part];
  }
  return 1 - double(gone) / double(total);
}

void Body::updateViewObject(ViewObject& obj) const {
  if (hasHealth())
    obj.setAttribute(ViewObject::Attribute::HEALTH, health);
  else
    obj.setAttribute(ViewObject::Attribute::HEALTH, getBodyPartHealth());
  switch (material) {
    case Material::SPIRIT:
    case Material::FIRE:
      obj.setModifier(ViewObject::Modifier::SPIRIT_DAMAGE);
      break;
    default:
      break;
  }
}

bool Body::heal(WCreature c, double amount) {
  INFO << c->getName().the() << " heal";
  if (health < 1) {
    health = min(1., health + amount);
    if (health >= 1) {
      c->you(MsgType::ARE, "fully healed");
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
    case LastingEffect::SUNLIGHT_VULNERABLE:
      return material == Material::UNDEAD_FLESH;
    case LastingEffect::POISON_RESISTANT:
      return material != Material::FLESH;
    case LastingEffect::SLEEP_RESISTANT:
      switch (material) {
        case Material::WATER:
        case Material::FIRE:
        case Material::SPIRIT:
        case Material::CLAY:
        case Material::ROCK:
        case Material::IRON:
        case Material::LAVA:
          return true;
        default:
          return false;
      }
    case LastingEffect::FLYING:
      return numGood(BodyPart::WING) >= 2;
    default:
      return false;
  }
}

bool Body::isImmuneTo(LastingEffect effect) const {
  switch (effect) {
    case LastingEffect::BLEEDING:
      return material != Material::FLESH;
    case LastingEffect::TIED_UP:
    case LastingEffect::ENTANGLED:
      switch (material) {
        case Material::WATER:
        case Material::FIRE:
        case Material::SPIRIT:
          return true;
        default:
          break;
      }
      break;
    default:
      break;
  }
  return false;
}

bool Body::affectByPoisonGas(WCreature c, double amount) {
  PROFILE;
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

bool Body::affectByFire(WCreature c, double amount) {
  c->you(MsgType::ARE, "burnt by the fire");
  bleed(c, 6. * amount / double(1 + c->getAttr(AttrType::DEFENSE)));
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
    /*case Material::WATER:
    case Material::FIRE:
    case Material::SPIRIT: return false;*/
    default: return true;
  }
}

bool Body::isWounded() const {
  return health < 1;
}

bool Body::isSeriouslyWounded() const {
  return health < 0.5;
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
      return true;
    default:
      return false;
  }
}

bool Body::needsToSleep() const {
  switch (material) {
    case Material::FLESH:
    case Material::BONE:
    case Material::UNDEAD_FLESH:
      return true;
    default:
      return false;
  }
}

bool Body::canPush(const Body& other) {
  return int(size) > int(other.size) || other.canAlwaysPush;
}

bool Body::canPerformRituals() const {
  switch (material) {
    case Material::FIRE:
    case Material::SPIRIT:
      return true;
    default:
      return false;
  }
}

bool Body::canBeCaptured() const {
  return xhumanoid && !isImmuneTo(LastingEffect::TIED_UP);
}

bool Body::fallsApartFromDamage() const {
  return !hasHealth();
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
