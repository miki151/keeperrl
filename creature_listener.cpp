#include "stdafx.h"
#include "creature_listener.h"
#include "creature.h"

void CreatureListener::subscribeToCreature(Creature* c) {
  creatures.push_back(c->eventGenerator.get());
  c->eventGenerator->addListener(this);
}
 
void CreatureListener::unsubscribeFromCreature(CreatureListener::Generator* c) {
  removeElement(creatures, c);
  c->removeListener(this);
}

void CreatureListener::unsubscribeFromCreature(Creature* c) {
  unsubscribeFromCreature(c->eventGenerator.get());
}

CreatureListener::~CreatureListener() {
  for (Generator* c : creatures)
    c->removeListener(this);
}

template <class Archive> 
void CreatureListener::serialize(Archive& ar, const unsigned int version) {
  serializeAll(ar, creatures);
}

SERIALIZABLE(CreatureListener);


