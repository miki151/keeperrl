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
#include "immigration.h"
#include "tile_efficiency.h"

SERIALIZE_DEF(Tutorial, state)

enum class Tutorial::State {
  WELCOME,
  CUT_TREES,
  BUILD_STORAGE,
  CONTROLS1,
  GET_200_WOOD,
  DIG_ROOM,
  BUILD_DOOR,
  BUILD_LIBRARY,
  DIG_2_ROOMS,
  ACCEPT_IMMIGRANT,
  TORCHES,
  FLOORS,
  FINISHED,
};


bool Tutorial::canContinue(const WGame game) const {
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
    case State::GET_200_WOOD:
      return collective->numResource(CollectiveResourceId::WOOD) >= 200;
    case State::DIG_ROOM:
      return getHighlightedSquaresHigh(game).empty();
    case State::BUILD_DOOR:
      return collective->getConstructions().getBuiltCount(FurnitureType::DOOR)
          + collective->getConstructions().getBuiltCount(FurnitureType::LOCKED_DOOR) >= 1;
    case State::BUILD_LIBRARY:
      return collective->getConstructions().getBuiltCount(FurnitureType::BOOK_SHELF) >= 5;
    case State::DIG_2_ROOMS:
      return getHighlightedSquaresHigh(game).empty();
    case State::ACCEPT_IMMIGRANT:
      return collective->getCreatures(MinionTrait::FIGHTER).size() >= 1;
    case State::TORCHES:
      for (auto furniture : {FurnitureType::BOOK_SHELF, FurnitureType::TRAINING_WOOD})
        for (auto pos : collective->getConstructions().getBuiltPositions(furniture))
          if (collective->getTileEfficiency().getEfficiency(pos) < 0.99)
            return false;
      return true;
    case State::FLOORS:
      return getHighlightedSquaresLow(game).empty();
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
          "Press SPACE to pause and continue the game. You can still give orders and use all controls while the game "
          "is paused.";
    case State::GET_200_WOOD:
      return "Cut some more trees until how have gathered at least 200 wood.\n \n";
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
    case State::DIG_2_ROOMS:
      return "Dig out some more rooms. "
          "If the library is blocking your tunnels, use the 'remove construction' order to get it out of the way.\n \n"
          "Change the speed of the game using the keys 1-4 or by clicking in the "
          "lower left corner if it's taking too long.";
    case State::ACCEPT_IMMIGRANT:
      return "Your dungeon has attracted an orc. "
          "Before your new minion joins you, you must fullfil a few requirements. "
          "Hover your mouse over the immigrant icon to see them. "
          "Once you are ready, accept the immigrant with a left click. Click on the '?' icon immediately to the left "
          "to learn more about immigration.";
    case State::TORCHES:
      return "We need to make sure that your minions have good working conditions when studying and training. Place some "
          "torches in your rooms to light them up. Hover your mouse over the book shelves and training dummies "
          "and look in the lower right corner to make sure that their efficiency is at least 100.";
    case State::FLOORS:
      return "Furniture is more efficient if there is a nice floor underneath and around it. For now you can only "
          "afford wooden floor, but it should do.";
    case State::FINISHED:
      return "Congratulations, you have completed the tutorial! Go play the game now :)";
  }
}

EnumSet<TutorialHighlight> Tutorial::getHighlights(const WGame game) const {
  if (canContinue(game))
    return {};
  switch (state) {
    case State::DIG_ROOM:
    case State::CUT_TREES:
      return {TutorialHighlight::DIG_OR_CUT_TREES};
    case State::DIG_2_ROOMS:
      return {TutorialHighlight::DIG_OR_CUT_TREES, TutorialHighlight::REMOVE_CONSTRUCTION};
    case State::BUILD_STORAGE:
      return {TutorialHighlight::RESOURCE_STORAGE};
    case State::GET_200_WOOD:
      return {TutorialHighlight::WOOD_RESOURCE};
    case State::ACCEPT_IMMIGRANT:
      return {TutorialHighlight::ACCEPT_IMMIGRANT, TutorialHighlight::BUILD_BED, TutorialHighlight::TRAINING_ROOM};
    case State::BUILD_LIBRARY:
      return {TutorialHighlight::BUILD_LIBRARY};
    case State::BUILD_DOOR:
      return {TutorialHighlight::BUILD_DOOR};
    case State::TORCHES:
      return {TutorialHighlight::BUILD_TORCH};
    case State::FLOORS:
      return {TutorialHighlight::BUILD_FLOOR};
    default:
      return {};
  }
}

static void clearDugOutSquares(const WGame game, vector<Vec2>& highlights) {
  for (auto elem : Iter(highlights)) {
    if (auto furniture = Position(*elem, game->getPlayerCollective()->getLevel())
        .getFurniture(FurnitureLayer::MIDDLE))
      if (furniture->canDestroy(DestroyAction::Type::DIG))
        continue;
    elem.markToErase();
  }
}

vector<Vec2> Tutorial::getHighlightedSquaresHigh(const WGame game) const {
  auto collective = game->getPlayerCollective();
  const Vec2 entry(122, 84);
  const int corridor = 6;
  int roomWidth = 5;
  const Vec2 firstRoom(entry - Vec2(0, corridor + roomWidth / 2));
  switch (state) {
    case State::DIG_ROOM: {
      vector<Vec2> ret;
      for (int i : Range(0, corridor))
        ret.push_back(entry - Vec2(0, 1) * i);
      for (Vec2 v : Rectangle::centered(firstRoom, roomWidth / 2))
        ret.push_back(v);
      clearDugOutSquares(game, ret);
      return ret;
    }
    case State::DIG_2_ROOMS: {
      vector<Vec2> ret {firstRoom + Vec2(roomWidth / 2 + 1, 0), firstRoom - Vec2(0, roomWidth / 2 + 1)};
      for (Vec2 v : Rectangle::centered(firstRoom + Vec2(roomWidth + 1, 0), 2))
        ret.push_back(v);
      for (Vec2 v : Rectangle::centered(firstRoom - Vec2(0, roomWidth + 1), 2))
        ret.push_back(v);
      clearDugOutSquares(game, ret);
      return ret;
    }
    default:
      return {};
  }
}

vector<Vec2> Tutorial::getHighlightedSquaresLow(const WGame game) const {
  auto collective = game->getPlayerCollective();
  switch (state) {
    case State::FLOORS: {
      vector<Vec2> ret;
      for (auto furniture : {FurnitureType::BOOK_SHELF, FurnitureType::TRAINING_WOOD})
        for (auto pos : collective->getConstructions().getBuiltPositions(furniture))
          for (auto floorPos : concat({pos}, pos.neighbors8()))
            if (collective->canAddFurniture(floorPos, FurnitureType::FLOOR_WOOD1) &&
                !contains(ret, floorPos.getCoord()))
              ret.push_back(floorPos.getCoord());
      return ret;
    }
    default:
      return {};
  }
}

Tutorial::Tutorial() : state(State::WELCOME) {

}

void Tutorial::refreshInfo(const WGame game, optional<TutorialInfo>& info) const {
  info = TutorialInfo {
      getMessage(),
      canContinue(game),
      (int) state > 0,
      getHighlights(game),
      getHighlightedSquaresHigh(game),
      getHighlightedSquaresLow(game)
  };
}

void Tutorial::continueTutorial(const WGame game) {
  if (canContinue(game))
    state = (State)((int) state + 1);
}

void Tutorial::goBack() {
  if ((int) state > 0)
    state = (State)((int) state - 1);
}

bool Tutorial::showImmigrant() const {
  return state == State::ACCEPT_IMMIGRANT;
}

void Tutorial::createTutorial(Game& game) {
  auto tutorial = make_shared<Tutorial>();
  game.getPlayerControl()->setTutorial(tutorial);
  game.getPlayerCollective()->init(CollectiveConfig::keeper(0, 10, {}, {
      ImmigrantInfo(CreatureId::IMP, {MinionTrait::WORKER, MinionTrait::NO_LIMIT, MinionTrait::NO_EQUIPMENT})
          .setSpawnLocation(NearLeader{})
          .setKeybinding(Keybinding::CREATE_IMP)
          .setSound(Sound(SoundId::CREATE_IMP).setPitch(2))
          .setNoAuto()
          .addRequirement(ExponentialCost{ CostInfo(CollectiveResourceId::MANA, 20), 5, 4 }),
      ImmigrantInfo(CreatureId::ORC, {MinionTrait::FIGHTER})
          .setLimit(1)
          .setTutorialHighlight(TutorialHighlight::ACCEPT_IMMIGRANT)
          .addRequirement(0.0, TutorialRequirement {tutorial})
          .addRequirement(0.1, AttractionInfo{1, FurnitureType::TRAINING_WOOD})
  }),
      Immigration(game.getPlayerCollective()));
}
