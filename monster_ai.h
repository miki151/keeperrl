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

#ifndef _ACTOR_H
#define _ACTOR_H

#include "location.h"
#include "creature_action.h"

class Creature;

class Creature;

struct MoveInfo {
  MoveInfo(double val, CreatureAction m) : value(m ? val : 0), move(m) {}
  MoveInfo(CreatureAction m) : MoveInfo(1.0, m) {}

  double value;
  CreatureAction move;

  operator bool() const {
    return move;
  }

  MoveInfo setValue(double v) {
    MoveInfo ret(*this);
    ret.value = v;
    return ret;
  }
};

const MoveInfo NoMove = {0.0, CreatureAction()};

enum MonsterAIType { 
  MONSTER,
  WILDLIFE_NON_PREDATOR,
  BIRD_FLY_AWAY,
  FOLLOWER,
};

class Behaviour {
  public:
  Behaviour(Creature*);
  virtual MoveInfo getMove() { return NoMove; }
  virtual void onAttacked(const Creature* attacker) {}
  virtual double itemValue(const Item*) { return 0; }
  Item* getBestWeapon();
  const Creature* getClosestEnemy();
  MoveInfo tryToApplyItem(EffectType, double maxTurns);

  virtual ~Behaviour() {}

  SERIALIZATION_DECL(Behaviour);

  protected:
  Creature* SERIAL(creature);
};

class MonsterAI {
  public:
  void makeMove();

  SERIALIZATION_DECL(MonsterAI);

  template <class Archive>
  static void registerTypes(Archive& ar);

  private:
  friend class MonsterAIFactory;
  MonsterAI(Creature*, const vector<Behaviour*>& behaviours, const vector<int>& weights, bool pickItems = true);
  vector<PBehaviour> SERIAL(behaviours);
  vector<int> SERIAL(weights);
  Creature* SERIAL(creature);
  bool SERIAL(pickItems);
};

class Collective;

class MonsterAIFactory {
  public:
  PMonsterAI getMonsterAI(Creature* c);

  static MonsterAIFactory collective(Collective*);
  static MonsterAIFactory monster();
  static MonsterAIFactory stayInLocation(Location*, bool moveRandomly = true);
  static MonsterAIFactory guardSquare(Vec2 pos);
  static MonsterAIFactory wildlifeNonPredator();
  static MonsterAIFactory doorEater();
  static MonsterAIFactory scavengerBird(Vec2 corpsePos);
  static MonsterAIFactory summoned(Creature*, int ttl);
  static MonsterAIFactory moveRandomly();
  static MonsterAIFactory idle();

  private:
  typedef function<MonsterAI*(Creature*)> MakerFun;
  MonsterAIFactory(MakerFun);
  MakerFun maker;
};

#endif
