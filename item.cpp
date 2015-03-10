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

#include "item.h"
#include "creature.h"
#include "level.h"
#include "statistics.h"
#include "effect.h"
#include "square.h"
#include "view_object.h"
#include "model.h"

template <class Archive> 
void Item::serialize(Archive& ar, const unsigned int version) {
  if (version == 0)   // OBSOLETE
    ItemAttributes::serialize(ar, version);
  ar& SUBCLASS(UniqueEntity);
  if (version >= 1)
    ar & SUBCLASS(ItemAttributes);
  ar& SUBCLASS(Renderable)
    & SVAR(discarded)
    & SVAR(shopkeeper)
    & SVAR(fire);
  CHECK_SERIAL;
}

SERIALIZABLE(Item);
SERIALIZATION_CONSTRUCTOR_IMPL(Item);

template <class Archive> 
void Item::CorpseInfo::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(canBeRevived)
    & BOOST_SERIALIZATION_NVP(hasHead)
    & BOOST_SERIALIZATION_NVP(isSkeleton);
}

SERIALIZABLE(Item::CorpseInfo);


Item::Item(const ItemAttributes& attr) : ItemAttributes(attr),
    Renderable(ViewObject(*attr.viewId, ViewLayer::ITEM, *attr.name)), fire(*weight, flamability) {
}

Item::~Item() {
}

string Item::getTrapName(TrapType type) {
  switch (type) {
    case TrapType::BOULDER: return "boulder";
    case TrapType::POISON_GAS: return "poison gas";
    case TrapType::ALARM: return "alarm";
    case TrapType::WEB: return "web";
    case TrapType::SURPRISE: return "surprise";
    case TrapType::TERROR: return "terror";
  }
  FAIL << "wef";
  return "";
}

ItemPredicate Item::effectPredicate(EffectType type) {
  return [type](const Item* item) { return item->getEffectType() == type; };
}

ItemPredicate Item::classPredicate(ItemClass cl) {
  return [cl](const Item* item) { return item->getClass() == cl; };
}

ItemPredicate Item::classPredicate(vector<ItemClass> cl) {
  return [cl](const Item* item) { return contains(cl, item->getClass()); };
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
          toString<int>(elem.second.size()) + " " 
          + elem.second[0]->getNameAndModifiers(true) + suffix(elem.second[0]), elem.second);
    else
      ret.push_back(elem);
  return ret;
}

void Item::onEquip(Creature* c) {
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
    modViewObject().setAttribute(ViewObject::Attribute::BURNING, fire.getSize());
  }
}

double Item::getFireSize() const {
  return fire.getSize();
}

void Item::tick(double time, Level* level, Vec2 position) {
  if (fire.isBurning()) {
    Debug() << getName() << " burning " << fire.getSize();
    level->getSafeSquare(position)->setOnFire(fire.getSize());
    modViewObject().setAttribute(ViewObject::Attribute::BURNING, fire.getSize());
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
  if (effect && getClass() == ItemClass::POTION) {
    Effect::applyToCreature(c, *effect, EffectStrength::NORMAL);
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

ItemClass Item::getClass() const {
  return *itemClass;
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

optional<TrapType> Item::getTrapType() const {
  return trapType;
}

optional<CollectiveResourceId> Item::getResourceId() const {
  return resourceId;
}

void Item::apply(Creature* c, Level* l) {
  if (itemClass == ItemClass::SCROLL)
    l->getModel()->getStatistics().add(StatId::SCROLL_READ);
  if (effect)
    Effect::applyToCreature(c, *effect, EffectStrength::NORMAL);
  if (uses > -1 && --uses == 0) {
    discarded = true;
    if (usedUpMsg)
      c->playerMessage(getTheName() + " is used up.");
  }
}

string Item::getApplyMsgThirdPerson() const {
  switch (getClass()) {
    case ItemClass::SCROLL: return "reads " + getAName();
    case ItemClass::POTION: return "drinks " + getAName();
    case ItemClass::BOOK: return "reads " + getAName();
    case ItemClass::TOOL: return "applies " + getAName();
    case ItemClass::FOOD: return "eats " + getAName();
    default: FAIL << "Bad type for applying " << (int)getClass();
  }
  return "";
}

string Item::getApplyMsgFirstPerson() const {
  switch (getClass()) {
    case ItemClass::SCROLL: return "read " + getAName();
    case ItemClass::POTION: return "drink " + getAName();
    case ItemClass::BOOK: return "read " + getAName();
    case ItemClass::TOOL: return "apply " + getAName();
    case ItemClass::FOOD: return "eat " + getAName();
    default: FAIL << "Bad type for applying " << (int)getClass();
  }
  return "";
}

string Item::getNoSeeApplyMsg() const {
  switch (getClass()) {
    case ItemClass::SCROLL: return "You hear someone reading";
    case ItemClass::POTION: return "";
    case ItemClass::BOOK: return "You hear someone reading ";
    case ItemClass::TOOL: return "";
    case ItemClass::FOOD: return "";
    default: FAIL << "Bad type for applying " << (int)getClass();
  }
  return "";
}

void Item::setName(const string& n) {
  name = n;
}

string Item::getName(bool plural, bool blind) const {
  string suff;
  if (fire.isBurning())
    suff.append(" (burning)");
  if (getShopkeeper())
    suff += " (" + toString(getPrice()) + (plural ? " zorkmids each)" : " zorkmids)");
  if (blind)
    return getBlindName(plural);
  return getVisibleName(plural) + suff;
}

string Item::getAName(bool getPlural, bool blind) const {
  if (noArticle || getPlural)
    return getName(getPlural, blind);
  else
    return addAParticle(getName(getPlural, blind));
}

string Item::getTheName(bool getPlural, bool blind) const {
  string the = (noArticle || getPlural) ? "" : "the ";
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
    return "+" + toString(a);
  else
    return toString(a);
}

string Item::getArtifactName() const {
  CHECK(artifactName);
  return *artifactName;
}

string Item::getModifiers(bool shorten) const {
  string artStr;
  if (artifactName) {
    artStr = *artifactName;
    if (!shorten)
      artStr = " named " + artStr;
  }
  EnumSet<ModifierType> printMod;
  switch (getClass()) {
    case ItemClass::WEAPON:
      printMod.insert(ModifierType::ACCURACY);
      printMod.insert(ModifierType::DAMAGE);
      break;
    case ItemClass::ARMOR:
      printMod.insert(ModifierType::DEFENSE);
      break;
    case ItemClass::RANGED_WEAPON:
    case ItemClass::AMMO:
      printMod.insert(ModifierType::FIRED_ACCURACY);
      break;
    default: break;
  }
  if (!shorten)
    for (auto mod : ENUM_ALL(ModifierType))
      if (modifiers[mod] != 0)
        printMod.insert(mod);
  vector<string> attrStrings;
  for (auto mod : printMod)
    attrStrings.push_back(withSign(modifiers[mod]) + (shorten ? "" : " " + Creature::getModifierName(mod)));
  if (!shorten)
    for (auto attr : ENUM_ALL(AttrType))
      if (attrs[attr] != 0)
        attrStrings.push_back(withSign(attrs[attr]) + " " + Creature::getAttrName(attr));
  string attrString = combine(attrStrings, true);
  if (!attrString.empty())
    attrString = " (" + attrString + ")";
  if (uses > -1 && displayUses) 
    attrString += " (" + toString(uses) + " uses left)";
  return artStr + attrString;
}

string Item::getShortName(bool shortMod) const {
  string name = getModifiers(shortMod);
  if (shortName)
    name = *shortName + " " + name;
  if (getShopkeeper())
    name = name + " (unpaid)";
  return name;
}

string Item::getNameAndModifiers(bool getPlural, bool blind) const {
  return getName(getPlural, blind) + getModifiers();
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

optional<EffectType> Item::getEffectType() const {
  return effect;
}

optional<EffectType> Item::getAttackEffect() const {
  return attackEffect;
}

bool Item::canEquip() const {
  return !!equipmentSlot;
}

EquipmentSlot Item::getEquipmentSlot() const {
  CHECK(canEquip());
  return *equipmentSlot;
}

void Item::addModifier(ModifierType type, int value) {
  modifiers[type] += value;
}

int Item::getModifier(ModifierType type) const {
  return modifiers[type];
}

int Item::getAttr(AttrType type) const {
  return attrs[type];
}
 
AttackType Item::getAttackType() const {
  return attackType;
}

bool Item::isWieldedTwoHanded() const {
  return twoHanded;
}

int Item::getMinStrength() const {
  return 10 + *weight;
}

