#include "stdafx.h"
#include "sound.h"
#include "position.h"

Sound::Sound(SoundId i) : id(i) {
}

SERIALIZE_DEF(Sound, id, pitch)
SERIALIZATION_CONSTRUCTOR_IMPL(Sound)

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

#include "pretty_archive.h"
template void Sound::serialize(PrettyInputArchive&, unsigned);
