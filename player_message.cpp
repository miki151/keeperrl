#include "stdafx.h"
#include "player_message.h"


PlayerMessage::PlayerMessage(const string& t, Priority p) : text(makeSentence(t)), priority(p), freshness(1) {}
PlayerMessage::PlayerMessage(const char* t, Priority p) : text(makeSentence(t)), priority(p), freshness(1) {}

string PlayerMessage::getText() const {
  if (isClickable())
    return text.substr(0, text.size() - 1);
  return text;
}

double PlayerMessage::getFreshness() const {
  return freshness;
}

void PlayerMessage::setFreshness(double val) {
  freshness = val;
}

PlayerMessage::Priority PlayerMessage::getPriority() const {
  return priority;
}

PlayerMessage& PlayerMessage::setPosition(Vec2 pos) {
  position = pos;
  return *this;
}

Optional<Vec2> PlayerMessage::getPosition() const {
  return position;
}

PlayerMessage& PlayerMessage::setCreature(UniqueEntity<Creature>::Id id) {
  creature = id;
  return *this;
}

Optional<UniqueEntity<Creature>::Id> PlayerMessage::getCreature() const {
  return creature;
}

bool PlayerMessage::isClickable() const {
  return position || creature;
}

template <class Archive> 
void PlayerMessage::serialize(Archive& ar, const unsigned int version) { 
  ar & SUBCLASS(UniqueEntity) 
     & SVAR(text)
     & SVAR(priority)
     & SVAR(freshness)
     & SVAR(position)
     & SVAR(creature);
  CHECK_SERIAL;
}

SERIALIZABLE(PlayerMessage);
SERIALIZATION_CONSTRUCTOR_IMPL(PlayerMessage);
