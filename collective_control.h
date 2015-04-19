#ifndef _COLLECTIVE_CONTROL_H
#define _COLLECTIVE_CONTROL_H

#include "monster_ai.h"
#include "square_type.h"
#include "player_message.h"

class Creature;
class Collective;
class Tribe;

class CollectiveControl {
  public:
  CollectiveControl(Collective*);
  virtual MoveInfo getMove(Creature*);
  virtual void tick(double time) = 0;
  virtual void onCreatureKilled(const Creature* victim, const Creature* killer);
  virtual void update(Creature*);
  virtual void addMessage(const PlayerMessage&) {}
  virtual void addAssaultNotification(const Collective*, const vector<Creature*>&, const string& message) {}
  virtual void removeAssaultNotification(const Collective*) {}
  virtual void onDiscoveredLocation(const Location*) {}
  virtual void onConstructed(Vec2, SquareType) {}
  virtual void onNoEnemies() {}

  SERIALIZATION_DECL(CollectiveControl);

  virtual ~CollectiveControl();

  static PCollectiveControl idle(Collective*);

  template <class Archive>
  static void registerTypes(Archive& ar, int version);

  const vector<Creature*>& getCreatures() const;
  vector<Creature*>& getCreatures();

  protected:
  Collective* getCollective();
  const Collective* getCollective() const;

  private:
  Collective* SERIAL(collective) = nullptr;
};


#endif
