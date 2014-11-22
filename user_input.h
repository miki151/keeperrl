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

#ifndef _USER_INPUT_H
#define _USER_INPUT_H

#include "util.h"
#include "enum_variant.h"
#include "unique_entity.h"

enum class UserInputId {
// common
    IDLE,
    REFRESH,
    DRAW_LEVEL_MAP,
    EXIT,
// real-time actions
    BUILD,
    LIBRARY,
    DEITIES,
    RECT_SELECTION,
    RECT_DESELECTION,
    POSSESS,
    BUTTON_RELEASE,
    CREATURE_BUTTON,
    CREATE_TEAM,
    EDIT_TEAM,
    CANCEL_TEAM,
    COMMAND_TEAM,
    SET_TEAM_LEADER,
    TECHNOLOGY,
// turn-based actions
    MOVE,
    MOVE_TO,
    TRAVEL,
    FIRE,
    PICK_UP,
    EXT_PICK_UP,
    PICK_UP_ITEM,
    DROP,
    EXT_DROP,
    SHOW_INVENTORY,
    APPLY_ITEM,
    EQUIPMENT,
    THROW,
    THROW_DIR,
    SHOW_HISTORY,
    HIDE,
    PAY_DEBT,
    CHAT,
    WAIT,
    UNPOSSESS,
    CAST_SPELL,
    CONSUME,
};

struct BuildingInfo : public NamedTupleBase<Vec2, int> {
  NAMED_TUPLE_STUFF(BuildingInfo);
  NAME_ELEM(0, pos);
  NAME_ELEM(1, building);
};

struct TeamLeaderInfo : public NamedTupleBase<TeamId, UniqueEntity<Creature>::Id> {
  NAMED_TUPLE_STUFF(TeamLeaderInfo);
  NAME_ELEM(0, team);
  NAME_ELEM(1, creatureId);
};

typedef EnumVariant<UserInputId, TYPES(BuildingInfo, int, Vec2, TeamLeaderInfo),
        ASSIGN(BuildingInfo,
            UserInputId::BUILD,
            UserInputId::LIBRARY,
            UserInputId::BUTTON_RELEASE),
        ASSIGN(int,
            UserInputId::TECHNOLOGY,
            UserInputId::DEITIES,
            UserInputId::CREATURE_BUTTON,
            UserInputId::EDIT_TEAM,
            UserInputId::CANCEL_TEAM,
            UserInputId::PICK_UP_ITEM,
            UserInputId::COMMAND_TEAM),
        ASSIGN(Vec2,
            UserInputId::POSSESS,
            UserInputId::MOVE, 
            UserInputId::MOVE_TO, 
            UserInputId::TRAVEL, 
            UserInputId::FIRE,
            UserInputId::THROW_DIR,
            UserInputId::RECT_SELECTION,
            UserInputId::RECT_DESELECTION),
        ASSIGN(TeamLeaderInfo,
            UserInputId::SET_TEAM_LEADER)> UserInput;

#endif
