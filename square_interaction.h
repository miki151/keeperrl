#ifndef _SQUARE_INTERACTION
#define _SQUARE_INTERACTION

#include "util.h"

enum class SquareInteraction {
  KEEPER_BOARD,
  STAIRS
};

class Position;
class Creature;

class SquareInteractions {
  public:
  static SquareApplyType getApplyType(SquareInteraction);
//  static SquareApplyType getApplyType(SquareInteraction, const Creature*);
  static void apply(SquareInteraction, Position, Creature*);
  static double getApplyTime(SquareInteraction);
};

#endif
