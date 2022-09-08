#include "steam_input.h"
#include "extern/steamworks/public/steam/isteaminput.h"

void MySteamInput::init() {
  if (auto steamInput = SteamInput()) {
    steamInput->Init(true);
    auto getActionInfo = [&] (string name) {
      return ActionInfo {
        steamInput->GetDigitalActionHandle(name.data()),
        name
      };
    };
    actionSets[ActionSet::MENU] = steamInput->GetActionSetHandle("MenuControls");
    actionSets[ActionSet::GAME] = steamInput->GetActionSetHandle("GameControls");
    actionSets[ActionSet::DIRECTION] = steamInput->GetActionSetHandle("DirectionChoiceControls");
    actionHandles[C_MENU_UP] = getActionInfo("menu_up");
    actionHandles[C_MENU_DOWN] = getActionInfo("menu_down");
    actionHandles[C_MENU_SELECT] = getActionInfo("menu_select");
    actionHandles[C_MENU_CANCEL] = getActionInfo("menu_cancel");
    actionHandles[C_ZLEVEL_UP] = getActionInfo("z_level_up");
    actionHandles[C_ZLEVEL_DOWN] = getActionInfo("z_level_down");
    actionHandles[C_OPEN_MENU] = getActionInfo("open_menu");
    actionHandles[C_SCROLL_NORTH] = getActionInfo("scroll_north");
    actionHandles[C_SCROLL_SOUTH] = getActionInfo("scroll_south");
    actionHandles[C_SCROLL_WEST] = getActionInfo("scroll_west");
    actionHandles[C_SCROLL_EAST] = getActionInfo("scroll_east");
    actionHandles[C_EXIT_CONTROL_MODE] = getActionInfo("exit_control");
    actionHandles[C_TOGGLE_CONTROL_MODE] = getActionInfo("toggle_control");
    actionHandles[C_CHAT] = getActionInfo("chat");
    actionHandles[C_FIRE_PROJECTILE] = getActionInfo("fire_projectile");
    actionHandles[C_WAIT] = getActionInfo("wait");
    actionHandles[C_WALK] = getActionInfo("walk");
    actionHandles[C_COMMANDS] = getActionInfo("commands");
    actionHandles[C_CHANGE_Z_LEVEL] = getActionInfo("change_z_level");
    actionHandles[C_DIRECTION_CONFIRM] = getActionInfo("direction_confirm");
    actionHandles[C_DIRECTION_CANCEL] = getActionInfo("direction_cancel");
    joyHandles[ControllerJoy::SCROLLING] = steamInput->GetAnalogActionHandle("map_scrolling_joy");
    joyHandles[ControllerJoy::WALKING] = steamInput->GetAnalogActionHandle("walking_joy");
    joyHandles[ControllerJoy::DIRECTION] = steamInput->GetAnalogActionHandle("direction_joy");
    gameActionLayers[GameActionLayer::TURNED_BASED] = steamInput->GetActionSetHandle("TurnBased");
    gameActionLayers[GameActionLayer::REAL_TIME] = steamInput->GetActionSetHandle("RealTime");
    controllers = vector<InputHandle_t>(STEAM_INPUT_MAX_COUNT, 0);
    steamInput->RunFrame();
    int cnt = steamInput->GetConnectedControllers(controllers.data());
    controllers.resize(cnt);
    CHECK(cnt > 0) << "No controller found";
    pushActionSet(ActionSet::GAME);
  }
}

void MySteamInput::runFrame() {
  if (auto steamInput = SteamInput())
    steamInput->RunFrame();
}

void MySteamInput::pushActionSet(ActionSet a) {
  if (auto steamInput = SteamInput()) {
    actionSetStack.push_back(a);
    steamInput->ActivateActionSet(STEAM_INPUT_HANDLE_ALL_CONTROLLERS, actionSets.at(a));
    std::cout << "Pushed action set " << int(a) << std::endl;
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
  if (actionSetStack.back() == ActionSet::GAME)
    if (auto steamInput = SteamInput()) {
//      std::cout << "Game action layer " << int(layer) << std::endl;
      steamInput->DeactivateAllActionSetLayers(STEAM_INPUT_HANDLE_ALL_CONTROLLERS);
      steamInput->ActivateActionSetLayer(STEAM_INPUT_HANDLE_ALL_CONTROLLERS, gameActionLayers.at(layer));
    }
}

void MySteamInput::popActionSet() {
  if (auto steamInput = SteamInput()) {
    auto prev = actionSetStack.back();
    actionSetStack.pop_back();
    steamInput->ActivateActionSet(STEAM_INPUT_HANDLE_ALL_CONTROLLERS, actionSets.at(actionSetStack.back()));
    std::cout << "Popped action set " << int(prev) << " now: " << int(actionSetStack.back()) << std::endl;
  }
}

optional<ControllerKey> MySteamInput::getEvent() {
  if (auto steamInput = SteamInput()) {
    for (auto input : controllers)
      for (auto action : actionHandles) {
        auto res = steamInput->GetDigitalActionData(input, action.second.handle);
        if (res.bState)
          std::cout << "SI " << action.second.name << " " << res.bActive << std::endl;
      }
    for (auto input : controllers)
      for (auto action : actionHandles) {
        auto res = steamInput->GetDigitalActionData(input, action.second.handle);
        if (res.bState && res.bActive) {
          if (!pressed) {
            pressed = true;
            return action.first;
          } else
            return none;
        }
      }
  }
  if (pressed) {
    std::cout << "Nothing pressed " << std::endl;
    pressed = false;
  }
  return none;
}
