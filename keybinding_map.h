#pragma once

#include "sdl.h"
#include "keybinding.h"
#include "file_path.h"

class FilePath;

class KeybindingMap {
  public:
  KeybindingMap(const FilePath& defaults, const FilePath& user);
  bool matches(Keybinding, SDL::SDL_Keysym);
  static string getText(SDL::SDL_Keysym, string delimiter = " + ");
  optional<string> getText(Keybinding);
  bool set(Keybinding, SDL::SDL_Keysym);
  void reset();
  
  private:
  unordered_map<Keybinding, SDL::SDL_Keysym, CustomHash<Keybinding>> bindings;
  FilePath defaultsPath;
  FilePath userPath;
  void save();
};

namespace SDL {
  struct SDL_Keysym;
}

class PrettyInputArchive;

void serialize(PrettyInputArchive&, SDL::SDL_Keysym&);