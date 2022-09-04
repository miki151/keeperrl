#include "steam_input.h"
#include "extern/steamworks/public/steam/isteaminput.h"

void MySteamInput::init() {
  if (auto steamInput = SteamInput()) {
    steamInput->Init(true);
    actionSetHandle = steamInput->GetActionSetHandle("MenuControls");
    actionHandles[C_MENU_UP] = steamInput->GetDigitalActionHandle("menu_up");
    actionHandles[C_MENU_DOWN] = steamInput->GetDigitalActionHandle("menu_down");
    actionHandles[C_MENU_SELECT] = steamInput->GetDigitalActionHandle("menu_select");
    actionHandles[C_MENU_CANCEL] = steamInput->GetDigitalActionHandle("menu_cancel");
    controllers = vector<InputHandle_t>(STEAM_INPUT_MAX_COUNT, 0);
    steamInput->RunFrame();
    int cnt = steamInput->GetConnectedControllers(controllers.data());
    controllers.resize(cnt);
    for (auto input : controllers)
      steamInput->ActivateActionSet(input, actionSetHandle);
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
            return action.first;
          } else
            return none;
        }
  }
  pressed = false;
  return none;
}
