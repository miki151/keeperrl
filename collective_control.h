#ifndef _COLLECTIVE_CONTROL_H
#define _COLLECTIVE_CONTROL_H

#include "move_info.h"
#include "player_message.h"

class Creature;
class Collective;
class Tribe;
class SquareType;
class CollectiveAttack;
struct TriggerInfo;

class CollectiveControl {
  public:
  CollectiveControl(Collective*);
  virtual void update(bool currentlyActive);
  virtual void tick();
  virtual void onMemberKilled(const Creature* victim, const Creature* killer);
  virtual void onOtherKilled(const Creature* victim, const Creature* killer);
  virtual void addMessage(const PlayerMessage&) {}
  virtual void addAttack(const CollectiveAttack&) {}
  virtual void onConstructed(Position, const SquareType&) {}
  virtual void onConstructed(Position, FurnitureType) {};
  virtual void onNoEnemies() {}
  virtual void onRansomPaid() {}
  virtual vector<TriggerInfo> getTriggers(const Collective* against) const;

  SERIALIZATION_DECL(CollectiveControl);

  virtual ~CollectiveControl();

  static PCollectiveControl idle(Collective*);

  template <class Archive>
  static void registerTypes(Archive& ar, int version);

  const vector<Creature*>& getCreatures() const;

  protected:
  Collective* getCollective() const;

  private:
  Collective* SERIAL(collective) = nullptr;
};


#endif
