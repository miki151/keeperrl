#ifndef _CREATURE_LISTENER
#define _CREATURE_LISTENER

#include "util.h"
#include "event_generator.h"

class Creature;

class CreatureListener {
  public:
  ~CreatureListener();

  typedef EventGenerator<CreatureListener> Generator;

  void subscribeToCreature(Creature*);
  void unsubscribeFromCreature(Creature*);
  void unsubscribeFromCreature(Generator*);

  virtual void onKilled(Creature* victim, Creature* attacker) {}
  virtual void onMoved(Creature*) {}

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  private:
  vector<Generator*> SERIAL(creatures);
};

#endif
