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
#include "monster_ai.h"
#include "task.h"
#include "game_info.h"

class AttackTrigger;
typedef unique_ptr<AttackTrigger> PAttackTrigger;

class Tribe;

class VillageControl : public EventListener, public Task::Callback {
  public:
  virtual ~VillageControl();
  void initialize(vector<Creature*>);

  virtual MoveInfo getMove(Creature* c) = 0;

  void tick(double time);

  bool isConquered() const;
  bool isAnonymous() const;
  vector<Creature*> getAliveCreatures() const;
  bool currentlyAttacking() const;

  virtual void onKillEvent(const Creature* victim, const Creature* killer) override;

  GameInfo::VillageInfo::Village getVillageInfo() const;

  void setPowerTrigger(double killedCoeff, double powerCoeff);
  void setFinalTrigger(vector<VillageControl*> otherControls);
  void setOnFirstContact();

  friend class AttackTrigger;
  friend class PowerTrigger;
  friend class FinalTrigger;
  friend class FirstContact;

  static PVillageControl topLevelVillage(Collective* villain, const Location* location);
  static PVillageControl dwarfVillage(Collective* villain, Level*, StairDirection dir, StairKey key);
  static PVillageControl topLevelAnonymous(Collective* villain, const Location* location);

  void setTrigger(unique_ptr<AttackTrigger>);

  VillageControl();
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  template <class Archive>
  static void registerTypes(Archive& ar);

  protected:
  VillageControl(Collective* villain, const Location*);
  vector<Creature*> SERIAL(allCreatures);
  Collective* SERIAL2(villain, nullptr);
  const Level* SERIAL2(level, nullptr);
  string SERIAL(name);
  Tribe* SERIAL2(tribe, nullptr);
  unique_ptr<AttackTrigger> SERIAL(attackTrigger);
  bool SERIAL2(atWar, false);
  Task::Mapping SERIAL(taskMap);
  vector<Vec2> SERIAL(beds);
};

#endif
