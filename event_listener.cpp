#include "stdafx.h"
#include "event_listener.h"
#include "model.h"
#include "collective.h"
#include "player_control.h"
#include "village_control.h"
#include "player.h"

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

bool EventListener::isSubscribed() const {
  return !generator;
}

EventListener::~EventListener() {
  unsubscribe();
}

SERIALIZE_DEF(EventListener, generator);


