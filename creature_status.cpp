#include "stdafx.h"
#include "creature_status.h"
#include "color.h"
#include "t_string.h"

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

TStringId getName(CreatureStatus status) {
  switch (status) {
    case CreatureStatus::PRISONER:
      return TStringId("PRISONER_STATUS");
    case CreatureStatus::CIVILIAN:
      return TStringId("CIVILIAN_STATUS");
    case CreatureStatus::LEADER:
      return TStringId("TRIBE_LEADER_STATUS");
    case CreatureStatus::FIGHTER:
      return TStringId("FIGHTER_STATUS");
  }
}

optional<TStringId> getDescription(CreatureStatus status) {
  switch (status) {
    case CreatureStatus::PRISONER:
      return TStringId("PRISONER_STATUS_DESCRIPTION");
    case CreatureStatus::CIVILIAN:
      return none;
    case CreatureStatus::LEADER:
      return TStringId("TRIBE_LEADER_STATUS_DESCRIPTION");
    case CreatureStatus::FIGHTER:
      return TStringId("FIGHTER_STATUS_DESCRIPTION");
  }
}

EnumSet<CreatureStatus> getDisplayedOnMinions() {
  return {CreatureStatus::PRISONER};
}

