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
#include "animation_id.h"
#include "item_types.h"
#include "health_type.h"
#include "game_event.h"
#include "effect_type.h"
#include "resource_id.h"

static double getDefaultWeight(Body::Size size) {
  switch (size) {
    case Body::Size::HUGE: return 1000;
    case Body::Size::LARGE: return 90;
    case Body::Size::MEDIUM: return 45;
    case Body::Size::SMALL: return 20;
  }
}

template <class Archive>
void Body::serializeImpl(Archive& ar, const unsigned int) {
  ar(OPTION(xhumanoid), OPTION(size), OPTION(weight), OPTION(bodyParts), OPTION(injuredBodyParts), OPTION(lostBodyParts));
  ar(OPTION(material), OPTION(health), OPTION(minionFood), NAMED(deathSound), OPTION(intrinsicAttacks), OPTION(minPushSize));
  ar(OPTION(noHealth), OPTION(fallsApart), OPTION(drops), OPTION(canCapture), OPTION(xCanPickUpItems), OPTION(droppedPartUpgrade));
}

template <class Archive>
void Body::serialize(Archive& ar1, const unsigned int v) {
  serializeImpl(ar1, v);
}

SERIALIZABLE(Body)

SERIALIZATION_CONSTRUCTOR_IMPL(Body)

static int getDefaultIntrinsicDamage(Body::Size size) {
  switch (size) {
    case Body::Size::HUGE: return 8;
    case Body::Size::LARGE: return 5;
    case Body::Size::MEDIUM: return 3;
    case Body::Size::SMALL: return 1;
  }
}

Body::Body(bool humanoid, Material m, Size size) : xhumanoid(humanoid), size(size),
    weight(getDefaultWeight(size)), material(m),
    deathSound(humanoid ? SoundId::HUMANOID_DEATH : SoundId::BEAST_DEATH),
    minPushSize(Size((int)size + 1)) {
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

void Body::addWithoutUpdatingPermanentEffects(BodyPart part, int cnt) {
  bodyParts[part] += cnt;
}

void Body::setWeight(double w) {
  weight = w;
}

void Body::setSize(BodySize s) {
  size = s;
}

void Body::setBodyParts(const EnumMap<BodyPart, int>& p) {
  bodyParts = p;
  bodyParts[BodyPart::TORSO] = 1;
  bodyParts[BodyPart::BACK] = 1;
}

void Body::initializeIntrinsicAttack(const ContentFactory* factory) {
  for (auto bodyPart : ENUM_ALL(BodyPart))
    for (auto& attack : intrinsicAttacks[bodyPart])
      attack.initializeItem(factory);
}

void Body::addIntrinsicAttack(BodyPart part, IntrinsicAttack attack) {
  if (!numGood(part)) {
    part = BodyPart::TORSO;
    CHECK(numGood(part));
  }
  intrinsicAttacks[part].push_back(std::move(attack));
}

void Body::setMinPushSize(Body::Size size) {
  minPushSize = size;
}

void Body::setHumanoid(bool h) {
  xhumanoid = h;
}

vector<pair<Item*, double>> Body::chooseRandomWeapon(vector<Item*> weapons, vector<double> multipliers) const {
  vector<pair<Item*, double>> ret;
  for (auto part : ENUM_ALL(BodyPart))
    for (auto& attack : intrinsicAttacks[part])
      if (numGood(part) > 0 && !attack.isExtraAttack && weapons.size() < multipliers.size())
        weapons.push_back(attack.item.get());
  weapons = Random.permutation(weapons);
  for (auto i : All(weapons))
    if (i < multipliers.size())
      ret.push_back({weapons[i], multipliers[i]});
  for (auto part : ENUM_ALL(BodyPart))
    for (auto& attack : intrinsicAttacks[part])
      if (numGood(part) > 0 && attack.isExtraAttack)
        ret.push_back({attack.item.get(), 1});
  return ret;
}

Item* Body::chooseFirstWeapon() const {
  for (auto part : ENUM_ALL(BodyPart))
    for (auto& attack : intrinsicAttacks[part])
      if (numGood(part) > 0 && attack.active)
        return attack.item.get();
  return nullptr;
}

EnumMap<BodyPart, vector<IntrinsicAttack>>& Body::getIntrinsicAttacks() {
  return intrinsicAttacks;
}

const EnumMap<BodyPart, vector<IntrinsicAttack>>& Body::getIntrinsicAttacks() const {
  return intrinsicAttacks;
}

void Body::setHumanoidBodyParts(int intrinsicDamage) {
  setBodyParts({{BodyPart::LEG, 2}, {BodyPart::ARM, 2}, {BodyPart::HEAD, 1}, {BodyPart::BACK, 1},
      {BodyPart::TORSO, 1}});
  addIntrinsicAttack(BodyPart::ARM, IntrinsicAttack(ItemType::fists(intrinsicDamage)));
  addIntrinsicAttack(BodyPart::LEG, IntrinsicAttack(ItemType::legs(intrinsicDamage)));
}

void Body::setHorseBodyParts(int intrinsicDamage) {
  setBodyParts({{BodyPart::LEG, 4}, {BodyPart::HEAD, 1}, {BodyPart::BACK, 1},
      {BodyPart::TORSO, 1}});
  addIntrinsicAttack(BodyPart::HEAD, IntrinsicAttack(ItemType::fangs(intrinsicDamage), true));
  addIntrinsicAttack(BodyPart::LEG, IntrinsicAttack(ItemType::legs(intrinsicDamage), true));
}

void Body::setBirdBodyParts(int intrinsicDamage) {
  setBodyParts({{BodyPart::LEG, 2}, {BodyPart::WING, 2}, {BodyPart::HEAD, 1}, {BodyPart::BACK, 1},
      {BodyPart::TORSO, 1}});
  addIntrinsicAttack(BodyPart::HEAD, IntrinsicAttack(ItemType::beak(intrinsicDamage)));
}

void Body::setMinionFood() {
  minionFood = true;
}

void Body::setDeathSound(optional<SoundId> s) {
  deathSound = s;
}

bool Body::canHeal(HealthType type) const {
  return health < 1 && hasHealth(type);
}

bool Body::hasAnyHealth() const {
  for (auto type : ENUM_ALL(HealthType))
    if (hasHealth(type))
      return true;
  return false;
}

const char* Body::getDeathDescription() const {
  switch (material) {
    case Body::Material::SPIRIT:
    case Body::Material::UNDEAD_FLESH:
    case Body::Material::FLESH:
      return "killed";
    case Body::Material::BONE:
    case Body::Material::WATER:
    case Body::Material::LAVA:
    case Body::Material::ROCK:
    case Body::Material::FIRE:
    case Body::Material::WOOD:
    case Body::Material::CLAY:
    case Body::Material::IRON:
    case Body::Material::ADA:
    case Body::Material::ICE:
    case Body::Material::GOLD:
      return "destroyed";
  }
}

bool Body::hasHealth(HealthType type) const {
  switch (material) {
    case Material::FLESH:
      return !noHealth && type == HealthType::FLESH;
    case Material::FIRE:
    case Material::SPIRIT:
      return !noHealth && type == HealthType::SPIRIT;
    default:
      return false;
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
  if (!hasAnyHealth())
    return Random.chance(damage / strength);
  if (material == Material::FLESH)
    return damage >= strength;
  else
    return false;
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

bool Body::fallsApartDueToLostBodyParts() const {
  if (fallsApart && !hasAnyHealth()) {
    int ret = 0;
    for (BodyPart part : ENUM_ALL(BodyPart))
      ret += injuredBodyParts[part];
    for (BodyPart part : ENUM_ALL(BodyPart))
      ret += lostBodyParts[part];
    return ret >= 4;
  } else
    return false;
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
  if (good.empty())
    return none;
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

bool Body::healBodyParts(Creature* creature, int max) {
  bool result = false;
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
  for (BodyPart part : ENUM_ALL(BodyPart)) {
    if (int count = numInjured(part)) {
      result = true;
      creature->you(MsgType::YOUR,
          string(getName(part)) + (count > 1 ? "s are" : " is") + " in better shape");
      clearInjured(part);
      updateEffects(part, count);
      if (--max == 0)
        break;
    }
    if (int count = numLost(part)) {
      result = true;
      creature->you(MsgType::YOUR, string(getName(part)) + (count > 1 ? "s grow back!" : " grows back!"));
      clearLost(part);
      updateEffects(part, count);
      if (--max == 0)
        break;
    }
  }
  return result;
}

bool Body::injureBodyPart(Creature* creature, BodyPart part, bool drop) {
  auto game = creature->getGame();
  if (bodyParts[part] == 0 || (!drop && injuredBodyParts[part] == bodyParts[part]))
    return false;
  if (drop) {
    if (contains({BodyPart::LEG, BodyPart::ARM, BodyPart::WING}, part))
      game->getStatistics().add(StatId::CHOPPED_LIMB);
    else if (part == BodyPart::HEAD)
      game->getStatistics().add(StatId::CHOPPED_HEAD);
    if (auto item = getBodyPartItem(creature->getAttributes().getName().bare(), part, game->getContentFactory())) {
      if (droppedPartUpgrade && game->effectFlags.count("abomination_upgrades")) {
        item->setUpgradeInfo(ItemUpgradeInfo{ItemUpgradeType::BODY_PART, 
          Effect(Effects::Chain{{Effect(Effects::AddBodyPart{part, 1}), std::move(*droppedPartUpgrade)}})});
        item->setResourceId(none);
        droppedPartUpgrade = none;
      }
      creature->getPosition().dropItem(std::move(item));
    }
    if (looseBodyPart(part))
      return true;
  } else if (injureBodyPart(part))
    return true;
  switch (part) {
    case BodyPart::HEAD:
      if (numGood(BodyPart::HEAD) == 0)
        creature->addPermanentEffect(LastingEffect::BLIND);
      break;
    case BodyPart::LEG:
      creature->addPermanentEffect(LastingEffect::COLLAPSED);
      break;
    case BodyPart::ARM:
      // Drop the weapon anyway even if the other arm is still ok.
      creature->dropWeapon();
      break;
    case BodyPart::WING:
      creature->removePermanentEffect(LastingEffect::FLYING);
      break;
    default: break;
  }
  creature->dropUnsupportedEquipment();
  return false;
}

template <typename T>
void consumeBodyAttr(T& mine, const T& his, vector<string>& adjectives, const string& adj) {
  if (mine < his) {
    mine = his;
    if (!adj.empty())
      adjectives.push_back(adj);
  }
}

void Body::addBodyPart(BodyPart parts, int count) {
  CHECK(count > 0);
  bodyParts[parts] += count;
}

void Body::consumeBodyParts(Creature* c, Body& other, vector<string>& adjectives) {
  for (BodyPart part : ENUM_ALL(BodyPart)) {
    int cnt = other.bodyParts[part] - bodyParts[part];
    if (cnt > 0) {
      string what = getPlural(getName(part), cnt);
      c->verb("grow", "grows", what);
      c->addPersonalEvent(c->getName().the() + " grows "_s + what);
      bodyParts[part] = other.bodyParts[part];
    }
    if (!other.intrinsicAttacks[part].empty())
      intrinsicAttacks[part].clear();
    for (auto& attack : other.intrinsicAttacks[part]) {
      c->verb("develop", "develops",  "a " + attack.item->getNameAndModifiers() + " attack");
      c->addPersonalEvent(c->getName().the() + " develops a " + attack.item->getNameAndModifiers() + " attack");
      intrinsicAttacks[part].push_back(std::move(attack));
    }
    other.intrinsicAttacks[part].clear();
  }
  if (other.isHumanoid() && !isHumanoid() && numBodyParts(BodyPart::ARM) >= 2 &&
      numBodyParts(BodyPart::LEG) >= 2 && numBodyParts(BodyPart::HEAD) >= 1) {
    c->you(MsgType::BECOME, "a humanoid");
    c->addPersonalEvent(c->getName().the() + " turns into a humanoid");
    xhumanoid = true;
  }
  consumeBodyAttr(size, other.size, adjectives, "larger");
}

Body::Material Body::getMaterial() const {
  return material;
}

bool Body::looseBodyPart(BodyPart part) {
  if (bodyParts[part] > 0) {
    --bodyParts[part];
    ++lostBodyParts[part];
    if (injuredBodyParts[part] > bodyParts[part])
      --injuredBodyParts[part];
  }
  return isCritical(part);
}

bool Body::injureBodyPart(BodyPart part) {
  if (injuredBodyParts[part] < bodyParts[part])
    ++injuredBodyParts[part];
  return isCritical(part);
}

string sizeStr(Body::Size s) {
  switch (s) {
    case Body::Size::SMALL: return "small";
    case Body::Size::MEDIUM: return "medium";
    case Body::Size::LARGE: return "large";
    case Body::Size::HUGE: return "huge";
  }
}

const char* getMaterialName(Body::Material material) {
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
    case Body::Material::GOLD: return "gold";
    case Body::Material::ICE: return "ice";
  }
}

string Body::getMaterialAndSizeAdjectives() const {
  vector<string> ret {sizeStr(size)};
  switch (material) {
    case Material::FLESH: break;
    case Material::UNDEAD_FLESH: ret.push_back("undead"); break;
    default: ret.push_back("made of "_s + getMaterialName(material));
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
  auto numHeads = numBodyParts(BodyPart::HEAD);
  if (numHeads == 0)
    ret.push_back("no "_s + getName(BodyPart::HEAD));
  else if (numHeads > 1)
    ret.push_back(getPluralText(getName(BodyPart::HEAD), numHeads));
  string limbDescription = ret.size() > 0 ? " with " + combine(ret) : "";
  return getMaterialAndSizeAdjectives() + limbDescription + ".";
}

bool Body::isHumanoid() const {
  return xhumanoid;
}

bool Body::canPickUpItems() const {
  return xCanPickUpItems || isHumanoid();
}

static int numCorpseItems(Body::Size size) {
  switch (size) {
    case Body::Size::LARGE: return Random.get(30, 50);
    case Body::Size::HUGE: return Random.get(80, 120);
    case Body::Size::MEDIUM: return Random.get(15, 30);
    case Body::Size::SMALL: return Random.get(1, 8);
  }
}

PItem Body::getBodyPartItem(const string& name, BodyPart part, const ContentFactory* factory) const {
  switch (material) {
    case Material::FLESH:
    case Material::UNDEAD_FLESH:
      return ItemType::severedLimb(name, part, weight / 8, isMinionFood() ? ItemClass::FOOD : ItemClass::CORPSE, factory);
    case Material::CLAY:
    case Material::ROCK:
      return ItemType(CustomItemId("Rock")).get(factory);
    case Material::BONE:
      return ItemType(CustomItemId("Bone")).get(factory);
    case Material::IRON:
      return ItemType(CustomItemId("IronOre")).get(factory);
    case Material::WOOD:
      return ItemType(CustomItemId("WoodPlank")).get(factory);
    case Material::ADA:
      return ItemType(CustomItemId("AdaOre")).get(factory);
    case Material::GOLD:
      return ItemType(CustomItemId("GoldPiece")).get(factory);
    default: return nullptr;
  }
}

static bool bodyPartCanBeDropped(BodyPart part) {
  switch (part) {
    case BodyPart::ARM:
    case BodyPart::LEG:
    case BodyPart::HEAD:
    case BodyPart::WING:
      return true;
    case BodyPart::TORSO:
    case BodyPart::BACK:
      return false;
  }
}

vector<PItem> Body::getCorpseItems(const string& name, Creature::Id id, bool instantlyRotten, const ContentFactory* factory,
    Game* game) const {
  vector<PItem> ret = [&] {
    switch (material) {
      case Material::FLESH:
      case Material::UNDEAD_FLESH:
        return makeVec(
            ItemType::corpse(name + " corpse", name + " skeleton", weight, factory, instantlyRotten,
              minionFood ? ItemClass::FOOD : ItemClass::CORPSE,
              {id, material != Material::UNDEAD_FLESH, numBodyParts(BodyPart::HEAD) > 0, false}));
      case Material::CLAY:
      case Material::ROCK:
        return ItemType(CustomItemId("Rock")).get(numCorpseItems(size), factory);
      case Material::BONE:
        return ItemType(CustomItemId("Bone")).get(numCorpseItems(size), factory);
      case Material::IRON:
        return ItemType(CustomItemId("IronOre")).get(numCorpseItems(size), factory);
      case Material::WOOD:
        return ItemType(CustomItemId("WoodPlank")).get(numCorpseItems(size), factory);
      case Material::ADA:
        return ItemType(CustomItemId("AdaOre")).get(numCorpseItems(size), factory);
      case Material::GOLD:
        return ItemType(CustomItemId("GoldPiece")).get(numCorpseItems(size), factory);
      default: return vector<PItem>();
    }
  }();
  if (!drops.empty())
    if (auto item = Random.choose(drops))
      ret.push_back(item->get(factory));
  if (game && droppedPartUpgrade && game->effectFlags.count("abomination_upgrades"))
    for (auto part : Random.permutation<BodyPart>())
      if (numGood(part) > 0 && bodyPartCanBeDropped(part))
        if (auto item = getBodyPartItem(name, part, game->getContentFactory())) {
          item->setUpgradeInfo(ItemUpgradeInfo{ItemUpgradeType::BODY_PART, 
              Effect(Effects::Chain{{Effect(Effects::AddBodyPart{part, 1}), *droppedPartUpgrade}})});
          item->setResourceId(none);
          ret.push_back(std::move(item));
          break;
        }
  return ret;
}

void Body::affectPosition(Position position) {
  if (material == Material::FIRE)
    position.fireDamage(0.2);
}

static void youHit(const Creature* c, BodyPart part, AttackType type) {
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
          case AttackType::SHOOT: c->you(MsgType::ARE, "shot in the heart!"); break;
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
          case AttackType::SHOOT: c->you(MsgType::ARE, "shot in the arm!"); break;
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
          case AttackType::SHOOT: c->you(MsgType::ARE, "shot in the wing!"); break;
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
          case AttackType::SHOOT: c->you(MsgType::ARE, "shot in the leg!"); break;
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

Body::DamageResult Body::takeDamage(const Attack& attack, Creature* creature, double damage) {
  PROFILE;
  bleed(creature, damage);
  if (auto part = getBodyPart(attack.level, creature->isAffected(LastingEffect::FLYING),
      creature->isAffected(LastingEffect::COLLAPSED)))
    if (isPartDamaged(*part, damage)) {
      youHit(creature, *part, attack.type);
      if (injureBodyPart(creature, *part,
          contains({AttackType::CUT, AttackType::BITE}, attack.type) && bodyPartCanBeDropped(*part))) {
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
    if (hasAnyHealth())
      creature->you(MsgType::ARE, "wounded");
    else if (attack.effect.empty()) {
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

bool Body::tick(const Creature* c) {
  if (fallsApartDueToLostBodyParts()) {
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
  if (hasAnyHealth())
    obj.setAttribute(ViewObject::Attribute::HEALTH, health);
  else
    obj.setAttribute(ViewObject::Attribute::HEALTH, getBodyPartHealth());
  obj.setModifier(ViewObjectModifier::HEALTH_BAR);
  if (hasHealth(HealthType::SPIRIT))
    obj.setModifier(ViewObject::Modifier::SPIRIT_DAMAGE);
}

bool Body::heal(Creature* c, double amount) {
  INFO << c->getName().the() << " heal";
  if (health < 1) {
    health = min(1., health + amount);
    if (health >= 1) {
      c->you(MsgType::ARE, hasHealth(HealthType::FLESH) ? "fully healed" : "fully materialized");
      health = 1;
      return true;
    }
    updateViewObject(c->modViewObject());
  }
  return false;
}

bool Body::isIntrinsicallyAffected(LastingEffect effect) const {
  switch (effect) {
    case LastingEffect::COLD_RESISTANT:
      switch (material) {
        case Material::ICE:
          return true;
        default:
          return false;
      }
    case LastingEffect::ACID_RESISTANT:
      switch (material) {
        case Material::FIRE:
        case Material::SPIRIT:
          return true;
        default:
          return false;
      }
    case LastingEffect::FIRE_RESISTANT:
      switch (material) {
        case Material::FLESH:
        case Material::SPIRIT:
        case Material::UNDEAD_FLESH:
        case Material::ICE:
        case Material::WOOD:
          return false;
        default:
          return true;
      }
    case LastingEffect::SUNLIGHT_VULNERABLE:
      return material == Material::UNDEAD_FLESH;
    case LastingEffect::FLYING:
      return numGood(BodyPart::WING) >= 2;
    case LastingEffect::LIGHT_SOURCE:
      return material == Material::FIRE;
    default:
      return false;
  }
}

bool Body::isImmuneTo(LastingEffect effect) const {
  switch (effect) {
    case LastingEffect::RESTED:
    case LastingEffect::SATIATED:
    case LastingEffect::SLEEP:
      switch (material) {
        case Material::BONE:
        case Material::FLESH:
        case Material::UNDEAD_FLESH:
          return false;
        default:
          return true;
      }
    case LastingEffect::RAGE:
    case LastingEffect::PANIC:
    case LastingEffect::TELEPATHY:
    case LastingEffect::INSANITY:
      return !hasBrain();
    case LastingEffect::FROZEN:
      return material == Material::FIRE;
    case LastingEffect::POISON:
    case LastingEffect::PLAGUE:
    case LastingEffect::LIFE_SAVED:
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

bool Body::affectByPoisonGas(Creature* c, double amount) {
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

bool Body::affectByAcid(Creature* c) {
  c->you(MsgType::ARE, "hurt by the acid");
  bleed(c, 0.2);
  return health <= 0;
}

bool Body::affectByFire(Creature* c, double amount) {
  c->you(MsgType::ARE, "burnt by the fire");
  bleed(c, 6. * amount / double(1 + c->getAttr(AttrType::DEFENSE)));
  return health <= 0;
}

void Body::bleed(Creature* c, double amount) {
  if (hasAnyHealth()) {
    health -= amount;
    c->updateViewObject();
    if (c->getStatus().contains(CreatureStatus::LEADER))
      if (auto game = c->getGame())
        game->addEvent(EventInfo::LeaderWounded{c});
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
  return material == Material::FLESH || material == Material::UNDEAD_FLESH;
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

bool Body::burnsIntrinsically() const {
  switch (material) {
    case Material::WOOD:
      return true;
    default:
      return false;
  }
}

bool Body::canPush(const Body& other) {
  return int(size) >= int(other.minPushSize);
}

bool Body::canPerformRituals() const {
  return xhumanoid && !isImmuneTo(LastingEffect::TIED_UP);
}

bool Body::canBeCaptured() const {
  if (canCapture)
    return *canCapture;
  return xhumanoid && !isImmuneTo(LastingEffect::TIED_UP);
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

optional<AnimationId> Body::getDeathAnimation() const {
  if (isHumanoid() && hasAnyHealth())
    return AnimationId::DEATH;
  else
    return none;
}

double Body::getBoulderDamage() const {
  switch (size) {
    case Body::Size::HUGE: return 1;
    case Body::Size::LARGE: return 0.3;
    case Body::Size::MEDIUM: return 0.15;
    case Body::Size::SMALL: return 0;
  }
}

int Body::getCarryLimit() const {
  switch (size) {
    case Body::Size::HUGE: return 200;
    case Body::Size::LARGE: return 80;
    case Body::Size::MEDIUM: return 60;
    case Body::Size::SMALL: return 6;
  }
}

#include "pretty_archive.h"

RICH_ENUM(BodyType,
    Humanoid,
    HumanoidLike,
    Bird,
    FourLegged,
    NonHumanoid
);

struct BodyTypeReader {
  static Body* body;
  void serialize(PrettyInputArchive& ar1, unsigned v) {
    BodyType type;
    BodySize size;
    ar1(type, size);
    body->setWeight(getDefaultWeight(size));
    body->setSize(size);
    body->setMinPushSize(BodySize((int)size + 1));
    switch (type) {
      case BodyType::Humanoid:
        body->setHumanoidBodyParts(getDefaultIntrinsicDamage(size));
        body->setDeathSound(SoundId::HUMANOID_DEATH);
        body->setHumanoid(true);
        break;
      case BodyType::HumanoidLike:
        body->setHumanoidBodyParts(getDefaultIntrinsicDamage(size));
        body->setDeathSound(SoundId::BEAST_DEATH);
        break;
      case BodyType::Bird:
        body->setBirdBodyParts(getDefaultIntrinsicDamage(size));
        body->setDeathSound(SoundId::BEAST_DEATH);
        break;
      case BodyType::FourLegged:
        body->setHorseBodyParts(getDefaultIntrinsicDamage(size));
        body->setDeathSound(SoundId::BEAST_DEATH);
        break;
      default:
        body->setDeathSound(SoundId::BEAST_DEATH);
        break;
    }
  }
};

Body* BodyTypeReader::body = nullptr;

template <>
void Body::serialize(PrettyInputArchive& ar1, unsigned v) {
  BodyTypeReader type;
  BodyTypeReader::body = this;
  ar1(NAMED(type));
  serializeImpl(ar1, v);
  EnumMap<BodyPart, int> addBodyPart;
  ar1(OPTION(addBodyPart));
  ar1(endInput());
  for (auto part : ENUM_ALL(BodyPart)) {
    bodyParts[part] += addBodyPart[part];
    if (bodyParts[part] == 0 && !intrinsicAttacks[part].empty())
      ar1.error("Creature has an intrinsic attack attached to non-existent body part"_s);
  }
}
