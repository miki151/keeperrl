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

  enum class WelcomeMessage;

  int SERIAL(minPopulation);
  int SERIAL(minTeamSize);
  vector<AttackTrigger> SERIAL(triggers);
  HeapAllocated<AttackBehaviour> SERIAL(attackBehaviour);
  optional<WelcomeMessage> SERIAL(welcomeMessage);
  optional<pair<double, int>> SERIAL(ransom);

  PTask getAttackTask(VillageControl* self) const;
  double getAttackProbability(const VillageControl* self) const;
  double getTriggerValue(const AttackTrigger&, const VillageControl* self) const;

  ~VillageBehaviour();

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

RICH_ENUM(VillageBehaviour::WelcomeMessage, DRAGON_WELCOME);
