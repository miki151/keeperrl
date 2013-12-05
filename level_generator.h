#ifndef _LEVEL_GENERATOR
#define _LEVEL_GENERATOR

#include <string>
#include "util.h"
#include "level.h"
#include "square_factory.h"
#include "item_factory.h"
#include "creature_factory.h"

class StaticLevelGenerator {
  public:

  Level* createLevel(const char* filename,
      SquareFactory* squareFactory,
      ItemFactory* itemFactory,
      CreatureFactory* createFactory);
};


#endif
