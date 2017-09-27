#pragma once

#include "util.h"
#include "enum_variant.h"
#include "creature_factory.h"
#include "attack_trigger.h"

class AttackBehaviour;

class VillageBehaviour {
  public:
  VillageBehaviour();
  VillageBehaviour(const VillageBehaviour&);
  VillageBehaviour& operator = (const VillageBehaviour&);

  typedef AttackTrigger Trigger;
  enum WelcomeMessage {
    DRAGON_WELCOME,
  };

  enum ItemTheftMessage {
    DRAGON_THEFT,
  };

  int SERIAL(minPopulation);
  int SERIAL(minTeamSize);
  vector<Trigger> SERIAL(triggers);
  HeapAllocated<AttackBehaviour> SERIAL(attackBehaviour);
  optional<WelcomeMessage> SERIAL(welcomeMessage);
  optional<pair<double, int>> SERIAL(ransom);

  PTask getAttackTask(VillageControl* self);
  double getAttackProbability(const VillageControl* self) const;
  double getTriggerValue(const Trigger&, const VillageControl* self) const;
  bool contains(WConstCreature);

  ~VillageBehaviour();

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

