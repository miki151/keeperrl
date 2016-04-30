#include "stdafx.h"
#include "event_generator.h"
#include "creature_listener.h"

template<typename Listener>
EventGenerator<Listener>::~EventGenerator<Listener>() {
  for (Listener* l : copyOf(listeners))
    l->unsubscribeFromCreature(this);
}

template<typename Listener>
void EventGenerator<Listener>::addListener(Listener* l) {
  listeners.push_back(l);
}

template<typename Listener>
void EventGenerator<Listener>::removeListener(Listener* l) {
  removeElement(listeners, l);
}

template<typename Listener>
const vector<Listener*> EventGenerator<Listener>::getListeners() const {
  return listeners;
}

template <class Listener>
template <class Archive> 
void EventGenerator<Listener>::serialize(Archive& ar, const unsigned int version) {
  serializeAll(ar, listeners);
}

SERIALIZABLE_TMPL(EventGenerator, CreatureListener);
