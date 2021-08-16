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
#include "creature_attributes.h"
#include "content_factory.h"
#include "creature_factory.h"
#include "effect_type.h"
#include "creature_debt.h"

template <class Archive> 
void Item::serialize(Archive& ar, const unsigned int version) {
  ar & SUBCLASS(OwnedObject<Item>) & SUBCLASS(UniqueEntity) & SUBCLASS(Renderable);
  ar(attributes, discarded, shopkeeper, fire, classCache, canEquipCache, timeout, abilityInfo);
}

SERIALIZABLE(Item)
SERIALIZATION_CONSTRUCTOR_IMPL(Item)

Item::Item(const ItemAttributes& attr, const ContentFactory* factory)
    : Renderable(ViewObject(attr.viewId, ViewLayer::ITEM, capitalFirst(attr.name))),
      attributes(attr), fire(attr.burnTime), canEquipCache(!!attributes->equipmentSlot),
      classCache(attributes->itemClass) {
  if (!attributes->prefixes.empty())
    modViewObject().setModifier(ViewObject::Modifier::AURA);
  modViewObject().setGenericId(getUniqueId().getGenericId());
  modViewObject().partIds = attr.partIds;
  updateAbility(factory);
}

void Item::updateAbility(const ContentFactory* factory) {
  for (auto id : attributes->equipedAbility)
    abilityInfo = ItemAbility { *factory->getCreatures().getSpell(id), none, getUniqueId().getGenericId() };
}

Item::~Item() {
}

PItem Item::getCopy(const ContentFactory* f) const {
  auto ret = makeOwner<Item>(*attributes, f);
  ret->getAbility().reset();
  return ret;
}

ItemPredicate Item::classPredicate(ItemClass cl) {
  return [cl](const Item* item) { return item->getClass() == cl; };
}

ItemPredicate Item::equipmentSlotPredicate(EquipmentSlot slot) {
  return [slot](const Item* item) { return item->canEquip() && item->getEquipmentSlot() == slot; };
}

ItemPredicate Item::classPredicate(vector<ItemClass> cl) {
  return [cl](const Item* item) { return cl.contains(item->getClass()); };
}

ItemPredicate Item::namePredicate(const string& name) {
  return [name](const Item* item) { return item->getName() == name; };
}

vector<vector<Item*>> Item::stackItems(vector<Item*> items, function<string(const Item*)> suffix) {
  map<string, vector<Item*>> stacks = groupBy<Item*, string>(items, [suffix](const Item* item) {
        return item->getNameAndModifiers() + suffix(item) + toString(item->getViewObject().id().getColor());
      });
  vector<vector<Item*>> ret;
  for (auto& elem : stacks)
    ret.push_back(elem.second);
  return ret;
}

void Item::onOwned(Creature* c, bool msg) {
  if (attributes->ownedEffect)
    c->addPermanentEffect(*attributes->ownedEffect, 1, msg);
}

void Item::onDropped(Creature* c, bool msg) {
  if (attributes->ownedEffect)
    c->removePermanentEffect(*attributes->ownedEffect, 1, msg);
}

const vector<LastingEffect>& Item::getEquipedEffects() const {
  return attributes->equipedEffect;
}

void Item::onEquip(Creature* c, bool msg) {
  for (auto& e : attributes->equipedEffect)
    c->addPermanentEffect(e, 1, msg);
  if (attributes->equipedCompanion)
    c->getAttributes().companions.push_back(*attributes->equipedCompanion);
}

void Item::onUnequip(Creature* c, bool msg) {
  for (auto& e : attributes->equipedEffect)
    c->removePermanentEffect(e, 1, msg);
  if (attributes->equipedCompanion)
    [&, &companions = c->getAttributes().companions] {
      for (int i : All(companions))
        if (companions[i] == *attributes->equipedCompanion) {
          c->removeCompanions(i);
          return;
        }
      FATAL << "Error removing companion";
    }();
}

void Item::fireDamage(Position position) {
  bool burning = fire->isBurning();
  string noBurningName = getTheName();
  fire->set();
  if (!burning && fire->isBurning()) {
    position.globalMessage(noBurningName + " catches fire");
    modViewObject().setModifier(ViewObject::Modifier::BURNING);
  }
}

void Item::iceDamage(Position) {
}

const Fire& Item::getFire() const {
  return *fire;
}

void Item::tick(Position position, bool carried) {
  PROFILE_BLOCK("Item::tick");
  if (fire->isBurning()) {
    INFO << getName() << " burning ";
    position.fireDamage(0.2);
    modViewObject().setModifier(ViewObject::Modifier::BURNING);
    fire->tick();
    if (!fire->isBurning()) {
      position.globalMessage(getTheName() + " burns out");
      discarded = true;
    }
  }
  specialTick(position);
  if (timeout) {
    if (position.getGame()->getGlobalTime() >= *timeout) {
      position.globalMessage(getTheName() + " disappears!");
      discarded = true;
    }
  }
  if (carried && attributes->carriedTickEffect)
    attributes->carriedTickEffect->apply(position);
}

void Item::applyPrefix(const ItemPrefix& prefix, const ContentFactory* factory) {
  modViewObject().setModifier(ViewObject::Modifier::AURA);
  ::applyPrefix(factory, prefix, *attributes);
  updateAbility(factory);
}

void Item::setTimeout(GlobalTime t) {
  timeout = t;
}

const optional<AutomatonPart>& Item::getAutomatonPart() const {
  return attributes->automatonPart;
}

void Item::onHitSquareMessage(Position pos, const Attack& attack, int numItems) {
  if (attributes->fragile) {
    pos.globalMessage(getPluralTheNameAndVerb(numItems, "crashes", "crash") + " on the " + pos.getName());
    pos.unseenMessage("You hear a crash");
    discarded = true;
  } else
    pos.globalMessage(getPluralTheNameAndVerb(numItems, "hits", "hit") + " the " + pos.getName());
  if (attributes->ownedEffect == LastingEffect::LIGHT_SOURCE)
    pos.fireDamage(1);
  if (attributes->effect && effectAppliedWhenThrown())
    attributes->effect->apply(pos, attack.attacker);
}

bool Item::effectAppliedWhenThrown() const {
  return attributes->effectAppliedWhenThrown;
}

const optional<CreaturePredicate>& Item::getApplyPredicate() const {
  return attributes->applyPredicate;
}

optional<ItemAbility>& Item::getAbility() {
  if (abilityInfo && abilityInfo->itemId != getUniqueId().getGenericId()) {
    abilityInfo->itemId = getUniqueId().getGenericId();
    abilityInfo->timeout = none;
  }
  return abilityInfo;
}

void Item::onHitCreature(Creature* c, const Attack& attack, int numItems) {
  if (attributes->fragile) {
    c->you(numItems > 1 ? MsgType::ITEM_CRASHES_PLURAL : MsgType::ITEM_CRASHES, getPluralTheName(numItems));
    discarded = true;
  } else
    c->you(numItems > 1 ? MsgType::HIT_THROWN_ITEM_PLURAL : MsgType::HIT_THROWN_ITEM, getPluralTheName(numItems));
  if (attributes->effect && effectAppliedWhenThrown())
    attributes->effect->apply(c->getPosition(), attack.attacker);
  c->takeDamage(attack, true);
  if (!c->isDead() && attributes->ownedEffect == LastingEffect::LIGHT_SOURCE)
    c->affectByFire(1);
}

TimeInterval Item::getApplyTime() const {
  return attributes->applyTime;
}

double Item::getWeight() const {
  return attributes->weight;
}

vector<string> Item::getDescription(const ContentFactory* factory) const {
  vector<string> ret;
  if (!attributes->description.empty())
    ret.push_back(attributes->description);
  if (attributes->effectDescription)
    if (auto& effect = attributes->effect) {
      ret.push_back("Usage effect: " + effect->getName(factory));
      ret.push_back(effect->getDescription(factory));
    }
  for (auto& effect : getWeaponInfo().victimEffect)
    ret.push_back("Victim affected by: " + effect.effect.getName(factory) + " (" + toPercentage(effect.chance) + " chance)");
  for (auto& effect : getWeaponInfo().attackerEffect)
    ret.push_back("Attacker affected by: " + effect.getName(factory));
  for (auto& effect : attributes->equipedEffect) {
    ret.push_back("Effect when equipped: " + LastingEffects::getName(effect));
    ret.push_back(LastingEffects::getDescription(effect));
  }
  if (auto& info = attributes->upgradeInfo)
    ret.append(info->getDescription(factory));
  if (abilityInfo)
    ret.push_back("Grants ability: "_s + abilityInfo->spell.getName(factory));
  if (auto& part = attributes->automatonPart) {
    ret.push_back(part->effect.getDescription(factory));
  }
  for (auto attr : ENUM_ALL(AttrType))
    if (auto& elem = attributes->specialAttr[attr])
      ret.push_back(toStringWithSign(elem->first) + " " + ::getName(attr) + " " + elem->second.getName());
  return ret;
}

optional<LastingEffect> Item::getOwnedEffect() const {
  return attributes->ownedEffect;
}

const WeaponInfo& Item::getWeaponInfo() const {
  return attributes->weaponInfo;
}

ItemClass Item::getClass() const {
  return classCache;
}

const vector<StorageId>& Item::getStorageIds() const {
  return attributes->storageIds;
}

int Item::getPrice() const {
  return attributes->price;
}

CostInfo Item::getCraftingCost() const {
  return attributes->craftingCost;
}

bool Item::isShopkeeper(const Creature* c) const {
  return shopkeeper == c->getUniqueId();
}

void Item::setShopkeeper(const Creature* s) {
  if (s)
    shopkeeper = s->getUniqueId();
  else
    shopkeeper = none;
}

bool Item::isOrWasForSale() const {
  return !!shopkeeper;
}

optional<CollectiveResourceId> Item::getResourceId() const {
  return attributes->resourceId;
}

void Item::setResourceId(optional<CollectiveResourceId> id) {
  attributes->resourceId = id;
}

const optional<ItemUpgradeInfo>& Item::getUpgradeInfo() const {
  return attributes->upgradeInfo;
}

void Item::setUpgradeInfo(ItemUpgradeInfo info) {
  attributes->upgradeInfo = std::move(info);
}

optional<ItemUpgradeType> Item::getAppliedUpgradeType() const {
  auto c = getClass();
  if (attributes->automatonPart && attributes->automatonPart->prefixType)
    c = *attributes->automatonPart->prefixType;
  if (attributes->upgradeType)
    return attributes->upgradeType;
  switch (c) {
    case ItemClass::ARMOR:
      return ItemUpgradeType::ARMOR;
    case ItemClass::WEAPON:
      return ItemUpgradeType::WEAPON;
    case ItemClass::RANGED_WEAPON:
      return ItemUpgradeType::RANGED_WEAPON;
    default:
      return none;
  }
}

int Item::getMaxUpgrades() const {
  return attributes->maxUpgrades;
}

const optional<string>& Item::getIngredientType() const {
  return attributes->ingredientType;
}

void Item::apply(Creature* c, bool noSound) {
  if (attributes->applySound && !noSound)
    c->addSound(*attributes->applySound);
  applySpecial(c);
}

bool Item::canApply() const {
  return !!attributes->effect;
}

void Item::applySpecial(Creature* c) {
  if (attributes->uses > -1 && --attributes->uses == 0) {
    discarded = true;
    if (attributes->usedUpMsg)
      c->privateMessage(getTheName() + " is used up.");
  }
  if (attributes->effect)
    attributes->effect->apply(c->getPosition(), c);
}

string Item::getApplyMsgThirdPerson(const Creature* owner) const {
  if (attributes->applyMsgThirdPerson)
    return *attributes->applyMsgThirdPerson;
  switch (getClass()) {
    case ItemClass::FOOD: return "eats " + getAName(false, owner);
    default: return attributes->applyVerb.second + " " + getAName(false, owner);
  }
}

string Item::getApplyMsgFirstPerson(const Creature* owner) const {
  if (attributes->applyMsgFirstPerson)
    return *attributes->applyMsgFirstPerson;
  switch (getClass()) {
    case ItemClass::FOOD: return "eat " + getAName(false, owner);
    default: return attributes->applyVerb.first + " " + getAName(false, owner);
  }
}

optional<StatId> Item::getProducedStat() const {
  return attributes->producedStat;
}

void Item::setName(const string& n) {
  attributes->name = n;
}

Creature* Item::getShopkeeper(const Creature* owner) const {
  if (shopkeeper) {
    for (Creature* c : owner->getDebt().getCreditors(owner))
      if (c->getUniqueId() == *shopkeeper)
        return c;
    for (Creature* c : owner->getVisibleCreatures())
      if (c->getUniqueId() == *shopkeeper)
        return c;
  }
  return nullptr;
}

string Item::getName(bool plural, const Creature* owner) const {
  PROFILE;
  string suff;
  if (fire->isBurning())
    suff.append(" (burning)");
  if (owner && getShopkeeper(owner))
    suff += " (" + toString(getPrice()) + (plural ? " gold each)" : " gold)");
  if (owner && owner->isAffected(LastingEffect::BLIND))
    return getBlindName(plural);
  return getVisibleName(plural) + suff;
}

string Item::getAName(bool getPlural, const Creature* owner) const {
  if (attributes->noArticle || getPlural)
    return getName(getPlural, owner);
  else
    return addAParticle(getName(getPlural, owner));
}

string Item::getTheName(bool getPlural, const Creature* owner) const {
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

string Item::getPluralAName(int count) const {
  if (count > 1)
    return toString(count) + " " + getTheName(true);
  else
    return getAName(false);
}

string Item::getPluralTheNameAndVerb(int count, const string& verbSingle, const string& verbPlural) const {
  return getPluralTheName(count) + " " + (count > 1 ? verbPlural : verbSingle);
}

static void appendWithSpace(string& s, const string& suf) {
  if (!s.empty() && !suf.empty())
    s += " ";
  s += suf;
}

string Item::getVisibleName(bool getPlural) const {
  string ret;
  if (!getPlural)
    ret = attributes->name;
  else {
    if (attributes->plural)
      ret = *attributes->plural;
    else if (attributes->name.back() != 's')
      ret = attributes->name + "s";
    else
      ret = attributes->name;
  }
  appendWithSpace(ret, getSuffix());
  return ret;
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

string Item::getSuffix() const {
  string artStr;
  if (!attributes->prefixes.empty())
    artStr += attributes->prefixes.back();
  if (auto& part = attributes->automatonPart)
    if (!part->prefixes.empty())
      artStr += part->prefixes.back().name;
  if (attributes->artifactName)
    appendWithSpace(artStr, "named " + *attributes->artifactName);
  if (fire->isBurning())
    appendWithSpace(artStr, "(burning)");
  return artStr;
}

string Item::getModifiers(bool shorten) const {
  EnumSet<AttrType> printAttr;
  if (!shorten) {
    for (auto attr : ENUM_ALL(AttrType))
      if (attributes->modifiers[attr] != 0)
        printAttr.insert(attr);
  } else
    switch (getClass()) {
      case ItemClass::RANGED_WEAPON:
        for (auto attr : {AttrType::RANGED_DAMAGE, AttrType::SPELL_DAMAGE})
          if (attributes->modifiers[attr] > 0)
            printAttr.insert(attr);
        break;
      case ItemClass::WEAPON:
        printAttr.insert(getWeaponInfo().meleeAttackAttr);
        break;
      case ItemClass::ARMOR:
        printAttr.insert(AttrType::DEFENSE);
        if (attributes->modifiers[AttrType::PARRY])
          printAttr.insert(AttrType::PARRY);
        break;
      default: break;
    }
  vector<string> attrStrings;
  for (auto attr : printAttr)
    attrStrings.push_back(withSign(attributes->modifiers[attr]) + (shorten ? "" : " " + ::getName(attr)));
  string attrString = combine(attrStrings, true);
  if (!attrString.empty())
    attrString = "(" + attrString + ")";
  if (attributes->uses > -1 && attributes->displayUses) 
    appendWithSpace(attrString, "(" + toString(attributes->uses) + " uses left)");
  return attrString;
}

string Item::getShortName(const Creature* owner, bool plural) const {
  PROFILE;
  if (owner && owner->isAffected(LastingEffect::BLIND) && attributes->blindName)
    return getBlindName(plural);
  if (attributes->artifactName)
    return *attributes->artifactName + " " + getModifiers(true);
  string name;
  if (!attributes->prefixes.empty())
    name = attributes->prefixes.back();
  else if (attributes->shortName) {
    name = *attributes->shortName;
    appendWithSpace(name, getSuffix());
  } else
    name = getVisibleName(plural);
  appendWithSpace(name, getModifiers(true));
  return name;
}

string Item::getNameAndModifiers(bool getPlural, const Creature* owner) const {
  auto ret = getName(getPlural, owner);
  appendWithSpace(ret, getModifiers());
  return ret;
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

const optional<pair<int, CreaturePredicate>>& Item::getSpecialModifier(AttrType attr) const {
  return attributes->specialAttr[attr];
}

optional<CorpseInfo> Item::getCorpseInfo() const {
  return none;
}

void Item::getAttackMsg(const Creature* c, const string& enemyName) const {
  auto weaponInfo = getWeaponInfo();
  auto swingMsg = [&] (const char* verb) {
    if (!weaponInfo.itselfMessage) {
      c->secondPerson("You "_s + verb + " your " + getName() + " at " + enemyName);
      c->thirdPerson(c->getName().the() + " " + verb + "s " + his(c->getAttributes().getGender()) + " " + getName() + " at " + enemyName);
    } else {
      c->secondPerson("You "_s + verb + " yourself at " + enemyName);
      c->thirdPerson(c->getName().the() + " " + verb + "s " + himself(c->getAttributes().getGender()) + " at " + enemyName);
    }
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
    case AttackMsg::SPELL:
      biteMsg("curse", "curses");
      break;
    case AttackMsg::PUNCH:
      biteMsg("punch", "punches");
      break;
  }
}
