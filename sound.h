#ifndef _SOUND_H
#define _SOUND_H

#include "util.h"
#include "position.h"

RICH_ENUM(SoundId,
  BEAST_DEATH,
  HUMANOID_DEATH,
  BLUNT_DAMAGE,
  BLADE_DAMAGE,
  BLUNT_NO_DAMAGE,
  BLADE_NO_DAMAGE,
  MISSED_ATTACK,
  DIG_MARK,
  DIG_UNMARK,
  ADD_CONSTRUCTION,
  REMOVE_CONSTRUCTION,
  DIGGING,
  CONSTRUCTING
);

class Sound {
  public:
  Sound(SoundId);
  Sound& setPosition(const Position&);
  Sound& setPitch(double);
  const optional<Position>& getPosition() const;
  SoundId getId() const;
  double getPitch() const;

  private:
  SoundId id;
  optional<Position> position;
  double pitch = 1;
};

#endif
