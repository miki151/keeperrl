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

template <class Archive>
void EventGenerator::serialize(Archive& ar, const unsigned int) {
  ar & SUBCLASS(OwnedObject<EventGenerator>);
  serializeAll(ar, listeners);
}
SERIALIZABLE(EventGenerator);
