#include "stdafx.h"
#include "creature_status.h"
#include "renderer.h"

Color getColor(CreatureStatus status) {
  switch (status) {
    case CreatureStatus::CIVILIAN:
      return Color::YELLOW;
    case CreatureStatus::LEADER:
      return Color::PURPLE;
    case CreatureStatus::FIGHTER:
      return Color::RED;
  }
}

const char* getName(CreatureStatus status) {
  switch (status) {
    case CreatureStatus::CIVILIAN:
      return "Civilian";
    case CreatureStatus::LEADER:
      return "Tribe leader";
    case CreatureStatus::FIGHTER:
      return "Fighter";
  }
}

const char* getDescription(CreatureStatus status) {
  switch (status) {
    case CreatureStatus::CIVILIAN:
      return "Can be captured by conquering tribe";
    case CreatureStatus::LEADER:
      return "Killing will stop immigration";
    case CreatureStatus::FIGHTER:
      return "Must be killed to conquer tribe";
  }
}
