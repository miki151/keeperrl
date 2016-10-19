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

RICH_ENUM(ViewObjectModifier, BLIND, PLAYER, HIDDEN, INVISIBLE, ILLUSION, POISONED, PLANNED,
    TEAM_LEADER_HIGHLIGHT, TEAM_HIGHLIGHT, DRAW_MORALE, ROAD, NO_UP_MOVEMENT, REMEMBER, SPIRIT_DAMAGE);
RICH_ENUM(ViewObjectAttribute, WOUNDED, BURNING, ATTACK, DEFENSE, LEVEL, WATER_DEPTH, EFFICIENCY, MORALE);

struct MovementInfo {
  enum Type { MOVE, ATTACK };
  MovementInfo(Vec2, double, double, Type);
  MovementInfo();
  Vec2 direction;
  double tBegin;
  double tEnd;
  Type type;
};

class ViewObject {
  public:
  typedef ViewObjectModifier Modifier;
  typedef ViewObjectAttribute Attribute;
  ViewObject(ViewId id, ViewLayer l, const string& description);
  ViewObject(ViewId id, ViewLayer l);

  ViewObject& setModifier(Modifier);
  ViewObject& removeModifier(Modifier);
  bool hasModifier(Modifier) const;

  static void setHallu(bool);
  
  ViewObject& setAttachmentDir(Dir);
  optional<Dir> getAttachmentDir() const;

  ViewObject& setAttribute(Attribute, double);
  optional<float> getAttribute(Attribute) const;

  vector<string> getLegend() const;
  const char* getDescription() const;

  void setIndoors(bool);

  ViewLayer layer() const;
  ViewId id() const;
  void setId(ViewId);

  void setAdjectives(const vector<string>&);
  void setDescription(const string&);

  void addMovementInfo(MovementInfo);
  void clearMovementInfo();
  bool hasAnyMovementInfo() const;
  MovementInfo getLastMovementInfo() const;
  Vec2 getMovementInfo(double tBegin, double tEnd, UniqueEntity<Creature>::Id controlledId) const;

  void setCreatureId(UniqueEntity<Creature>::Id);
  optional<UniqueEntity<Creature>::Id> getCreatureId() const;

  const static ViewObject& unknownMonster();
  const static ViewObject& empty();
  const static ViewObject& mana();

  SERIALIZATION_DECL(ViewObject);

  private:
  string getAttributeString(Attribute) const;
  const char* getDefaultDescription() const;
  enum EnemyStatus { HOSTILE, FRIENDLY, UNKNOWN };
  EnumSet<Modifier> SERIAL(modifiers);
  EnumMap<Attribute, optional<float>> SERIAL(attributes);
  ViewId SERIAL(resource_id);
  ViewLayer SERIAL(viewLayer);
  optional<string> SERIAL(description);
  optional<Dir> SERIAL(attachmentDir);
  optional<UniqueEntity<Creature>::Id> SERIAL(creatureId);
  vector<string> SERIAL(adjectives);
  optional<bool> indoors;

  class MovementQueue {
    public:
    void add(MovementInfo);
    const MovementInfo& getLast() const;
    Vec2 getTotalMovement(double tBegin, double tEnd) const;
    bool hasAny() const;
    void clear();

    private:
    int makeGoodIndex(int index) const;
    std::array<MovementInfo, 6> elems;
    int index = 0;
    int totalMoves = 0;
  } movementQueue;
};

