#include "team_member_action.h"
#include "view_object_action.h"

ViewObjectAction getText(TeamMemberAction action) {
  switch (action) {
    case TeamMemberAction::CHANGE_LEADER:
      return ViewObjectAction::SWITCH_LEADER;
    case TeamMemberAction::REMOVE_MEMBER:
      return ViewObjectAction::REMOVE_FROM_TEAM;
    case TeamMemberAction::MOVE_NOW:
      return ViewObjectAction::MOVE_NOW;
  }
}
