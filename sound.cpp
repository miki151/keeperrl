#include "stdafx.h"
#include "sound.h"
#include "position.h"

Sound::Sound(SoundId i) : id(i) {
}

SERIALIZE_DEF(Sound, NAMED(id), OPTION(volume), OPTION(pitch))
SERIALIZATION_CONSTRUCTOR_IMPL(Sound)

Sound& Sound::setPosition(const Position& p) {
  position = p;
  return *this;
}

Sound& Sound::setPitch(double v) {
  pitch = v;
  return *this;
}

Sound& Sound::setVolume(double v) {
  volume = v;
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

double Sound::getVolume() const {
  return volume;
}

#include "pretty_archive.h"
template void Sound::serialize(PrettyInputArchive&, unsigned);
