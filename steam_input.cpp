#include "steam_input.h"
#include "extern/steamworks/public/steam/isteaminput.h"
#include "extern/steamworks/public/steam/isteamutils.h"
#include "clock.h"

void MySteamInput::init() {
  if (auto steamInput = SteamInput()) {
    steamInput->Init(true);
    detectControllers();
  }
}

void MySteamInput::detectControllers() {
  auto currentTime = Clock::getRealMillis();
  if (!!lastControllersCheck && currentTime - *lastControllersCheck < milliseconds{1000})
    return;
  lastControllersCheck = currentTime;
  if (auto steamInput = SteamInput()) {
    auto getActionInfo = [&] (string name, ActionSet actionSet) {
      return ActionInfo {
        steamInput->GetDigitalActionHandle(name.data()),
        name,
        actionSet
      };
    };
    actionSets[ActionSet::MENU] = steamInput->GetActionSetHandle("MenuControls");
    actionSets[ActionSet::GAME] = steamInput->GetActionSetHandle("GameControls");
    actionHandles[C_MENU_UP] = getActionInfo("menu_up", ActionSet::MENU);
    actionHandles[C_MENU_DOWN] = getActionInfo("menu_down", ActionSet::MENU);
    actionHandles[C_MENU_SCROLL_UP] = getActionInfo("menu_scroll_up", ActionSet::MENU);
    actionHandles[C_MENU_SCROLL_DOWN] = getActionInfo("menu_scroll_down", ActionSet::MENU);
    actionHandles[C_MENU_LEFT] = getActionInfo("menu_left", ActionSet::MENU);
    actionHandles[C_MENU_RIGHT] = getActionInfo("menu_right", ActionSet::MENU);
    actionHandles[C_MENU_SELECT] = getActionInfo("menu_select", ActionSet::MENU);
    actionHandles[C_MENU_CANCEL] = getActionInfo("menu_cancel", ActionSet::MENU);
    actionHandles[C_ZLEVEL_UP] = getActionInfo("z_level_up", ActionSet::GAME);
    actionHandles[C_ZLEVEL_DOWN] = getActionInfo("z_level_down", ActionSet::GAME);
    actionHandles[C_OPEN_MENU] = getActionInfo("open_menu", ActionSet::GAME);
    actionHandles[C_EXIT_CONTROL_MODE] = getActionInfo("exit_control", ActionSet::GAME);
    actionHandles[C_TOGGLE_CONTROL_MODE] = getActionInfo("toggle_control", ActionSet::GAME);
    actionHandles[C_CHAT] = getActionInfo("chat", ActionSet::GAME);
    actionHandles[C_FIRE_PROJECTILE] = getActionInfo("fire_projectile", ActionSet::GAME);
    actionHandles[C_WAIT] = getActionInfo("wait", ActionSet::GAME);
    actionHandles[C_SKIP_TURN] = getActionInfo("skip_turn", ActionSet::GAME);
    actionHandles[C_STAND_GROUND] = getActionInfo("stand_ground", ActionSet::GAME);
    actionHandles[C_IGNORE_ENEMIES] = getActionInfo("ignore_enemies", ActionSet::GAME);
    actionHandles[C_COMMANDS] = getActionInfo("commands", ActionSet::GAME);
    actionHandles[C_INVENTORY] = getActionInfo("inventory", ActionSet::GAME);
    actionHandles[C_ABILITIES] = getActionInfo("abilities", ActionSet::GAME);
    actionHandles[C_WORLD_MAP] = getActionInfo("world_map", ActionSet::GAME);
    actionHandles[C_MINI_MAP] = getActionInfo("mini_map", ActionSet::GAME);
    actionHandles[C_ZOOM] = getActionInfo("zoom", ActionSet::GAME);
    actionHandles[C_BUILDINGS_MENU] = getActionInfo("buildings_menu", ActionSet::GAME);
    actionHandles[C_MINIONS_MENU] = getActionInfo("minions_menu", ActionSet::GAME);
    actionHandles[C_TECH_MENU] = getActionInfo("tech_menu", ActionSet::GAME);
    actionHandles[C_HELP_MENU] = getActionInfo("help_menu", ActionSet::GAME);
    actionHandles[C_VILLAINS_MENU] = getActionInfo("villains_menu", ActionSet::GAME);
    actionHandles[C_PAUSE] = getActionInfo("pause", ActionSet::GAME);
    actionHandles[C_SPEED_UP] = getActionInfo("speed_up", ActionSet::GAME);
    actionHandles[C_SPEED_DOWN] = getActionInfo("speed_down", ActionSet::GAME);
    actionHandles[C_BUILDINGS_UP] = getActionInfo("buildings_up", ActionSet::GAME);
    actionHandles[C_BUILDINGS_DOWN] = getActionInfo("buildings_down", ActionSet::GAME);
    actionHandles[C_BUILDINGS_LEFT] = getActionInfo("buildings_left", ActionSet::GAME);
    actionHandles[C_BUILDINGS_RIGHT] = getActionInfo("buildings_right", ActionSet::GAME);
    actionHandles[C_BUILDINGS_CONFIRM] = getActionInfo("buildings_confirm", ActionSet::GAME);
    actionHandles[C_BUILDINGS_CANCEL] = getActionInfo("buildings_cancel", ActionSet::GAME);
    actionHandles[C_SHIFT] = getActionInfo("shift", ActionSet::GAME);
    joyHandles[ControllerJoy::MAP_SCROLLING] = steamInput->GetAnalogActionHandle("map_scrolling_joy");
    joyHandles[ControllerJoy::MENU_SCROLLING] = steamInput->GetAnalogActionHandle("menu_scrolling_joy");
    joyHandles[ControllerJoy::WALKING] = steamInput->GetAnalogActionHandle("walking_joy");
    gameActionLayers[GameActionLayer::TURNED_BASED] = steamInput->GetActionSetHandle("TurnBased");
    gameActionLayers[GameActionLayer::REAL_TIME] = steamInput->GetActionSetHandle("RealTime");
    if (actionSetStack.empty())
      pushActionSet(ActionSet::GAME);
    vector<InputHandle_t> controllersTmp(STEAM_INPUT_MAX_COUNT, 0);
    steamInput->RunFrame();
    int cnt = steamInput->GetConnectedControllers(controllersTmp.data());
    if (cnt == controllers.size())
      return;
    actionGlyphs.clear();
    controllersTmp.resize(cnt);
    controllers = controllersTmp;
    steamInput->RunFrame();
    for (auto c : controllers) {
      steamInput->DeactivateAllActionSetLayers(c);
      auto actionSet = actionSetStack.back();
      steamInput->ActivateActionSet(c, actionSets.at(actionSet));
      CHECK(steamInput->GetCurrentActionSet(c) == actionSets.at(actionSet));
      if (actionSet == ActionSet::GAME && !!actionLayer)
        steamInput->ActivateActionSetLayer(c, gameActionLayers.at(*actionLayer));
    }
  }
}

bool MySteamInput::isPressed(ControllerKey key) {
  if (auto steamInput = SteamInput()) {
    auto action = actionHandles.find(key);
    for (auto input : controllers)
      if (action->second.actionSet == actionSetStack.back() &&
          steamInput->GetDigitalActionData(input, action->second.handle).bState)
        return true;
  }
  return false;
}

optional<FilePath> MySteamInput::getGlyph(ControllerKey key) {
  if (auto v = getReferenceMaybe(actionGlyphs, key))
    if (!v->empty())
      return FilePath::fromFullPath(*v);
  return none;
}

void MySteamInput::runFrame() {
  if (auto steamInput = SteamInput()) {
    detectControllers();
    if (actionSetStack.back() == ActionSet::GAME && !actionLayer)
      return;
    steamInput->RunFrame();
    auto nowPressed = [&] {
      for (auto input : controllers)
        for (auto action : actionHandles) {
          EInputActionOrigin origins[STEAM_INPUT_MAX_ORIGINS];
          auto a = actionSets.at(action.second.actionSet);
          if (actionLayer)
            a = gameActionLayers.at(*actionLayer);
          auto res = steamInput->GetDigitalActionOrigins(input, a, action.second.handle, origins);
          if (res > 0)
            actionGlyphs[action.first] = steamInput->GetGlyphPNGForActionOrigin(origins[0], k_ESteamInputGlyphSize_Small, 0);
          if (action.second.actionSet == actionSetStack.back() &&
              steamInput->GetDigitalActionData(input, action.second.handle).bState) {
            return true;
          }
        }
      return false;
    }();
    auto curTime = Clock::getRealMillis();
    auto repeatDelay = rapidFire ? milliseconds{100} : milliseconds{500};
    if (!nowPressed) {
      lastPressed = none;
      rapidFire = false;
    } else if (!lastPressed || *lastPressed < curTime - repeatDelay) {
      if (!!lastPressed)
        rapidFire = true;
      for (auto input : controllers)
        for (auto action : actionHandles) {
          auto res = steamInput->GetDigitalActionData(input, action.second.handle);
          if (res.bState) {
            actionQueue.push(action.first);
            lastPressed = curTime;
          }
        }
    }
  }
}

void MySteamInput::pushActionSet(ActionSet a) {
  if (auto steamInput = SteamInput()) {
    actionSetStack.push_back(a);
    steamInput->RunFrame();
    steamInput->DeactivateAllActionSetLayers(STEAM_INPUT_HANDLE_ALL_CONTROLLERS);
    steamInput->ActivateActionSet(STEAM_INPUT_HANDLE_ALL_CONTROLLERS, actionSets.at(a));
    actionLayer = none;
  }
}

pair<double, double> MySteamInput::getJoyPos(ControllerJoy j) {
  if (auto steamInput = SteamInput()) {
    for (auto input : controllers) {
      auto ret = steamInput->GetAnalogActionData(input, joyHandles.at(j));
      return {ret.x, ret.y};
    }
  }
  return {0, 0};
}

void MySteamInput::setGameActionLayer(GameActionLayer layer) {
  if (SteamInput() && actionSetStack.back() == ActionSet::GAME && actionLayer != layer)
    if (auto steamInput = SteamInput()) {
      steamInput->DeactivateAllActionSetLayers(STEAM_INPUT_HANDLE_ALL_CONTROLLERS);
      steamInput->ActivateActionSetLayer(STEAM_INPUT_HANDLE_ALL_CONTROLLERS, gameActionLayers.at(layer));
      actionLayer = layer;
    }
}

void MySteamInput::popActionSet() {
  if (auto steamInput = SteamInput()) {
    auto prev = actionSetStack.back();
    actionSetStack.pop_back();
    steamInput->RunFrame();
    steamInput->DeactivateAllActionSetLayers(STEAM_INPUT_HANDLE_ALL_CONTROLLERS);
    steamInput->ActivateActionSet(STEAM_INPUT_HANDLE_ALL_CONTROLLERS, actionSets.at(actionSetStack.back()));
    actionLayer = none;
  }
}

optional<ControllerKey> MySteamInput::getEvent() {
  if (!actionQueue.empty()) {
    auto a = actionQueue.front();
    actionQueue.pop();
    return a;
  }
  return none;
}

void MySteamInput::showFloatingKeyboard(Rectangle field) {
  SteamUtils()->ShowFloatingGamepadTextInput(k_EFloatingGamepadTextInputModeModeSingleLine,
      field.left(), field.top(), field.width(), field.height());
}

void MySteamInput::dismissFloatingKeyboard() {
  SteamUtils()->DismissFloatingGamepadTextInput();
}
