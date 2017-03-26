#include "stdafx.h"
#include "tutorial.h"
#include "game_info.h"
#include "game.h"
#include "collective.h"
#include "resource_id.h"
#include "zones.h"
#include "tutorial_highlight.h"
#include "territory.h"
#include "level.h"
#include "furniture.h"
#include "destroy_action.h"

SERIALIZE_DEF(Tutorial, state)

enum class Tutorial::State {
  WELCOME,
  CUT_TREES,
  BUILD_STORAGE,
  CONTROLS1,
  GET_1000_WOOD,
  DIG_ROOM,
  FINISHED,
};


bool Tutorial::canContinue(const Game* game) const {
  auto collective = game->getPlayerCollective();
  switch (state) {
    case State::DIG_ROOM:
      return getHighlightedSquares(game).empty();
    case State::GET_1000_WOOD:
      return collective->numResource(CollectiveResourceId::WOOD) >= 1000;
    case State::BUILD_STORAGE:
      return collective->getZones().getPositions(ZoneId::STORAGE_RESOURCES).size() >= 9;
    case State::CONTROLS1:
      return true;
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
    case State::WELCOME:
      return "Welcome to the KeeperRL tutorial!\n \n"
          "Together we will work to build a small dungeon, and assemble a military force. Nearby us lies "
          "a small village inhabited mostly by innocents, whom we will all murder.\n \n"
          "This should get you up to speed with the game!";
    case State::CUT_TREES:
      return "Great things come from small beginnings. Your first task is to gather some wood! "
          "Select the \"Dig or cut tree\" order and click on "
          "a few trees to order your imps to cut them down.\n \nWhen you're done, click the right mouse button "
          "or press escape to clear the order.";
    case State::BUILD_STORAGE:
      return "You need to store the wood somewhere before you can use it. Select resource storage and designate it "
          "by clicking on the map.\n \n"
          "Create at least a 3x3 storage area.";
    case State::CONTROLS1:
      return "Time to learn a few controls! Try scrolling the map using the arrow keys or by right clicking on the map "
          "and dragging it.\n \n"
          "Press SPACE to pause the game and the numbers 1-4 to change its speed. You can "
          "do the same using the mouse in the bottom left corner of the screen.";
    case State::GET_1000_WOOD:
      return "Cut some more trees until how have gathered at least 1000 wood.\n \n";
          //"You can mark things in bulk by dragging the mouse or by holding SHIFT and selecting a rectangle. Try it!";
    case State::DIG_ROOM:
      return "Time to strike the mountain! Start by digging out a one tile-wide tunnel and finish it with at least "
          "a 5x5 room.\n \n"
          "Hold shift to select a rectangular area.";
    case State::FINISHED:
      return "Congratulations, you have completed the tutorial! Go play the game now :)";
  }
}

EnumSet<TutorialHighlight> Tutorial::getHighlights(const Game* game) const {
  if (canContinue(game))
    return {};
  switch (state) {
    case State::DIG_ROOM:
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

vector<Vec2> Tutorial::getHighlightedSquares(const Game* game) const {
  switch (state) {
    case State::DIG_ROOM: {
      vector<Vec2> ret {Vec2(120, 86), Vec2(120, 85), Vec2(120, 84), Vec2(120, 83)};
      for (Vec2 v : Rectangle::centered(Vec2(120, 80), 2))
        ret.push_back(v);
      for (auto elem : Iter(ret)) {
        if (auto furniture = Position(*elem, game->getPlayerCollective()->getLevel())
            .getFurniture(FurnitureLayer::MIDDLE))
          if (furniture->canDestroy(DestroyAction::Type::DIG))
            continue;
        elem.markToErase();
      }
      return ret;
    }
    default:
      return {};
  }
}

void Tutorial::refreshInfo(const Game* game, optional<TutorialInfo>& info) const {
  info = TutorialInfo {
      getMessage(),
      canContinue(game),
      (int) state > 0,
      getHighlights(game),
      getHighlightedSquares(game)
  };
}

void Tutorial::continueTutorial(const Game* game) {
  if (canContinue(game))
    state = (State)((int) state + 1);
}

void Tutorial::goBack() {
  if ((int) state > 0)
    state = (State)((int) state - 1);
}
