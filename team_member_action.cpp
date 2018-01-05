#include "team_member_action.h"


const char* getText(TeamMemberAction action) {
  switch (action) {
    case TeamMemberAction::CHANGE_LEADER:
      return "Switch leader";
    case TeamMemberAction::REMOVE_MEMBER:
      return "Remove from team";
    case TeamMemberAction::MOVE_NOW:
      return "Move now";
  }
}
