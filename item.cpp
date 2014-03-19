#include "stdafx.h"

#include "item.h"
#include "creature.h"
#include "level.h"
#include "statistics.h"

template <class Archive> 
void Item::serialize(Archive& ar, const unsigned int version) {
  ItemAttributes::serialize(ar, version);
  ar& SUBCLASS(UniqueEntity)
    & BOOST_SERIALIZATION_NVP(viewObject)
    & BOOST_SERIALIZATION_NVP(discarded)
    & BOOST_SERIALIZATION_NVP(inspected)
    & BOOST_SERIALIZATION_NVP(shopkeeper)
    & BOOST_SERIALIZATION_NVP(fire);
}

SERIALIZABLE(Item);

template <class Archive> 
void Item::CorpseInfo::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(canBeRevived)
    & BOOST_SERIALIZATION_NVP(hasHead)
    & BOOST_SERIALIZATION_NVP(isSkeleton);
}

SERIALIZABLE(Item::CorpseInfo);


Item::Item(ViewObject o, const ItemAttributes& attr)
    : ItemAttributes(attr), viewObject(o), inspected(everythingIdentified), fire(*weight, flamability) {
}

Item::~Item() {
}

void Item::identifyEverything() {
  everythingIdentified = true;
}

bool Item::isEverythingIdentified() {
  return everythingIdentified;
}

bool Item::everythingIdentified = false;
 
vector<Item*> Item::extractRefs(const vector<PItem>& item) {
  vector<Item*> ref(item.size());
  transform(item.begin(), item.end(), ref.begin(), [](const PItem& it) { return it.get();});
  return ref;
}

static set<string> ident;
bool Item::isIdentified(const string& name) {
  return everythingIdentified || ident.count(name);
}

ItemPredicate Item::effectPredicate(EffectType type) {
  return [type](const Item* item) { return item->getEffectType() == type; };
}

ItemPredicate Item::typePredicate(ItemType type) {
  return [type](const Item* item) { return item->getType() == type; };
}

ItemPredicate Item::typePredicate(vector<ItemType> type) {
  return [&type](const Item* item) { return contains(type, item->getType()); };
}

ItemPredicate Item::namePredicate(const string& name) {
  return [name](const Item* item) { return item->getName() == name; };
}

vector<pair<string, vector<Item*>>> Item::stackItems(vector<Item*> items, function<string(const Item*)> suffix) {
  map<string, vector<Item*>> stacks = groupBy<Item*, string>(items, [suffix](const Item* item) {
        return item->getNameAndModifiers() + suffix(item);
      });
  vector<pair<string, vector<Item*>>> ret;
  for (auto elem : stacks)
    if (elem.second.size() > 1)
      ret.emplace_back(
          convertToString<int>(elem.second.size()) + " " 
          + elem.second[0]->getNameAndModifiers(true) + suffix(elem.second[0]), elem.second);
    else
      ret.push_back(elem);
  return ret;
}

void Item::identify(const string& name) {
  Debug() << "Identify " << name;
  ident.insert(name);
}

void Item::identify() {
  identify(*name);
  inspected = true;
}

bool Item::canIdentify() const {
  return identifiable;
}

bool Item::isIdentified() const {
  return isIdentified(*name);
}

void Item::onEquip(Creature* c) {
  if (identifyOnEquip && c->isPlayer())
    identify();
  onEquipSpecial(c);
}

void Item::onUnequip(Creature* c) {
  onUnequipSpecial(c);
}

void Item::setOnFire(double amount, const Level* level, Vec2 position) {
  bool burning = fire.isBurning();
  string noBurningName = getTheName();
  fire.set(amount);
  if (!burning && fire.isBurning()) {
    level->globalMessage(position, noBurningName + " catches fire.");
    viewObject.setBurning(fire.getSize());
  }
}

double Item::getFireSize() const {
  return fire.getSize();
}

void Item::tick(double time, Level* level, Vec2 position) {
  if (fire.isBurning()) {
    Debug() << getName() << " burning " << fire.getSize();
    level->getSquare(position)->setOnFire(fire.getSize());
    viewObject.setBurning(fire.getSize());
    fire.tick(level, position);
    if (!fire.isBurning()) {
      level->globalMessage(position, getTheName() + " burns out");
      discarded = true;
    }
  }
  specialTick(time, level, position);
}

void Item::onHitSquareMessage(Vec2 position, Square* s, bool plural) {
  if (fragile) {
    s->getConstLevel()->globalMessage(position,
        getTheName(plural) + chooseElem<string>({" crashes", " crash"}, int(plural)) + " on the " + s->getName(),
        "You hear a crash");
    discarded = true;
  } else
    s->getConstLevel()->globalMessage(position, getTheName(plural) +
        chooseElem<string>({" hits", " hit"}, int(plural)) + " the " + s->getName());
}

void Item::onHitCreature(Creature* c, const Attack& attack, bool plural) {
  if (fragile) {
    c->you(plural ? MsgType::ITEM_CRASHES_PLURAL : MsgType::ITEM_CRASHES, getTheName(plural));
    discarded = true;
  } else
    c->you(plural ? MsgType::HIT_THROWN_ITEM_PLURAL : MsgType::HIT_THROWN_ITEM, getTheName(plural));
  if (c->takeDamage(attack))
    return;
  if (effect && getType() == ItemType::POTION) {
    Effect::applyToCreature(c, *effect, EffectStrength::NORMAL);
    if (c->getLevel()->playerCanSee(c->getPosition()))
      identify();
  }
}

double Item::getApplyTime() const {
  return applyTime;
}

double Item::getWeight() const {
  return *weight;
}

string Item::getDescription() const {
  return description;
}

const ViewObject& Item::getViewObject() const {
  return viewObject;
}

ItemType Item::getType() const {
  return *type;
}

int Item::getPrice() const {
  return price;
}

void Item::setShopkeeper(const Creature* s) {
  shopkeeper = s;
}

const Creature* Item::getShopkeeper() const {
  if (!shopkeeper || shopkeeper->isDead())
    return nullptr;
  else
    return shopkeeper;
}

Optional<TrapType> Item::getTrapType() const {
  return trapType;
}

void Item::apply(Creature* c, Level* l) {
  if (type == ItemType::SCROLL)
    Statistics::add(StatId::SCROLL_READ);
  if (identifyOnApply && l->playerCanSee(c->getPosition()))
    identify(*name);
  if (effect)
    Effect::applyToCreature(c, *effect, EffectStrength::NORMAL);
  if (uses > -1 && --uses == 0) {
    discarded = true;
    if (usedUpMsg)
      c->privateMessage(getTheName() + " is used up.");
  }
}

string Item::getApplyMsgThirdPerson() const {
  switch (getType()) {
    case ItemType::SCROLL: return "reads " + getAName();
    case ItemType::POTION: return "drinks " + getAName();
    case ItemType::BOOK: return "reads " + getAName();
    case ItemType::TOOL: return "applies " + getAName();
    case ItemType::FOOD: return "eats " + getAName();
    default: FAIL << "Bad type for applying " << (int)getType();
  }
  return "";
}

string Item::getApplyMsgFirstPerson() const {
  switch (getType()) {
    case ItemType::SCROLL: return "read " + getAName();
    case ItemType::POTION: return "drink " + getAName();
    case ItemType::BOOK: return "read " + getAName();
    case ItemType::TOOL: return "apply " + getAName();
    case ItemType::FOOD: return "eat " + getAName();
    default: FAIL << "Bad type for applying " << (int)getType();
  }
  return "";
}

string Item::getNoSeeApplyMsg() const {
  switch (getType()) {
    case ItemType::SCROLL: return "You hear someone reading";
    case ItemType::POTION: return "";
    case ItemType::BOOK: return "You hear someone reading ";
    case ItemType::TOOL: return "";
    case ItemType::FOOD: return "";
    default: FAIL << "Bad type for applying " << (int)getType();
  }
  return "";
}

void Item::setName(const string& n) {
  name = n;
}

string Item::getName(bool plural, bool blind) const {
  string suff = uses > -1 && displayUses && inspected ? string(" (") + convertToString(uses) + " uses left)" : "";
  if (fire.isBurning())
    suff.append(" (burning)");
  if (getShopkeeper())
    suff += " (" + convertToString(getPrice()) + (plural ? " zorkmids each)" : " zorkmids)");
  if (blind)
    return getBlindName(plural);
  if (isIdentified(*name))
    return getRealName(plural) + suff;
  else
    return getVisibleName(plural) + suff;
}

string Item::getAName(bool getPlural, bool blind) const {
  if (noArticle || getPlural)
    return getName(getPlural, blind);
  else
    return addAParticle(getName(getPlural, blind));
}

string Item::getTheName(bool getPlural, bool blind) const {
  string the = noArticle ? "" : "the ";
  return the + getName(getPlural, blind);
}

string Item::getVisibleName(bool getPlural) const {
  if (!getPlural)
    return *name;
  else {
    if (plural)
      return *plural;
    else
      return *name + "s";
  }
}

static string withSign(int a) {
  if (a >= 0)
    return "+" + convertToString(a);
  else
    return convertToString(a);
}

string Item::getArtifactName() const {
  CHECK(artifactName);
  return *artifactName;
}

string Item::getNameAndModifiers(bool getPlural, bool blind) const {
  if (inspected) {
    string artStr = artifactName ? " named " + *artifactName : "";
    if (getType() == ItemType::WEAPON)
      return getName(getPlural, blind) + artStr + 
          " (" + withSign(toHit) + " accuracy, " + withSign(damage) + " damage)";
    if (getType() == ItemType::RANGED_WEAPON)
      return getName(getPlural, blind) + artStr + 
          " (" + withSign(rangedWeaponAccuracy) + " accuracy)";
    if (getType() == ItemType::ARMOR)
      return getName(getPlural, blind) + artStr + " (" + withSign(defense) + " defense)";
  }
  return getName(getPlural, blind);
}

string Item::getRealName(bool getPlural) const {
  if (!realName)
    return getVisibleName(getPlural);
  if (!getPlural)
    return *realName;
  else {
    if (realPlural)
      return *realPlural;
    else
      return *realName + "s";
  }
}

string Item::getBlindName(bool plural) const {
  if (blindName)
    return *blindName + (plural ? "s" : "");
  else
    return getName(plural, false);
}

bool Item::isDiscarded() {
  return discarded;
}

Optional<EffectType> Item::getEffectType() const {
  return effect;
}

bool Item::canEquip() const {
  return getType() == ItemType::WEAPON 
      || getType() == ItemType::RANGED_WEAPON
      || getType() == ItemType::ARMOR
      || getType() == ItemType::AMULET;
}

EquipmentSlot Item::getEquipmentSlot() const {
  CHECK(canEquip());
  if (getType() == ItemType::WEAPON)
    return EquipmentSlot::WEAPON;
  if (getType() == ItemType::RANGED_WEAPON)
    return EquipmentSlot::RANGED_WEAPON;
  if (getType() == ItemType::AMULET)
    return EquipmentSlot::AMULET;
  if (armorType == ArmorType::BODY_ARMOR)
    return EquipmentSlot::BODY_ARMOR;
  if (armorType == ArmorType::HELMET)
    return EquipmentSlot::HELMET;
  if (armorType == ArmorType::BOOTS)
    return EquipmentSlot::BOOTS;
  FAIL << "other equipment slot";
  return EquipmentSlot::HELMET;
}

int Item::getAccuracy() const {
  return rangedWeaponAccuracy;
}

void Item::addModifier(AttrType attributeType, int value) {
  switch (attributeType) {
    case AttrType::TO_HIT: toHit += value; break;
    case AttrType::THROWN_TO_HIT: thrownToHit += value; break;
    case AttrType::DEFENSE: defense += value; break;
    case AttrType::DAMAGE: damage += value; break;
    case AttrType::THROWN_DAMAGE: thrownDamage += value; break;
    case AttrType::STRENGTH: strength += value; break;
    case AttrType::DEXTERITY: dexterity += value; break;
    case AttrType::SPEED: speed += value; break;
    case AttrType::INV_LIMIT: break;
    default: FAIL << "Attribute not handled";
  }
}

int Item::getModifier(AttrType attributeType) const {
  switch (attributeType) {
    case AttrType::TO_HIT: return toHit;
    case AttrType::THROWN_TO_HIT: return thrownToHit;
    case AttrType::DEFENSE: return defense;
    case AttrType::DAMAGE: return damage;
    case AttrType::THROWN_DAMAGE: return thrownDamage;
    case AttrType::STRENGTH: return strength;
    case AttrType::DEXTERITY: return dexterity;
    case AttrType::SPEED: return speed;
    case AttrType::INV_LIMIT: return 0;
    default: FAIL << "Attribute not handled";
  }
  return 0;
}
  
AttackType Item::getAttackType() const {
  return attackType;
}

bool Item::isWieldedTwoHanded() const {
  return twoHanded;
}
