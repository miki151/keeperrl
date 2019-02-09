#include "stdafx.h"
#include "view_object_action.h"

const char* getText(ViewObjectAction action) {
  switch (action) {
    case ViewObjectAction::PET:
      return "pet";
    case ViewObjectAction::CHAT:
      return "chat";
    case ViewObjectAction::PUSH:
      return "push";
    case ViewObjectAction::ATTACK:
      return "attack";
    case ViewObjectAction::ATTACK_USING_WEAPON:
      return "attack using weapon";
    case ViewObjectAction::ATTACK_USING_ARM:
      return "punch";
    case ViewObjectAction::ATTACK_USING_LEG:
      return "kick";
    case ViewObjectAction::ATTACK_USING_BACK:
      return "attack using back";
    case ViewObjectAction::ATTACK_USING_HEAD:
      return "butt or bite";
    case ViewObjectAction::ATTACK_USING_WING:
      return "attack using wing";
    case ViewObjectAction::ATTACK_USING_TORSO:
      return "attack using torso";
    case ViewObjectAction::MOVE_NOW:
      return "move now";
    case ViewObjectAction::LOCK_DOOR:
      return "lock door";
    case ViewObjectAction::SKIP_TURN:
      return "skip turn";
    case ViewObjectAction::ADD_TO_TEAM:
      return "add to team";
    case ViewObjectAction::UNLOCK_DOOR:
      return "unlock door";
    case ViewObjectAction::ORDER_CAPTURE:
      return "order capture";
    case ViewObjectAction::SWAP_POSITION:
      return "swap position";
    case ViewObjectAction::SWITCH_LEADER:
      return "switch leader";
    case ViewObjectAction::WRITE_ON_BOARD:
      return "write on board";
    case ViewObjectAction::REMOVE_FROM_TEAM:
      return "remove from team";
    case ViewObjectAction::CANCEL_CAPTURE_ORDER:
      return "cancel capture order";
  }
}
