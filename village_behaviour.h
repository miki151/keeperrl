#pragma once

#include "util.h"
#include "enum_variant.h"
#include "creature_factory.h"
#include "attack_behaviour.h"

class AttackTrigger;

class VillageBehaviour {
  public:
  VillageBehaviour();

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
  bool contains(const Creature*);

  ~VillageBehaviour();

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

