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

SERIALIZE_DEF(ViewObject, resource_id, viewLayer, description, modifiers, attributes, attachmentDir, genericId, goodAdjectives, badAdjectives, creatureAttributes, status, clickAction, extendedActions)

SERIALIZATION_CONSTRUCTOR_IMPL(ViewObject);

constexpr float noAttributeValue = -1234;

static const char* getDefaultDescription(const ViewId& id) {
  if (id == ViewId("unknown_monster"))
    return "Unknown creature";
  else
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

ViewIdList ViewObject::getViewIdList() const {
  return concat({id()}, partIds);
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

ViewObject& ViewObject::resetAttribute(Attribute attr) {
  attributes[attr] = noAttributeValue;
  return *this;
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

void ViewObject::setLayer(ViewLayer l) {
  viewLayer = l;
}

void ViewObject::setId(ViewId id) {
  resource_id = id;
}

void ViewObject::setId(ViewIdList id) {
  resource_id = id.front();
  if (id.size() > 1)
    partIds = id.getSubsequence(1);
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
