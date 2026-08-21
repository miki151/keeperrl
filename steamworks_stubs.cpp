// Stub implementations used when building without Steamworks (NO_STEAMWORKS).
//
// The real steam_*.cpp files (steam_achievements.cpp, steam_input.cpp, ...)
// pull in the proprietary Steamworks SDK headers under extern/steamworks/,
// which are not present in a non-Steam build, so those translation units are
// excluded from the build in the Makefile. However, the rest of the codebase
// still references SteamAchievements and MySteamInput unconditionally, so these
// no-op stubs provide the symbols the linker needs.

#include "stdafx.h"
#include "steam_achievements.h"
#include "achievement_id.h"
#include "steam_input.h"

SteamAchievements::SteamAchievements() {
}

void SteamAchievements::achieve(AchievementId) {
  // no-op: Steamworks integration disabled
}

void MySteamInput::init() {
}

void MySteamInput::detectControllers() {
}

bool MySteamInput::isPressed(ControllerKey key) {
  return false;
}

optional<FilePath> MySteamInput::getGlyph(ControllerKey key) {
  return none;
}

void MySteamInput::runFrame() {
}

pair<double, double> MySteamInput::getJoyPos(ControllerJoy j) {
  return {0.0, 0.0};
}

void MySteamInput::setGameActionLayer(GameActionLayer layer) {
}

optional<ControllerKey> MySteamInput::getEvent() {
  return none;
}

void MySteamInput::showBindingScreen() {
}

bool MySteamInput::isRunningOnDeck() {
  return false;
}

void MySteamInput::showFloatingKeyboard(Rectangle field) {
}

void MySteamInput::dismissFloatingKeyboard() {
}
