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

#pragma once

#include "debug.h"
#include "enums.h"
#include "util.h"
#include "unique_entity.h"
#include "view_layer.h"
#include "attr_type.h"
#include "game_time.h"
#include "movement_info.h"
#include "creature_status.h"
#include "view_object_modifier.h"
#include "fx_variant_name.h"
#include "view_object_action.h"
#include "view_id.h"
#include "color.h"

RICH_ENUM(ViewObjectAttribute, HEALTH, WATER_DEPTH, LUXURY, MORALE, FLANKED_MOD, BURNING);

class ViewObject {
  public:
  typedef ViewObjectModifier Modifier;
  typedef ViewObjectAttribute Attribute;
  ViewObject(ViewId id, ViewLayer l, const string& description);
  ViewObject(ViewId id, ViewLayer l);

  ViewObject& setModifier(Modifier, bool state = true);
  bool hasModifier(Modifier) const;
  EnumSet<Modifier> getAllModifiers() const;

  EnumSet<CreatureStatus>& getCreatureStatus();
  const EnumSet<CreatureStatus>& getCreatureStatus() const;

  ViewObject& setAttachmentDir(Dir);
  optional<Dir> getAttachmentDir() const;

  ViewObject& setAttribute(Attribute, double);
  ViewObject& resetAttribute(Attribute);
  optional<float> getAttribute(Attribute) const;

  using CreatureAttributes = vector<pair<ViewId, std::uint16_t>>;
  void setCreatureAttributes(CreatureAttributes);
  const optional<CreatureAttributes>& getCreatureAttributes() const;

  const char* getDescription() const;

  ViewLayer layer() const;
  void setLayer(ViewLayer);
  ViewId id() const;
  void setId(ViewId);
  void setId(ViewIdList);
  void setColorVariant(Color);

  void setGoodAdjectives(const string&);
  void setBadAdjectives(const string&);
  const string& getGoodAdjectives() const;
  const string& getBadAdjectives() const;

  void setDescription(const string&);

  void addMovementInfo(MovementInfo, GenericId);
  void clearMovementInfo();
  bool hasAnyMovementInfo() const;
  const MovementInfo& getLastMovementInfo() const;
  Vec2 getMovementInfo(int moveCounter) const;

  void setGenericId(GenericId);
  optional<GenericId> getGenericId() const;

  void setClickAction(ViewObjectAction);
  optional<ViewObjectAction> getClickAction() const;
  void setExtendedActions(EnumSet<ViewObjectAction>);
  const EnumSet<ViewObjectAction>& getExtendedActions() const;
  ViewIdList getViewIdList() const;

  SERIALIZATION_DECL(ViewObject)

  EnumSet<FXVariantName> particleEffects;
  vector<ViewId> partIds;

  private:
  EnumSet<Modifier> SERIAL(modifiers);
  EnumSet<CreatureStatus> SERIAL(status);
  EnumMap<Attribute, float> SERIAL(attributes);
  ViewId SERIAL(resource_id);
  ViewLayer SERIAL(viewLayer);
  string SERIAL(description);
  optional<Dir> SERIAL(attachmentDir);
  GenericId SERIAL(genericId) = 0;
  string SERIAL(goodAdjectives);
  string SERIAL(badAdjectives);
  optional<CreatureAttributes> SERIAL(creatureAttributes);
  optional<ViewObjectAction> SERIAL(clickAction);
  EnumSet<ViewObjectAction> SERIAL(extendedActions);

  class MovementQueue {
    public:
    void add(MovementInfo);
    const MovementInfo& getLast() const;
    Vec2 getTotalMovement(int moveCounter) const;
    bool hasAny() const;
    void clear();

    private:
    int makeGoodIndex(int index) const;
    std::array<MovementInfo, 6> elems;
    std::uint8_t index = 0;
    std::uint8_t totalMoves = 0;
  };
  heap_optional<MovementQueue> movementQueue;
};

static_assert(std::is_nothrow_move_constructible<ViewObject>::value, "T should be noexcept MoveConstructible");
