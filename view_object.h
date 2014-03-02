#ifndef _VIEW_OBJECT_H
#define _VIEW_OBJECT_H

#include "debug.h"
#include "enums.h"
#include "util.h"

class ViewObject {
  public:
  ViewObject(ViewId id, ViewLayer l, const string& description, bool castsShadow = false);

  void setBleeding(double);
  double getBleeding() const;

  enum EnemyStatus { HOSTILE, FRIENDLY, UNKNOWN };
  void setEnemyStatus(EnemyStatus);
  bool isHostile() const;
  bool isFriendly() const;

  void setBlind(bool);

  void setPlayer(bool);
  bool isPlayer() const;

  void setHidden(bool);
  bool isHidden() const;

  void setInvisible(bool);
  bool isInvisible() const;

  void setIllusion(bool);
  bool isIllusion() const;

  void setPoisoned(bool);
  bool isPoisoned() const;

  static void setHallu(bool);

  void setBurning(double);
  double getBurning() const;

  void setHeight(double);
  double getHeight() const;

  void setSizeIncrease(double);
  double getSizeIncrease() const;

  void setAttack(int);
  void setDefense(int);

  Optional<int> getAttack() const;
  Optional<int> getDefense() const;

  bool castsShadow() const;

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
  double bleeding = 0;
  EnemyStatus enemyStatus = UNKNOWN;
  bool blind = false;
  bool invisible = false;
  bool illusion = false;
  bool poisoned = false;
  bool player = false;
  ViewId resource_id;
  ViewLayer viewLayer;
  string description;
  bool hidden = false;
  double burning = false;
  double height = 0;
  double sizeIncrease = 0;
  bool shadow;
  int attack = -1;
  int defense = -1;
  double waterDepth = -1;
};


#endif
