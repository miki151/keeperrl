#include "stdafx.h"
#include "keybinding_map.h"
#include "gui_elem.h"

KeybindingMap::KeybindingMap(const FilePath&) {
}

bool KeybindingMap::matches(Keybinding key, SDL::SDL_Keysym sym) {
  switch (key) {
    case Keybinding::CREATE_IMP:
      return sym.sym == SDL::SDLK_i && !GuiFactory::isAlt(sym) && !GuiFactory::isCtrl(sym) && !GuiFactory::isShift(sym);
    case Keybinding::FIRE_PROJECTILE:
      return sym.sym == SDL::SDLK_f && !GuiFactory::isAlt(sym) && !GuiFactory::isCtrl(sym) && !GuiFactory::isShift(sym);
  }
}
