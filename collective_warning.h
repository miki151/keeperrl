#pragma once

#include "util.h"

RICH_ENUM(CollectiveWarning,
    DIGGING,
    RESOURCE_STORAGE,
    EQUIPMENT_STORAGE,
    LIBRARY,
    BEDS,
    TRAINING,
    TRAINING_UPGRADE,
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
    MANA,
    MORE_LIGHTS
);

class Collective;

class CollectiveWarnings {
  public:

  typedef CollectiveWarning Warning;

  void considerWarnings(Collective*);
  bool isWarning(Warning) const;
  void setWarning(Warning, bool state = true);
  optional<const char*> getNextWarning(double localTime);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  void considerWeaponWarning(Collective*);
  void considerMoraleWarning(Collective*);
  void considerTorchesWarning(Collective*);
  void considerTrainingRoomWarning(Collective*);
  const char*getText(Warning w);
  EnumSet<Warning> SERIAL(warnings);
  EnumMap<CollectiveWarning, optional<double>> SERIAL(warningTimes);
  double SERIAL(lastWarningTime) = -10000;
};

