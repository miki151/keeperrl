#pragma once

#include "sdl.h"

class RandomGen;

class SdlEventGenerator {
  public:
  static SDL::SDL_Event getRandom(RandomGen&, Vec2 screenSize);
};
