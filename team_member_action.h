#pragma once

#include "enums.h"

enum class TeamMemberAction {
  CHANGE_LEADER,
  REMOVE_MEMBER,
  MOVE_NOW
};

ViewObjectAction getText(TeamMemberAction);
