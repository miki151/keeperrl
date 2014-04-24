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
  static PSquare get(SquareType);
  static PSquare getStairs(StairDirection, StairKey, StairLook = StairLook::NORMAL);
  static PSquare getAltar(Deity*);
  static PSquare getWater(double depth);

  template <class Archive>
  static void registerTypes(Archive& ar);

  private:
  static Square* getPtr(SquareType s);
};

#endif
