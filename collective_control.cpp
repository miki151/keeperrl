#include "stdafx.h"
#include "collective_control.h"
#include "collective.h"
#include "attack_trigger.h"

SERIALIZE_DEF(CollectiveControl, SUBCLASS(OwnedObject<CollectiveControl>), collective)
SERIALIZATION_CONSTRUCTOR_IMPL(CollectiveControl);

CollectiveControl::CollectiveControl(WCollective c) : collective(c) {
}

void CollectiveControl::update(bool currentlyActive) {
}

void CollectiveControl::tick() {
}

const vector<WCreature>& CollectiveControl::getCreatures() const {
  return collective->getCreatures();
}

void CollectiveControl::onMemberKilled(WConstCreature victim, WConstCreature killer) {
}

void CollectiveControl::onOtherKilled(WConstCreature victim, WConstCreature killer) {
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

vector<TriggerInfo> CollectiveControl::getTriggers(WConstCollective against) const {
  return {};
}

PCollectiveControl CollectiveControl::idle(WCollective col) {
  return makeOwner<IdleControl>(col);
}

REGISTER_TYPE(IdleControl);

