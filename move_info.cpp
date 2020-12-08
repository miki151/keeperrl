#include "stdafx.h"
#include "move_info.h"
#include "creature.h"

MoveInfo::MoveInfo(double val, CreatureAction m) : value(m ? val : 0), move(std::move(m)) {

}

MoveInfo::MoveInfo(CreatureAction m) : MoveInfo(1.0, std::move(m)) {
}

MoveInfo::operator bool() const {
  return !!move;
}

MoveInfo MoveInfo::withValue(double v) const {
  if (!move)
    return *this;
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
