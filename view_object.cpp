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
#include "experience_type.h"

SERIALIZE_DEF(ViewObject, resource_id, viewLayer, description, modifiers, attributes, attachmentDir, creatureId, goodAdjectives, badAdjectives, creatureAttributes)

SERIALIZATION_CONSTRUCTOR_IMPL(ViewObject);

ViewObject::ViewObject(ViewId id, ViewLayer l, const string& d)
    : resource_id(id), viewLayer(l), description(d) {
}

ViewObject::ViewObject(ViewId id, ViewLayer l)
    : resource_id(id), viewLayer(l) {
}

void ViewObject::setCreatureId(UniqueEntity<Creature>::Id id) {
  creatureId = id;
}

optional<UniqueEntity<Creature>::Id> ViewObject::getCreatureId() const {
  return creatureId;
}

MovementInfo::MovementInfo(Vec2 dir, double b, double e, Type t) : direction(dir), tBegin(b), tEnd(e),
  type(t) {
}

MovementInfo::MovementInfo() {
}

void ViewObject::addMovementInfo(MovementInfo info) {
  movementQueue.add(info);
}

bool ViewObject::hasAnyMovementInfo() const {
  return movementQueue.hasAny();
}

MovementInfo ViewObject::getLastMovementInfo() const {
  return movementQueue.getLast();
}

Vec2 ViewObject::getMovementInfo(double tBegin) const {
  if (!movementQueue.hasAny())
    return Vec2(0, 0);
  CHECK(creatureId);
  return movementQueue.getTotalMovement(tBegin);
}

void ViewObject::clearMovementInfo() {
  movementQueue.clear();
}

void ViewObject::MovementQueue::clear() {
  index = totalMoves = 0;
}

void ViewObject::MovementQueue::add(MovementInfo info) {
  elems[index] = info;
  ++totalMoves;
  index = makeGoodIndex(index + 1);
}

const MovementInfo& ViewObject::MovementQueue::getLast() const {
  CHECK(hasAny());
  return elems[makeGoodIndex(index - 1)];
}

Vec2 ViewObject::MovementQueue::getTotalMovement(double tBegin) const {
  Vec2 ret;
  bool attack = false;
  for (int i : Range(min<int>(totalMoves, elems.size())))
    if (elems[i].tBegin >= tBegin) {
      if (elems[i].type == MovementInfo::ATTACK/* && ret.length8() == 0*/) {
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
  modifiers.insert(mod);
  return *this;
}

ViewObject& ViewObject::removeModifier(Modifier mod) {
  modifiers.erase(mod);
  return *this;
}

bool ViewObject::hasModifier(Modifier mod) const {
  return modifiers.contains(mod);
}

ViewObject& ViewObject::setAttribute(Attribute attr, double d) {
  attributes[attr] = d;
  return *this;
}

optional<float> ViewObject::getAttribute(Attribute attr) const {
  return attributes[attr];
}

void ViewObject::setCreatureAttributes(ViewObject::CreatureAttributes attribs) {
  creatureAttributes = attribs;
}

const optional<ViewObject::CreatureAttributes>& ViewObject::getCreatureAttributes() const {
  return creatureAttributes;
}

void ViewObject::setDescription(const string& s) {
  description = s;
}

const char* ViewObject::getDefaultDescription() const {
  switch (resource_id) {
    case ViewId::UNKNOWN_MONSTER: return "Unknown creature";
    case ViewId::ALTAR: return "Shrine";
    case ViewId::UP_STAIRCASE:
    case ViewId::DOWN_STAIRCASE: return "Stairs";
    case ViewId::BRIDGE: return "Bridge";
    case ViewId::GRASS: return "Grass";
    case ViewId::CROPS2:
    case ViewId::CROPS: return "Wheat";
    case ViewId::MUD: return "Mud";
    case ViewId::ROAD: return "Road";
    case ViewId::CASTLE_WALL:
    case ViewId::MUD_WALL:
    case ViewId::WALL: return "Wall";
    case ViewId::GOLD_ORE: return "Gold ore";
    case ViewId::IRON_ORE: return "Iron ore";
    case ViewId::STONE: return "Granite";
    case ViewId::WOOD_WALL: return "Wooden wall";
    case ViewId::MOUNTAIN: return "Mountain";
    case ViewId::HILL: return "Hill";
    case ViewId::WATER: return "Water";
    case ViewId::MAGMA: return "Magma";
    case ViewId::SAND: return "Sand";
    case ViewId::DECID_TREE:
    case ViewId::CANIF_TREE: return "Tree";
    case ViewId::BUSH: return "Bush";
    case ViewId::TREE_TRUNK: return "Tree trunk";
    case ViewId::BURNT_TREE: return "Burnt tree";
    case ViewId::BED: return "Bed";
    case ViewId::DORM: return "Dormitory";
    case ViewId::TORCH: return "Torch";
    case ViewId::PRISON: return "Prison";
    case ViewId::WELL: return "Well";
    case ViewId::TORTURE_TABLE: return "Torture room";
    case ViewId::BEAST_CAGE: return "Beast cage";
    case ViewId::TRAINING_WOOD: return "Wooden training dummy";
    case ViewId::TRAINING_IRON: return "Iron training dummy";
    case ViewId::TRAINING_STEEL: return "Steel training dummy";
    case ViewId::THRONE: return "Throne";
    case ViewId::WHIPPING_POST: return "Whipping post";
    case ViewId::NOTICE_BOARD: return "Message board";
    case ViewId::SOKOBAN_HOLE: return "Hole";
    case ViewId::DEMON_SHRINE: return "Demon shrine";
    case ViewId::IMPALED_HEAD: return "Impaled head";
    case ViewId::EYEBALL: return "Eyeball";
    case ViewId::BOOKCASE_WOOD: return "Wooden bookcase";
    case ViewId::BOOKCASE_IRON: return "Iron bookcase";
    case ViewId::BOOKCASE_GOLD: return "Golden bookcase";
    case ViewId::CAULDRON: return "Cauldron";
    case ViewId::LABORATORY: return "Laboratory";
    case ViewId::FORGE: return "Forge";
    case ViewId::WORKSHOP: return "Workshop";
    case ViewId::JEWELER: return "Jeweler";
    case ViewId::STEEL_FURNACE: return "Steel furnace";
    case ViewId::MINION_STATUE: return "Statue";
    case ViewId::CREATURE_ALTAR: return "Shrine";
    case ViewId::FOUNTAIN: return "Fountain";
    case ViewId::TREASURE_CHEST:
    case ViewId::CHEST: return "Chest";
    case ViewId::OPENED_CHEST: return "Opened chest";
    case ViewId::COFFIN: return "Coffin";
    case ViewId::CEMETERY: return "Cemetery";
    case ViewId::GRAVE: return "Grave";
    case ViewId::DOOR: return "Door (click to lock)";
    case ViewId::LOCKED_DOOR: return "Door (click to unlock)";
    case ViewId::BARRICADE: return "Barricade";
    case ViewId::WOOD_FLOOR1:
    case ViewId::WOOD_FLOOR2:
    case ViewId::STONE_FLOOR1:
    case ViewId::STONE_FLOOR2:
    case ViewId::CARPET_FLOOR1:
    case ViewId::CARPET_FLOOR2:
    case ViewId::KEEPER_FLOOR:
    case ViewId::FLOOR: return "Floor";
    case ViewId::BORDER_GUARD: return "Wall";
    default: return "";
  }
}

const char* ViewObject::getDescription() const {
  if (description)
    return description->c_str();
  else
    return getDefaultDescription();
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
    return toString<int>(*getAttribute(attr) * 100);
  else
    return toString(*getAttribute(attr));
}

ViewLayer ViewObject::layer() const {
  return viewLayer;
}

static vector<ViewId> creatureIds {
  ViewId::PLAYER,
  ViewId::KEEPER,
  ViewId::RETIRED_KEEPER,
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
  ViewId::GREEN_DRAGON,
  ViewId::RED_DRAGON,
  ViewId::CYCLOPS,
  ViewId::WITCH,
  ViewId::GHOST,
  ViewId::SPIRIT,
  ViewId::KNIGHT,
  ViewId::DUKE,
  ViewId::ARCHER,
  ViewId::UNICORN,
  ViewId::PESEANT,
  ViewId::CHILD,
  ViewId::SHAMAN,
  ViewId::WARRIOR,
  ViewId::ORC,
  ViewId::DEMON_DWELLER,
  ViewId::DEMON_LORD,
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
  ViewId::SNAKE,
  ViewId::VULTURE,
  ViewId::RAVEN,
  ViewId::GOBLIN,
  ViewId::KRAKEN_HEAD,
  ViewId::KRAKEN_LAND,
  ViewId::KRAKEN_WATER,
  ViewId::FIRE_SPHERE,
  ViewId::DEATH,
  ViewId::SPECIAL_BLBN,
  ViewId::SPECIAL_BLBW,
  ViewId::SPECIAL_BLGN,
  ViewId::SPECIAL_BLGW,
  ViewId::SPECIAL_BMBN,
  ViewId::SPECIAL_BMBW,
  ViewId::SPECIAL_BMGN,
  ViewId::SPECIAL_BMGW,
  ViewId::SPECIAL_HLBN,
  ViewId::SPECIAL_HLBW,
  ViewId::SPECIAL_HLGN,
  ViewId::SPECIAL_HLGW,
  ViewId::SPECIAL_HMBN,
  ViewId::SPECIAL_HMBW,
  ViewId::SPECIAL_HMGN,
  ViewId::SPECIAL_HMGW,
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
  ViewId::SCROLL,
  ViewId::AMULET1,
  ViewId::AMULET2,
  ViewId::AMULET3,
  ViewId::AMULET4,
  ViewId::AMULET5,
  ViewId::BOOK,
  ViewId::FIRST_AID,
  ViewId::POTION1,
  ViewId::POTION2,
  ViewId::POTION3,
  ViewId::POTION4,
  ViewId::POTION5,
  ViewId::POTION6,
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
  ViewId::FIRE_TRAP,
  ViewId::ROCK,
  ViewId::IRON_ROCK,
  ViewId::WOOD_PLANK,
  ViewId::MUSHROOM1, 
  ViewId::MUSHROOM2,
  ViewId::MUSHROOM3,
  ViewId::MUSHROOM4,
  ViewId::MUSHROOM5,
  ViewId::MUSHROOM6 
};

static bool hallu = false;

vector<ViewId> shuffledCreatures;
vector<ViewId> shuffledItems;

void ViewObject::setHallu(bool b) {
  if (!hallu && b) {
    shuffledCreatures = Random.permutation(creatureIds);
    shuffledItems = Random.permutation(itemIds);
  }
  hallu = b;
}

void ViewObject::setId(ViewId id) {
  resource_id = id;
}

void ViewObject::setGoodAdjectives(const string& v) {
  goodAdjectives = v;
}

void ViewObject::setBadAdjectives(const string& v) {
  badAdjectives = v;
}

const string& ViewObject::getGoodAdjectives() const {
  return goodAdjectives;
}

const string& ViewObject::getBadAdjectives() const {
  return badAdjectives;
}

ViewId ViewObject::id() const {
  if (hallu) {
    if (auto elem = creatureIds.findElement(resource_id))
      return shuffledCreatures[*elem];
    if (auto elem = itemIds.findElement(resource_id))
      return shuffledItems[*elem];
  }
  return resource_id;
}

const ViewObject& ViewObject::unknownMonster() {
  static ViewObject ret(ViewId::UNKNOWN_MONSTER, ViewLayer::CREATURE);
  return ret;
}

const ViewObject& ViewObject::empty() {
  static ViewObject ret(ViewId::BORDER_GUARD, ViewLayer::FLOOR);
  return ret;
}
