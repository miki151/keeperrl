#include "stdafx.h"
#include "collective_control.h"
#include "collective.h"
#include "attack_trigger.h"

SERIALIZE_DEF(CollectiveControl, SUBCLASS(OwnedObject<CollectiveControl>), collective)
SERIALIZATION_CONSTRUCTOR_IMPL(CollectiveControl);

CollectiveControl::CollectiveControl(Collective* c) : collective(c) {
}

void CollectiveControl::update(bool currentlyActive) {
}

void CollectiveControl::tick() {
}

const vector<Creature*>& CollectiveControl::getCreatures() const {
  return collective->getCreatures();
}

void CollectiveControl::onMemberKilledOrStunned(Creature* victim, const Creature* killer) {
}

void CollectiveControl::onOtherKilled(const Creature* victim, const Creature* killer) {
}

CollectiveControl::~CollectiveControl() {
}

class IdleControl : public CollectiveControl {
  public:

  using CollectiveControl::CollectiveControl;

  virtual void tick() override {
  }

  SERIALIZATION_CONSTRUCTOR(IdleControl);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(CollectiveControl);
  }
};

vector<TriggerInfo> CollectiveControl::getAllTriggers(const Collective* against) const {
  return {};
}

vector<TriggerInfo> CollectiveControl::getTriggers(const Collective* against) const {
  return getAllTriggers(against).filter([](auto t) { return t.value > 0; });
}

PCollectiveControl CollectiveControl::idle(Collective* col) {
  return makeOwner<IdleControl>(col);
}

REGISTER_TYPE(IdleControl);

