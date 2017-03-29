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
#include "construction_map.h"
#include "player_control.h"
#include "collective_config.h"
#include "immigrant_info.h"
#include "keybinding.h"

SERIALIZE_DEF(Tutorial, state)

enum class Tutorial::State {
  WELCOME,
  CUT_TREES,
  BUILD_STORAGE,
  CONTROLS1,
  GET_1000_WOOD,
  DIG_ROOM,
  BUILD_DOOR,
  BUILD_LIBRARY,
  ACCEPT_IMMIGRANT,
  FINISHED,
};


bool Tutorial::canContinue(const Game* game) const {
  auto collective = game->getPlayerCollective();
  switch (state) {
    case State::WELCOME:
      return true;
    case State::CUT_TREES:
      return collective->getZones().getPositions(ZoneId::FETCH_ITEMS).size() > 0;
    case State::BUILD_STORAGE:
      return collective->getZones().getPositions(ZoneId::STORAGE_RESOURCES).size() >= 9;
    case State::CONTROLS1:
      return true;
    case State::GET_1000_WOOD:
      return collective->numResource(CollectiveResourceId::WOOD) >= 1000;
    case State::DIG_ROOM:
      return getHighlightedSquares(game).empty();
    case State::BUILD_DOOR:
      return collective->getConstructions().getBuiltCount(FurnitureType::DOOR)
          + collective->getConstructions().getBuiltCount(FurnitureType::LOCKED_DOOR) >= 1;
    case State::BUILD_LIBRARY:
      return collective->getConstructions().getBuiltCount(FurnitureType::BOOK_SHELF) >= 5;
    case State::ACCEPT_IMMIGRANT:
      return collective->getCreatures(MinionTrait::FIGHTER).size() >= 1;
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
    case State::BUILD_DOOR:
      return "Build a door at the entrance to your dungeon. This will slow down any enemies, "
          "as they will need to destroy it before they can enter. "
          "Your minions can pass through doors freely, unless you lock a door by left clicking on it.\n \n"
          "Try locking and unlocking the door.";
    case State::BUILD_LIBRARY:
      return "The first room that you need to build is a library. This is where the Keeper and other minions "
          "will learn spells, and research new technology. It is also a source of mana. Place at least 5 book shelves "
          "in the new room. Remember that book shelves and other furniture blocks your minions' movement.";
    case State::ACCEPT_IMMIGRANT:
      return "Your dungeon has attracted an orc. "
          "Before your new minion joins you, you must fullfil a few requirements. "
          "Hover your mouse over the immigrant icon to see them. "
          "Once you are ready, accept the immigrant with a left click. Click on the '?' icon immediately to the left "
          "to learn more about immigration.";
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
    case State::ACCEPT_IMMIGRANT:
      return {TutorialHighlight::ACCEPT_IMMIGRANT, TutorialHighlight::BUILD_BED, TutorialHighlight::TRAINING_ROOM};
    case State::BUILD_LIBRARY:
      return {TutorialHighlight::BUILD_LIBRARY};
    case State::BUILD_DOOR:
      return {TutorialHighlight::BUILD_DOOR};
    default:
      return {};
  }
}

vector<Vec2> Tutorial::getHighlightedSquares(const Game* game) const {
  switch (state) {
    case State::DIG_ROOM: {
      vector<Vec2> ret {Vec2(80, 121), Vec2(80, 120), Vec2(80, 119)};
      for (Vec2 v : Rectangle::centered(Vec2(80, 116), 2))
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

Tutorial::Tutorial() : state(State::DIG_ROOM) {

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

void Tutorial::createTutorial(Game& game) {
  game.getPlayerControl()->setTutorial(make_shared<Tutorial>());
  game.getPlayerCollective()->setConfig(CollectiveConfig::keeper(0, 10, {}, {
      ImmigrantInfo(CreatureId::IMP, {MinionTrait::WORKER, MinionTrait::NO_LIMIT, MinionTrait::NO_EQUIPMENT})
          .setSpawnLocation(NearLeader{})
          .setKeybinding(Keybinding::CREATE_IMP)
          .setSound(Sound(SoundId::CREATE_IMP).setPitch(2))
          .setNoAuto()
          .addRequirement(ExponentialCost{ CostInfo(CollectiveResourceId::MANA, 20), 5, 4 }),
      ImmigrantInfo(CreatureId::ORC, {MinionTrait::FIGHTER})
          .setLimit(1)
          .setTutorialHighlight(TutorialHighlight::ACCEPT_IMMIGRANT)
          .addRequirement(0.1, AttractionInfo{1, FurnitureType::TRAINING_WOOD})
          .addRequirement(0.0, AttractionInfo{5, FurnitureType::BOOK_SHELF})
  }));
}
