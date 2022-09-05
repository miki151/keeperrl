#include "steam_input.h"
#include "extern/steamworks/public/steam/isteaminput.h"

void MySteamInput::init() {
  if (auto steamInput = SteamInput()) {
    steamInput->Init(true);
    actionSets[ActionSet::MENU] = steamInput->GetActionSetHandle("MenuControls");
    actionSets[ActionSet::GAME] = steamInput->GetActionSetHandle("GameControls");
    actionHandles[C_MENU_UP] = steamInput->GetDigitalActionHandle("menu_up");
    actionHandles[C_MENU_DOWN] = steamInput->GetDigitalActionHandle("menu_down");
    actionHandles[C_MENU_SELECT] = steamInput->GetDigitalActionHandle("menu_select");
    actionHandles[C_MENU_CANCEL] = steamInput->GetDigitalActionHandle("menu_cancel");
    actionHandles[C_ZLEVEL_UP] = steamInput->GetDigitalActionHandle("z_level_up");
    actionHandles[C_ZLEVEL_DOWN] = steamInput->GetDigitalActionHandle("z_level_down");
    actionHandles[C_OPEN_MENU] = steamInput->GetDigitalActionHandle("open_menu");
    actionHandles[C_SCROLL_NORTH] = steamInput->GetDigitalActionHandle("scroll_north");
    actionHandles[C_SCROLL_SOUTH] = steamInput->GetDigitalActionHandle("scroll_south");
    actionHandles[C_SCROLL_WEST] = steamInput->GetDigitalActionHandle("scroll_west");
    actionHandles[C_SCROLL_EAST] = steamInput->GetDigitalActionHandle("scroll_east");
    actionHandles[C_EXIT_CONTROL_MODE] = steamInput->GetDigitalActionHandle("exit_control");
    actionHandles[C_TOGGLE_CONTROL_MODE] = steamInput->GetDigitalActionHandle("toggle_control");
    actionHandles[C_CHAT] = steamInput->GetDigitalActionHandle("chat");
    actionHandles[C_FIRE_PROJECTILE] = steamInput->GetDigitalActionHandle("fire_projectile");
    actionHandles[C_WAIT] = steamInput->GetDigitalActionHandle("wait");
    controllers = vector<InputHandle_t>(STEAM_INPUT_MAX_COUNT, 0);
    steamInput->RunFrame();
    int cnt = steamInput->GetConnectedControllers(controllers.data());
    controllers.resize(cnt);
    CHECK(cnt > 0) << "No controller found";
    pushActionSet(ActionSet::GAME);
  }
}

void MySteamInput::pushActionSet(ActionSet a) {
  if (auto steamInput = SteamInput()) {
    actionSetStack.push_back(a);
    steamInput->ActivateActionSet(STEAM_INPUT_HANDLE_ALL_CONTROLLERS, actionSets.at(a));
    std::cout << "Action set " << int(a) << std::endl;
  }
}

void MySteamInput::popActionSet() {
  if (auto steamInput = SteamInput()) {
    actionSetStack.pop_back();
    steamInput->ActivateActionSet(STEAM_INPUT_HANDLE_ALL_CONTROLLERS, actionSets.at(actionSetStack.back()));
    std::cout << "Action set " << int(actionSetStack.back()) << std::endl;
  }
}

optional<ControllerKey> MySteamInput::getEvent() {
  if (auto steamInput = SteamInput()) {
    steamInput->RunFrame();
    for (auto input : controllers)
      for (auto action : actionHandles)
        if (steamInput->GetDigitalActionData(input, action.second).bState) {
          if (!pressed) {
            pressed = true;
            std::cout << "SI " << int(action.first) - (1 << 20) << std::endl;
            return action.first;
          } else
            return none;
        }
  }
  pressed = false;
  return none;
}
