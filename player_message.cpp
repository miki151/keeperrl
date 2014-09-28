#include "stdafx.h"
#include "player_message.h"


PlayerMessage::PlayerMessage(const string& t, Priority p) : text(makeSentence(t)), priority(p), freshness(1) {}
PlayerMessage::PlayerMessage(const char* t, Priority p) : text(makeSentence(t)), priority(p), freshness(1) {}

const string& PlayerMessage::getText() const {
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

template <class Archive> 
void PlayerMessage::serialize(Archive& ar, const unsigned int version) { 
  ar & SVAR(text) & SVAR(priority) & SVAR(freshness);
  CHECK_SERIAL;
}

SERIALIZABLE(PlayerMessage);
SERIALIZATION_CONSTRUCTOR_IMPL(PlayerMessage);
