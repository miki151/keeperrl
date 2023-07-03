#pragma once

#include "player_message.h"
#include "furniture_type.h"
#include "view_id.h"

class Creature;
class Collective;
class Tribe;
class CollectiveAttack;
struct TriggerInfo;

class CollectiveControl : public OwnedObject<CollectiveControl> {
  public:
  CollectiveControl(Collective*);
  virtual void update(bool currentlyActive);
  virtual void tick();
  virtual void onMemberKilled(const Creature* victim, const Creature* killer);
  virtual void onOtherKilled(const Creature* victim, const Creature* killer);
  virtual void onMemberAdded(Creature*) {}
  virtual void onConquered(Creature* victim, Creature* killer) {}
  virtual void addMessage(const PlayerMessage&) {}
  virtual void addWindowMessage(ViewIdList, const string&) {}
  virtual void addAttack(const CollectiveAttack&) {}
  virtual void onConstructed(Position, FurnitureType) {}
  virtual void onClaimedSquare(Position) {}
  virtual void onDestructed(Position, FurnitureType, const DestroyAction&) {}
  virtual void onRansomPaid() {}
  virtual bool canPillage(const Collective* by) const { return false; }
  virtual void launchAllianceAttack(vector<Collective*> allies) { fail(); }
  virtual bool considerVillainAmbush(const vector<Creature*>& travellers) { return false; }
  virtual vector<TriggerInfo> getAllTriggers(const Collective* against) const;
  vector<TriggerInfo> getTriggers(const Collective* against) const;
  virtual bool canPerformAttack() const { fail(); }

  SERIALIZATION_DECL(CollectiveControl)

  virtual ~CollectiveControl();

  static PCollectiveControl idle(Collective*);

  const vector<Creature*>& getCreatures() const;

  Collective* SERIAL(collective) = nullptr;
};


