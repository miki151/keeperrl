#include "stdafx.h"

using namespace std;

Quest::Quest() : state(State::CLOSED) {}

bool Quest::isClosed() const {
  return state == State::CLOSED;
}

bool Quest::isOpen() const {
  return state == State::OPEN;
}

bool Quest::isFinished() const {
  return state == State::FINISHED;
}

void Quest::open() {
  state = State::OPEN;
}

void Quest::finish() {
  Debug() << "Quest finished";
  state = State::FINISHED;
}

void Quest::initialize() {
  quests.insert(make_pair(QuestName::POKPOK, Quest()));
}

Quest& Quest::get(QuestName name) {
  TRY(return quests.at(name), "Unknown quest.");
}

map<QuestName, Quest> Quest::quests;
