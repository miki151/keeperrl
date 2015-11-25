#include "stdafx.h"
#include "sound.h"
#include "position.h"

Sound::Sound(SoundId i) : id(i) {
}

Sound& Sound::setPosition(const Position& p) {
  position = p;
  return *this;
}

Sound& Sound::setPitch(double v) {
  pitch = v;
  return *this;
}

SoundId Sound::getId() const {
  return id;
}

const optional<Position>& Sound::getPosition() const {
  return position;
}

double Sound::getPitch() const {
  return pitch;
}
