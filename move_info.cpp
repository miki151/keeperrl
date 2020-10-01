#include "stdafx.h"
#include "move_info.h"
#include "creature.h"

MoveInfo::MoveInfo(double val, const CreatureAction& m) : value(m ? val : 0), move(m) {

}

MoveInfo::MoveInfo(const CreatureAction& m) : MoveInfo(1.0, m) {
}

MoveInfo::operator bool() const {
  return !!move;
}

MoveInfo MoveInfo::withValue(double v) const {
  MoveInfo ret(*this);
  ret.value = v;
  return ret;
}

MoveInfo MoveInfo::orWait() const {
  if (!move)
    return MoveInfo(1.0, Creature::wait());
  else
    return *this;
}

void MoveInfo::setValue(double v) {
  value = v;
}

CreatureAction MoveInfo::getMove() const {
  return move;
}

double MoveInfo::getValue() const {
  return value;
}

