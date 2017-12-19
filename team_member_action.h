#pragma once

enum class TeamMemberAction {
  CHANGE_LEADER,
  REMOVE_MEMBER,
  MOVE_NOW
};

extern const char* getText(TeamMemberAction);
