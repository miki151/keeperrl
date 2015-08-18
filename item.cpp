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
#include "view_object.h"
#include "model.h"
#include "player_message.h"
#include "fire.h"
#include "item_attributes.h"

template <class Archive> 
void Item::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(UniqueEntity) & SUBCLASS(Renderable)
    & SVAR(attributes)
    & SVAR(discarded)
    & SVAR(shopkeeper)
    & SVAR(fire);
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


Item::Item(const ItemAttributes& attr) : Renderable(ViewObject(*attr.viewId, ViewLayer::ITEM, *attr.name)),
    attributes(attr), fire(*attr.weight, attr.flamability) {
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
  if (attributes->lastingEffect)
    c->addPermanentEffect(*attributes->lastingEffect);
}

void Item::onUnequip(Creature* c) {
  onUnequipSpecial(c);
  if (attributes->lastingEffect)
    c->removePermanentEffect(*attributes->lastingEffect);
}

void Item::setOnFire(double amount, Position position) {
  bool burning = fire->isBurning();
  string noBurningName = getTheName();
  fire->set(amount);
  if (!burning && fire->isBurning()) {
    position.globalMessage(noBurningName + " catches fire");
    modViewObject().setAttribute(ViewObject::Attribute::BURNING, fire->getSize());
  }
}

double Item::getFireSize() const {
  return fire->getSize();
}

void Item::tick(double time, Position position) {
  if (fire->isBurning()) {
    Debug() << getName() << " burning " << fire->getSize();
    position.setOnFire(fire->getSize());
    modViewObject().setAttribute(ViewObject::Attribute::BURNING, fire->getSize());
    fire->tick();
    if (!fire->isBurning()) {
      position.globalMessage(getTheName() + " burns out");
      discarded = true;
    }
  }
  specialTick(time, position);
}

void Item::onHitSquareMessage(Position pos, int numItems) {
  if (attributes->fragile) {
    pos.globalMessage(
        getPluralTheNameAndVerb(numItems, "crashes", "crash") + " on the " + pos.getName(), "You hear a crash");
    discarded = true;
  } else
    pos.globalMessage(getPluralTheNameAndVerb(numItems, "hits", "hit") + " the " + pos.getName());
}

void Item::onHitCreature(Creature* c, const Attack& attack, int numItems) {
  if (attributes->fragile) {
    c->you(attributes->plural ? MsgType::ITEM_CRASHES_PLURAL : MsgType::ITEM_CRASHES,
        getPluralTheName(numItems));
    discarded = true;
  } else
    c->you(attributes->plural ? MsgType::HIT_THROWN_ITEM_PLURAL : MsgType::HIT_THROWN_ITEM,
        getPluralTheName(numItems));
  if (c->takeDamage(attack))
    return;
  if (attributes->effect && getClass() == ItemClass::POTION) {
    Effect::applyToCreature(c, *attributes->effect, EffectStrength::NORMAL);
  }
}

double Item::getApplyTime() const {
  return attributes->applyTime;
}

double Item::getWeight() const {
  return *attributes->weight;
}

string Item::getDescription() const {
  return attributes->description;
}

ItemClass Item::getClass() const {
  return *attributes->itemClass;
}

int Item::getPrice() const {
  return attributes->price;
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
  return attributes->trapType;
}

optional<CollectiveResourceId> Item::getResourceId() const {
  return attributes->resourceId;
}

void Item::apply(Creature* c) {
  if (attributes->itemClass == ItemClass::SCROLL)
    c->getLevel()->getModel()->getStatistics().add(StatId::SCROLL_READ);
  if (attributes->effect)
    Effect::applyToCreature(c, *attributes->effect, EffectStrength::NORMAL);
  if (attributes->uses > -1 && --attributes->uses == 0) {
    discarded = true;
    if (attributes->usedUpMsg)
      c->playerMessage(getTheName() + " is used up.");
  }
}

string Item::getApplyMsgThirdPerson(bool blind) const {
  switch (getClass()) {
    case ItemClass::SCROLL: return "reads " + getAName(false, blind);
    case ItemClass::POTION: return "drinks " + getAName(false, blind);
    case ItemClass::BOOK: return "reads " + getAName(false, blind);
    case ItemClass::TOOL: return "applies " + getAName(false, blind);
    case ItemClass::FOOD: return "eats " + getAName(false, blind);
    default: FAIL << "Bad type for applying " << (int)getClass();
  }
  return "";
}

string Item::getApplyMsgFirstPerson(bool blind) const {
  switch (getClass()) {
    case ItemClass::SCROLL: return "read " + getAName(false, blind);
    case ItemClass::POTION: return "drink " + getAName(false, blind);
    case ItemClass::BOOK: return "read " + getAName(false, blind);
    case ItemClass::TOOL: return "apply " + getAName(false, blind);
    case ItemClass::FOOD: return "eat " + getAName(false, blind);
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
  attributes->name = n;
}

string Item::getName(bool plural, bool blind) const {
  string suff;
  if (fire->isBurning())
    suff.append(" (burning)");
  if (getShopkeeper())
    suff += " (" + toString(getPrice()) + (plural ? " zorkmids each)" : " zorkmids)");
  if (blind)
    return getBlindName(plural);
  return getVisibleName(plural) + suff;
}

string Item::getAName(bool getPlural, bool blind) const {
  if (attributes->noArticle || getPlural)
    return getName(getPlural, blind);
  else
    return addAParticle(getName(getPlural, blind));
}

string Item::getTheName(bool getPlural, bool blind) const {
  string the = (attributes->noArticle || getPlural) ? "" : "the ";
  return the + getName(getPlural, blind);
}

string Item::getPluralTheName(int count) const {
  if (count > 1)
    return toString(count) + " " + getTheName(true);
  else
    return getTheName(false);
}

string Item::getPluralTheNameAndVerb(int count, const string& verbSingle, const string& verbPlural) const {
  return getPluralTheName(count) + " " + (count > 1 ? verbPlural : verbSingle);
}

string Item::getVisibleName(bool getPlural) const {
  if (!getPlural)
    return *attributes->name;
  else {
    if (attributes->plural)
      return *attributes->plural;
    else
      return *attributes->name + "s";
  }
}

static string withSign(int a) {
  if (a >= 0)
    return "+" + toString(a);
  else
    return toString(a);
}

string Item::getArtifactName() const {
  CHECK(attributes->artifactName);
  return *attributes->artifactName;
}

string Item::getModifiers(bool shorten) const {
  string artStr;
  if (attributes->artifactName) {
    artStr = *attributes->artifactName;
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
      if (attributes->modifiers[mod] != 0)
        printMod.insert(mod);
  vector<string> attrStrings;
  for (auto mod : printMod)
    attrStrings.push_back(withSign(attributes->modifiers[mod]) +
        (shorten ? "" : " " + Creature::getModifierName(mod)));
  if (!shorten)
    for (auto attr : ENUM_ALL(AttrType))
      if (attributes->attrs[attr] != 0)
        attrStrings.push_back(withSign(attributes->attrs[attr]) + " " + Creature::getAttrName(attr));
  string attrString = combine(attrStrings, true);
  if (!attrString.empty())
    attrString = " (" + attrString + ")";
  if (attributes->uses > -1 && attributes->displayUses) 
    attrString += " (" + toString(attributes->uses) + " uses left)";
  return artStr + attrString;
}

string Item::getShortName(bool blind, bool noSuffix) const {
  if (blind && attributes->blindName)
    return getBlindName(false);
  string name = getModifiers(true);
  if (attributes->shortName)
    name = *attributes->shortName + " " + name;
  if (getShopkeeper() && !noSuffix)
    name = name + " (unpaid)";
  if (fire->isBurning() && !noSuffix)
    name.append(" (burning)");
  return name;
}

string Item::getNameAndModifiers(bool getPlural, bool blind) const {
  return getName(getPlural, blind) + getModifiers();
}

string Item::getBlindName(bool plural) const {
  if (attributes->blindName)
    return *attributes->blindName + (plural ? "s" : "");
  else
    return getName(plural, false);
}

bool Item::isDiscarded() {
  return discarded;
}

optional<EffectType> Item::getEffectType() const {
  return attributes->effect;
}

optional<EffectType> Item::getAttackEffect() const {
  return attributes->attackEffect;
}

bool Item::canEquip() const {
  return !!attributes->equipmentSlot;
}

EquipmentSlot Item::getEquipmentSlot() const {
  CHECK(canEquip());
  return *attributes->equipmentSlot;
}

void Item::addModifier(ModifierType type, int value) {
  attributes->modifiers[type] += value;
}

int Item::getModifier(ModifierType type) const {
  CHECK(abs(attributes->modifiers[type]) < 10000) << EnumInfo<ModifierType>::getString(type) << " "
      << attributes->modifiers[type] << " " << getName();
  return attributes->modifiers[type];
}

int Item::getAttr(AttrType type) const {
  return attributes->attrs[type];
}
 
AttackType Item::getAttackType() const {
  return attributes->attackType;
}

bool Item::isWieldedTwoHanded() const {
  return attributes->twoHanded;
}

int Item::getMinStrength() const {
  return 10 + getWeight();
}

