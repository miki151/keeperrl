#include "stdafx.h"
#include "creature_status.h"
#include "color.h"

Color getColor(CreatureStatus status) {
  switch (status) {
    case CreatureStatus::PRISONER:
      return Color::WHITE;
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
    case CreatureStatus::PRISONER:
      return "Prisoner";
    case CreatureStatus::CIVILIAN:
      return "Civilian";
    case CreatureStatus::LEADER:
      return "Tribe leader";
    case CreatureStatus::FIGHTER:
      return "Fighter";
  }
}

optional<const char*> getDescription(CreatureStatus status) {
  switch (status) {
    case CreatureStatus::PRISONER:
      return "Captured from a hostile tribe";
    case CreatureStatus::CIVILIAN:
      return none;
    case CreatureStatus::LEADER:
      return "Killing will stop immigration";
    case CreatureStatus::FIGHTER:
      return "Must be killed to conquer tribe";
  }
}

EnumSet<CreatureStatus> getDisplayedOnMinions() {
  return {CreatureStatus::PRISONER};
}

