#pragma once

#include "util.h"
#include "sdl.h"
#include "file_path.h"

enum ControllerKey {
  C_MENU_CANCEL = (1 << 20),
  C_MENU_UP,
  C_MENU_DOWN,
  C_MENU_LEFT,
  C_MENU_RIGHT,
  C_MENU_SELECT,
  C_MENU_SCROLL_UP,
  C_MENU_SCROLL_DOWN,
  C_ZLEVEL_UP,
  C_ZLEVEL_DOWN,
  C_OPEN_MENU,
  C_EXIT_CONTROL_MODE,
  C_TOGGLE_CONTROL_MODE,
  C_CHAT,
  C_FIRE_PROJECTILE,
  C_WAIT,
  C_SKIP_TURN,
  C_STAND_GROUND,
  C_IGNORE_ENEMIES,
  C_COMMANDS,
  C_INVENTORY,
  C_ABILITIES,
  C_WORLD_MAP,
  C_MINI_MAP,
  C_ZOOM,
  C_BUILDINGS_MENU,
  C_MINIONS_MENU,
  C_TECH_MENU,
  C_HELP_MENU,
  C_VILLAINS_MENU,
  C_PAUSE,
  C_SPEED_UP,
  C_SPEED_DOWN,
  C_BUILDINGS_UP,
  C_BUILDINGS_DOWN,
  C_BUILDINGS_LEFT,
  C_BUILDINGS_RIGHT,
  C_BUILDINGS_CONFIRM,
  C_BUILDINGS_CANCEL,
  C_SHIFT
};

enum class ControllerJoy {
  WALKING,
  MAP_SCROLLING,
  MENU_SCROLLING
};

class MySteamInput {
  public:
  using Handle = unsigned long long;
  enum class ActionSet {
    MENU,
    GAME
  };
  enum class GameActionLayer {
    TURNED_BASED,
    REAL_TIME
  };
  struct ActionInfo {
    Handle handle;
    string name;
    ActionSet actionSet;
  };
  map<ActionSet, Handle> actionSets;
  map<GameActionLayer, Handle> gameActionLayers;
  map<ControllerKey, ActionInfo> actionHandles;
  vector<Handle> controllers;
  map<ControllerJoy, Handle> joyHandles;
  void init();
  void showFloatingKeyboard(Rectangle field);
  void dismissFloatingKeyboard();
  optional<ControllerKey> getEvent();
  optional<milliseconds> lastPressed;
  bool rapidFire = false;
  vector<ActionSet> actionSetStack;
  void pushActionSet(ActionSet);
  void popActionSet();
  void setGameActionLayer(GameActionLayer);
  pair<double, double> getJoyPos(ControllerJoy);
  void runFrame();
  bool isPressed(ControllerKey);
  optional<FilePath> getGlyph(ControllerKey);
  queue<ControllerKey> actionQueue;
  optional<GameActionLayer> actionLayer;
  private:
  void detectControllers();
  optional<milliseconds> lastControllersCheck;
  unordered_map<ControllerKey, string, CustomHash<ControllerKey>> actionGlyphs;
};
