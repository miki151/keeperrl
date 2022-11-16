#pragma once

#include "sdl.h"
#include "keybinding.h"
#include "file_path.h"
#include "color.h"
#include "steam_input.h"

class FilePath;
class GuiFactory;

class KeybindingMap {
  public:
  KeybindingMap(const FilePath& defaults, const FilePath& user);
  bool matches(Keybinding, SDL::SDL_Keysym);
  static string getText(SDL::SDL_Keysym, string delimiter = " + ");
  static optional<ControllerKey> getControllerMapping(Keybinding);
  SGuiElem getGlyph(SGuiElem label, GuiFactory*, Keybinding, Color = Color::WHITE);
  SGuiElem getGlyph(SGuiElem label, GuiFactory*, optional<ControllerKey>, optional<string> alternative, Color = Color::WHITE);
  optional<string> getText(Keybinding);
  bool set(Keybinding, SDL::SDL_Keysym);
  void reset();

  private:
  using KeyMap = unordered_map<Keybinding, SDL::SDL_Keysym, CustomHash<Keybinding>>;
  KeyMap bindings;
  KeyMap defaults;
  FilePath defaultsPath;
  FilePath userPath;
  void save();
};

namespace SDL {
  struct SDL_Keysym;
}

class PrettyInputArchive;

void serialize(PrettyInputArchive&, SDL::SDL_Keysym&);