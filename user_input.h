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
#include "village_action.h"
#include "minion_task.h"

class PlayerMessage;

#undef TECHNOLOGY

enum class UserInputId {
// common
    IDLE,
    REFRESH,
    DRAW_LEVEL_MAP,
    DRAW_WORLD_MAP,
    EXIT,
    MESSAGE_INFO,
// real-time actions
    BUILD,
    TILE_CLICK,
    LIBRARY,
    RECT_SELECTION,
    RECT_DESELECTION,
    BUTTON_RELEASE,
    ADD_TO_TEAM,
    ADD_GROUP_TO_TEAM,
    REMOVE_FROM_TEAM,
    CREATURE_BUTTON,
    CREATURE_BUTTON2,
    CREATURE_GROUP_BUTTON,
    CREATURE_TASK_ACTION,
    CREATURE_EQUIPMENT_ACTION,
    CREATURE_CONTROL,
    CREATURE_RENAME,
    CREATURE_BANISH,
    CREATURE_WHIP,
    CREATURE_EXECUTE,
    CREATURE_TORTURE,
    GO_TO_ENEMY,
    CREATE_TEAM,
    CREATE_TEAM_FROM_GROUP,
    CANCEL_TEAM,
    SELECT_TEAM,
    ACTIVATE_TEAM,
    TECHNOLOGY,
    VILLAGE_ACTION,
    GO_TO_VILLAGE,
    PAY_RANSOM,
    IGNORE_RANSOM,
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
    TRANSFER,
    SWAP_TEAM,
    CAST_SPELL,
    CONSUME,
    INVENTORY_ITEM,
    CHEAT_ATTRIBUTES
};

struct BuildingInfo {
  Vec2 SERIAL(pos);
  int SERIAL(building);
  SERIALIZE_ALL(pos, building);
};

struct TeamCreatureInfo {
  TeamId SERIAL(team);
  UniqueEntity<Creature>::Id SERIAL(creatureId);
  SERIALIZE_ALL(team, creatureId);
};

struct InventoryItemInfo {
  vector<UniqueEntity<Item>::Id> SERIAL(items);
  ItemAction SERIAL(action);
  SERIALIZE_ALL(items, action);
};

struct VillageActionInfo {
  int SERIAL(villageIndex);
  VillageAction SERIAL(action);
  SERIALIZE_ALL(villageIndex, action);
};

struct TaskActionInfo {
  UniqueEntity<Creature>::Id SERIAL(creature);
  optional<MinionTask> SERIAL(switchTo);
  EnumSet<MinionTask> SERIAL(lock);
  SERIALIZE_ALL(creature, switchTo, lock);
};

struct EquipmentActionInfo {
  UniqueEntity<Creature>::Id SERIAL(creature);
  vector<UniqueEntity<Item>::Id> SERIAL(ids);
  optional<EquipmentSlot> SERIAL(slot);
  ItemAction SERIAL(action);
  SERIALIZE_ALL(creature, ids, slot, action);
};

struct RenameActionInfo {
  UniqueEntity<Creature>::Id SERIAL(creature);
  string SERIAL(name);
  SERIALIZE_ALL(creature, name);
};

enum class SpellId;

class UserInput : public EnumVariant<UserInputId, TYPES(BuildingInfo, int, UniqueEntity<Creature>::Id,
    UniqueEntity<PlayerMessage>::Id, InventoryItemInfo, Vec2, TeamCreatureInfo, SpellId, VillageActionInfo,
    TaskActionInfo, EquipmentActionInfo, RenameActionInfo),
        ASSIGN(BuildingInfo,
            UserInputId::BUILD,
            UserInputId::LIBRARY,
            UserInputId::BUTTON_RELEASE),
        ASSIGN(UniqueEntity<Creature>::Id,
            UserInputId::CREATURE_BUTTON,
            UserInputId::CREATE_TEAM,
            UserInputId::CREATE_TEAM_FROM_GROUP,
            UserInputId::CREATURE_BUTTON2,
            UserInputId::CREATURE_GROUP_BUTTON,
            UserInputId::CREATURE_CONTROL,
            UserInputId::CREATURE_BANISH,
            UserInputId::CREATURE_WHIP,
            UserInputId::CREATURE_EXECUTE,
            UserInputId::CREATURE_TORTURE,
            UserInputId::GO_TO_ENEMY
            ),
        ASSIGN(UniqueEntity<PlayerMessage>::Id,
            UserInputId::MESSAGE_INFO
            ),
        ASSIGN(int,
            UserInputId::TECHNOLOGY,
            UserInputId::CANCEL_TEAM,
            UserInputId::ACTIVATE_TEAM,
            UserInputId::SELECT_TEAM,
            UserInputId::PICK_UP_ITEM,
            UserInputId::PICK_UP_ITEM_MULTI,
            UserInputId::GO_TO_VILLAGE),
        ASSIGN(InventoryItemInfo,
            UserInputId::INVENTORY_ITEM),
        ASSIGN(Vec2,
            UserInputId::TILE_CLICK,
            UserInputId::MOVE, 
            UserInputId::MOVE_TO, 
            UserInputId::TRAVEL, 
            UserInputId::FIRE,
            UserInputId::RECT_SELECTION,
            UserInputId::RECT_DESELECTION),
        ASSIGN(TeamCreatureInfo,
            UserInputId::ADD_TO_TEAM,
            UserInputId::ADD_GROUP_TO_TEAM,
            UserInputId::REMOVE_FROM_TEAM),
        ASSIGN(SpellId,
            UserInputId::CAST_SPELL),
        ASSIGN(VillageActionInfo,
            UserInputId::VILLAGE_ACTION),
        ASSIGN(TaskActionInfo,
            UserInputId::CREATURE_TASK_ACTION),
        ASSIGN(EquipmentActionInfo,
            UserInputId::CREATURE_EQUIPMENT_ACTION),
        ASSIGN(RenameActionInfo,
            UserInputId::CREATURE_RENAME)
        > {
  using EnumVariant::EnumVariant;
};

#endif
