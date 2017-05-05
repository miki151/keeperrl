#pragma once

#include "move_info.h"
#include "player_message.h"

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
  virtual void onMemberKilled(WConstCreature victim, WConstCreature killer);
  virtual void onOtherKilled(WConstCreature victim, WConstCreature killer);
  virtual void onMemberAdded(WConstCreature) {}
  virtual void addMessage(const PlayerMessage&) {}
  virtual void addAttack(const CollectiveAttack&) {}
  virtual void onConstructed(Position, FurnitureType) {}
  virtual void onClaimedSquare(Position) {}
  virtual void onDestructed(Position, const DestroyAction&) {}
  virtual void onNoEnemies() {}
  virtual void onRansomPaid() {}
  virtual vector<TriggerInfo> getTriggers(WConstCollective against) const;

  WCollective getCollective() const;

  SERIALIZATION_DECL(CollectiveControl)

  virtual ~CollectiveControl();

  static PCollectiveControl idle(WCollective);

  const vector<WCreature>& getCreatures() const;

  private:
  WCollective SERIAL(collective) = nullptr;
};


