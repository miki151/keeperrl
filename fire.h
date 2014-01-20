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

  private:
  double burnt = 0;
  double size = 0;
  double weight;
  double flamability;
};

#endif
