#include "stdafx.h"
#include "event_listener.h"
#include "model.h"

EventListener::EventListener() {
}

EventListener::EventListener(Model* m) {
  subscribeTo(m);
}

void EventListener::subscribeTo(Model* m) {
  generator = m->eventGenerator.get();
  generator->addListener(this);
}
 
void EventListener::unsubscribe() {
  if (generator) {
    generator->removeListener(this);
    generator = nullptr;
  }
}

EventListener::~EventListener() {
  unsubscribe();
}

SERIALIZE_DEF(EventListener, generator);

