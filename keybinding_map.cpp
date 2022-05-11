#include "stdafx.h"
#include "keybinding_map.h"
#include "gui_elem.h"

KeybindingMap::KeybindingMap(const FilePath&) {
}

bool KeybindingMap::matches(Keybinding key, SDL::SDL_Keysym sym) {
  if (auto k = getReferenceMaybe(bindings, key))
    return k->sym == sym.sym && k->mod == sym.mod;
  return false;
}
