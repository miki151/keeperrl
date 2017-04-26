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
#include "container_range.h"
#include "technology.h"
#include "workshops.h"
#include "item_index.h"
#include "minion_task.h"
#include "creature.h"
#include "equipment.h"

SERIALIZE_DEF(Tutorial, state)

enum class Tutorial::State {
  WELCOME,
  INTRO,
  INTRO2,
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
  RESEARCH_CRAFTING,
  BUILD_WORKSHOP,
  SCHEDULE_WORKSHOP_ITEMS,
  ORDER_CRAFTING,
  EQUIP_WEAPON,
  ACCEPT_MORE_IMMIGRANTS,
  FINISHED,
};


bool Tutorial::canContinue(WConstGame game) const {
  auto collective = game->getPlayerCollective();
  switch (state) {
    case State::WELCOME:
    case State::INTRO:
    case State::INTRO2:
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
    case State::RESEARCH_CRAFTING:
      return collective->hasTech(TechId::CRAFTING);
    case State::BUILD_WORKSHOP:
      return collective->getConstructions().getBuiltCount(FurnitureType::WORKSHOP) >= 2 &&
          collective->getZones().getPositions(ZoneId::STORAGE_EQUIPMENT).size() >= 1;
    case State::SCHEDULE_WORKSHOP_ITEMS: {
      int numWeapons = collective->getNumItems(ItemIndex::WEAPON);
      for (auto& item : collective->getWorkshops().get(WorkshopType::WORKSHOP).getQueued())
        if (item.type.getId() == ItemId::CLUB)
          ++numWeapons;
      return numWeapons >= 1;
    }
    case State::ORDER_CRAFTING:
      return collective->getNumItems(ItemIndex::WEAPON) >= 1;
    case State::EQUIP_WEAPON:
      for (auto c : collective->getCreatures(MinionTrait::FIGHTER))
        if (c != collective->getLeader() && !c->getEquipment().getSlotItems(EquipmentSlot::WEAPON).empty())
          return true;
      return false;
    case State::ACCEPT_MORE_IMMIGRANTS:
      if (collective->getCreatures(MinionTrait::FIGHTER).size() < 4)
        return false;
      for (auto c : collective->getCreatures(MinionTrait::FIGHTER))
        if (!collective->hasTrait(c, MinionTrait::NO_EQUIPMENT) &&
            c->getEquipment().getSlotItems(EquipmentSlot::WEAPON).empty())
          return false;
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
    case State::INTRO:
      return "Let's check out some things that you see on the map. The little wizard in the red robe is you. "
          "If he dies, then it's game over, so be careful!\n \n"
          "Remember that KeeperRL features perma-death, which means that you can't reload the game after a failure.";
    case State::INTRO2:
      return "The four little creatures are your imps and they are here to perform your orders. Try hovering the mouse "
          "over other things on the map, and notice the hints in the lower right corner.";
    case State::CUT_TREES:
      return "Great things come from small beginnings. Your first task is to gather some wood! "
          "Select the \"Dig or cut tree\" order and click on "
          "a few trees to order your imps to cut them down.\n \nWhen you're done, click the right mouse button "
          "or press escape to clear the order.";
    case State::BUILD_STORAGE:
      return "You need to store the wood somewhere before you can use it. Select resource storage and designate it "
          "by clicking on the map. If you want to remove it, just click again.\n \n"
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
      return "Minions are more efficient if there is a nice floor wherever they are working. For now you can only "
          "afford wooden floor, but it should do.\n \n"
          "Make sure you have enough wood!";
    case State::RESEARCH_CRAFTING:
      return "Your minions will need equipment, such as weapons, armor, and consumables, to be more efficient in "
          "combat.\n \n"
          "Before you can produce anything, click on your library, bring up the research menu and unlock crafting.";
    case State::BUILD_WORKSHOP:
      return "Build at least 2 workshop stands in your dungeon. It's best to dig out a dedicated room for them. "
          "You will also need a storage area for equipment. Place it somewhere near your workshop.";
    case State::SCHEDULE_WORKSHOP_ITEMS:
      return "Weapons are the most important piece of equipment, because unarmed, your minions have little chance "
          "against an enemy. Open the workshop menu by clicking on any of your workshop stands and schedule the "
          "production of a club.";
    case State::ORDER_CRAFTING:
      return "To have your item produced, order your orc to pick up crafting. "
          "Click and drag him onto a workshop stand. "
          "Pausing the game will make it a bit easier. Also, make sure you have enough wood!\n \n"
          "You can check the progress of production when you click on the workshop.";
    case State::EQUIP_WEAPON:
      return "Your minions will automatically pick up weapons and other equipment that's in storage, "
          "but you can also control it manually. Click on your orc and on the weapon slot to assign him the club that "
          "he has just produced. He will go and pick it up.\n \n";
    case State::ACCEPT_MORE_IMMIGRANTS:
      return "You are ready to grow your military force. Accept 3 more orc immigrants, and equip them with "
          "weapons. Equip your Keeper as well!\n \n"
          "You can also accept goblins, which don't fight, but are excellent craftsmen.";
    case State::FINISHED:
      return "Congratulations, you have completed the tutorial! Go play the game now :)";
  }
}

EnumSet<TutorialHighlight> Tutorial::getHighlights(WConstGame game) const {
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
    case State::RESEARCH_CRAFTING:
      return {TutorialHighlight::RESEARCH_CRAFTING};
    case State::BUILD_WORKSHOP:
      return {TutorialHighlight::EQUIPMENT_STORAGE, TutorialHighlight::BUILD_WORKSHOP};
    case State::SCHEDULE_WORKSHOP_ITEMS:
      return {TutorialHighlight::SCHEDULE_CLUB};
    case State::EQUIP_WEAPON:
      return {TutorialHighlight::EQUIPMENT_SLOT_WEAPON};
    default:
      return {};
  }
}

bool Tutorial::blockAutoEquipment() const {
  return state <= State::EQUIP_WEAPON;
}

static void clearDugOutSquares(WConstGame game, vector<Vec2>& highlights) {
  for (auto elem : Iter(highlights)) {
    if (auto furniture = Position(*elem, game->getPlayerCollective()->getLevel())
        .getFurniture(FurnitureLayer::MIDDLE))
      if (furniture->canDestroy(DestroyAction::Type::DIG))
        continue;
    elem.markToErase();
  }
}

vector<Vec2> Tutorial::getHighlightedSquaresHigh(WConstGame game) const {
  auto collective = game->getPlayerCollective();
  const Vec2 entry(123, 112);
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

vector<Vec2> Tutorial::getHighlightedSquaresLow(WConstGame game) const {
  auto collective = game->getPlayerCollective();
  switch (state) {
    case State::FLOORS: {
      vector<Vec2> ret;
      for (auto furniture : {FurnitureType::BOOK_SHELF, FurnitureType::TRAINING_WOOD})
        for (auto pos : collective->getConstructions().getBuiltPositions(furniture))
          for (auto floorPos : concat({pos}, pos.neighbors8()))
            if (floorPos.canConstruct(FurnitureType::FLOOR_WOOD1) && !contains(ret, floorPos.getCoord()))
              ret.push_back(floorPos.getCoord());
      return ret;
    }
    case State::RESEARCH_CRAFTING:
      return transform2(collective->getConstructions().getBuiltPositions(FurnitureType::BOOK_SHELF),
          [](const Position& pos) { return pos.getCoord(); });
    case State::SCHEDULE_WORKSHOP_ITEMS:
      return transform2(collective->getConstructions().getBuiltPositions(FurnitureType::WORKSHOP),
          [](const Position& pos) { return pos.getCoord(); });
    default:
      return {};
  }
}

Tutorial::Tutorial() : state(State::WELCOME) {

}

void Tutorial::refreshInfo(WConstGame game, optional<TutorialInfo>& info) const {
  info = TutorialInfo {
      getMessage(),
      canContinue(game),
      (int) state > 0,
      getHighlights(game),
      getHighlightedSquaresHigh(game),
      getHighlightedSquaresLow(game)
  };
}

void Tutorial::onNewState(WConstGame game) {
  auto collective = game->getPlayerCollective();
  switch (state) {
    case State::ACCEPT_MORE_IMMIGRANTS:
      for (auto c : collective->getCreatures())
        collective->removeTrait(c, MinionTrait::NO_AUTO_EQUIPMENT);
      break;
    default:
      break;
  }
}

void Tutorial::continueTutorial(WConstGame game) {
  if (canContinue(game))
    state = (State)((int) state + 1);
  onNewState(game);
}

void Tutorial::goBack() {
  if ((int) state > 0)
    state = (State)((int) state - 1);
}

bool Tutorial::showImmigrant(const ImmigrantInfo& info) const {
  return info.getId(0) == CreatureId::IMP ||
      (state == State::ACCEPT_IMMIGRANT && info.isPersistent() && info.getLimit() == 1) ||
      (state >= State::ACCEPT_MORE_IMMIGRANTS && !info.isPersistent());
}

void Tutorial::createTutorial(Game& game) {
  auto tutorial = make_shared<Tutorial>();
  game.getPlayerControl()->setTutorial(tutorial);
  auto collective = game.getPlayerCollective();
  collective->setTrait(collective->getLeader(), MinionTrait::NO_AUTO_EQUIPMENT);
  collective->init(CollectiveConfig::keeper(50, 10, {}, {
      ImmigrantInfo(CreatureId::IMP, {MinionTrait::WORKER, MinionTrait::NO_LIMIT, MinionTrait::NO_EQUIPMENT})
          .setSpawnLocation(NearLeader{})
          .setKeybinding(Keybinding::CREATE_IMP)
          .setSound(Sound(SoundId::CREATE_IMP).setPitch(2))
          .setNoAuto()
          .addRequirement(ExponentialCost{ CostInfo(CollectiveResourceId::MANA, 20), 5, 4 }),
      ImmigrantInfo(CreatureId::ORC, {MinionTrait::FIGHTER, MinionTrait::NO_AUTO_EQUIPMENT})
          .setLimit(1)
          .setTutorialHighlight(TutorialHighlight::ACCEPT_IMMIGRANT)
          .addRequirement(0.0, TutorialRequirement {tutorial})
          .addRequirement(0.1, AttractionInfo{1, FurnitureType::TRAINING_WOOD})
          .setHiddenInHelp(),
      ImmigrantInfo(CreatureId::ORC, {MinionTrait::FIGHTER})
          .setLimit(3)
          .setFrequency(0.5)
          .addRequirement(0.0, TutorialRequirement {tutorial})
          .addRequirement(0.1, AttractionInfo{1, FurnitureType::TRAINING_WOOD}),
      ImmigrantInfo(CreatureId::GOBLIN, {MinionTrait::FIGHTER, MinionTrait::NO_EQUIPMENT})
          .setLimit(1)
          .setFrequency(0.5)
          .addRequirement(0.0, TutorialRequirement {tutorial})
          .addRequirement(0.1, AttractionInfo{1, FurnitureType::TRAINING_WOOD})
  }),
      Immigration(collective));
}
