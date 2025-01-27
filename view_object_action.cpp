#include "stdafx.h"
#include "view_object_action.h"
#include "t_string.h"

TStringId getText(ViewObjectAction action) {
  switch (action) {
    case ViewObjectAction::PET:
      return TStringId("PET_ACTION");
    case ViewObjectAction::MOUNT:
      return TStringId("MOUNT_ACTION");
    case ViewObjectAction::DISMOUNT:
      return TStringId("DISMOUNT_ACTION");
    case ViewObjectAction::CHAT:
      return TStringId("CHAT_ACTION");
    case ViewObjectAction::PUSH:
      return TStringId("PUSH_ACTION");
    case ViewObjectAction::ATTACK:
      return TStringId("ATTACK_ACTION");
    case ViewObjectAction::MOVE_NOW:
      return TStringId("MOVE_NOW_ACTION");
    case ViewObjectAction::LOCK_DOOR:
      return TStringId("LOCK_DOOR_ACTION");
    case ViewObjectAction::SKIP_TURN:
      return TStringId("SKIP_TURN_ACTION");
    case ViewObjectAction::ADD_TO_TEAM:
      return TStringId("ADD_TO_TEAM_ACTION");
    case ViewObjectAction::UNLOCK_DOOR:
      return TStringId("UNLOCK_DOOR_ACTION");
    case ViewObjectAction::ORDER_CAPTURE:
      return TStringId("ORDER_CAPTURE_ACTION");
    case ViewObjectAction::SWAP_POSITION:
      return TStringId("SWAP_POSITION_ACTION");
    case ViewObjectAction::SWITCH_LEADER:
      return TStringId("SWITCH_LEADER_ACTION");
    case ViewObjectAction::WRITE_ON_BOARD:
      return TStringId("WRITE_ON_BOARD_ACTION");
    case ViewObjectAction::REMOVE_FROM_TEAM:
      return TStringId("REMOVE_FROM_TEAM_ACTION");
    case ViewObjectAction::CANCEL_CAPTURE_ORDER:
      return TStringId("CANCEL_CAPTURE_ORDER_ACTION");
  }
}
