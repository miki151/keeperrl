#pragma once

#include "util.h"
#include "sdl.h"

enum ControllerKey {
  C_MENU_CANCEL = SDL::SDLK_ESCAPE,
  C_MENU_UP = (1 << 20),
  C_MENU_DOWN,
  C_MENU_LEFT,
  C_MENU_RIGHT,
  C_MENU_SELECT,
  C_MENU_SCROLL_UP,
  C_MENU_SCROLL_DOWN,
  C_ZLEVEL_UP,
  C_ZLEVEL_DOWN,
  C_OPEN_MENU,
  C_SCROLL_NORTH,
  C_SCROLL_SOUTH,
  C_SCROLL_EAST,
  C_SCROLL_WEST,
  C_EXIT_CONTROL_MODE,
  C_TOGGLE_CONTROL_MODE,
  C_CHAT,
  C_FIRE_PROJECTILE,
  C_WAIT,
  C_SKIP_TURN,
  C_STAND_GROUND,
  C_IGNORE_ENEMIES,
  C_WALK,
  C_COMMANDS,
  C_INVENTORY,
  C_ABILITIES,
  C_CHANGE_Z_LEVEL,
  C_WORLD_MAP,
  C_MINI_MAP,
  C_ZOOM,
  C_DIRECTION_CONFIRM,
  C_DIRECTION_CANCEL,
  C_BUILDINGS_MENU,
  C_MINIONS_MENU,
  C_TECH_MENU,
  C_HELP_MENU,
  C_SPEED_MENU,
  C_BUILDINGS_UP,
  C_BUILDINGS_DOWN,
  C_BUILDINGS_LEFT,
  C_BUILDINGS_RIGHT,
  C_BUILDINGS_CONFIRM,
  C_SHIFT
};

enum class ControllerJoy {
  WALKING,
  SCROLLING,
  DIRECTION
};

class MySteamInput {
  public:
  using Handle = unsigned long long;
  enum class ActionSet {
    MENU,
    GAME,
    DIRECTION
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
  void setMenuActionSet();
  void setGameActionSet();
  void showFloatingKeyboard(Rectangle field);
  void dismissFloatingKeyboard();
  optional<ControllerKey> getEvent();
  bool pressed = false;
  vector<ActionSet> actionSetStack;
  void pushActionSet(ActionSet);
  void popActionSet();
  void setGameActionLayer(GameActionLayer);
  pair<double, double> getJoyPos(ControllerJoy);
  void runFrame();
  bool isPressed(ControllerKey);
  const char* getGlyph(ControllerKey);
  queue<ControllerKey> actionQueue;
  optional<GameActionLayer> actionLayer;
  private:
  void detectControllers();
  optional<milliseconds> lastControllersCheck;
  unordered_map<ControllerKey, string, CustomHash<ControllerKey>> actionGlyphs;
};
