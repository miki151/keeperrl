#ifndef _SQUARE_FACTORY
#define _SQUARE_FACTORY

#include "pantheon.h"

class Square;


inline StairDirection opposite(StairDirection d) {
  switch (d) {
    case StairDirection::DOWN: return StairDirection::UP;
    case StairDirection::UP: return StairDirection::DOWN;
  }
  return StairDirection::DOWN;
}

class SquareFactory {
  public:
  static Square* get(SquareType);
  static Square* getStairs(StairDirection, StairKey, StairLook = StairLook::NORMAL);
  static Square* getAltar(Deity*);
  static Square* getWater(double depth);

  private:
};

#endif
