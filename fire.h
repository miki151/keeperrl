#ifndef _FIRE_H
#define _FIRE_H

#include "util.h"

class Level;
class Fire {
  public:
  Fire(double objectWeight, double objectFlamability);
  void tick(Level* level, Vec2 position);
  void set(double amount);
  bool isBurning() const;
  double getSize() const;
  bool isBurntOut() const;
  double getFlamability() const;

  SERIALIZATION_DECL(Fire);

  private:
  double SERIAL2(burnt, 0);
  double SERIAL2(size, 0);
  double SERIAL(weight);
  double SERIAL(flamability);
};

#endif
