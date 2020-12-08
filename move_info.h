#pragma once

#include "creature_action.h"

struct MoveInfo {
  MoveInfo(double val, CreatureAction);
  MoveInfo(CreatureAction);
  explicit operator bool() const;

  void setValue(double v);
  MoveInfo withValue(double v) const;
  MoveInfo orWait() const;

  CreatureAction getMove() const;
  double getValue() const;

  private:
  double value;
  CreatureAction move;
};


const MoveInfo NoMove = {0.0, CreatureAction()};

