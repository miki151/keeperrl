#ifndef _VIEW_OBJECT_H
#define _VIEW_OBJECT_H

#include "debug.h"
#include "enums.h"

class ViewObject {
  public:
  ViewObject(ViewId id, ViewLayer l, const string& description, bool castsShadow = false);

  ViewObject() { Debug(FATAL) << "Don't call this constructor"; }

  void setBleeding(double);
  double getBleeding() const;

  void setHostile(bool);
  void setBlind(bool);
  void setInvisible(bool);

  void setPlayer(bool);
  bool isPlayer() const;

  void setHidden(bool);
  bool isHidden() const;
  bool isInvisible() const;

  static void setHallu(bool);

  void setBurning(double);
  double getBurning() const;

  void setHeight(double);
  double getHeight() const;

  void setSizeIncrease(double);
  double getSizeIncrease() const;

  bool castsShadow() const;

  string getDescription() const;
  string getBareDescription() const;

  ViewLayer layer() const;
  ViewId id() const;

  const static ViewObject& unknownMonster();
  const static ViewObject& empty();

  private:
  double bleeding = 0;
  bool hostile = false;
  bool blind = false;
  bool invisible = false;
  bool player = false;
  ViewId resource_id;
  ViewLayer viewLayer;
  string description;
  bool hidden = false;
  double burning = false;
  double height = 0;
  double sizeIncrease = 0;
  bool shadow;
};


#endif
