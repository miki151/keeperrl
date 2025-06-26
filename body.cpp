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
#include "content_factory.h"

static double getDefaultWeight(Body::Size size) {
  switch (size) {
    case Body::Size::HUGE: return 1000;
    case Body::Size::LARGE: return 90;
    case Body::Size::MEDIUM: return 45;
    case Body::Size::SMALL: return 20;
  }
}

template <class Archive>
void Body::serializeImpl(Archive& ar, const unsigned int version) {
  ar(OPTION(xhumanoid), OPTION(size), OPTION(weight), OPTION(bodyParts), OPTION(injuredBodyParts), OPTION(lostBodyParts));
  ar(OPTION(material), OPTION(health), OPTION(minionFood), NAMED(deathSound), OPTION(intrinsicAttacks), OPTION(minPushSize));
  ar(OPTION(noHealth), OPTION(fallsApart), OPTION(drops), OPTION(canCapture), OPTION(xCanPickUpItems), OPTION(droppedPartUpgrade));
  ar(OPTION(corpseIngredientType), OPTION(canBeRevived), OPTION(overrideDiningFurniture), OPTION(ambientSound));
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

Body::Body(bool humanoid, BodyMaterialId m, Size size) : xhumanoid(humanoid), size(size),
    weight(getDefaultWeight(size)), material(m),
    deathSound(humanoid ? SoundId("HUMANOID_DEATH") : SoundId("BEAST_DEATH")),
    minPushSize(Size((int)size + 1)) {
  if (humanoid)
    setHumanoidBodyParts(getDefaultIntrinsicDamage(size));
}

Body Body::humanoid(BodyMaterialId m, Size s) {
  return Body(true, m, s);
}

Body Body::humanoid(Size s) {
  return humanoid(BodyMaterialId("FLESH"), s);
}

Body Body::nonHumanoid(BodyMaterialId m, Size s) {
  return Body(false, m, s);
}

Body Body::nonHumanoid(Size s) {
  return nonHumanoid(BodyMaterialId("FLESH"), s);
}

Body Body::humanoidSpirit(Size s) {
  return Body(true, BodyMaterialId("SPIRIT"), s);
}

Body Body::nonHumanoidSpirit(Size s) {
  return Body(false, BodyMaterialId("SPIRIT"), s);
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

void Body::setDeathSound(optional<SoundId> s) {
  deathSound = s;
}

bool Body::canHeal(HealthType type, const ContentFactory* factory) const {
  return health < 1 && hasHealth(type, factory);
}

bool Body::hasAnyHealth(const ContentFactory* factory) const {
  return !noHealth && !!factory->bodyMaterials.at(material).healthType;
}

FurnitureType Body::getDiningFurniture() const {
  if (overrideDiningFurniture)
    return *overrideDiningFurniture;
  if (isHumanoid())
    return FurnitureType("DINING_TABLE");
  else
    return FurnitureType("HAYPILE");
}

const TString& Body::getDeathDescription(const ContentFactory* factory) const {
  return factory->bodyMaterials.at(material).deathDescription;
}

bool Body::hasHealth(HealthType type, const ContentFactory* factory) const {
  return !noHealth && factory->bodyMaterials.at(material).healthType == type;
}

bool Body::isPartDamaged(BodyPart part, double damage, const ContentFactory* factory) const {
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
  if (!hasAnyHealth(factory))
    return Random.chance(damage / strength);
  if (factory->bodyMaterials.at(material).canLoseBodyParts)
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

bool Body::isCritical(BodyPart part, const ContentFactory* factory) const {
  return contains({BodyPart::TORSO}, part)
    || (part == BodyPart::HEAD && numGood(part) == 0 && factory->bodyMaterials.at(material).losingHeadsMeansDeath);
}


int Body::numBodyParts(BodyPart part) const {
  return bodyParts[part];
}

int Body::numLost(BodyPart part) const {
  return lostBodyParts[part];
}

bool Body::fallsApartDueToLostBodyParts(const ContentFactory* factory) const {
  if (fallsApart && !hasAnyHealth(factory)) {
    return getBodyPartHealth() < 0.1;
  } else
    return false;
}

int Body::numInjured(BodyPart part) const {
  return injuredBodyParts[part];
}

int Body::numGood(BodyPart part) const {
  return numBodyParts(part) - numInjured(part);
}

void Body::clearInjured(BodyPart part, int count) {
  injuredBodyParts[part] = max(0, injuredBodyParts[part] - count);
}

void Body::clearLost(BodyPart part, int count) {
  count = min(lostBodyParts[part], count);
  bodyParts[part] += count;
  lostBodyParts[part] -= count;
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

optional<BodyPart> Body::getBodyPart(const ContentFactory* factory) const {
  vector<BodyPart> allowed {BodyPart::LEG, BodyPart::ARM, BodyPart::WING};
  const auto health = hasAnyHealth(factory) ? getHealth() : getBodyPartHealth();
  if (health < 0.4 || !factory->bodyMaterials.at(material).losingHeadsMeansDeath)
    allowed.push_back(BodyPart::HEAD);
  if (allowed.empty() || health < 0.4)
    allowed.push_back(BodyPart::TORSO);
  for (auto part : Random.permutation(allowed))
    if (numGood(part))
      return part;
  for (auto part : Random.permutation<BodyPart>())
    if (numGood(part))
      return part;
  return none;
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
      count = min(max, count);
      auto partName = count == 1 ? getName(part) : makePlural(getName(part));
      creature->verb(TStringId("YOUR_BODY_PART_IS_IN_BETTER_SHAPE"), TStringId("HIS_BODY_PART_IS_IN_BETTER_SHAPE"),
          partName);
      creature->addPersonalEvent(TSentence("HIS_BODY_PART_IS_IN_BETTER_SHAPE", creature->getName().a(), partName));
      clearInjured(part, count);
      updateEffects(part, count);
      max -= count;
      if (max <= 0)
        break;
    }
    if (int count = numLost(part)) {
      result = true;
      count = min(max, count);
      auto partName = count == 1 ? getName(part) : makePlural(getName(part));
      creature->verb(TStringId("YOUR_BODY_PART_GROWS_BACK"), TStringId("HIS_BODY_PART_GROWS_BACK"), partName);
      creature->addPersonalEvent(TSentence("HIS_BODY_PART_GROWS_BACK", creature->getName().a(), partName));
      clearLost(part, count);
      updateEffects(part, count);
      max -= count;
      if (max <= 0)
        break;
    }
  }
  return result;
}

static void setBodyPartUpgrade(Item* item, BodyPart part, Effect upgrade, const ContentFactory* f) {
  item->modViewObject().setModifier(ViewObject::Modifier::AURA);
  auto name = upgrade.getName(f);
  auto desc = upgrade.getDescription(f);
  item->setUpgradeInfo(ItemUpgradeInfo{ItemUpgradeType::BODY_PART, Effect(Effects::Description{desc, Effect(Effects::Name{name, 
    Effect(Effects::Chain{{Effect(Effects::AddBodyPart{part, 1}), std::move(upgrade)}})})})});
  item->setResourceId(none);
}

static Effect getDefaultBodyPartUpgrade() {
  return Effect(Effects::Description(TStringId("INCREASE_DAMAGE_OR_DEFENSE_BY_ONE"), Effect(Effects::ChooseRandom({
      Effect(Effects::IncreaseAttr{AttrType("DAMAGE"), 1}),
      Effect(Effects::IncreaseAttr{AttrType("DEFENSE"), 1})
  }))));
}

bool Body::injureBodyPart(Creature* creature, BodyPart part, bool drop) {
  auto game = creature->getGame();
  auto factory = game->getContentFactory();
  if (bodyParts[part] == 0 || (!drop && injuredBodyParts[part] == bodyParts[part]))
    return false;
  if (drop) {
    if (contains({BodyPart::LEG, BodyPart::ARM, BodyPart::WING}, part))
      game->getStatistics().add(StatId::CHOPPED_LIMB);
    else if (part == BodyPart::HEAD)
      game->getStatistics().add(StatId::CHOPPED_HEAD);
    if (auto item = getBodyPartItem(creature->getAttributes().getName().bare(), part, factory)) {
      if (material == BodyMaterialId("FLESH") && game->effectFlags.count("abomination_upgrades")) {
        auto upgrade = droppedPartUpgrade.value_or_f(&getDefaultBodyPartUpgrade);
        setBodyPartUpgrade(item.get(), part, std::move(upgrade), factory);
        droppedPartUpgrade = none;
      }
      creature->getPosition().dropItem(std::move(item));
    }
    if (looseBodyPart(part, factory))
      return true;
  } else if (injureBodyPart(part, factory))
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
void consumeBodyAttr(T& mine, const T& his, vector<TString>& adjectives, const TString& adj) {
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

void Body::consumeBodyParts(Creature* c, Body& other, vector<TString>& adjectives) {
  auto factory = c->getGame()->getContentFactory();
  for (BodyPart part : ENUM_ALL(BodyPart)) {
    int cnt = other.bodyParts[part] - bodyParts[part];
    if (cnt > 0) {
      if (cnt > 1) {
        auto what = makePlural(getName(part));
        c->verb(TStringId("YOU_GROW_LIMBS"), TStringId("GROWS_LIMBS"), what);
        c->addPersonalEvent(TSentence("GROWS_LIMBS", c->getName().the(), what));
      } else {
        auto what = getName(part);
        c->verb(TStringId("YOU_GROW_LIMB"), TStringId("GROWS_LIMB"), what);
        c->addPersonalEvent(TSentence("GROWS_LIMB", c->getName().the(), what));
      }
      bodyParts[part] = other.bodyParts[part];
    }
    if (!other.intrinsicAttacks[part].empty())
      intrinsicAttacks[part].clear();
    for (auto& attack : other.intrinsicAttacks[part]) {
      c->verb(TStringId("YOU_DEVELOP_ATTACK"), TStringId("DEVELOPS_ATTACK"),  attack.item->getNameAndModifiers(factory));
      c->addPersonalEvent(TSentence("DEVELOPS_ATTACK", c->getName().the(), attack.item->getNameAndModifiers(factory)));
      intrinsicAttacks[part].push_back(std::move(attack));
    }
    other.intrinsicAttacks[part].clear();
  }
  if (other.isHumanoid() && !isHumanoid() && numBodyParts(BodyPart::ARM) >= 2 &&
      numBodyParts(BodyPart::LEG) >= 2 && numBodyParts(BodyPart::HEAD) >= 1) {
    c->you(MsgType::BECOME, TStringId("HUMANOID"));
    c->addPersonalEvent(TSentence("BECOMES", c->getName().the(), TStringId("HUMANOID")));
    xhumanoid = true;
  }
  consumeBodyAttr(size, other.size, adjectives, TStringId("LARGER"));
}

BodyMaterialId Body::getMaterial() const {
  return material;
}

bool Body::looseBodyPart(BodyPart part, const ContentFactory* factory) {
  if (bodyParts[part] > 0) {
    --bodyParts[part];
    ++lostBodyParts[part];
    if (injuredBodyParts[part] > bodyParts[part])
      --injuredBodyParts[part];
  }
  return isCritical(part, factory);
}

bool Body::injureBodyPart(BodyPart part, const ContentFactory* factory) {
  if (injuredBodyParts[part] < bodyParts[part])
    ++injuredBodyParts[part];
  return isCritical(part, factory);
}

TStringId getName(Body::Size s) {
  switch (s) {
    case Body::Size::SMALL: return TStringId("BODY_SIZE_SMALL");
    case Body::Size::MEDIUM: return TStringId("BODY_SIZE_MEDIUM");
    case Body::Size::LARGE: return TStringId("BODY_SIZE_LARGE");
    case Body::Size::HUGE: return TStringId("BODY_SIZE_HUGE");
  }
}

TString Body::getDescription(const ContentFactory* factory) const {
  vector<TString> ret;
  bool anyLimbs = false;
  vector<BodyPart> listParts = {BodyPart::ARM, BodyPart::LEG, BodyPart::WING};
  for (BodyPart part : listParts)
    if (int num = numBodyParts(part)) {
      ret.push_back(TSentence("WITH_LIMB", getPluralText(part, num)));
      anyLimbs = true;
    }
  if (xhumanoid) {
    bool noArms = numBodyParts(BodyPart::ARM) == 0;
    bool noLegs = numBodyParts(BodyPart::LEG) == 0;
    if (noArms && noLegs)
      ret.push_back(TStringId("NO_LIMBS"));
    else if (noArms)
      ret.push_back(TStringId("NO_ARMS"));
    else if (noLegs)
      ret.push_back(TStringId("NO_LEGS"));
  }
  else if (!anyLimbs)
    ret.push_back(TStringId("NO_LIMBS"));
  auto numHeads = numBodyParts(BodyPart::HEAD);
  if (numHeads == 0)
    ret.push_back(TStringId("NO_HEAD"));
  else if (numHeads > 1)
    ret.push_back(getPluralText(BodyPart::HEAD, numHeads));
  return TSentence("BODY_SIZE_AND_MATERIAL_AND_LIMBS", {
      getName(size),
      factory->bodyMaterials.at(material).name,
      combineWithAnd(ret)});
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

PItem Body::getBodyPartItem(const TString& name, BodyPart part, const ContentFactory* factory) const {
  if (material == BodyMaterialId("FLESH") || material == BodyMaterialId("UNDEAD_FLESH"))
    return ItemType::severedLimb(name, part, weight / 8, isFarmAnimal() ? ItemClass::FOOD : ItemClass::CORPSE, factory);
  if (auto& t = factory->bodyMaterials.at(material).bodyPartItem)
    return t->get(factory);
  return nullptr;
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

vector<PItem> Body::getCorpseItems(const TString& name, Creature::Id id, bool instantlyRotten, const ContentFactory* factory,
    Game* game) const {
  vector<PItem> ret = [&] {
    if (material == BodyMaterialId("FLESH") || material == BodyMaterialId("UNDEAD_FLESH"))
      return makeVec(
          ItemType::corpse(TSentence("CORPSE_ITEM", name), TSentence("SKELETON_ITEM", name), weight, factory, instantlyRotten,
            minionFood ? ItemClass::FOOD : ItemClass::CORPSE,
            CorpseInfo {id, !corpseIngredientType && canBeRevived && material != BodyMaterialId("UNDEAD_FLESH"),
                numBodyParts(BodyPart::HEAD) > 0, false},
            corpseIngredientType));
    if (auto& t = factory->bodyMaterials.at(material).bodyPartItem)
      return t->get(numCorpseItems(size), factory);
    return vector<PItem>();
  }();
  if (!drops.empty())
    if (auto item = Random.choose(drops))
      ret.push_back(item->get(factory));
  if (game && droppedPartUpgrade && game->effectFlags.count("abomination_upgrades"))
    for (auto part : Random.permutation<BodyPart>())
      if (numGood(part) > 0 && bodyPartCanBeDropped(part))
        if (auto item = getBodyPartItem(name, part, game->getContentFactory())) {
          setBodyPartUpgrade(item.get(), part, std::move(*droppedPartUpgrade), factory);
          ret.push_back(std::move(item));
          break;
        }
  return ret;
}

void Body::affectPosition(Position position) {
  if (material == BodyMaterialId("FIRE"))
    position.fireDamage(10, nullptr);
}

static void youHit(const Creature* c, BodyPart part, const Attack& attack, const ContentFactory* factory) {
  if (auto& elem = factory->attrInfo.at(attack.damageType).bodyPartInjury)
    c->verb(TStringId("YOUR_BODY_PART_IS"), TStringId("HIS_BODY_PART_IS"), getName(part), *elem);
  else
    switch (part) {
      case BodyPart::BACK:
          switch (attack.type) {
            case AttackType::SHOOT:
              c->verb(TStringId("YOU_ARE_SHOT_IN_THE"), TStringId("IS_SHOT_IN_THE"),
                  TString(TStringId("BACK_BODY_PART")));
              break;
            case AttackType::BITE:
              c->verb(TStringId("YOU_ARE_BITTEN_IN_THE_NECK"), TStringId("IS_BITTEN_IN_THE_NECK"));
              break;
            case AttackType::CUT:
              c->verb(TStringId("YOUR_THROAT_IS_SLIT"), TStringId("HIS_THROAT_IS_SLIT"));
              break;
            case AttackType::CRUSH:
              c->verb(TStringId("YOUR_BODY_PART_IS_CRUSHED"), TStringId("HIS_BODY_PART_IS_CRUSHED"),
                  TString(TStringId("SPINE_BODY_PART")));
              break;
            case AttackType::HIT:
              c->verb(TStringId("YOUR_BODY_PART_IS_BROKEN"), TStringId("HIS_BODY_PART_IS_BROKEN"),
                  TString(TStringId("NECK_BODY_PART")));
              break;
            case AttackType::STAB:
              c->verb(TStringId("YOU_ARE_STABBED_IN_THE"), TStringId("IS_STABBED_IN_THE"),
                  TString(TStringId("BACK_BODY_PART")));
              break;
            case AttackType::SPELL:
              c->verb(TStringId("YOU_ARE_RIPPED_TO_PIECES"), TStringId("IS_RIPPED_TO_PIECES"));
              break;
          }
          break;
      case BodyPart::HEAD:
          switch (attack.type) {
            case AttackType::SHOOT:
              c->verb(TStringId("YOU_ARE_SHOT_IN_THE"), TStringId("IS_SHOT_IN_THE"),
                  Random.choose(TString(TStringId("EYE_BODY_PART")), TString(TStringId("NECK_BODY_PART")),
                  TString(TStringId("NECK_BODY_PART")))
              );
              break;
            case AttackType::BITE:
              c->verb(TStringId("YOUR_BODY_PART_IS_BITTEN_OFF"), TStringId("HIS_BODY_PART_IS_BITTEN_OFF"),
                  TString(TStringId("HEAD_BODY_PART")));
              break;
            case AttackType::CUT:
              c->verb(TStringId("YOUR_BODY_PART_IS_CHOPPED_OFF"), TStringId("HIS_BODY_PART_IS_CHOPPED_OFF"),
                  TString(TStringId("HEAD_BODY_PART")));
              break;
            case AttackType::CRUSH:
              c->verb(TStringId("YOUR_SKULL_IS_SHATTERED"), TStringId("HIS_SKULL_IS_SHATTERED"));
              break;
            case AttackType::HIT:
              c->verb(TStringId("YOUR_BODY_PART_IS_BROKEN"), TStringId("HIS_BODY_PART_IS_BROKEN"),
                  TString(TStringId("NECK_BODY_PART")));
              break;
            case AttackType::STAB:
              c->verb(TStringId("YOU_ARE_STABBED_IN_THE"), TStringId("IS_STABBED_IN_THE"), TString(TStringId("EYE_BODY_PART")));
              break;
            case AttackType::SPELL:
              c->verb(TStringId("YOUR_BODY_PART_IS_RIPPED_TO_PIECES"), TStringId("HIS_BODY_PART_IS_RIPPED_TO_PIECES"),
                  TString(TStringId("HEAD_BODY_PART")));
              break;
          }
          break;
      case BodyPart::TORSO:
          switch (attack.type) {
            case AttackType::SHOOT:
              c->verb(TStringId("YOU_ARE_SHOT_IN_THE"), TStringId("IS_SHOT_IN_THE"), TString(TStringId("HEART")));
              break;
            case AttackType::BITE:
              c->verb(TStringId("YOUR_INTERNAL_ORGANS_ARE_RIPPED_OUT"), TStringId("HIS_INTERNAL_ORGANS_ARE_RIPPED_OUT"));
              break;
            case AttackType::CUT:
              c->verb(TStringId("YOU_ARE_CUT_IN_HALF"), TStringId("IS_CUT_IN_HALF"));
              break;
            case AttackType::STAB:
              c->verb(TStringId("YOU_ARE_STABBED_IN_THE"), TStringId("IS_STABBED_IN_THE"),
                  Random.choose(TString(TStringId("STOMACH")), TString(TStringId("HEART"))));
              break;
            case AttackType::CRUSH:
              c->verb(TStringId("YOUR_INTERNAL_ORGANS_ARE_CRUSHED"), TStringId("HIS_INTERNAL_ORGANS_ARE_CRUSHED"));
              break;
            case AttackType::HIT:
              c->verb(TStringId("YOUR_STOMACH_RECEIVES_A_DEADLY_BLOW"), TStringId("HIS_STOMACH_RECEIVES_A_DEADLY_BLOW"));
              break;
            case AttackType::SPELL:
              c->verb(TStringId("YOU_ARE_RIPPED_TO_PIECES"), TStringId("IS_RIPPED_TO_PIECES"));
              break;
          }
          break;
      case BodyPart::ARM:
          switch (attack.type) {
            case AttackType::SHOOT:
              c->verb(TStringId("YOU_ARE_SHOT_IN_THE"), TStringId("IS_SHOT_IN_THE"), TString(TStringId("ARM_BODY_PART")));
              break;
            case AttackType::BITE:
              c->verb(TStringId("YOUR_BODY_PART_IS_BITTEN_OFF"), TStringId("HIS_BODY_PART_IS_BITTEN_OFF"),
                  TString(TStringId("ARM_BODY_PART")));
              break;
            case AttackType::CUT:
              c->verb(TStringId("YOUR_BODY_PART_IS_CHOPPED_OFF"), TStringId("HIS_BODY_PART_IS_CHOPPED_OFF"),
                  TString(TStringId("ARM_BODY_PART")));
              break;
            case AttackType::STAB:
              c->verb(TStringId("YOU_ARE_STABBED_IN_THE"), TStringId("IS_STABBED_IN_THE"),
                  TString(TStringId("ARM_BODY_PART")));
              break;
            case AttackType::CRUSH:
              c->verb(TStringId("YOUR_BODY_PART_IS_CRUSHED"), TStringId("HIS_BODY_PART_IS_CRUSHED"),
                  TString(TStringId("ARM_BODY_PART")));
              break;
            case AttackType::HIT:
              c->verb(TStringId("YOUR_BODY_PART_IS_BROKEN"), TStringId("HIS_BODY_PART_IS_BROKEN"),
                  TString(TStringId("ARM_BODY_PART")));
              break;
            case AttackType::SPELL:
              c->verb(TStringId("YOUR_BODY_PART_IS_RIPPED_TO_PIECES"), TStringId("HIS_BODY_PART_IS_RIPPED_TO_PIECES"),
                  TString(TStringId("ARM_BODY_PART")));
              break;
          }
          break;
      case BodyPart::WING:
          switch (attack.type) {
            case AttackType::SHOOT:
              c->verb(TStringId("YOU_ARE_SHOT_IN_THE"), TStringId("IS_SHOT_IN_THE"), TString(TStringId("WING_BODY_PART")));
              break;
            case AttackType::BITE:
              c->verb(TStringId("YOUR_BODY_PART_IS_BITTEN_OFF"), TStringId("HIS_BODY_PART_IS_BITTEN_OFF"),
                  TString(TStringId("WING_BODY_PART")));
              break;
            case AttackType::CUT:
              c->verb(TStringId("YOUR_BODY_PART_IS_CHOPPED_OFF"), TStringId("HIS_BODY_PART_IS_CHOPPED_OFF"),
                  TString(TStringId("WING_BODY_PART")));
              break;
            case AttackType::STAB:
              c->verb(TStringId("YOU_ARE_STABBED_IN_THE"), TStringId("IS_STABBED_IN_THE"),
                  TString(TStringId("WING_BODY_PART")));
              break;
            case AttackType::CRUSH:
              c->verb(TStringId("YOUR_BODY_PART_IS_CRUSHED"), TStringId("HIS_BODY_PART_IS_CRUSHED"),
                  TString(TStringId("WING_BODY_PART")));
              break;
            case AttackType::HIT:
              c->verb(TStringId("YOUR_BODY_PART_IS_BROKEN"), TStringId("HIS_BODY_PART_IS_BROKEN"),
                  TString(TStringId("WING_BODY_PART")));
              break;
            case AttackType::SPELL:
              c->verb(TStringId("YOUR_BODY_PART_IS_RIPPED_TO_PIECES"), TStringId("HIS_BODY_PART_IS_RIPPED_TO_PIECES"),
                  TString(TStringId("WING_BODY_PART")));
              break;
          }
          break;
      case BodyPart::LEG:
          switch (attack.type) {
            case AttackType::SHOOT:
              c->verb(TStringId("YOU_ARE_SHOT_IN_THE"), TStringId("IS_SHOT_IN_THE"), TString(TStringId("LEG_BODY_PART")));
              break;
            case AttackType::BITE:
              c->verb(TStringId("YOUR_BODY_PART_IS_BITTEN_OFF"), TStringId("HIS_BODY_PART_IS_BITTEN_OFF"),
                  TString(TStringId("LEG_BODY_PART")));
              break;
            case AttackType::CUT:
              c->verb(TStringId("YOUR_BODY_PART_IS_CHOPPED_OFF"), TStringId("HIS_BODY_PART_IS_CHOPPED_OFF"),
                  TString(TStringId("LEG_BODY_PART")));
              break;
            case AttackType::STAB:
              c->verb(TStringId("YOU_ARE_STABBED_IN_THE"), TStringId("IS_STABBED_IN_THE"),
                  TString(TStringId("LEG_BODY_PART")));
              break;
            case AttackType::CRUSH:
              c->verb(TStringId("YOUR_BODY_PART_IS_CRUSHED"), TStringId("HIS_BODY_PART_IS_CRUSHED"),
                  TString(TStringId("LEG_BODY_PART")));
              break;
            case AttackType::HIT:
              c->verb(TStringId("YOUR_BODY_PART_IS_BROKEN"), TStringId("HIS_BODY_PART_IS_BROKEN"),
                  TString(TStringId("LEG_BODY_PART")));
              break;
            case AttackType::SPELL:
              c->verb(TStringId("YOUR_BODY_PART_IS_RIPPED_TO_PIECES"), TStringId("HIS_BODY_PART_IS_RIPPED_TO_PIECES"),
                  TString(TStringId("LEG_BODY_PART")));
              break;
          }
          break;
      }
}

Body::DamageResult Body::takeDamage(const Attack& attack, Creature* creature, double damage) {
  PROFILE;
  bleed(creature, damage);
  auto factory = creature->getGame()->getContentFactory();
  if (auto part = getBodyPart(factory))
    if (isPartDamaged(*part, damage, factory)) {
      youHit(creature, *part, attack, factory);
      if (injureBodyPart(creature, *part,
          contains({AttackType::CUT, AttackType::BITE}, attack.type) && bodyPartCanBeDropped(*part))) {
        creature->you(MsgType::DIE);
        creature->dieWithAttacker(attack.attacker);
        return Body::KILLED;
      }
      creature->addEffect(BuffId("BLEEDING"), 50_visible);
      if (health <= 0)
        health = 0.1;
      creature->updateViewObject(factory);
      return Body::HURT;
    }
  if (health <= 0) {
    creature->verb(TStringId("YOU_ARE_CRITICALLY_WOUNDED"), TStringId("IS_CRITICALLY_WOUNDED"));
    creature->you(MsgType::DIE);
    creature->dieWithAttacker(attack.attacker);
    return Body::KILLED;
  } else
  if (health < 0.5) {
    creature->verb(TStringId("YOU_ARE_CRITICALLY_WOUNDED"), TStringId("IS_CRITICALLY_WOUNDED"));
    return Body::HURT;
  } else {
    if (hasAnyHealth(factory))
      creature->verb(TStringId("YOU_ARE_WOUNDED"), TStringId("IS_WOUNDED"));
    else if (attack.effect.empty()) {
      creature->verb(TStringId("YOU_ARE_NOT_HURT"), TStringId("IS_NOT_HURT"));
      return Body::NOT_HURT;
    }
    return Body::HURT;
  }
}

void Body::getBadAdjectives(vector<AdjectiveInfo>& ret) const {
  if (health < 1)
    ret.push_back({TStringId("WOUNDED"), TString()});
  for (BodyPart part : ENUM_ALL(BodyPart))
    if (int num = numInjured(part))
      ret.push_back({TSentence("INJURED_BODY_PART", getPluralText(part, num)), TString()});
  for (BodyPart part : ENUM_ALL(BodyPart))
    if (int num = numLost(part))
      ret.push_back({TSentence("LOST_BODY_PART", getPluralText(part, num)), TString()});
}

BodySize Body::getSize() const {
  return size;
}

bool Body::tick(const Creature* c) {
  if (fallsApartDueToLostBodyParts(c->getGame()->getContentFactory())) {
    c->verb(TStringId("YOU_FALL_APART"), TStringId("FALLS_APART"));
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
  return 1 - min<double>(20, gone) / min<double>(20, total);
}

void Body::updateViewObject(ViewObject& obj, const ContentFactory* factory) const {
  if (hasAnyHealth(factory))
    obj.setAttribute(ViewObject::Attribute::HEALTH, health);
  else
    obj.setAttribute(ViewObject::Attribute::HEALTH, getBodyPartHealth());
  obj.setModifier(ViewObjectModifier::HEALTH_BAR);
  if (hasHealth(HealthType::SPIRIT, factory))
    obj.setModifier(ViewObject::Modifier::SPIRIT_DAMAGE);
}

bool Body::heal(Creature* c, double amount) {
  if (health < 1) {
    health = min(1., health + amount);
    auto factory = c->getGame()->getContentFactory();
    updateViewObject(c->modViewObject(), factory);
    if (health >= 1) {
      health = 1;
      return true;
    }
  }
  return false;
}

bool Body::isIntrinsicallyAffected(LastingOrBuff l, const ContentFactory* factory) const {
  auto ret = factory->bodyMaterials.at(material).intrinsicallyAffected.count(l);
  if (l == LastingEffect::FLYING)
    ret = ret || numGood(BodyPart::WING) >= 2;
  return ret;
}

bool Body::isImmuneTo(LastingOrBuff l, const ContentFactory* factory) const {
  auto ret = factory->bodyMaterials.at(material).immuneTo.count(l);
  if (isOneOf(l, LastingEffect::RAGE, LastingEffect::PANIC, LastingEffect::TELEPATHY, LastingEffect::INSANITY))
    ret = ret || !hasBrain(factory);
  return ret;
}

void Body::bleed(Creature* c, double amount) {
  auto factory = c->getGame()->getContentFactory();
  if (hasAnyHealth(factory)) {
    health -= amount;
    c->updateViewObject(factory);
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

bool Body::isKilledByBoulder(const ContentFactory* factory) const {
  return factory->bodyMaterials.at(material).killedByBoulder;
}

bool Body::isFarmAnimal() const {
  return minionFood;
}

bool Body::canCopulateWith(const ContentFactory* factory) const {
  return isHumanoid() && factory->bodyMaterials.at(material).canCopulate;
}

bool Body::canConsume() const {
  return true;
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

bool Body::hasBrain(const ContentFactory* factory) const {
  return factory->bodyMaterials.at(material).hasBrain;
}

bool Body::burnsIntrinsically(const ContentFactory* factory) const {
  return factory->bodyMaterials.at(material).flamable;
}

bool Body::canPush(const Body& other) {
  return int(size) >= int(other.minPushSize);
}

bool Body::canPerformRituals(const ContentFactory* factory) const {
  return xhumanoid && !isImmuneTo(LastingEffect::TIED_UP, factory);
}

void Body::setCanBeCaptured(bool value) {
  canCapture = value;
}

bool Body::canBeCaptured(const ContentFactory* factory) const {
  return hasAnyHealth(factory) && (!!canCapture ? *canCapture : !isImmuneTo(LastingEffect::TIED_UP, factory));
}

bool Body::isUndead(const ContentFactory* factory) const {
  return factory->bodyMaterials.at(material).undead;
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

double Body::getDeathSoundPitch() const {
  switch (size) {
    case Body::Size::HUGE: return 0.6;
    case Body::Size::LARGE: return 0.9;
    case Body::Size::MEDIUM: return 1.3;
    case Body::Size::SMALL: return 3.0;
  }
}

optional<Sound> Body::getDeathSound() const {
  if (!deathSound)
    return none;
  else
    return Sound(*deathSound).setPitch(getDeathSoundPitch());
}

optional<Sound> Body::rollAmbientSound() const {
  if (ambientSound && Random.chance(ambientSound->first))
    return ambientSound->second;
  return none;
}

optional<AnimationId> Body::getDeathAnimation(const ContentFactory* factory) const {
  if (isHumanoid() && hasAnyHealth(factory))
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
        body->setDeathSound(SoundId("HUMANOID_DEATH"));
        body->setHumanoid(true);
        break;
      case BodyType::HumanoidLike:
        body->setHumanoidBodyParts(getDefaultIntrinsicDamage(size));
        body->setDeathSound(SoundId("BEAST_DEATH"));
        break;
      case BodyType::Bird:
        body->setBirdBodyParts(getDefaultIntrinsicDamage(size));
        body->setDeathSound(SoundId("BEAST_DEATH"));
        break;
      case BodyType::FourLegged:
        body->setHorseBodyParts(getDefaultIntrinsicDamage(size));
        body->setDeathSound(SoundId("BEAST_DEATH"));
        break;
      default:
        body->setDeathSound(SoundId("BEAST_DEATH"));
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
