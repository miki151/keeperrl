#pragma once

#include "util.h"
#include "input.h"
#include "sdl.h"
#include "file_path.h"

#ifndef USE_STEAMWORKS
#error steam_input.h included despite USE_STEAMWORKS not being defined!
#endif

class MySteamInput {
  public:
  using Handle = unsigned long long;
  enum class GameActionLayer {
    TURNED_BASED,
    REAL_TIME
  };
  struct ActionInfo {
    Handle handle;
    string name;
  };
  Handle actionSet;
  map<GameActionLayer, Handle> gameActionLayers;
  map<ControllerKey, ActionInfo> actionHandles;
  vector<Handle> controllers;
  map<ControllerJoy, Handle> joyHandles;
  void init();
  void showBindingScreen();
  void showFloatingKeyboard(Rectangle field);
  void dismissFloatingKeyboard();
  optional<ControllerKey> getEvent();
  optional<milliseconds> lastPressed;
  bool rapidFire = false;
  void setGameActionLayer(GameActionLayer);
  pair<double, double> getJoyPos(ControllerJoy);
  void runFrame();
  bool isPressed(ControllerKey);
  bool isRunningOnDeck();
  optional<FilePath> getGlyph(ControllerKey);
  queue<ControllerKey> actionQueue;
  optional<GameActionLayer> actionLayer;
  private:
  void detectControllers();
  optional<milliseconds> lastControllersCheck;
  HashMap<ControllerKey, string> actionGlyphs;
};
