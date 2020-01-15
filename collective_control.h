#pragma once

#include "player_message.h"
#include "furniture_type.h"

class Creature;
class Collective;
class Tribe;
class CollectiveAttack;
struct TriggerInfo;

class CollectiveControl : public OwnedObject<CollectiveControl> {
  public:
  CollectiveControl(WCollective);
  virtual void update(bool currentlyActive);
  virtual void tick();
  virtual void onMemberKilled(const Creature* victim, const Creature* killer);
  virtual void onOtherKilled(const Creature* victim, const Creature* killer);
  virtual void onMemberAdded(Creature*) {}
  virtual void onConquered(Creature* victim, Creature* killer) {}
  virtual void addMessage(const PlayerMessage&) {}
  virtual void addAttack(const CollectiveAttack&) {}
  virtual void onConstructed(Position, FurnitureType) {}
  virtual void onClaimedSquare(Position) {}
  virtual void onDestructed(Position, FurnitureType, const DestroyAction&) {}
  virtual void onRansomPaid() {}
  virtual vector<TriggerInfo> getTriggers(WConstCollective against) const;

  SERIALIZATION_DECL(CollectiveControl)

  virtual ~CollectiveControl();

  static PCollectiveControl idle(WCollective);

  const vector<Creature*>& getCreatures() const;

  WCollective SERIAL(collective) = nullptr;
};


