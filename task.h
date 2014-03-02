#ifndef _TASK_H
#define _TASK_H

#include "monster_ai.h"
#include "unique_entity.h"

class Task : public UniqueEntity {
  public:
  Task(Collective*, Vec2 position);
  virtual ~Task();

  virtual MoveInfo getMove(Creature*) = 0;
  virtual string getInfo() = 0;
  virtual bool isImpossible(const Level*) { return false; }
  virtual bool canTransfer() { return true; }
  virtual void cancel() {}
  bool isDone();

  Vec2 getPosition();

  static PTask construction(Collective*, Vec2 target, SquareType);
  static PTask bringItem(Collective*, Vec2 position, vector<Item*>, Vec2 target);
  static PTask applyItem(Collective* col, Vec2 position, Item* item, Vec2 target);
  static PTask applySquare(Collective*, set<Vec2> squares);
  static PTask eat(Collective*, set<Vec2> hatcherySquares);
  static PTask equipItem(Collective* col, Vec2 position, Item* item);
  static PTask pickItem(Collective* col, Vec2 position, vector<Item*> items);

  SERIALIZATION_DECL(Task);

  template <class Archive>
  static void registerTypes(Archive& ar);

  protected:
  void setDone();
  void setPosition(Vec2);
  MoveInfo getMoveToPosition(Creature*);
  Collective* getCollective();

  private:
  Vec2 position;
  bool done = false;
  Collective* collective;
};

#endif
