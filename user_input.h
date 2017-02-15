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

#include "util.h"
#include "enum_variant.h"
#include "unique_entity.h"
#include "village_action.h"
#include "minion_task.h"
#include "entity_set.h"

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
    CHEAT_ATTRIBUTES,
// real-time actions
    BUILD,
    TILE_CLICK,
    RECT_SELECTION,
    RECT_DESELECTION,
    RECT_CONFIRM,
    RECT_CANCEL,
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
    CREATURE_EXECUTE,
    CREATURE_CONSUME,
    CREATURE_DRAG_DROP,
    CREATURE_DRAG,
    IMMIGRANT_ACCEPT,
    IMMIGRANT_REJECT,
    IMMIGRANT_AUTO_ACCEPT,
    IMMIGRANT_AUTO_REJECT,
    TEAM_DRAG_DROP,
    GO_TO_ENEMY,
    CREATE_TEAM,
    CREATE_TEAM_FROM_GROUP,
    CANCEL_TEAM,
    SELECT_TEAM,
    ACTIVATE_TEAM,
    TECHNOLOGY,
    WORKSHOP,
    WORKSHOP_ADD,
    WORKSHOP_ITEM_ACTION,
    VILLAGE_ACTION,
    GO_TO_VILLAGE,
    PAY_RANSOM,
    IGNORE_RANSOM,
    SHOW_HISTORY,
// turn-based actions
    MOVE,
    TRAVEL,
    FIRE,
    PICK_UP_ITEM,
    PICK_UP_ITEM_MULTI,
    PLAYER_COMMAND,
    CAST_SPELL,
    INVENTORY_ITEM,
    PAY_DEBT,
};

struct CreatureDropInfo {
  Vec2 SERIAL(pos);
  UniqueEntity<Creature>::Id SERIAL(creatureId);
  SERIALIZE_ALL(pos, creatureId)
};

struct TeamDropInfo {
  Vec2 SERIAL(pos);
  TeamId SERIAL(teamId);
  SERIALIZE_ALL(pos, teamId)
};

struct BuildingInfo {
  Vec2 SERIAL(pos);
  int SERIAL(building);
  SERIALIZE_ALL(pos, building)
};

struct TeamCreatureInfo {
  TeamId SERIAL(team);
  UniqueEntity<Creature>::Id SERIAL(creatureId);
  SERIALIZE_ALL(team, creatureId)
};

struct InventoryItemInfo {
  EntitySet<Item> SERIAL(items);
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
  EntitySet<Item> SERIAL(ids);
  optional<EquipmentSlot> SERIAL(slot);
  ItemAction SERIAL(action);
  SERIALIZE_ALL(creature, ids, slot, action);
};

struct WorkshopQueuedActionInfo {
  int SERIAL(itemIndex);
  ItemAction SERIAL(action);
  SERIALIZE_ALL(itemIndex, action);
};

struct RenameActionInfo {
  UniqueEntity<Creature>::Id SERIAL(creature);
  string SERIAL(name);
  SERIALIZE_ALL(creature, name);
};

enum class SpellId;

class UserInput : public EnumVariant<UserInputId, TYPES(BuildingInfo, int, UniqueEntity<Creature>::Id,
    UniqueEntity<PlayerMessage>::Id, InventoryItemInfo, Vec2, TeamCreatureInfo, SpellId, VillageActionInfo,
    TaskActionInfo, EquipmentActionInfo, RenameActionInfo, WorkshopQueuedActionInfo, CreatureDropInfo, TeamDropInfo),
        ASSIGN(BuildingInfo,
            UserInputId::BUILD,
            UserInputId::RECT_SELECTION,
            UserInputId::RECT_CONFIRM),
        ASSIGN(UniqueEntity<Creature>::Id,
            UserInputId::CREATURE_BUTTON,
            UserInputId::CREATE_TEAM,
            UserInputId::CREATE_TEAM_FROM_GROUP,
            UserInputId::CREATURE_BUTTON2,
            UserInputId::CREATURE_GROUP_BUTTON,
            UserInputId::CREATURE_CONTROL,
            UserInputId::CREATURE_BANISH,
            UserInputId::CREATURE_EXECUTE,
            UserInputId::CREATURE_CONSUME,
            UserInputId::CREATURE_DRAG,
            UserInputId::GO_TO_ENEMY
            ),
        ASSIGN(UniqueEntity<PlayerMessage>::Id,
            UserInputId::MESSAGE_INFO
            ),
        ASSIGN(int,
            UserInputId::TECHNOLOGY,
            UserInputId::WORKSHOP,
            UserInputId::WORKSHOP_ADD,
            UserInputId::CANCEL_TEAM,
            UserInputId::ACTIVATE_TEAM,
            UserInputId::SELECT_TEAM,
            UserInputId::PICK_UP_ITEM,
            UserInputId::PICK_UP_ITEM_MULTI,
            UserInputId::GO_TO_VILLAGE,
            UserInputId::PLAYER_COMMAND,
            UserInputId::IMMIGRANT_ACCEPT,
            UserInputId::IMMIGRANT_REJECT,
            UserInputId::IMMIGRANT_AUTO_ACCEPT,
            UserInputId::IMMIGRANT_AUTO_REJECT
        ),
        ASSIGN(InventoryItemInfo,
            UserInputId::INVENTORY_ITEM),
        ASSIGN(Vec2,
            UserInputId::TILE_CLICK,
            UserInputId::MOVE, 
            UserInputId::TRAVEL, 
            UserInputId::FIRE,
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
        ASSIGN(WorkshopQueuedActionInfo,
            UserInputId::WORKSHOP_ITEM_ACTION),
        ASSIGN(RenameActionInfo,
            UserInputId::CREATURE_RENAME),
        ASSIGN(CreatureDropInfo,
            UserInputId::CREATURE_DRAG_DROP),
        ASSIGN(TeamDropInfo,
            UserInputId::TEAM_DRAG_DROP)
        > {
  using EnumVariant::EnumVariant;
};

