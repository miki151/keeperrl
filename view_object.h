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

#ifndef _VIEW_OBJECT_H
#define _VIEW_OBJECT_H

#include "debug.h"
#include "enums.h"
#include "util.h"

enum class ViewLayer {
  CREATURE,
  LARGE_ITEM,
  ITEM,
  FLOOR,
  FLOOR_BACKGROUND,
};

const static vector<ViewLayer> allLayers =
    {ViewLayer::FLOOR_BACKGROUND, ViewLayer::FLOOR, ViewLayer::ITEM, ViewLayer::LARGE_ITEM, ViewLayer::CREATURE};

ENUM_HASH(ViewLayer);

RICH_ENUM(ViewObjectModifier, BLIND, PLAYER, HIDDEN, INVISIBLE, ILLUSION, POISONED, CASTS_SHADOW, PLANNED, LOCKED,
    ROUND_SHADOW, MOVE_UP, TEAM_HIGHLIGHT, DARK);
RICH_ENUM(ViewObjectAttribute, BLEEDING, BURNING, HEIGHT, ATTACK, DEFENSE, LEVEL, WATER_DEPTH, EFFICIENCY);

class ViewObject {
  public:
  typedef ViewObjectModifier Modifier;
  typedef ViewObjectAttribute Attribute;
  ViewObject(ViewId id, ViewLayer l, const string& description);

  enum EnemyStatus { HOSTILE, FRIENDLY, UNKNOWN };
  void setEnemyStatus(EnemyStatus);
  bool isHostile() const;
  bool isFriendly() const;

  ViewObject& setModifier(Modifier);
  ViewObject& removeModifier(Modifier);
  bool hasModifier(Modifier) const;

  static void setHallu(bool);

  ViewObject& setAttribute(Attribute, double);
  double getAttribute(Attribute) const;

  string getDescription(bool stats = false) const;
  string getBareDescription() const;

  ViewLayer layer() const;
  ViewId id() const;
  void setId(ViewId);

  const static ViewObject& unknownMonster();
  const static ViewObject& empty();
  const static ViewObject& mana();

  SERIALIZATION_DECL(ViewObject);

  private:
  string getAttributeString(Attribute) const;
  EnemyStatus SERIAL2(enemyStatus, UNKNOWN);
  EnumSet<Modifier> SERIAL(modifiers);
  EnumMap<Attribute, double> SERIAL(attributes);
  ViewId SERIAL(resource_id);
  ViewLayer SERIAL(viewLayer);
  string SERIAL(description);
};

#endif
