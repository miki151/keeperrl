#pragma once

#include "sdl.h"
#include "keybinding.h"

class KeybindingMap {
  public:
  KeybindingMap(const string& fromPath);
  bool matches(Keybinding, SDL::SDL_Keysym);

  private:
};
