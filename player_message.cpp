#include "stdafx.h"
#include "player_message.h"
#include "location.h"

PlayerMessage::PlayerMessage(const string& t, Priority p) : text(makeSentence(t)), priority(p), freshness(1) {}
PlayerMessage::PlayerMessage(const char* t, Priority p) : text(makeSentence(t)), priority(p), freshness(1) {}

PlayerMessage::PlayerMessage(const string& title, const string& t) : text(t), priority(NORMAL), 
    announcementTitle(title) {}

PlayerMessage PlayerMessage::announcement(const string& title, const string& text) {
  return PlayerMessage(title, text);
}

optional<string> PlayerMessage::getAnnouncementTitle() const {
  return announcementTitle;
}

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

PlayerMessage& PlayerMessage::setPosition(Position pos) {
  position = pos;
  return *this;
}

optional<Position> PlayerMessage::getPosition() const {
  return position;
}

PlayerMessage& PlayerMessage::setCreature(UniqueEntity<Creature>::Id id) {
  creature = id;
  return *this;
}

optional<UniqueEntity<Creature>::Id> PlayerMessage::getCreature() const {
  return creature;
}

PlayerMessage& PlayerMessage::setLocation(const Location* l) {
  location = l;
  return *this;
}

const Location* PlayerMessage::getLocation() const {
  return location;
}

bool PlayerMessage::isClickable() const {
  return position || creature || location;
}

int PlayerMessage::getHash() const {
  return combineHash(text, priority, freshness);
}

template <class Archive> 
void PlayerMessage::serialize(Archive& ar, const unsigned int version) { 
  ar & SUBCLASS(UniqueEntity) 
     & SVAR(text)
     & SVAR(priority)
     & SVAR(freshness)
     & SVAR(announcementTitle)
     & SVAR(position)
     & SVAR(creature)
     & SVAR(location);
}

SERIALIZABLE(PlayerMessage);
SERIALIZATION_CONSTRUCTOR_IMPL(PlayerMessage);
