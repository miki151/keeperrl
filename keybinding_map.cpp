#include "stdafx.h"
#include "keybinding_map.h"
#include "gui_elem.h"
#include "pretty_archive.h"
#include "pretty_printing.h"

KeybindingMap::KeybindingMap(const FilePath& path) {
  while (true) {
    if (auto error = PrettyPrinting::parseObject(bindings, {path}, nullptr)) {
      USER_INFO << "Error loading keybindings: " << *error;
      bindings.clear();
    } else
      break;  
  }
}

static SDL::Uint16 getMod(SDL::Uint16 m) {
  if (m & SDL::KMOD_RCTRL)
    m = m | SDL::KMOD_LCTRL;
  if (m & SDL::KMOD_RSHIFT)
    m = m | SDL::KMOD_LSHIFT;
  if (m & SDL::KMOD_RALT)
    m = m | SDL::KMOD_LALT;
  return m & (SDL::KMOD_LCTRL | SDL::KMOD_LSHIFT | SDL::KMOD_LALT);
}

bool KeybindingMap::matches(Keybinding key, SDL::SDL_Keysym sym) {
  if (auto k = getReferenceMaybe(bindings, key))
    return k->sym == sym.sym && getMod(k->mod) == getMod(sym.mod);
  return false;
}

static const map<string, SDL::SDL_Keycode> keycodes {
  {"A", SDL::SDLK_a},
  {"B", SDL::SDLK_b},
  {"C", SDL::SDLK_c},
  {"D", SDL::SDLK_d},
  {"E", SDL::SDLK_e},
  {"F", SDL::SDLK_f},
  {"G", SDL::SDLK_g},
  {"H", SDL::SDLK_h},
  {"I", SDL::SDLK_i},
  {"J", SDL::SDLK_j},
  {"K", SDL::SDLK_k},
  {"L", SDL::SDLK_l},
  {"M", SDL::SDLK_m},
  {"N", SDL::SDLK_n},
  {"O", SDL::SDLK_o},
  {"P", SDL::SDLK_p},
  {"Q", SDL::SDLK_q},
  {"R", SDL::SDLK_r},
  {"S", SDL::SDLK_s},
  {"T", SDL::SDLK_t},
  {"U", SDL::SDLK_u},
  {"V", SDL::SDLK_v},
  {"W", SDL::SDLK_w},
  {"X", SDL::SDLK_x},
  {"Y", SDL::SDLK_y},
  {"Z", SDL::SDLK_z},
  {"0", SDL::SDLK_0},
  {"1", SDL::SDLK_1},
  {"2", SDL::SDLK_2},
  {"3", SDL::SDLK_3},
  {"4", SDL::SDLK_4},
  {"5", SDL::SDLK_5},
  {"6", SDL::SDLK_6},
  {"7", SDL::SDLK_7},
  {"8", SDL::SDLK_8},
  {"9", SDL::SDLK_9},
  {"SPACE", SDL::SDLK_SPACE},
  {"COMMA", SDL::SDLK_COMMA},
  {"PERIOD", SDL::SDLK_PERIOD},
};

string KeybindingMap::getText(SDL::SDL_Keysym sym) {
  static unordered_map<SDL::SDL_Keycode, string> keys = [] {
    unordered_map<SDL::SDL_Keycode, string> ret;
    for (auto& elem : keycodes)
      ret[elem.second] = elem.first;
    return ret;
  }();
  string ret = keys.at(sym.sym);
  if (sym.mod & SDL::KMOD_LCTRL)
    ret = "ctrl+" + ret;
  if (sym.mod & SDL::KMOD_LSHIFT)
    ret = "shift+" + ret;
  if (sym.mod & SDL::KMOD_LALT)
    ret = "alt+" + ret;
  return ret;
}

optional<string> KeybindingMap::getText(Keybinding key) {
  if (auto ks = getReferenceMaybe(bindings, key))
    return getText(*ks);
  return none;
}

void serialize(PrettyInputArchive& ar, SDL::SDL_Keysym& sym) {
  sym.mod = 0;
  while (true) {
    string s;
    ar.readText(s);
    if (s == "ctrl")
      sym.mod = sym.mod | SDL::KMOD_LCTRL;
    else if (s == "shift")
      sym.mod = sym.mod | SDL::KMOD_LSHIFT;
    else if (s == "alt")
      sym.mod = sym.mod | SDL::KMOD_LALT;
    else if (auto code = getValueMaybe(keycodes, s)) {
      sym.sym = *code;
      break;
    } else
      ar.error("Unknown key code: \"" + s + "\"");
  }
}