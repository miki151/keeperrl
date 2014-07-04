#include "stdafx.h"
#include "collective_control.h"
#include "collective.h"

template <class Archive>
void CollectiveControl::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(collective);
}

SERIALIZABLE(CollectiveControl);
SERIALIZATION_CONSTRUCTOR_IMPL(CollectiveControl);

CollectiveControl::CollectiveControl(Collective* c) : collective(c) {
}

Collective* CollectiveControl::getCollective() {
  return NOTNULL(collective);
}

const Collective* CollectiveControl::getCollective() const {
  return NOTNULL(collective);
}

CollectiveControl::~CollectiveControl() {
}

class IdleControl : public CollectiveControl {
  public:

  using CollectiveControl::CollectiveControl;

  virtual MoveInfo getMove(Creature*) override {
    return NoMove;
  }

  virtual void tick(double time) override {
  }

  SERIALIZATION_CONSTRUCTOR(IdleControl);
  
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
  }
};

PCollectiveControl CollectiveControl::idle(Collective* col) {
  return PCollectiveControl(new IdleControl(col));
}

template <class Archive>
void CollectiveControl::registerTypes(Archive& ar) {
  REGISTER_TYPE(ar, IdleControl);
}

REGISTER_TYPES(CollectiveControl);

