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

#ifndef _MONSTER_H
#define _MONSTER_H

#include "creature.h"
#include "shortest_path.h"
#include "enums.h"
#include "monster_ai.h"

class Monster : public Controller {
  public:
  Monster(Creature* creature, MonsterAIFactory);
  
  virtual void you(MsgType type, const string& param) const override;
  virtual void you(const string& param) const override;
  
  virtual void makeMove() override;
  virtual bool isPlayer() const;
  virtual const MapMemory& getMemory() const;

  virtual void onBump(Creature*);

  static ControllerFactory getFactory(MonsterAIFactory);

  SERIALIZATION_DECL(Monster);

  private:
  PMonsterAI SERIAL(actor);
  vector<const Creature*> SERIAL(enemies);
};

#endif
