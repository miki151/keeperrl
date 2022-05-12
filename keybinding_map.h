#pragma once

#include "sdl.h"
#include "keybinding.h"
#include "file_path.h"

class FilePath;

class KeybindingMap {
  public:
  KeybindingMap(const FilePath&);
  bool matches(Keybinding, SDL::SDL_Keysym);

  private:
  unordered_map<Keybinding, SDL::SDL_Keysym, CustomHash<Keybinding>> bindings;
};

namespace SDL {
  struct SDL_Keysym;
}

class PrettyInputArchive;

void serialize(PrettyInputArchive&, SDL::SDL_Keysym&);