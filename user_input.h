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
#include "minion_activity.h"
#include "entity_set.h"
#include "team_member_action.h"
#include "tech_id.h"

class PlayerMessage;

#undef TECHNOLOGY

enum class UserInputId {
// common
  IDLE,
  REFRESH,
  DRAW_WORLD_MAP,
  SCROLL_TO_HOME,
  EXIT,
  MESSAGE_INFO,
  CHEAT_ATTRIBUTES,
  TUTORIAL_CONTINUE,
  TUTORIAL_GO_BACK,
  TAKE_SCREENSHOT,
  CANCEL_SCREENSHOT,
  AI_TYPE,
  SCROLL_STAIRS,
// real-time actions
  TILE_CLICK,
  RECT_SELECTION,
  RECT_CONFIRM,
  RECT_CANCEL,
  ADD_TO_TEAM,
  ADD_GROUP_TO_TEAM,
  REMOVE_FROM_TEAM,
  CREATURE_BUTTON,
  CREATURE_MAP_CLICK,
  CREATURE_MAP_CLICK_EXTENDED,
  CREATURE_GROUP_BUTTON,
  CREATURE_TASK_ACTION,
  CREATURE_EQUIPMENT_ACTION,
  EQUIPMENT_GROUP_ACTION,
  CREATURE_PROMOTE,
  CREATURE_CONTROL,
  CREATURE_RENAME,
  CREATURE_BANISH,
  CREATURE_CONSUME,
  CREATURE_LOCATE,
  CREATURE_DRAG_DROP,
  CREATURE_DRAG,
  ASSIGN_QUARTERS,
  IMMIGRANT_ACCEPT,
  IMMIGRANT_REJECT,
  IMMIGRANT_AUTO_ACCEPT,
  IMMIGRANT_AUTO_REJECT,
  TEAM_DRAG_DROP,
  CREATURE_GROUP_DRAG_ON_MAP,
  GO_TO_ENEMY,
  CREATE_TEAM,
  CREATE_TEAM_FROM_GROUP,
  CANCEL_TEAM,
  SELECT_TEAM,
  ACTIVATE_TEAM,
  WORKSHOP,
  WORKSHOP_ADD,
  WORKSHOP_CHANGE_COUNT,
  WORKSHOP_UPGRADE,
  WORKSHOP_TAB,
  LIBRARY_ADD,
  VILLAGE_ACTION,
  DISMISS_VILLAGE_INFO,
  SHOW_HISTORY,
  DISMISS_NEXT_WAVE,
  DISMISS_WARNING_WINDOW,
// turn-based actions
  MOVE,
  PICK_UP_ITEM,
  PICK_UP_ITEM_MULTI,
  PLAYER_COMMAND,
  TOGGLE_CONTROL_MODE,
  EXIT_CONTROL_MODE,
  TOGGLE_TEAM_ORDER,
  TEAM_MEMBER_ACTION,
  CAST_SPELL,
  INVENTORY_ITEM,
  INTRINSIC_ATTACK,
  PAY_DEBT,
  APPLY_EFFECT,
  CREATE_ITEM,
  SUMMON_ENEMY,
  LEVEL_UP
};

struct CreatureDropInfo {
  Vec2 pos;
  UniqueEntity<Creature>::Id creatureId;
};

struct CreatureGroupDropInfo {
  Vec2 pos;
  string group;
};

struct TeamDropInfo {
  Vec2 pos;
  TeamId teamId;
};

struct BuildingClickInfo {
  Vec2 pos;
  int building;
};

struct TeamCreatureInfo {
  TeamId team;
  UniqueEntity<Creature>::Id creatureId;
};

struct TeamGroupInfo {
  TeamId team;
  string group;
};

struct InventoryItemInfo {
  EntitySet<Item> items;
  ItemAction action;
};

struct VillageActionInfo {
  UniqueEntity<Collective>::Id id;
  VillageAction action;
};

struct TaskActionInfo {
  UniqueEntity<Creature>::Id creature;
  optional<MinionActivity> switchTo;
  EnumSet<MinionActivity> lock;
  EnumSet<MinionActivity> lockGroup;
  string groupName;
};

struct AIActionInfo {
  UniqueEntity<Creature>::Id creature;
  AIType switchTo;
  bool override;
  string groupName;
};

struct EquipmentActionInfo {
  UniqueEntity<Creature>::Id creature;
  EntitySet<Item> ids;
  optional<EquipmentSlot> slot;
  ItemAction action;
};

struct RenameActionInfo {
  UniqueEntity<Creature>::Id creature;
  string name;
};

struct TeamMemberActionInfo {
  TeamMemberAction action;
  UniqueEntity<Creature>::Id memberId;
};

struct AssignQuartersInfo {
  optional<int> index;
  UniqueEntity<Creature>::Id minionId;
};

struct DismissVillageInfo {
  UniqueEntity<Collective>::Id collectiveId;
  string infoText;
};

struct WorkshopUpgradeInfo {
  int itemIndex;
  vector<int> increases;
  int numItems;
};

struct WorkshopCountInfo {
  int itemIndex;
  int count;
  int newCount;
};

struct PromotionActionInfo {
  UniqueEntity<Creature>::Id minionId;
  int promotionIndex;
};

struct EquipmentGroupAction {
  string group;
  unordered_set<string> flip;
};

class UserInput : public EnumVariant<UserInputId, TYPES(BuildingClickInfo, int, UniqueEntity<Creature>::Id,
    UniqueEntity<PlayerMessage>::Id, InventoryItemInfo, Vec2, TeamCreatureInfo, TeamGroupInfo, VillageActionInfo,
    TaskActionInfo, EquipmentActionInfo, RenameActionInfo, CreatureDropInfo, CreatureGroupDropInfo, TeamDropInfo,
    string, TechId, TeamMemberActionInfo, AssignQuartersInfo, TeamOrder, DismissVillageInfo, WorkshopUpgradeInfo,
    WorkshopCountInfo, AIActionInfo, PromotionActionInfo, EquipmentGroupAction),
        ASSIGN(BuildingClickInfo,
            UserInputId::RECT_SELECTION,
            UserInputId::RECT_CONFIRM),
        ASSIGN(UniqueEntity<Creature>::Id,
            UserInputId::CREATURE_BUTTON,
            UserInputId::CREATE_TEAM,
            UserInputId::CREATURE_CONTROL,
            UserInputId::CREATURE_BANISH,
            UserInputId::CREATURE_CONSUME,
            UserInputId::CREATURE_LOCATE,
            UserInputId::CREATURE_DRAG,
            UserInputId::GO_TO_ENEMY
        ),
        ASSIGN(PromotionActionInfo,
            UserInputId::CREATURE_PROMOTE
        ),
        ASSIGN(AIActionInfo,
            UserInputId::AI_TYPE
        ),
        ASSIGN(UniqueEntity<PlayerMessage>::Id,
            UserInputId::MESSAGE_INFO
            ),
        ASSIGN(int,
            UserInputId::WORKSHOP,
            UserInputId::WORKSHOP_ADD,
            UserInputId::WORKSHOP_TAB,
            UserInputId::CANCEL_TEAM,
            UserInputId::ACTIVATE_TEAM,
            UserInputId::SELECT_TEAM,
            UserInputId::PICK_UP_ITEM,
            UserInputId::PICK_UP_ITEM_MULTI,
            UserInputId::PLAYER_COMMAND,
            UserInputId::IMMIGRANT_ACCEPT,
            UserInputId::IMMIGRANT_REJECT,
            UserInputId::IMMIGRANT_AUTO_ACCEPT,
            UserInputId::IMMIGRANT_AUTO_REJECT,
            UserInputId::CAST_SPELL,
            UserInputId::SCROLL_STAIRS
        ),
        ASSIGN(InventoryItemInfo,
            UserInputId::INVENTORY_ITEM,
            UserInputId::INTRINSIC_ATTACK
        ),
        ASSIGN(Vec2,
            UserInputId::TILE_CLICK,
            UserInputId::CREATURE_MAP_CLICK,
            UserInputId::CREATURE_MAP_CLICK_EXTENDED,
            UserInputId::MOVE,
            UserInputId::TAKE_SCREENSHOT),
        ASSIGN(TeamCreatureInfo,
            UserInputId::ADD_TO_TEAM,
            UserInputId::REMOVE_FROM_TEAM),
        ASSIGN(TeamGroupInfo,
            UserInputId::ADD_GROUP_TO_TEAM),
        ASSIGN(VillageActionInfo,
            UserInputId::VILLAGE_ACTION),
        ASSIGN(TaskActionInfo,
            UserInputId::CREATURE_TASK_ACTION),
        ASSIGN(EquipmentActionInfo,
            UserInputId::CREATURE_EQUIPMENT_ACTION),
        ASSIGN(RenameActionInfo,
            UserInputId::CREATURE_RENAME),
        ASSIGN(CreatureGroupDropInfo,
            UserInputId::CREATURE_GROUP_DRAG_ON_MAP),
        ASSIGN(CreatureDropInfo,
            UserInputId::CREATURE_DRAG_DROP),
        ASSIGN(TeamDropInfo,
            UserInputId::TEAM_DRAG_DROP),
        ASSIGN(TeamMemberActionInfo,
            UserInputId::TEAM_MEMBER_ACTION),
        ASSIGN(AssignQuartersInfo,
            UserInputId::ASSIGN_QUARTERS),
        ASSIGN(DismissVillageInfo,
            UserInputId::DISMISS_VILLAGE_INFO),
        ASSIGN(string,
            UserInputId::CREATE_ITEM,
            UserInputId::APPLY_EFFECT,
            UserInputId::CREATE_TEAM_FROM_GROUP,
            UserInputId::CREATURE_GROUP_BUTTON,
            UserInputId::SUMMON_ENEMY
        ),
        ASSIGN(TechId,
            UserInputId::LIBRARY_ADD
        ),
        ASSIGN(TeamOrder,
            UserInputId::TOGGLE_TEAM_ORDER
        ),
        ASSIGN(WorkshopUpgradeInfo,
            UserInputId::WORKSHOP_UPGRADE
        ),
        ASSIGN(WorkshopCountInfo,
            UserInputId::WORKSHOP_CHANGE_COUNT
        ),
        ASSIGN(EquipmentGroupAction,
            UserInputId::EQUIPMENT_GROUP_ACTION
        )
        > {
  using EnumVariant::EnumVariant;
};

