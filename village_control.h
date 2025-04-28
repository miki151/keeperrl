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

#pragma once

#include "collective_control.h"
#include "enum_variant.h"
#include "entity_set.h"
#include "event_listener.h"

class Task;
class VillageBehaviour;

class VillageControl : public CollectiveControl, public EventListener<VillageControl> {
  public:
  static PVillageControl create(Collective* col, optional<VillageBehaviour> v);
  static PVillageControl copyOf(Collective* col, const VillageControl*);

  void onEvent(const GameEvent&);

  void updateAggression(EnemyAggressionLevel);

  private:
  struct Private {};

  public:
  VillageControl(Private, Collective*, optional<VillageBehaviour>);

  protected:
  virtual void update(bool currentlyActive) override;
  virtual void onMemberKilledOrStunned(Creature* victim, const Creature* killer) override;
  virtual void onOtherKilled(const Creature* victim, const Creature* killer) override;
  virtual void onRansomPaid() override;
  virtual vector<TriggerInfo> getAllTriggers(const Collective* against) const override;
  virtual void launchAllianceAttack(vector<Collective*> allies) override;
  virtual bool canPerformAttack() const override;
  virtual bool canPillage(const Collective* by) const override;
  virtual bool considerVillainAmbush(const vector<Creature*>& travellers) override;

  SERIALIZATION_DECL(VillageControl)

  private:
  friend class VillageBehaviour;
  void launchAttack(vector<Creature*> attackers, bool duel);
  void considerCancellingAttack();
  bool isEnemy(const Creature*);
  Collective* getEnemyCollective() const;
  void acceptImmigration();
  void considerAcceptingPrisoners();

  heap_optional<VillageBehaviour> SERIAL(behaviour);

  double SERIAL(victims) = 0;
  EntitySet<Item> SERIAL(myItems);
  mutable optional<bool> canPillageCache;
  int SERIAL(stolenItemCount) = 0;
  map<TeamId, int> SERIAL(attackSizes);
  bool SERIAL(entries) = false;
  double SERIAL(maxEnemyPower) = 0;
  void healAllCreatures();
  bool isEnemy() const;
  vector<Creature*> getAttackers() const;
  unordered_set<TeamId> SERIAL(cancelledAttacks);
};

