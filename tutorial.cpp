#include "stdafx.h"
#include "tutorial.h"
#include "game_info.h"
#include "game.h"
#include "collective.h"
#include "resource_id.h"
#include "zones.h"
#include "tutorial_highlight.h"

SERIALIZE_DEF(Tutorial, state)

enum class Tutorial::State {
  WELCOME,
  CUT_TREES,
  BUILD_STORAGE,
  GET_1000_WOOD,
  FINISHED,
};


bool Tutorial::canContinue(const Game* game) const {
  auto collective = game->getPlayerCollective();
  switch (state) {
    case State::GET_1000_WOOD:
      return collective->numResource(CollectiveResourceId::WOOD) >= 1000;
    case State::BUILD_STORAGE:
      return collective->getZones().getPositions(ZoneId::STORAGE_RESOURCES).size() > 0;
    case State::CUT_TREES:
      return collective->getZones().getPositions(ZoneId::FETCH_ITEMS).size() > 0;
    case State::WELCOME:
      return true;
    case State::FINISHED:
      return false;
  }
}

string Tutorial::getMessage() const {
  switch (state) {
    case State::CUT_TREES:
      return "Your first task is to gather some wood! Select the \"Dig or cut tree\" order and click on "
          "a few trees to order your imps to cut them down.";
    case State::BUILD_STORAGE:
      return "You need to store the wood somewhere before you can use it. Select resource storage and click on some "
          "squares to designate storage.";
    case State::GET_1000_WOOD:
      return "Cut some more trees until how have gathered at least 1000 wood.";
    case State::WELCOME:
      return "Welcome to the KeeperRL tutorial! Let's get you up to speed with the game :)";
    case State::FINISHED:
      return "Congratulations, you have completed the tutorial! Go play the game now :)";
  }
}

EnumSet<TutorialHighlight> Tutorial::getHighlights(const Game* game) const {
  if (canContinue(game))
    return {};
  switch (state) {
    case State::CUT_TREES:
      return {TutorialHighlight::DIG_OR_CUT_TREES};
    case State::BUILD_STORAGE:
      return {TutorialHighlight::RESOURCE_STORAGE};
    case State::GET_1000_WOOD:
      return {TutorialHighlight::WOOD_RESOURCE};
    default:
      return {};
  }
}

void Tutorial::refreshInfo(const Game* game, optional<TutorialInfo>& info) const {
  info = TutorialInfo {getMessage(), canContinue(game), (int) state > 0, getHighlights(game)};
}

void Tutorial::continueTutorial(const Game* game) {
  if (canContinue(game))
    state = (State)((int) state + 1);
}

void Tutorial::goBack() {
  if ((int) state > 0)
    state = (State)((int) state - 1);
}
