#include "stdafx.h"
#include "keybinding_map.h"
#include "gui_elem.h"
#include "pretty_archive.h"
#include "pretty_printing.h"
#include "steam_input.h"
#include "t_string.h"

KeybindingMap::KeybindingMap(const FilePath& defaults, const FilePath& user)
    : defaultsPath(defaults), userPath(user) {
  vector<FilePath> paths { defaults };
  if (auto error = PrettyPrinting::parseObject(this->defaults, paths, nullptr))
    USER_FATAL << "Error loading default keybindings: " << *error;
  if (user.exists())
    paths.push_back(user);
  while (true) {
    if (auto error = PrettyPrinting::parseObject(bindings, paths, nullptr)) {
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

static SDL::SDL_Keycode getEquivalent(SDL::SDL_Keycode key){
  if (key == SDL::SDLK_KP_ENTER)
    return SDL::SDLK_RETURN;
  return key;
}

optional<SDL::SDL_Keycode> KeybindingMap::getBuiltinMapping(Keybinding key) {
  static HashMap<Keybinding, SDL::SDL_Keycode> bindings {
    {Keybinding("MENU_UP"), SDL::SDLK_KP_8},
    {Keybinding("MENU_DOWN"), SDL::SDLK_KP_2},
    {Keybinding("MENU_LEFT"), SDL::SDLK_KP_4},
    {Keybinding("MENU_RIGHT"), SDL::SDLK_KP_6},
  };
  return getValueMaybe(bindings, key);
}

optional<ControllerKey> KeybindingMap::getControllerMapping(Keybinding key) {
  static HashMap<Keybinding, ControllerKey> controllerBindings {
      {Keybinding("WAIT"), C_WAIT},
      {Keybinding("CHAT"), C_CHAT},
      {Keybinding("FIRE_PROJECTILE"), C_FIRE_PROJECTILE},
      {Keybinding("SKIP_TURN"), C_SKIP_TURN},
      {Keybinding("STAND_GROUND"), C_STAND_GROUND},
      {Keybinding("IGNORE_ENEMIES"), C_IGNORE_ENEMIES},
      {Keybinding("EXIT_CONTROL_MODE"), C_EXIT_CONTROL_MODE},
      {Keybinding("TOGGLE_CONTROL_MODE"), C_TOGGLE_CONTROL_MODE},
      {Keybinding("OPEN_WORLD_MAP"), C_WORLD_MAP},
      {Keybinding("PAUSE"), C_PAUSE},
      {Keybinding("SPEED_UP"), C_SPEED_UP},
      {Keybinding("SPEED_DOWN"), C_SPEED_DOWN},
      {Keybinding("MENU_DOWN"), C_BUILDINGS_DOWN},
      {Keybinding("MENU_UP"), C_BUILDINGS_UP},
      {Keybinding("MENU_LEFT"), C_BUILDINGS_LEFT},
      {Keybinding("MENU_RIGHT"), C_BUILDINGS_RIGHT},
      {Keybinding("MENU_SELECT"), C_BUILDINGS_CONFIRM},
      {Keybinding("EXIT_MENU"), C_BUILDINGS_CANCEL},
      {Keybinding("SCROLL_Z_UP"), C_ZLEVEL_UP},
      {Keybinding("SCROLL_Z_DOWN"), C_ZLEVEL_DOWN},
  };
  return getValueMaybe(controllerBindings, key);
}

bool KeybindingMap::matches(Keybinding key, SDL::SDL_Keysym sym) {
  if (getControllerMapping(key) == (ControllerKey)sym.sym)
    return true;
  if (getBuiltinMapping(key) == sym.sym)
    return true;
  if (auto k = getReferenceMaybe(bindings, key))
    return getEquivalent(k->sym) == getEquivalent(sym.sym) && getMod(k->mod) == getMod(sym.mod);
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
  {"KEYPAD0", SDL::SDLK_KP_0},
  {"KEYPAD1", SDL::SDLK_KP_1},
  {"KEYPAD2", SDL::SDLK_KP_2},
  {"KEYPAD3", SDL::SDLK_KP_3},
  {"KEYPAD4", SDL::SDLK_KP_4},
  {"KEYPAD5", SDL::SDLK_KP_5},
  {"KEYPAD6", SDL::SDLK_KP_6},
  {"KEYPAD7", SDL::SDLK_KP_7},
  {"KEYPAD8", SDL::SDLK_KP_8},
  {"KEYPAD9", SDL::SDLK_KP_9},
  {"SPACE", SDL::SDLK_SPACE},
  {"COMMA", SDL::SDLK_COMMA},
  {"DELETE", SDL::SDLK_DELETE},
  {"SLASH", SDL::SDLK_SLASH},
  {"BACKSLASH", SDL::SDLK_BACKSLASH},
  {"SEMICOLON", SDL::SDLK_SEMICOLON},
  {"PAGEUP", SDL::SDLK_PAGEUP},
  {"PAGEDOWN", SDL::SDLK_PAGEDOWN},
  {"PERIOD", SDL::SDLK_PERIOD},
  {"UP", SDL::SDLK_UP},
  {"DOWN", SDL::SDLK_DOWN},
  {"LEFT", SDL::SDLK_LEFT},
  {"RIGHT", SDL::SDLK_RIGHT},
  {"ESCAPE", SDL::SDLK_ESCAPE},
  {"ENTER", SDL::SDLK_RETURN},
};

TString KeybindingMap::getText(SDL::SDL_Keysym sym, string delimiter) {
  static unordered_map<SDL::SDL_Keycode, TString> keys = [] {
    unordered_map<SDL::SDL_Keycode, TString> ret;
    for (auto& elem : keycodes)
      ret[elem.second] = elem.first;
    return ret;
  }();
  TString ret = keys.at(sym.sym);
  if (sym.mod & SDL::KMOD_LCTRL)
    ret = TSentence("KEY_MODIFIER", string("Ctrl"), ret);
  if (sym.mod & SDL::KMOD_LSHIFT)
    ret = TSentence("KEY_MODIFIER", string("Shift"), ret);
  if (sym.mod & SDL::KMOD_LALT)
    ret = TSentence("KEY_MODIFIER", string("Alt"), ret);
  return ret;
}

optional<TString> KeybindingMap::getText(Keybinding key) {
  if (auto ks = getReferenceMaybe(bindings, key))
    return getText(*ks);
  return none;
}

SGuiElem KeybindingMap::getGlyph(SGuiElem label, GuiFactory* f, optional<ControllerKey> key,
    optional<TString> alternative) {
  SGuiElem add;
  auto steamInput = f->getSteamInput();
  if (steamInput && !steamInput->controllers.empty()) {
    if (key)
      add = f->steamInputGlyph(*key);
  } else if (alternative)
    add = f->label(TSentence("GLYPH_LABEL", *alternative));
  if (add)
    label = f->getListBuilder()
        .addElemAuto(std::move(label))
        .addSpace(10)
        .addElemAuto(std::move(add))
        .buildHorizontalList();
  return label;
}

SGuiElem KeybindingMap::getGlyph(SGuiElem label, GuiFactory* f, Keybinding key) {
  optional<TString> alternative;
  if (auto k = getReferenceMaybe(bindings, key))
    alternative = getText(*k);
  return getGlyph(std::move(label), f, getControllerMapping(key), std::move(alternative));
}

void KeybindingMap::save() {
  ofstream out(userPath.getPath());
  for (auto& elem : bindings) {
    out << elem.first.data() << " ";
    if (defaults.count(elem.first))
      out << "modify ";
    out << getText(elem.second, " ").data() << endl;
  }
}

void KeybindingMap::reset() {
  if (auto error = PrettyPrinting::parseObject(bindings, {defaultsPath}, nullptr)) {
    USER_INFO << "Error loading default keybindings: " << *error;
    bindings.clear();
  }
  save();
}

bool KeybindingMap::set(Keybinding k, SDL::SDL_Keysym s) {
  for (auto& elem : keycodes)
    if (elem.second == s.sym) {
      bindings[k] = s;
      save();
      return true;
    }
  return false;
}

void serialize(PrettyInputArchive& ar, SDL::SDL_Keysym& sym) {
  sym.mod = 0;
  while (true) {
    string s;
    ar.readText(s);
    if (lowercase(s) == "ctrl")
      sym.mod = sym.mod | SDL::KMOD_LCTRL;
    else if (lowercase(s) == "shift")
      sym.mod = sym.mod | SDL::KMOD_LSHIFT;
    else if (lowercase(s) == "alt")
      sym.mod = sym.mod | SDL::KMOD_LALT;
    else if (auto code = getValueMaybe(keycodes, s)) {
      sym.sym = *code;
      break;
    } else
      ar.error("Unknown key code: \"" + s + "\"");
  }
}
