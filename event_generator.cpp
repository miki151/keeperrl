#include "stdafx.h"
#include "event_generator.h"
#include "event_listener.h"

void EventGenerator::addEvent(const GameEvent& e) {
  for (auto& l : listeners)
    l.second->onEvent(e);
}

void EventGenerator::removeListener(EventGenerator::SubscriberId id) {
  CHECK(listeners.count(id));
  listeners.erase(id);
}

SERIALIZE_DEF(EventGenerator, listeners)
