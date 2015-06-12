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
#include "task.h"
#include "game_info.h"
#include "collective_control.h"

enum class VillageBehaviourId {
  KILL_LEADER,
  KILL_MEMBERS,
  STEAL_GOLD,
};
typedef EnumVariant<VillageBehaviourId, TYPES(int),
        ASSIGN(int, VillageBehaviourId::KILL_MEMBERS)> VillageBehaviour;

enum class AttackTriggerId {
  POWER,
  SELF_VICTIMS,
  ENEMY_POPULATION,
  GOLD,
  STOLEN_ITEMS,
};

typedef EnumVariant<AttackTriggerId, TYPES(int),
        ASSIGN(int, AttackTriggerId::ENEMY_POPULATION, AttackTriggerId::GOLD)> AttackTrigger;

class VillageControl : public CollectiveControl {
  public:
  typedef VillageBehaviour Behaviour;
  typedef AttackTrigger Trigger;

  enum AttackMessage {
    CREATURE_TITLE,
    TRIBE_AND_NAME,
  };

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
    Behaviour SERIAL(behaviour);
    AttackMessage SERIAL(attackMessage);
    optional<WelcomeMessage> SERIAL(welcomeMessage);
    bool SERIAL(leaderAttacks) = false;

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
  virtual MoveInfo getMove(Creature*);
  virtual void onMemberKilled(const Creature* victim, const Creature* killer) override;
  virtual void onOtherKilled(const Creature* victim, const Creature* killer) override;

  SERIALIZATION_DECL(VillageControl);

  private:
  const string& getAttackMessage(const Villain&) const;
  string getAttackMessage(const Villain&, const vector<Creature*> attackers) const;
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
