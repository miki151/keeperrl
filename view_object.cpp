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

SERIALIZE_DEF(ViewObject, resource_id, viewLayer, description, modifiers, attributes, attachmentDir, genericId, goodAdjectives, badAdjectives, creatureAttributes, status, clickAction, extendedActions)

SERIALIZATION_CONSTRUCTOR_IMPL(ViewObject);

constexpr float noAttributeValue = -1234;

static const char* getDefaultDescription(const ViewId& id) {
  if (id == ViewId("unknown_monster"))
    return "Unknown creature";
  if (id == ViewId("altar"))
    return "Altar";
  if (id == ViewId("altar_des"))
    return "Desecrated altar";
  if (id == ViewId("up_staircase") || id == ViewId("down_staircase"))
    return "Stairs";
  if (id == ViewId("bridge"))
    return "Bridge";
  if (id == ViewId("grass"))
    return "Grass";
  if (id == ViewId("crops2") || id == ViewId("crops"))
    return "Wheat";
  if (id == ViewId("mud"))
    return "Mud";
  if (id == ViewId("road"))
    return "Road";
  if (id == ViewId("castle_wall") || id == ViewId("mud_wall") || id == ViewId("wall"))
    return "Wall";
  if (id == ViewId("gold_ore"))
    return "Gold ore";
  if (id == ViewId("iron_ore"))
    return "Iron ore";
  if (id == ViewId("adamantium_ore"))
    return "Adamantium ore";
  if (id == ViewId("stone"))
    return "Granite";
  if (id == ViewId("wood_wall"))
    return "Wooden wall";
  if (id == ViewId("mountain"))
    return "Soft rock";
  if (id == ViewId("mountain2"))
    return "Hard rock";
  if (id == ViewId("dungeon_wall") || id == ViewId("dungeon_wall2"))
    return "Reinforced wall";
  if (id == ViewId("hill"))
    return "Hill";
  if (id == ViewId("water"))
    return "Water";
  if (id == ViewId("magma"))
    return "Magma";
  if (id == ViewId("sand"))
    return "Sand";
  if (id == ViewId("decid_tree") || id == ViewId("canif_tree"))
    return "Tree";
  if (id == ViewId("bush"))
    return "Bush";
  if (id == ViewId("tree_trunk"))
    return "Tree trunk";
  if (id == ViewId("burnt_tree"))
    return "Burnt tree";
  if (id == ViewId("bed1"))
    return "Basic bed";
  if (id == ViewId("bed2"))
    return "Fine bed";
  if (id == ViewId("bed3"))
    return "Luxurious bed";
  if (id == ViewId("torch"))
    return "Torch";
  if (id == ViewId("candelabrum_ns"))
    return "Candelabrum";
  if (id == ViewId("candelabrum_e"))
    return "Candelabrum";
  if (id == ViewId("candelabrum_w"))
    return "Candelabrum";
  if (id == ViewId("standing_torch"))
    return "Standing torch";
  if (id == ViewId("prison"))
    return "Prison";
  if (id == ViewId("well"))
    return "Well";
  if (id == ViewId("torture_table"))
    return "Torture room";
  if (id == ViewId("beast_cage"))
    return "Beast cage";
  if (id == ViewId("training_wood"))
    return "Wooden training dummy";
  if (id == ViewId("training_iron"))
    return "Iron training dummy";
  if (id == ViewId("training_ada"))
    return "Adamantine training dummy";
  if (id == ViewId("throne"))
    return "Throne";
  if (id == ViewId("whipping_post"))
    return "Whipping post";
  if (id == ViewId("notice_board"))
    return "Message board";
  if (id == ViewId("sokoban_hole"))
    return "Hole";
  if (id == ViewId("demon_shrine"))
    return "Demon shrine";
  if (id == ViewId("impaled_head"))
    return "Impaled head";
  if (id == ViewId("eyeball"))
    return "Eyeball";
  if (id == ViewId("bookcase_wood"))
    return "Wooden bookcase";
  if (id == ViewId("bookcase_iron"))
    return "Iron bookcase";
  if (id == ViewId("bookcase_gold"))
    return "Golden bookcase";
  if (id == ViewId("cauldron"))
    return "Cauldron";
  if (id == ViewId("forge"))
    return "Forge";
  if (id == ViewId("workshop"))
    return "Workshop";
  if (id == ViewId("jeweller"))
    return "Jeweller";
  if (id == ViewId("furnace"))
    return "Furnace";
  if (id == ViewId("minion_statue"))
    return "Statue";
  if (id == ViewId("stone_minion_statue"))
    return "Stone Statue";
  if (id == ViewId("fountain"))
    return "Fountain";
  if (id == ViewId("treasure_chest") || id == ViewId("chest"))
    return "Chest";
  if (id == ViewId("opened_chest"))
    return "Opened chest";
  if (id == ViewId("opened_coffin"))
    return "Opened coffin";
  if (id == ViewId("loot_coffin"))
    return "Coffin";
  if (id == ViewId("coffin1"))
    return "Basic coffin";
  if (id == ViewId("coffin2"))
    return "Fine coffin";
  if (id == ViewId("coffin3"))
    return "Luxurious coffin";
  if (id == ViewId("grave"))
    return "Grave";
  if (id == ViewId("portal"))
    return "Portal";
  if (id == ViewId("wood_door_ew"))
    return "Wooden door";
  if (id == ViewId("iron_door_ew"))
    return "Iron door";
  if (id == ViewId("ada_door_ew"))
    return "Adamantine door";
  if (id == ViewId("barricade"))
    return "Barricade";
  if (id == ViewId("archery_range"))
    return "Archery target";
  if (id == ViewId("ruin_wall"))
    return "Ruined wall";
  if (id == ViewId("wood_floor1") || id == ViewId("wood_floor2") || id == ViewId("wood_floor3") ||
      id == ViewId("wood_floor4") || id == ViewId("wood_floor5") || id == ViewId("stone_floor1") ||
      id == ViewId("stone_floor2") || id == ViewId("stone_floor3") || id == ViewId("stone_floor4") ||
      id == ViewId("stone_floor5") || id == ViewId("carpet_floor1") || id == ViewId("carpet_floor2") ||
      id == ViewId("carpet_floor3") || id == ViewId("carpet_floor4") || id == ViewId("carpet_floor5") ||
      id == ViewId("keeper_floor") || id == ViewId("floor"))
    return "Floor";
  return "";
}

ViewObject::ViewObject(ViewId id, ViewLayer l, const string& d)
    : resource_id(id), viewLayer(l), description(d) {
  for (auto a : ENUM_ALL(Attribute))
    attributes[a] = noAttributeValue;
}

ViewObject::ViewObject(ViewId id, ViewLayer l)
    : resource_id(id), viewLayer(l), description(getDefaultDescription(id)) {
  for (auto a : ENUM_ALL(Attribute))
    attributes[a] = noAttributeValue;
}

void ViewObject::setGenericId(GenericId id) {
  CHECK(id != 0);
  genericId = id;
}

optional<GenericId> ViewObject::getGenericId() const {
  if (genericId)
    return genericId;
  else
    return none;
}

void ViewObject::setClickAction(ViewObjectAction s) {
  clickAction = s;
}

optional<ViewObjectAction> ViewObject::getClickAction() const {
  return clickAction;
}

void ViewObject::setExtendedActions(EnumSet<ViewObjectAction> s) {
  extendedActions = s;
}

const EnumSet<ViewObjectAction>& ViewObject::getExtendedActions() const {
  return extendedActions;
}

void ViewObject::addMovementInfo(MovementInfo info, GenericId id) {
  CHECK(id);
  genericId = id;
  if (!movementQueue)
    movementQueue.reset(MovementQueue());
  movementQueue->add(info);
}

bool ViewObject::hasAnyMovementInfo() const {
  return !!movementQueue;
}

const MovementInfo& ViewObject::getLastMovementInfo() const {
  return movementQueue->getLast();
}

Vec2 ViewObject::getMovementInfo(int moveCounter) const {
  if (!movementQueue)
    return Vec2(0, 0);
  CHECK(genericId) << resource_id;
  return movementQueue->getTotalMovement(moveCounter);
}

void ViewObject::clearMovementInfo() {
  movementQueue.clear();
}

void ViewObject::MovementQueue::clear() {
  index = totalMoves = 0;
}

void ViewObject::MovementQueue::add(MovementInfo info) {
  elems[index] = info;
  if (totalMoves < elems.size())
    ++totalMoves;
  index = makeGoodIndex(index + 1);
}

const MovementInfo& ViewObject::MovementQueue::getLast() const {
  CHECK(hasAny());
  return elems[makeGoodIndex(index - 1)];
}

Vec2 ViewObject::MovementQueue::getTotalMovement(int moveCounter) const {
  Vec2 ret;
  bool attack = false;
  for (int i : Range(min<int>(totalMoves, elems.size())))
    if (elems[i].moveCounter >= moveCounter) {
      if (elems[i].type != MovementInfo::MOVE/* && ret.length8() == 0*/) {
        attack = true;
        ret = elems[i].getDir();
      } else {
        if (attack) {
          attack = false;
          ret = Vec2(0, 0);
        }
        ret += elems[i].getDir();
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

ViewObject& ViewObject::setModifier(Modifier mod, bool state) {
  if (state)
    modifiers.insert(mod);
  else
    modifiers.erase(mod);
  return *this;
}

bool ViewObject::hasModifier(Modifier mod) const {
  return modifiers.contains(mod);
}

EnumSet<ViewObject::Modifier> ViewObject::getAllModifiers() const {
  return modifiers;
}

EnumSet<CreatureStatus>& ViewObject::getCreatureStatus() {
  return status;
}

const EnumSet<CreatureStatus>& ViewObject::getCreatureStatus() const {
  return status;
}

ViewObject& ViewObject::setAttribute(Attribute attr, double d) {
  attributes[attr] = d;
  return *this;
}

optional<float> ViewObject::getAttribute(Attribute attr) const {
  if (attributes[attr] != noAttributeValue)
    return attributes[attr];
  else
    return none;
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

const char* ViewObject::getDescription() const {
  return description.c_str();
}

ViewObject&  ViewObject::setAttachmentDir(Dir dir) {
  attachmentDir = dir;
  return *this;
}

optional<Dir> ViewObject::getAttachmentDir() const {
  return attachmentDir;
}

ViewLayer ViewObject::layer() const {
  return viewLayer;
}

void ViewObject::setId(ViewId id) {
  resource_id = id;
}

void ViewObject::setColorVariant(Color color) {
  resource_id = ViewId(resource_id.data(), color);
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
  return resource_id;
}

#include "pretty_archive.h"
template<>
void ViewObject::serialize(PrettyInputArchive&, unsigned) {
}
