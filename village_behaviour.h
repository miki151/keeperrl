#pragma once

#include "util.h"
#include "enum_variant.h"
#include "creature_factory.h"

class AttackBehaviour;
class AttackTrigger;

class VillageBehaviour {
  public:
  VillageBehaviour();
  VillageBehaviour(const VillageBehaviour&);
  VillageBehaviour& operator = (const VillageBehaviour&);
  VillageBehaviour(VillageBehaviour&&) noexcept;
  VillageBehaviour& operator = (VillageBehaviour&&) noexcept;

  int SERIAL(minPopulation);
  int SERIAL(minTeamSize);
  vector<AttackTrigger> SERIAL(triggers);
  HeapAllocated<AttackBehaviour> SERIAL(attackBehaviour);
  optional<pair<double, int>> SERIAL(ransom);
  double SERIAL(ambushChance) = 0.0;

  PTask getAttackTask(VillageControl* self) const;
  bool isAttackBehaviourNonChasing() const;
  double getAttackProbability(const VillageControl* self) const;
  double getTriggerValue(const AttackTrigger&, const VillageControl* self) const;

  ~VillageBehaviour();

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

PTask getKillLeaderTask(Collective*);