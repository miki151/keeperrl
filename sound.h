#pragma once

#include "util.h"
#include "position.h"

class Sound {
  public:
  Sound(SoundId);
  Sound& setPosition(const Position&);
  Sound& setPitch(double);
  const optional<Position>& getPosition() const;
  SoundId getId() const;
  double getPitch() const;
  double getVolume() const;

  SERIALIZATION_DECL(Sound)

  private:
  SoundId SERIAL(id);
  optional<Position> position;
  double SERIAL(pitch) = 1.0;
  double SERIAL(volume) = 1.0;
};

