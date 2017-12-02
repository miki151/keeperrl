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
#include "game.h"
#include "player_message.h"
#include "fire.h"
#include "item_attributes.h"
#include "view.h"
#include "sound.h"
#include "item_class.h"
#include "corpse_info.h"
#include "equipment.h"
#include "attr_type.h"
#include "attack.h"
#include "lasting_effect.h"
#include "creature_name.h"

template <class Archive> 
void Item::serialize(Archive& ar, const unsigned int version) {
  ar & SUBCLASS(OwnedObject<Item>) & SUBCLASS(UniqueEntity) & SUBCLASS(Renderable);
  ar(attributes, discarded, shopkeeper, fire, classCache, canEquipCache);
}

SERIALIZABLE(Item)
SERIALIZATION_CONSTRUCTOR_IMPL(Item)

Item::Item(const ItemAttributes& attr) : Renderable(ViewObject(*attr.viewId, ViewLayer::ITEM, capitalFirst(*attr.name))),
    attributes(attr), fire(*attr.weight, attr.flamability), canEquipCache(!!attributes->equipmentSlot),
    classCache(*attributes->itemClass) {
}

Item::~Item() {
}

ItemPredicate Item::effectPredicate(Effect type) {
  return [type](WConstItem item) { return item->getEffect() == type; };
}

ItemPredicate Item::classPredicate(ItemClass cl) {
  return [cl](WConstItem item) { return item->getClass() == cl; };
}

ItemPredicate Item::equipmentSlotPredicate(EquipmentSlot slot) {
  return [slot](WConstItem item) { return item->canEquip() && item->getEquipmentSlot() == slot; };
}

ItemPredicate Item::classPredicate(vector<ItemClass> cl) {
  return [cl](WConstItem item) { return cl.contains(item->getClass()); };
}

ItemPredicate Item::namePredicate(const string& name) {
  return [name](WConstItem item) { return item->getName() == name; };
}

ItemPredicate Item::isRangedWeaponPredicate() {
 return [](WConstItem it) { return it->canEquip() && it->getEquipmentSlot() == EquipmentSlot::RANGED_WEAPON;};
}

vector<vector<WItem>> Item::stackItems(vector<WItem> items, function<string(WConstItem)> suffix) {
  map<string, vector<WItem>> stacks = groupBy<WItem, string>(items, [suffix](WConstItem item) {
        return item->getNameAndModifiers() + suffix(item);
      });
  vector<vector<WItem>> ret;
  for (auto& elem : stacks)
    ret.push_back(elem.second);
  return ret;
}

void Item::onEquip(WCreature c) {
  onEquipSpecial(c);
  if (attributes->equipedEffect)
    c->addPermanentEffect(*attributes->equipedEffect);
}

void Item::onUnequip(WCreature c) {
  onUnequipSpecial(c);
  if (attributes->equipedEffect)
    c->removePermanentEffect(*attributes->equipedEffect);
}

void Item::fireDamage(double amount, Position position) {
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

void Item::tick(Position position) {
  if (fire->isBurning()) {
    INFO << getName() << " burning " << fire->getSize();
    position.fireDamage(fire->getSize());
    modViewObject().setAttribute(ViewObject::Attribute::BURNING, fire->getSize());
    fire->tick();
    if (!fire->isBurning()) {
      position.globalMessage(getTheName() + " burns out");
      discarded = true;
    }
  }
  specialTick(position);
}

void Item::onHitSquareMessage(Position pos, int numItems) {
  if (attributes->fragile) {
    pos.globalMessage(getPluralTheNameAndVerb(numItems, "crashes", "crash") + " on the " + pos.getName());
    pos.unseenMessage("You hear a crash");
    discarded = true;
  } else
    pos.globalMessage(getPluralTheNameAndVerb(numItems, "hits", "hit") + " the " + pos.getName());
}

void Item::onHitCreature(WCreature c, const Attack& attack, int numItems) {
  if (attributes->fragile) {
    c->you(numItems > 1 ? MsgType::ITEM_CRASHES_PLURAL : MsgType::ITEM_CRASHES, getPluralTheName(numItems));
    discarded = true;
  } else
    c->you(numItems > 1 ? MsgType::HIT_THROWN_ITEM_PLURAL : MsgType::HIT_THROWN_ITEM, getPluralTheName(numItems));
  if (c->takeDamage(attack))
    return;
  if (attributes->effect && getClass() == ItemClass::POTION) {
    attributes->effect->applyToCreature(c, attack.attacker);
  }
}

TimeInterval Item::getApplyTime() const {
  return attributes->applyTime;
}

double Item::getWeight() const {
  return *attributes->weight;
}

string Item::getDescription() const {
  return attributes->description;
}

const WeaponInfo& Item::getWeaponInfo() const {
  return attributes->weaponInfo;
}

ItemClass Item::getClass() const {
  return classCache;
}

int Item::getPrice() const {
  return attributes->price;
}

bool Item::isShopkeeper(WConstCreature c) const {
  return shopkeeper == c->getUniqueId();
}

void Item::setShopkeeper(WConstCreature s) {
  if (s)
    shopkeeper = s->getUniqueId();
  else
    shopkeeper = none;
}

bool Item::isOrWasForSale() const {
  return !!shopkeeper;
}

optional<TrapType> Item::getTrapType() const {
  return attributes->trapType;
}

optional<CollectiveResourceId> Item::getResourceId() const {
  return attributes->resourceId;
}

void Item::apply(WCreature c, bool noSound) {
  if (attributes->applySound && !noSound)
    c->addSound(*attributes->applySound);
  applySpecial(c);
}

void Item::applySpecial(WCreature c) {
  if (attributes->itemClass == ItemClass::SCROLL)
    c->getGame()->getStatistics().add(StatId::SCROLL_READ);
  if (attributes->effect)
    attributes->effect->applyToCreature(c);
  if (attributes->uses > -1 && --attributes->uses == 0) {
    discarded = true;
    if (attributes->usedUpMsg)
      c->privateMessage(getTheName() + " is used up.");
  }
}

string Item::getApplyMsgThirdPerson(WConstCreature owner) const {
  if (attributes->applyMsgThirdPerson)
    return *attributes->applyMsgThirdPerson;
  switch (getClass()) {
    case ItemClass::SCROLL: return "reads " + getAName(false, owner);
    case ItemClass::POTION: return "drinks " + getAName(false, owner);
    case ItemClass::BOOK: return "reads " + getAName(false, owner);
    case ItemClass::TOOL: return "applies " + getAName(false, owner);
    case ItemClass::FOOD: return "eats " + getAName(false, owner);
    default: FATAL << "Bad type for applying " << (int)getClass();
  }
  return "";
}

string Item::getApplyMsgFirstPerson(WConstCreature owner) const {
  if (attributes->applyMsgFirstPerson)
    return *attributes->applyMsgFirstPerson;
  switch (getClass()) {
    case ItemClass::SCROLL: return "read " + getAName(false, owner);
    case ItemClass::POTION: return "drink " + getAName(false, owner);
    case ItemClass::BOOK: return "read " + getAName(false, owner);
    case ItemClass::TOOL: return "apply " + getAName(false, owner);
    case ItemClass::FOOD: return "eat " + getAName(false, owner);
    default: FATAL << "Bad type for applying " << (int)getClass();
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
    default: FATAL << "Bad type for applying " << (int)getClass();
  }
  return "";
}

void Item::setName(const string& n) {
  attributes->name = n;
}

WCreature Item::getShopkeeper(WConstCreature owner) const {
  if (shopkeeper)
    for (WCreature c : owner->getVisibleCreatures())
      if (c->getUniqueId() == *shopkeeper)
        return c;
  return nullptr;
}

string Item::getName(bool plural, WConstCreature owner) const {
  string suff;
  if (fire->isBurning())
    suff.append(" (burning)");
  if (owner && getShopkeeper(owner))
    suff += " (" + toString(getPrice()) + (plural ? " gold each)" : " gold)");
  if (owner && owner->isAffected(LastingEffect::BLIND))
    return getBlindName(plural);
  return getVisibleName(plural) + suff;
}

string Item::getAName(bool getPlural, WConstCreature owner) const {
  if (attributes->noArticle || getPlural)
    return getName(getPlural, owner);
  else
    return addAParticle(getName(getPlural, owner));
}

string Item::getTheName(bool getPlural, WConstCreature owner) const {
  string the = (attributes->noArticle || getPlural) ? "" : "the ";
  return the + getName(getPlural, owner);
}

string Item::getPluralName(int count) const {
  if (count > 1)
    return toString(count) + " " + getName(true);
  else
    return getName(false);
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

const optional<string>& Item::getArtifactName() const {
  return attributes->artifactName;
}

void Item::setArtifactName(const string& s) {
  attributes->artifactName = s;
}

string Item::getModifiers(bool shorten) const {
  string artStr;
  if (attributes->artifactName) {
    artStr = *attributes->artifactName;
    if (!shorten)
      artStr = " named " + artStr;
  }
  EnumSet<AttrType> printAttr;
  if (!shorten) {
    for (auto attr : ENUM_ALL(AttrType))
      if (attributes->modifiers[attr] != 0)
        printAttr.insert(attr);
  } else
    switch (getClass()) {
      case ItemClass::RANGED_WEAPON:
        printAttr.insert(getRangedWeapon()->getDamageAttr());
        break;
      case ItemClass::WEAPON:
        printAttr.insert(getWeaponInfo().meleeAttackAttr);
        break;
      case ItemClass::ARMOR:
        printAttr.insert(AttrType::DEFENSE);
        break;
      default: break;
    }
  vector<string> attrStrings;
  for (auto attr : printAttr)
    attrStrings.push_back(withSign(attributes->modifiers[attr]) + (shorten ? "" : " " + ::getName(attr)));
  string attrString = combine(attrStrings, true);
  if (!attrString.empty())
    attrString = " (" + attrString + ")";
  if (attributes->uses > -1 && attributes->displayUses) 
    attrString += " (" + toString(attributes->uses) + " uses left)";
  return artStr + attrString;
}

string Item::getShortName(WConstCreature owner, bool noSuffix) const {
  if (owner && owner->isAffected(LastingEffect::BLIND) && attributes->blindName)
    return getBlindName(false);
  string name = getModifiers(true);
  if (attributes->shortName) {
    if (!attributes->artifactName)
      name = *attributes->shortName + " " + name;
  } else
    name = *attributes->name + " " + name;
  if (fire->isBurning() && !noSuffix)
    name.append(" (burning)");
  return name;
}

string Item::getNameAndModifiers(bool getPlural, WConstCreature owner) const {
  return getName(getPlural, owner) + getModifiers();
}

string Item::getBlindName(bool plural) const {
  if (attributes->blindName)
    return *attributes->blindName + (plural ? "s" : "");
  else
    return getName(plural);
}

bool Item::isDiscarded() {
  return discarded;
}

const optional<Effect>& Item::getEffect() const {
  return attributes->effect;
}

bool Item::canEquip() const {
  return canEquipCache;
}

EquipmentSlot Item::getEquipmentSlot() const {
  CHECK(canEquip());
  return *attributes->equipmentSlot;
}

void Item::addModifier(AttrType type, int value) {
  attributes->modifiers[type] += value;
}

int Item::getModifier(AttrType type) const {
  CHECK(abs(attributes->modifiers[type]) < 10000) << EnumInfo<AttrType>::getString(type) << " "
      << attributes->modifiers[type] << " " << getName();
  return attributes->modifiers[type];
}

const optional<RangedWeapon>& Item::getRangedWeapon() const {
  return attributes->rangedWeapon;
}

optional<CorpseInfo> Item::getCorpseInfo() const {
  return none;
}

void Item::getAttackMsg(const Creature* c, const string& enemyName) const {
  auto weaponInfo = getWeaponInfo();
  auto swingMsg = [&] (const char* verb) {
    c->secondPerson("You "_s + verb + " your " + getName() + " at " + enemyName);
    c->thirdPerson(c->getName().the() + " " + verb + "s his " + getName() + " at " + enemyName);
  };
  auto biteMsg = [&] (const char* verb2, const char* verb3) {
    c->secondPerson("You "_s + verb2 + " " + enemyName);
    c->thirdPerson(c->getName().the() + " " + verb3 + " " + enemyName);
  };
  switch (weaponInfo.attackMsg) {
    case AttackMsg::SWING:
      swingMsg("swing");
      break;
    case AttackMsg::THRUST:
      swingMsg("thrust");
      break;
    case AttackMsg::WAVE:
      swingMsg("wave");
      break;
    case AttackMsg::KICK:
      biteMsg("kick", "kicks");
      break;
    case AttackMsg::BITE:
      biteMsg("bite", "bites");
      break;
    case AttackMsg::TOUCH:
      biteMsg("touch", "touches");
      break;
    case AttackMsg::CLAW:
      biteMsg("claw", "claws");
      break;
  }
}
