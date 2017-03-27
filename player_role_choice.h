#pragma once

#include "util.h"

RICH_ENUM(NonRoleChoice,
  TUTORIAL,
  LOAD_GAME,
  GO_BACK
);

using PlayerRoleChoice = variant<PlayerRole, NonRoleChoice>;
