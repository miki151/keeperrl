#include "stdafx.h"

#include "controller.h"
#include "map_memory.h"

ControllerFactory::ControllerFactory(function<Controller* (Creature*)> f) : fun(f) {}

PController ControllerFactory::get(Creature* c) {
  return PController(fun(c));
}

const MapMemory& DoNothingController::getMemory(const Level* l) const {
  return MapMemory::empty();
}

