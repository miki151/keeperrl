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

  template <class Archive>
  static void registerTypes(Archive& ar, int version);

  ~MonsterAI();

  private:
  friend class MonsterAIFactory;
  MonsterAI(WCreature, const vector<Behaviour*>& behaviours, const vector<int>& weights, bool pickItems = true);
  vector<PBehaviour> SERIAL(behaviours);
  vector<int> SERIAL(weights);
  WCreature SERIAL(creature);
  bool SERIAL(pickItems);
};

class Collective;

class MonsterAIFactory {
  public:
  PMonsterAI getMonsterAI(WCreature c) const;

  static MonsterAIFactory collective(WCollective);
  static MonsterAIFactory monster();
  static MonsterAIFactory singleTask(PTask&&);
  static MonsterAIFactory stayInLocation(Rectangle, bool moveRandomly = true);
  static MonsterAIFactory guardSquare(Position);
  static MonsterAIFactory wildlifeNonPredator();
  static MonsterAIFactory scavengerBird(Position corpsePos);
  static MonsterAIFactory summoned(WCreature, int ttl);
  static MonsterAIFactory dieTime(double time);
  static MonsterAIFactory moveRandomly();
  static MonsterAIFactory stayOnFurniture(FurnitureType);
  static MonsterAIFactory idle();
  static MonsterAIFactory splashHeroes(bool leader);
  static MonsterAIFactory splashMonsters();
  static MonsterAIFactory splashImps(const FilePath& splashPath);

  private:
  typedef function<MonsterAI*(WCreature)> MakerFun;
  MonsterAIFactory(MakerFun);
  MakerFun maker;
};
