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

class Tribe;

struct VillageControlInfo {
  enum Id { PEACEFUL, POWER_BASED, POWER_BASED_DISCOVER, FINAL_ATTACK, DRAGON } id;
  double killedCoeff;
  double powerCoeff;
};

class VillageControl : public EventListener, public Task::Callback, public CollectiveControl {
  public:
  VillageControl(Collective*, Collective* villain, const Location*);
  virtual ~VillageControl();

  bool isConquered() const;
  bool isAnonymous() const;
  vector<Creature*> getAliveCreatures() const;
  virtual bool currentlyAttacking() const;
  virtual void tick(double time) override;

  virtual void onCreatureKilled(const Creature* victim, const Creature* killer) override;

  GameInfo::VillageInfo::Village getVillageInfo() const;

  void setPowerTrigger(double killedCoeff, double powerCoeff);
  void setFinalTrigger(vector<VillageControl*> otherControls);
  void setOnFirstContact();

  static PVillageControl get(VillageControlInfo, Collective*, Collective* villain, const Location* location);
  static PVillageControl getFinalAttack(Collective*, Collective* villain, const Location* location,
      vector<VillageControl*> otherControls);

  SERIALIZATION_DECL(VillageControl);

  template <class Archive>
  static void registerTypes(Archive& ar);

  const string& getName() const;
  const Collective* getVillain() const;

  Tribe* getTribe();
  const Tribe* getTribe() const;

  protected:
  VillageControl(Collective* villain, const Location*);
  Collective* SERIAL2(villain, nullptr);
  const Location* SERIAL2(location, nullptr);
  string SERIAL(name);
  bool SERIAL2(atWar, false);
};

#endif
