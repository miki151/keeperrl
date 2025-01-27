#include "team_order.h"
#include "keybinding.h"
#include "t_string.h"

TStringId getName(TeamOrder order) {
  switch (order) {
    case TeamOrder::FLEE:
      return TStringId("DONT_CHASE_TEAM_ORDER");
    case TeamOrder::STAND_GROUND:
      return TStringId("STOP_TEAM_ORDER");
  }
}

TStringId getDescription(TeamOrder order) {
  switch (order) {
    case TeamOrder::FLEE:
      return TStringId("DONT_CHASE_TEAM_ORDER_DESCRIPTION");
    case TeamOrder::STAND_GROUND:
      return TStringId("STOP_TEAM_ORDER_DESCRIPTION");
  }
}

Keybinding getKeybinding(TeamOrder order) {
  switch (order) {
    case TeamOrder::FLEE:
      return Keybinding("IGNORE_ENEMIES");
    case TeamOrder::STAND_GROUND:
      return Keybinding("STAND_GROUND");
  }
}

bool conflict(TeamOrder o1, TeamOrder o2) {
  return o1 != o2;
}
