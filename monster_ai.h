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

#include "creature_action.h"
#include "position.h"
#include "file_path.h"
#include "game_time.h"

class Creature;

enum MonsterAIType { 
  MONSTER,
  WILDLIFE_NON_PREDATOR,
  BIRD_FLY_AWAY,
  FOLLOWER,
};

class Behavior;
class MonsterAI {
  public:
  void makeMove();

  SERIALIZATION_DECL(MonsterAI);

  ~MonsterAI();

  private:
  friend class MonsterAIFactory;
  MonsterAI(Creature*, const vector<Behaviour*>& behaviours, const vector<int>& weights, bool pickItems = true);
  vector<PBehaviour> SERIAL(behaviours);
  vector<int> SERIAL(weights);
  Creature* SERIAL(creature) = nullptr;
  bool SERIAL(pickItems);
};

class Collective;

class MonsterAIFactory {
  public:
  PMonsterAI getMonsterAI(Creature* c) const;

  static MonsterAIFactory collective(Collective*);
  static MonsterAIFactory monster();
  static MonsterAIFactory singleTask(PTask&&, bool chaseEnemies = true);
  static MonsterAIFactory stayInLocation(vector<Vec2>, bool moveRandomly = true);
  static MonsterAIFactory wildlifeNonPredator();
  static MonsterAIFactory scavengerBird();
  static MonsterAIFactory summoned(Creature*);
  static MonsterAIFactory moveRandomly();
  static MonsterAIFactory stayOnFurniture(FurnitureType);
  static MonsterAIFactory guard();
  static MonsterAIFactory idle();
  static MonsterAIFactory splashHeroes(bool leader);
  static MonsterAIFactory splashMonsters();
  static MonsterAIFactory splashImps(const FilePath& splashPath);
  static MonsterAIFactory warlord(shared_ptr<vector<Creature*>> team, shared_ptr<EnumSet<TeamOrder>> orders);
  private:
  typedef function<MonsterAI*(Creature*)> MakerFun;
  MonsterAIFactory(MakerFun);
  MakerFun maker;
};
