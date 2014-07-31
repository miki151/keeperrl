#ifndef _COLLECTIVE_H
#define _COLLECTIVE_H

#include "monster_ai.h"
#include "task.h"
#include "event.h"

class Creature;
class CollectiveControl;
class Tribe;
class Deity;
class Level;

RICH_ENUM(MinionTrait,
  LEADER,
  FIGHTER,
  WORKER,
);

class Collective : public EventListener {
  public:
  Collective();
  void addCreature(Creature*, vector<MinionTrait>);
  MoveInfo getMove(Creature*);
  void setControl(PCollectiveControl);
  void setLevel(Level*);
  void tick(double time);
  const Tribe* getTribe() const;
  Tribe* getTribe();
  double getStanding(const Deity*) const;
  double getWarLevel() const;
  Level* getLevel();
  const Level* getLevel() const;
  double getTime() const;

  virtual void onKillEvent(const Creature* victim, const Creature* killer) override;
  virtual void onWorshipEvent(Creature* who, const Deity*, WorshipType) override;
  virtual void onAttackEvent(Creature* victim, Creature* attacker) override;
  virtual void onSquareReplacedEvent(const Level*, Vec2 pos) override;
  virtual void onAlarmEvent(const Level*, Vec2 pos) override;

  void onConstructed(Vec2, SquareType);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  vector<Creature*>& getCreatures();
  const vector<Creature*>& getCreatures() const;

  vector<Creature*>& getCreatures(MinionTrait);
  const vector<Creature*>& getCreatures(MinionTrait) const;
  bool hasTrait(const Creature*, MinionTrait);
 // void setTrait(Creature* c, MinionTrait);

  double getTechCostMultiplier() const;
  double getCraftingMultiplier() const;
  double getWarMultiplier() const;
  double getBeastMultiplier() const;
  double getUndeadMultiplier() const;

  double getEfficiency(const Creature*) const;
  const Creature* getLeader() const;
  Creature* getLeader();

  const set<Vec2>& getSquares(SquareType) const;
  vector<SquareType> getSquareTypes() const;
  vector<Vec2> getAllSquares(const vector<SquareType>&, bool centerOnly = false) const;
  const set<Vec2>& getAllSquares() const;
  void changeSquareType(Vec2 pos, SquareType from, SquareType to);
  bool containsSquare(Vec2 pos) const;
  bool underAttack() const;

  const vector<Vec2>& getEnemyPositions() const;

  void updateEfficiency(Vec2, SquareType);
  double getEfficiency(Vec2) const;
  bool hasEfficiency(Vec2) const;

  bool isGuardPost(Vec2) const;
  void addGuardPost(Vec2);
  void removeGuardPost(Vec2);
  void freeFromGuardPost(const Creature*);

  ~Collective();

  private:
  double getStanding(EpithetId id) const;
  void onEpithetWorship(Creature*, WorshipType, EpithetId);
  void considerHealingLeader();
  vector<Creature*> SERIAL(creatures);
  EnumMap<MinionTrait, vector<Creature*>> SERIAL(byTrait);
  PCollectiveControl SERIAL(control);
  Task::Mapping SERIAL(taskMap);
  Tribe* SERIAL2(tribe, nullptr);
  map<const Deity*, double> SERIAL(deityStanding);
  Level* SERIAL2(level, nullptr);
  unordered_map<SquareType, set<Vec2>> SERIAL(mySquares);
  map<Vec2, int> SERIAL(squareEfficiency);
  set<Vec2> SERIAL(allSquares);
  struct AlarmInfo {
    double finishTime = -1000;
    Vec2 position;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version);
  } SERIAL(alarmInfo);
  MoveInfo getAlarmMove(Creature* c);
  struct GuardPostInfo {
    const Creature* attender;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version);
  };
  map<Vec2, GuardPostInfo> SERIAL(guardPosts);
  MoveInfo getGuardPostMove(Creature* c);
  vector<Vec2> SERIAL(enemyPos);
  void updateEnemyPos();
};

#endif
