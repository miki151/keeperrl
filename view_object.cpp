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

#include "view_object.h"
#include "view_id.h"

template <class Archive> 
void ViewObject::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(enemyStatus)
    & SVAR(resource_id)
    & SVAR(viewLayer)
    & SVAR(description)
    & SVAR(modifiers)
    & SVAR(attributes)
    & SVAR(attachmentDir)
    & SVAR(position)
    & SVAR(creatureId);
  if (version >= 1)
    ar & SVAR(adjectives);
  CHECK_SERIAL;
}

SERIALIZABLE(ViewObject);

SERIALIZATION_CONSTRUCTOR_IMPL(ViewObject);

ViewObject::ViewObject(ViewId id, ViewLayer l, const string& d)
    : resource_id(id), viewLayer(l), description(d) {
  if (islower(description[0]))
    description[0] = toupper(description[0]);
  for (Attribute attr : {
      Attribute::ATTACK,
      Attribute::DEFENSE,
      Attribute::LEVEL,
      Attribute::WATER_DEPTH,
      Attribute::EFFICIENCY})
    setAttribute(attr, -1);
}

void ViewObject::setCreatureId(UniqueEntity<Creature>::Id id) {
  creatureId = id;
}

optional<UniqueEntity<Creature>::Id> ViewObject::getCreatureId() const {
  return creatureId;
}

void ViewObject::addMovementInfo(MovementInfo info) {
  movementQueue.add(info);
}

bool ViewObject::hasAnyMovementInfo() const {
  return movementQueue.hasAny();
}

ViewObject::MovementInfo ViewObject::getLastMovementInfo() const {
  return movementQueue.getLast();
}

Vec2 ViewObject::getMovementInfo(double tBegin, double tEnd, UniqueEntity<Creature>::Id controlledId) const {
  if (!movementQueue.hasAny())
    return Vec2(0, 0);
  CHECK(creatureId);
  if (controlledId > *creatureId)
    return movementQueue.getTotalMovement(tBegin, tEnd);
  else
    return movementQueue.getTotalMovement(tBegin - 0.00000001, tEnd);
}

void ViewObject::MovementQueue::add(MovementInfo info) {
  elems[index] = info;
  ++totalMoves;
  index = makeGoodIndex(index + 1);
}

const ViewObject::MovementInfo& ViewObject::MovementQueue::getLast() const {
  CHECK(hasAny());
  return elems[makeGoodIndex(index - 1)];
}

Vec2 ViewObject::MovementQueue::getTotalMovement(double tBegin, double tEnd) const {
  Vec2 ret;
  bool attack = false;
  for (int i : Range(min<int>(totalMoves, elems.size())))
    if (elems[i].tBegin > tBegin) {
      if (elems[i].type == MovementInfo::ATTACK && ret.length8() == 0) {
        attack = true;
        ret = elems[i].direction;
      } else {
        if (attack) {
          attack = false;
          ret = Vec2(0, 0);
        }
        ret += elems[i].direction;
      }
    }
  return ret;
}

bool ViewObject::MovementQueue::hasAny() const {
  return totalMoves > 0;
}

int ViewObject::MovementQueue::makeGoodIndex(int index) const {
  return (index + elems.size()) % elems.size();
}

ViewObject& ViewObject::setModifier(Modifier mod) {
  modifiers[mod] = true;
  return *this;
}

ViewObject& ViewObject::removeModifier(Modifier mod) {
  modifiers[mod] = false;
  return *this;
}

bool ViewObject::hasModifier(Modifier mod) const {
  return modifiers[mod];
}

ViewObject& ViewObject::setAttribute(Attribute attr, double d) {
  attributes[attr] = d;
  return *this;
}

double ViewObject::getAttribute(Attribute attr) const {
  return attributes[attr];
}

string ViewObject::getBareDescription() const {
  return description;
}

ViewObject&  ViewObject::setAttachmentDir(Dir dir) {
  attachmentDir = dir;
  return *this;
}

optional<Dir> ViewObject::getAttachmentDir() const {
  return attachmentDir;
}

string ViewObject::getAttributeString(Attribute attr) const {
  if (attr == Attribute::EFFICIENCY)
    return toString<int>(100 * getAttribute(attr)) + "%";
  else
    return toString(getAttribute(attr));
}

void ViewObject::setAdjectives(const vector<string>& adj) {
  adjectives = adj;
}

vector<string> ViewObject::getDescription(bool stats) const {
  EnumMap<Attribute, string> namedAttr {
      { Attribute::ATTACK, "attack"},
      { Attribute::DEFENSE, "defense"},
      { Attribute::LEVEL, "level"},
      { Attribute::EFFICIENCY, "efficiency"},
  //    { Attribute::HEIGHT, "height"}
  };
  string attr;
  if (stats)
    for (Attribute a : ENUM_ALL(Attribute))
      if (!namedAttr[a].empty() && getAttribute(a) > -1)
        attr += " " + namedAttr[a] + ": " + getAttributeString(a);
  vector<string> mods;
  if (getAttribute(Attribute::BLEEDING) > 0) 
    mods.push_back("wounded");
  if (hasModifier(Modifier::BLIND))
    mods.push_back("blind");
  if (hasModifier(Modifier::POISONED))
    mods.push_back("poisoned");
  if (hasModifier(Modifier::PLANNED))
    mods.push_back("planned");
  if (hasModifier(Modifier::SLEEPING))
    mods.push_back("sleeping");
#ifndef RELEASE
  if (position.x > -1)
    mods.push_back(toString(position.x) + ", " + toString(position.y));
#endif
  vector<string> ret;
  if (mods.size() > 0)
    ret.push_back(description + attr + "(" + combine(mods) + ")");
  else
    ret.push_back(description + attr);
  append(ret, adjectives);
  return ret;
}

ViewLayer ViewObject::layer() const {
  return viewLayer;
}


static vector<ViewId> creatureIds {
  ViewId::PLAYER,
  ViewId::KEEPER,
  ViewId::ELF,
  ViewId::ELF_ARCHER,
  ViewId::ELF_CHILD,
  ViewId::ELF_LORD,
  ViewId::SHOPKEEPER,
  ViewId::LIZARDMAN,
  ViewId::LIZARDLORD,
  ViewId::DWARF,
  ViewId::DWARF_BARON,
  ViewId::IMP,
  ViewId::PRISONER,
  ViewId::OGRE,
  ViewId::CHICKEN,
  ViewId::DARK_KNIGHT,
  ViewId::GREEN_DRAGON,
  ViewId::RED_DRAGON,
  ViewId::CYCLOPS,
  ViewId::WITCH,
  ViewId::GHOST,
  ViewId::SPIRIT,
  ViewId::DEVIL,
  ViewId::KNIGHT,
  ViewId::CASTLE_GUARD,
  ViewId::AVATAR,
  ViewId::ARCHER,
  ViewId::PESEANT,
  ViewId::CHILD,
  ViewId::SHAMAN,
  ViewId::WARRIOR,
  ViewId::GREAT_ORC,
  ViewId::ORC,
  ViewId::BANDIT,
  ViewId::CLAY_GOLEM,
  ViewId::STONE_GOLEM,
  ViewId::IRON_GOLEM,
  ViewId::LAVA_GOLEM,
  ViewId::ZOMBIE,
  ViewId::SKELETON,
  ViewId::VAMPIRE,
  ViewId::VAMPIRE_LORD,
  ViewId::MUMMY,
  ViewId::MUMMY_LORD,
  ViewId::HORSE,
  ViewId::COW,
  ViewId::PIG,
  ViewId::GOAT,
  ViewId::JACKAL,
  ViewId::DEER,
  ViewId::BOAR,
  ViewId::FOX,
  ViewId::BEAR,
  ViewId::WOLF,
  ViewId::BAT,
  ViewId::RAT,
  ViewId::SPIDER,
  ViewId::FLY,
  ViewId::SCORPION,
  ViewId::SNAKE,
  ViewId::VULTURE,
  ViewId::RAVEN,
  ViewId::GOBLIN,
  ViewId::LEPRECHAUN,
  ViewId::KRAKEN_HEAD,
  ViewId::KRAKEN_LAND,
  ViewId::KRAKEN_WATER,
  ViewId::FIRE_SPHERE,
  ViewId::NIGHTMARE,
  ViewId::DEATH,
  ViewId::SPECIAL_BEAST,
  ViewId::SPECIAL_HUMANOID,
  ViewId::CANIF_TREE,
  ViewId::DECID_TREE,
};

static vector<ViewId> itemIds {
  ViewId::BODY_PART,
  ViewId::BONE,
  ViewId::SPEAR,
  ViewId::SWORD,
  ViewId::SPECIAL_SWORD,
  ViewId::ELVEN_SWORD,
  ViewId::KNIFE,
  ViewId::WAR_HAMMER,
  ViewId::SPECIAL_WAR_HAMMER,
  ViewId::BATTLE_AXE,
  ViewId::SPECIAL_BATTLE_AXE,
  ViewId::BOW,
  ViewId::ARROW,
  ViewId::SCROLL,
  ViewId::STEEL_AMULET,
  ViewId::COPPER_AMULET,
  ViewId::CRYSTAL_AMULET,
  ViewId::WOODEN_AMULET,
  ViewId::AMBER_AMULET,
  ViewId::BOOK,
  ViewId::FIRST_AID,
  ViewId::EFFERVESCENT_POTION,
  ViewId::MURKY_POTION,
  ViewId::SWIRLY_POTION,
  ViewId::VIOLET_POTION,
  ViewId::PUCE_POTION,
  ViewId::SMOKY_POTION,
  ViewId::FIZZY_POTION,
  ViewId::MILKY_POTION,
  ViewId::GOLD,
  ViewId::LEATHER_ARMOR,
  ViewId::LEATHER_HELM,
  ViewId::TELEPATHY_HELM,
  ViewId::CHAIN_ARMOR,
  ViewId::IRON_HELM,
  ViewId::LEATHER_BOOTS,
  ViewId::IRON_BOOTS,
  ViewId::SPEED_BOOTS,
  ViewId::BOULDER,
  ViewId::PORTAL,
  ViewId::GAS_TRAP,
  ViewId::ALARM_TRAP,
  ViewId::WEB_TRAP,
  ViewId::SURPRISE_TRAP,
  ViewId::TERROR_TRAP,
  ViewId::ROCK,
  ViewId::IRON_ROCK,
  ViewId::WOOD_PLANK,
  ViewId::MUSHROOM, 
};

static bool hallu = false;

vector<ViewId> shuffledCreatures;
vector<ViewId> shuffledItems;

void ViewObject::setHallu(bool b) {
  if (!hallu && b) {
    shuffledCreatures = randomPermutation(creatureIds);
    shuffledItems = randomPermutation(itemIds);
  }
  hallu = b;
}

void ViewObject::setId(ViewId id) {
  resource_id = id;
}

ViewId ViewObject::id() const {
  if (hallu) {
    if (auto elem = findElement(creatureIds, resource_id))
      return shuffledCreatures[*elem];
    if (auto elem = findElement(itemIds, resource_id))
      return shuffledItems[*elem];
  }
  return resource_id;
}

const ViewObject& ViewObject::unknownMonster() {
  static ViewObject ret(ViewId::UNKNOWN_MONSTER, ViewLayer::CREATURE, "Unknown creature");
  return ret;
}

const ViewObject& ViewObject::empty() {
  static ViewObject ret(ViewId::BORDER_GUARD, ViewLayer::FLOOR, "");
  return ret;
}

const ViewObject& ViewObject::mana() {
  static ViewObject ret(ViewId::MANA, ViewLayer::FLOOR, "");
  return ret;
}

void ViewObject::setPosition(Vec2 pos) {
  position = pos;
}

int ViewObject::getPositionHash() const {
  int a = position.x * position.y;
  return (a * (a + 3)) % 1487;
}
