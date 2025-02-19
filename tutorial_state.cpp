#include "stdafx.h"
#include "tutorial_state.h"
#include "t_string.h"

string rightClick(bool controller) {
  return controller ? "left trigger" : "right click";
}

TString getMessage(TutorialState state, bool controller) {
  switch (state) {
    case TutorialState::WELCOME:
      return TStringId("TUTORIAL_WELCOME");
    case TutorialState::INTRO:
      return TStringId("TUTORIAL_INTRO");
    case TutorialState::INTRO2:
      return TStringId("TUTORIAL_INTRO2");
    case TutorialState::CUT_TREES:
      return TSentence("TUTORIAL_CUT_TREES",
          controller
              ? TStringId("TUTORIAL_CUT_TREES_CONTROLLER")
              : TStringId("TUTORIAL_CUT_TREES_KEYBOARD"),
          controller
              ? TStringId("TUTORIAL_CUT_TREES_CONTROLLER2")
              : TStringId("TUTORIAL_CUT_TREES_KEYBOARD2"));
    case TutorialState::BUILD_STORAGE:
      return TStringId("TUTORIAL_BUILD_STORAGE");
    case TutorialState::CONTROLS1:
      return TSentence("TUTORIAL_CONTROLS1",
          controller
              ? TStringId("TUTORIAL_CONTROLS1_CONTROLLER")
              : TStringId("TUTORIAL_CONTROLS1_KEYBOARD"),
          controller
              ? TStringId("TUTORIAL_CONTROLS1_CONTROLLER2")
              : TStringId("TUTORIAL_CONTROLS1_KEYBOARD2"));
    case TutorialState::CONTROLS2:
      return TSentence("TUTORIAL_CONTROLS2", controller ? TStringId("TUTORIAL_CONTROLS2_CONTROLLER")
          : TStringId("TUTORIAL_CONTROLS2_KEYBOARD"));
    case TutorialState::GET_200_WOOD:
      return TStringId("TUTORIAL_GET_200_WOOD");
    case TutorialState::DIG_ROOM:
      return TStringId("TUTORIAL_DIG_ROOM");
    case TutorialState::BUILD_DOOR:
      return TStringId("TUTORIAL_BUILD_DOOR");
    case TutorialState::BUILD_LIBRARY:
      return TStringId("TUTORIAL_BUILD_LIBRARY");
    case TutorialState::DIG_2_ROOMS:
    return TSentence("TUTORIAL_DIG_2_ROOMS", controller ? TStringId("TUTORIAL_DIG_2_ROOMS_CONTROLLER")
        : TStringId("TUTORIAL_DIG_2_ROOMS_KEYBOARD"));
    case TutorialState::ACCEPT_IMMIGRANT:
      return TStringId("TUTORIAL_ACCEPT_IMMIGRANT");
    case TutorialState::TORCHES:
      return TStringId("TUTORIAL_TORCHES");
    case TutorialState::FLOORS:
      return TStringId("TUTORIAL_FLOORS");
    case TutorialState::BUILD_WORKSHOP:
      return TStringId("TUTORIAL_BUILD_WORKSHOP");
    case TutorialState::SCHEDULE_WORKSHOP_ITEMS:
      return TStringId("TUTORIAL_SCHEDULE_WORKSHOP_ITEMS");
    case TutorialState::ORDER_CRAFTING:
      return TStringId("TUTORIAL_ORDER_CRAFTING");
    case TutorialState::EQUIP_WEAPON:
      return TStringId("TUTORIAL_EQUIP_WEAPON");
    case TutorialState::ACCEPT_MORE_IMMIGRANTS:
      return TStringId("TUTORIAL_ACCEPT_MORE_IMMIGRANTS");
    case TutorialState::EQUIP_ALL_FIGHTERS:
      return TStringId("TUTORIAL_EQUIP_ALL_FIGHTERS");
    case TutorialState::CREATE_TEAM:
      return TStringId("TUTORIAL_CREATE_TEAM");
    case TutorialState::CONTROL_TEAM:
      return TStringId("TUTORIAL_CONTROL_TEAM");
    case TutorialState::CONTROL_MODE_MOVEMENT:
      return TSentence("TUTORIAL_CONTROL_MODE_MOVEMENT",
          controller
              ? TStringId("TUTORIAL_CONTROL_MODE_MOVEMENT_CONTROLLER")
              : TStringId("TUTORIAL_CONTROL_MODE_MOVEMENT_KEYBOARD"),
          controller
              ? TStringId("TUTORIAL_CONTROL_MODE_MOVEMENT_CONTROLLER2")
              : TStringId("TUTORIAL_CONTROL_MODE_MOVEMENT_KEYBOARD2"));
    case TutorialState::FULL_CONTROL:
      return TSentence("TUTORIAL_FULL_CONTROL", controller ? TStringId("TUTORIAL_FULL_CONTROL_CONTROLLER")
          : TStringId("TUTORIAL_FULL_CONTROL_KEYBOARD"));
    case TutorialState::DISCOVER_VILLAGE:
      return TSentence("TUTORIAL_DISCOVER_VILLAGE", controller ? TStringId("TUTORIAL_DISCOVER_VILLAGE_CONTROLLER")
          : TStringId("TUTORIAL_DISCOVER_VILLAGE_KEYBOARD"));
    case TutorialState::KILL_VILLAGE:
      return TStringId("TUTORIAL_KILL_VILLAGE");
    case TutorialState::LOOT_VILLAGE:
      return TStringId("TUTORIAL_LOOT_VILLAGE");
    case TutorialState::LEAVE_CONTROL:
      return TSentence("TUTORIAL_LEAVE_CONTROL", controller ? TStringId("TUTORIAL_LEAVE_CONTROL_CONTROLLER")
          : TStringId("TUTORIAL_LEAVE_CONTROL_KEYBOARD"));
    case TutorialState::SUMMARY1:
      return TStringId("TUTORIAL_SUMMARY1");
    case TutorialState::RESEARCH:
      return TSentence("TUTORIAL_RESEARCH", controller ? TStringId("TUTORIAL_RESEARCH_CONTROLLER")
          : TStringId("TUTORIAL_RESEARCH_KEYBOARD"));
    case TutorialState::HELP_TAB:
      return TSentence("TUTORIAL_HELP_TAB", controller ? TStringId("TUTORIAL_HELP_TAB_CONTROLLER")
          : TStringId("TUTORIAL_HELP_TAB_KEYBOARD"));
    case TutorialState::MINIMAP_BUTTONS:
      return TStringId("TUTORIAL_MINIMAP_BUTTONS");
    case TutorialState::SUMMARY2:
      return TStringId("TUTORIAL_SUMMARY2");
    case TutorialState::FINISHED:
      return TSentence("TUTORIAL_FINISHED", controller ? TStringId("TUTORIAL_FINISHED_CONTROLLER")
          : TStringId("TUTORIAL_FINISHED_KEYBOARD"));
  }
}
