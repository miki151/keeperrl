#include "team_order.h"

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

bool conflict(TeamOrder o1, TeamOrder o2) {
  return o1 != o2;
}
