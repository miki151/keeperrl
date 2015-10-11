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
    CREATURE_GROUP_BUTTON,
    CREATURE_TASK_ACTION,
    CREATURE_EQUIPMENT_ACTION,
    CREATURE_CONTROL,
    CREATURE_RENAME,
    CREATURE_BANISH,
    CREATURE_WHIP,
    CREATURE_EXECUTE,
    CREATURE_TORTURE,
    CREATE_TEAM,
    EDIT_TEAM,
    CONFIRM_TEAM,
    CANCEL_TEAM,
    COMMAND_TEAM,
    ACTIVATE_TEAM,
    SET_TEAM_LEADER,
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
    CAST_SPELL,
    CONSUME,
    INVENTORY_ITEM,
};

struct BuildingInfo {
  Vec2 SERIAL(pos);
  int SERIAL(building);
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SVAR(pos) & SVAR(building);
  }
};

struct TeamLeaderInfo {
  TeamId SERIAL(team);
  UniqueEntity<Creature>::Id SERIAL(creatureId);
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SVAR(team) & SVAR(creatureId);
  }
};

struct InventoryItemInfo {
  vector<UniqueEntity<Item>::Id> SERIAL(items);
  ItemAction SERIAL(action);
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SVAR(items) & SVAR(action);
  }};

struct VillageActionInfo {
  int SERIAL(villageIndex);
  VillageAction SERIAL(action);
  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SVAR(villageIndex) & SVAR(action);
  }
};

struct TaskActionInfo {
  UniqueEntity<Creature>::Id SERIAL(creature);
  optional<MinionTask> SERIAL(switchTo);
  EnumSet<MinionTask> SERIAL(lock);
  template <class Archive> 
    void serialize(Archive& ar, const unsigned int version) {
      ar & SVAR(creature) & SVAR(switchTo) & SVAR(lock);
    }
};

struct EquipmentActionInfo {
  UniqueEntity<Creature>::Id SERIAL(creature);
  vector<UniqueEntity<Item>::Id> SERIAL(ids);
  optional<EquipmentSlot> SERIAL(slot);
  ItemAction SERIAL(action);
  template <class Archive> 
    void serialize(Archive& ar, const unsigned int version) {
      ar & SVAR(creature) & SVAR(ids) & SVAR(slot) & SVAR(action);
    }
};

struct RenameActionInfo {
  UniqueEntity<Creature>::Id SERIAL(creature);
  string SERIAL(name);
  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SVAR(creature) & SVAR(name);
  }
};

enum class SpellId;

class UserInput : public EnumVariant<UserInputId, TYPES(BuildingInfo, int, InventoryItemInfo, Vec2, TeamLeaderInfo,
    SpellId, VillageActionInfo, TaskActionInfo, EquipmentActionInfo, RenameActionInfo),
        ASSIGN(BuildingInfo,
            UserInputId::BUILD,
            UserInputId::LIBRARY,
            UserInputId::BUTTON_RELEASE),
        ASSIGN(int,
            UserInputId::TECHNOLOGY,
            UserInputId::DEITIES,
            UserInputId::CREATURE_BUTTON,
            UserInputId::CREATURE_GROUP_BUTTON,
            UserInputId::CREATURE_CONTROL,
            UserInputId::CREATURE_EXECUTE,
            UserInputId::CREATURE_TORTURE,
            UserInputId::CREATURE_WHIP,
            UserInputId::CREATURE_BANISH,
            UserInputId::ADD_TO_TEAM,
            UserInputId::EDIT_TEAM,
            UserInputId::CANCEL_TEAM,
            UserInputId::ACTIVATE_TEAM,
            UserInputId::PICK_UP_ITEM,
            UserInputId::PICK_UP_ITEM_MULTI,
            UserInputId::MESSAGE_INFO,
            UserInputId::GO_TO_VILLAGE,
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
