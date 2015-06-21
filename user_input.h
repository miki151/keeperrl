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
    MESSAGE_INFO,
// real-time actions
    BUILD,
    LIBRARY,
    DEITIES,
    RECT_SELECTION,
    RECT_DESELECTION,
    POSSESS,
    BUTTON_RELEASE,
    ADD_TO_TEAM,
    CREATURE_BUTTON,
    CREATE_TEAM,
    EDIT_TEAM,
    CONFIRM_TEAM,
    CANCEL_TEAM,
    COMMAND_TEAM,
    ACTIVATE_TEAM,
    SET_TEAM_LEADER,
    TECHNOLOGY,
// turn-based actions
    MOVE,
    MOVE_TO,
    TRAVEL,
    FIRE,
    PICK_UP_ITEM,
    PICK_UP_ITEM_MULTI,
    SHOW_HISTORY,
    HIDE,
    PAY_DEBT,
    CHAT,
    WAIT,
    UNPOSSESS,
    CAST_SPELL,
    CONSUME,
    INVENTORY_ITEM,
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

struct InventoryItemInfo : public NamedTupleBase<vector<UniqueEntity<Item>::Id>, ItemAction> {
  NAMED_TUPLE_STUFF(InventoryItemInfo);
  NAME_ELEM(0, items);
  NAME_ELEM(1, action);
};

enum class SpellId;

typedef EnumVariant<UserInputId, TYPES(BuildingInfo, int, InventoryItemInfo, Vec2, TeamLeaderInfo, SpellId),
        ASSIGN(BuildingInfo,
            UserInputId::BUILD,
            UserInputId::LIBRARY,
            UserInputId::BUTTON_RELEASE),
        ASSIGN(int,
            UserInputId::TECHNOLOGY,
            UserInputId::DEITIES,
            UserInputId::CREATURE_BUTTON,
            UserInputId::ADD_TO_TEAM,
            UserInputId::EDIT_TEAM,
            UserInputId::CANCEL_TEAM,
            UserInputId::ACTIVATE_TEAM,
            UserInputId::PICK_UP_ITEM,
            UserInputId::PICK_UP_ITEM_MULTI,
            UserInputId::MESSAGE_INFO,
            UserInputId::COMMAND_TEAM),
        ASSIGN(InventoryItemInfo,
            UserInputId::INVENTORY_ITEM),
        ASSIGN(Vec2,
            UserInputId::POSSESS,
            UserInputId::MOVE, 
            UserInputId::MOVE_TO, 
            UserInputId::TRAVEL, 
            UserInputId::FIRE,
            UserInputId::RECT_SELECTION,
            UserInputId::RECT_DESELECTION),
        ASSIGN(TeamLeaderInfo,
            UserInputId::SET_TEAM_LEADER),
        ASSIGN(SpellId,
            UserInputId::CAST_SPELL)> UserInput;

#endif
