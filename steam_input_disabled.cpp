#ifndef USE_STEAMWORKS

#include "steam_input.h"
#include "clock.h"

void MySteamInput::init() { }

void MySteamInput::detectControllers() { }

bool MySteamInput::isPressed(ControllerKey key) {
  return false;
}

optional<FilePath> MySteamInput::getGlyph(ControllerKey key) {
  return none;
}

void MySteamInput::runFrame() { }

pair<double, double> MySteamInput::getJoyPos(ControllerJoy j) {
  return {0, 0};
}

void MySteamInput::setGameActionLayer(GameActionLayer layer) { }

optional<ControllerKey> MySteamInput::getEvent() {
  return none;
}

void MySteamInput::showBindingScreen() { }

bool MySteamInput::isRunningOnDeck() {
  return false;
}

void MySteamInput::showFloatingKeyboard(Rectangle field) { }

void MySteamInput::dismissFloatingKeyboard() { }

#endif
