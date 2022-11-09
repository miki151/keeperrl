#include "team_order.h"
#include "keybinding.h"

const char* getName(TeamOrder order) {
  switch (order) {
    case TeamOrder::FLEE:
      return "Ignore enemies";
    case TeamOrder::STAND_GROUND:
      return "Stand ground";
  }
}

const char* getDescription(TeamOrder order) {
  switch (order) {
    case TeamOrder::FLEE:
      return "Team members won't chase enemies and only fight those that are adjacent.";
    case TeamOrder::STAND_GROUND:
      return "Team members won't follow the leader.";
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
