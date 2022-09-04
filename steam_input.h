#pragma once

#include "util.h"

enum ControllerKey {
  C_MENU_UP = (1 << 20),
  C_MENU_DOWN,
  C_MENU_SELECT,
  C_MENU_CANCEL
};

class MySteamInput {
  public:
  using Handle = unsigned long long;
  Handle actionSetHandle;
  map<ControllerKey, Handle> actionHandles;
  vector<Handle> controllers;
  void init();
  optional<ControllerKey> getEvent();
  bool pressed = false;
};
