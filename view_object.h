#ifndef _VIEW_OBJECT_H
#define _VIEW_OBJECT_H

#include "debug.h"
#include "enums.h"
#include "util.h"

class ViewObject {
  public:
  ViewObject(ViewId id, ViewLayer l, const string& description);

  void setBleeding(double);
  double getBleeding() const;

  enum EnemyStatus { HOSTILE, FRIENDLY, UNKNOWN };
  void setEnemyStatus(EnemyStatus);
  bool isHostile() const;
  bool isFriendly() const;

  enum Modifier { BLIND, PLAYER, HIDDEN, INVISIBLE, ILLUSION, POISONED, CASTS_SHADOW, PLANNED, LOCKED,
    ROUND_SHADOW, MOVE_UP};
  ViewObject& setModifier(Modifier);
  ViewObject& removeModifier(Modifier);
  bool hasModifier(Modifier) const;

  static void setHallu(bool);

  void setBurning(double);
  double getBurning() const;

  void setHeight(double);
  double getHeight() const;

  void setAttack(int);
  void setDefense(int);

  Optional<int> getAttack() const;
  Optional<int> getDefense() const;

  ViewObject& setWaterDepth(double);
  double getWaterDepth() const;

  string getDescription(bool stats = false) const;
  string getBareDescription() const;

  ViewLayer layer() const;
  ViewId id() const;

  const static ViewObject& unknownMonster();
  const static ViewObject& empty();
  const static ViewObject& mana();

  SERIALIZATION_DECL(ViewObject);

  private:
  double SERIAL2(bleeding, 0);
  EnemyStatus SERIAL2(enemyStatus, UNKNOWN);
  set<Modifier> SERIAL(modifiers);
  ViewId SERIAL(resource_id);
  ViewLayer SERIAL(viewLayer);
  string SERIAL(description);
  double SERIAL2(burning, false);
  double SERIAL2(height, 0);
  int SERIAL2(attack, -1);
  int SERIAL2(defense, -1);
  double SERIAL2(waterDepth, -1);
};


#endif
