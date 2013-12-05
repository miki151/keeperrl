#ifndef _POISON_GAS_H
#define _POISON_GAS_H

#include "util.h"

class PoisonGas {
  public:
  void addAmount(double amount);
  void tick(Level*, Vec2 pos);
  double getAmount() const;

  private:
  double amount = 0;
};

#endif

