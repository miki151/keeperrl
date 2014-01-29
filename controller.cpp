#include "stdafx.h"

#include "controller.h"
#include "map_memory.h"
#include "creature.h"

ControllerFactory::ControllerFactory(function<Controller* (Creature*)> f) : fun(f) {}

PController ControllerFactory::get(Creature* c) {
  return PController(fun(c));
}

const MapMemory& DoNothingController::getMemory(const Level* l) const {
  return MapMemory::empty();
}

bool DoNothingController::isPlayer() const {
  return false;
}

void DoNothingController::you(MsgType type, const string& param) const {
}

void DoNothingController::you(const string& param) const {
}

void DoNothingController::makeMove() {
  creature->wait();
}

void DoNothingController::onBump(Creature* c) {
}

