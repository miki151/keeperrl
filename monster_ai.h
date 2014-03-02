#ifndef _ACTOR_H
#define _ACTOR_H

#include "creature.h"
#include "location.h"

struct MoveInfo {
  double value;
  function<void ()> move;

  bool isValid() const {
    return move != nullptr;
  }
};

const MoveInfo NoMove = {0.0, nullptr};

enum MonsterAIType { 
  MONSTER,
  WILDLIFE_NON_PREDATOR,
  BIRD_FLY_AWAY,
  FOLLOWER,
};

class Behaviour {
  public:
  Behaviour(Creature*);
  virtual MoveInfo getMove() { return {0, nullptr}; }
  virtual void onAttacked(const Creature* attacker) {}
  virtual double itemValue(const Item*) { return 0; }
  Item* getBestWeapon();
  const Creature* getClosestEnemy();
  MoveInfo tryToApplyItem(EffectType, double maxTurns);

  virtual ~Behaviour() {}

  SERIALIZATION_DECL(Behaviour);

  protected:
  Creature* creature;
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
  vector<PBehaviour> behaviours;
  vector<int> weights;
  Creature* creature;
  bool pickItems;
};

class Collective;
class VillageControl;

class MonsterAIFactory {
  public:
  PMonsterAI getMonsterAI(Creature* c);

  static MonsterAIFactory collective(Collective*);
  static MonsterAIFactory villageControl(VillageControl*, Location*);
  static MonsterAIFactory monster();
  static MonsterAIFactory stayInLocation(Location*, bool moveRandomly = true);
  static MonsterAIFactory guardSquare(Vec2 pos);
  static MonsterAIFactory wildlifeNonPredator();
  static MonsterAIFactory scavengerBird(Vec2 corpsePos);
  static MonsterAIFactory follower(Creature*, int radius);
  static MonsterAIFactory moveRandomly();
  static MonsterAIFactory idle();

  private:
  typedef function<MonsterAI*(Creature*)> MakerFun;
  MonsterAIFactory(MakerFun);
  MakerFun maker;
};

#endif
