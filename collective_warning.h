#pragma once

#include "util.h"
#include "game_time.h"

RICH_ENUM(CollectiveWarning,
    DIGGING,
    RESOURCE_STORAGE,
    EQUIPMENT_STORAGE,
    LIBRARY,
    BEDS,
    TRAINING,
    TRAINING_UPGRADE,
    GUARD_POSTS,
    NO_HATCHERY,
    WORKSHOP,
    NO_WEAPONS,
    LOW_MORALE,
    GRAVES,
    CHESTS,
    NO_PRISON,
    LARGER_PRISON,
    TORTURE_ROOM,
    MORE_CHESTS,
    DUNGEON_LEVEL,
    MORE_LIGHTS
);

class Collective;

class CollectiveWarnings {
  public:

  typedef CollectiveWarning Warning;

  void disable();
  void considerWarnings(Collective*);
  bool isWarning(Warning) const;
  void setWarning(Warning, bool state = true);
  optional<const char*> getNextWarning(LocalTime localTime);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  void considerWeaponWarning(Collective*);
  void considerMoraleWarning(Collective*);
  void considerTorchesWarning(Collective*);
  const char*getText(Warning w);
  EnumSet<Warning> SERIAL(warnings);
  EnumMap<CollectiveWarning, optional<LocalTime>> SERIAL(warningTimes);
  LocalTime SERIAL(lastWarningTime) = LocalTime(100);
};

