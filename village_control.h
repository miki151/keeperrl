/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef _VILLAGE_CONTROL_H
#define _VILLAGE_CONTROL_H

#include "event.h"
#include "collective_control.h"
#include "enum_variant.h"
#include "entity_set.h"
#include "creature_factory.h"

class Task;

enum class VillageBehaviourId {
  KILL_LEADER,
  KILL_MEMBERS,
  STEAL_GOLD,
  CAMP_AND_SPAWN,
};

typedef EnumVariant<VillageBehaviourId, TYPES(int, CreatureFactory),
        ASSIGN(int, VillageBehaviourId::KILL_MEMBERS),
        ASSIGN(CreatureFactory, VillageBehaviourId::CAMP_AND_SPAWN)> VillageBehaviour;

RICH_ENUM(AttackTriggerId,
  POWER,
  SELF_VICTIMS,
  ENEMY_POPULATION,
  GOLD,
  STOLEN_ITEMS,
  ROOM_BUILT,
  TIMER
);

typedef EnumVariant<AttackTriggerId, TYPES(int, SquareType),
        ASSIGN(int, AttackTriggerId::ENEMY_POPULATION, AttackTriggerId::GOLD, AttackTriggerId::TIMER),
        ASSIGN(SquareType, AttackTriggerId::ROOM_BUILT)> AttackTrigger;

enum class AttackPrerequisite { POK }; // OBSOLETE

class VillageControl : public CollectiveControl {
  public:
  typedef VillageBehaviour Behaviour;
  typedef AttackTrigger Trigger;

  enum WelcomeMessage {
    DRAGON_WELCOME,
  };

  enum ItemTheftMessage {
    DRAGON_THEFT,
  };

  struct Villain {
    int SERIAL(minPopulation);
    int SERIAL(minTeamSize);
    Collective* SERIAL(collective);
    vector<Trigger> SERIAL(triggers);
    vector<AttackPrerequisite> SERIAL(prerequisites);
    Behaviour SERIAL(behaviour);
    optional<WelcomeMessage> SERIAL(welcomeMessage);
    optional<pair<double, int>> SERIAL(ransom);

    PTask getAttackTask(VillageControl* self);
    double getAttackProbability(const VillageControl* self) const;
    double getTriggerValue(const Trigger&, const VillageControl* self, const Collective* villain) const;
    bool contains(const Creature*);

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version);
  };

  friend struct Villain;

  VillageControl(Collective*, vector<Villain>);

  protected:
  virtual void tick(double time) override;
  virtual void onMemberKilled(const Creature* victim, const Creature* killer) override;
  virtual void onOtherKilled(const Creature* victim, const Creature* killer) override;
  virtual void onRansomPaid() override;

  SERIALIZATION_DECL(VillageControl);

  private:
  void launchAttack(Villain&, vector<Creature*> attackers);
  optional<Villain&> getVillain(const Creature*);
  void considerWelcomeMessage();
  void considerCancellingAttack();

  REGISTER_HANDLER(PickupEvent, const Creature*, const vector<Item*>&);

  vector<Villain> SERIAL(villains);

  map<const Collective*, double> SERIAL(victims);
  EntitySet<Item> SERIAL(myItems);
  map<const Collective*, int> SERIAL(stolenItemCount);
  map<TeamId, int> SERIAL(attackSizes);
};

#endif
