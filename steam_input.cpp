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
    auto getActionInfo = [&] (string name) {
      return ActionInfo {
        steamInput->GetDigitalActionHandle(name.data()),
        name
      };
    };
    actionSet = steamInput->GetActionSetHandle("GameControls");
    actionHandles[C_ZLEVEL_UP] = getActionInfo("z_level_up");
    actionHandles[C_ZLEVEL_DOWN] = getActionInfo("z_level_down");
    actionHandles[C_OPEN_MENU] = getActionInfo("open_menu");
    actionHandles[C_EXIT_CONTROL_MODE] = getActionInfo("exit_control");
    actionHandles[C_TOGGLE_CONTROL_MODE] = getActionInfo("toggle_control");
    actionHandles[C_CHAT] = getActionInfo("chat");
    actionHandles[C_FIRE_PROJECTILE] = getActionInfo("fire_projectile");
    actionHandles[C_WAIT] = getActionInfo("wait");
    actionHandles[C_SKIP_TURN] = getActionInfo("skip_turn");
    actionHandles[C_STAND_GROUND] = getActionInfo("stand_ground");
    actionHandles[C_IGNORE_ENEMIES] = getActionInfo("ignore_enemies");
    actionHandles[C_COMMANDS] = getActionInfo("commands");
    actionHandles[C_INVENTORY] = getActionInfo("inventory");
    actionHandles[C_ABILITIES] = getActionInfo("abilities");
    actionHandles[C_WORLD_MAP] = getActionInfo("world_map");
    actionHandles[C_MINI_MAP] = getActionInfo("mini_map");
    actionHandles[C_ZOOM] = getActionInfo("zoom");
    actionHandles[C_BUILDINGS_MENU] = getActionInfo("buildings_menu");
    actionHandles[C_MINIONS_MENU] = getActionInfo("minions_menu");
    actionHandles[C_TECH_MENU] = getActionInfo("tech_menu");
    actionHandles[C_HELP_MENU] = getActionInfo("help_menu");
    actionHandles[C_VILLAINS_MENU] = getActionInfo("villains_menu");
    actionHandles[C_PAUSE] = getActionInfo("pause");
    actionHandles[C_SPEED_UP] = getActionInfo("speed_up");
    actionHandles[C_SPEED_DOWN] = getActionInfo("speed_down");
    actionHandles[C_BUILDINGS_UP] = getActionInfo("buildings_up");
    actionHandles[C_BUILDINGS_DOWN] = getActionInfo("buildings_down");
    actionHandles[C_BUILDINGS_LEFT] = getActionInfo("buildings_left");
    actionHandles[C_BUILDINGS_RIGHT] = getActionInfo("buildings_right");
    actionHandles[C_BUILDINGS_CONFIRM] = getActionInfo("buildings_confirm");
    actionHandles[C_BUILDINGS_CANCEL] = getActionInfo("buildings_cancel");
    joyHandles[ControllerJoy::MAP_SCROLLING] = steamInput->GetAnalogActionHandle("map_scrolling_joy");
    joyHandles[ControllerJoy::WALKING] = steamInput->GetAnalogActionHandle("walking_joy");
    gameActionLayers[GameActionLayer::TURNED_BASED] = steamInput->GetActionSetHandle("TurnBased");
    gameActionLayers[GameActionLayer::REAL_TIME] = steamInput->GetActionSetHandle("RealTime");
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
      if (!!actionLayer)
        steamInput->ActivateActionSetLayer(c, gameActionLayers.at(*actionLayer));
    }
  }
}

bool MySteamInput::isPressed(ControllerKey key) {
  if (auto steamInput = SteamInput()) {
    auto action = actionHandles.find(key);
    for (auto input : controllers)
      if (steamInput->GetDigitalActionData(input, action->second.handle).bState)
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
    steamInput->RunFrame();
    auto nowPressed = [&] {
      for (auto input : controllers)
        for (auto action : actionHandles) {
          EInputActionOrigin origins[STEAM_INPUT_MAX_ORIGINS];
          auto a = actionSet;
          if (actionLayer)
            a = gameActionLayers.at(*actionLayer);
          auto res = steamInput->GetDigitalActionOrigins(input, a, action.second.handle, origins);
          if (res > 0)
            actionGlyphs[action.first] = steamInput->GetGlyphPNGForActionOrigin(origins[0], k_ESteamInputGlyphSize_Small, 0);
          if (steamInput->GetDigitalActionData(input, action.second.handle).bState)
            return true;
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
  if (SteamInput() && actionLayer != layer)
    if (auto steamInput = SteamInput()) {
      steamInput->DeactivateAllActionSetLayers(STEAM_INPUT_HANDLE_ALL_CONTROLLERS);
      steamInput->ActivateActionSetLayer(STEAM_INPUT_HANDLE_ALL_CONTROLLERS, gameActionLayers.at(layer));
      actionLayer = layer;
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

void MySteamInput::showBindingScreen() {
  if (auto steamInput = SteamInput())
    for (auto input : controllers) {
      steamInput->ShowBindingPanel(input);
      break;
    }
}

void MySteamInput::showFloatingKeyboard(Rectangle field) {
  if (auto utils = SteamUtils())
    utils->ShowFloatingGamepadTextInput(k_EFloatingGamepadTextInputModeModeSingleLine,
      field.left(), field.top(), field.width(), field.height());
}

void MySteamInput::dismissFloatingKeyboard() {
  if (auto utils = SteamUtils())
    utils->DismissFloatingGamepadTextInput();
}
