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
#include "keybinding.h"
#include "immigration.h"
#include "container_range.h"
#include "technology.h"
#include "workshops.h"
#include "item_index.h"
#include "minion_activity.h"
#include "creature.h"
#include "body.h"
#include "equipment.h"
#include "collective_teams.h"
#include "collective_warning.h"
#include "creature_factory.h"
#include "workshop_item.h"
#include "tutorial_state.h"
#include "content_factory.h"
#include "immigrant_info.h"
#include "item_types.h"

SERIALIZE_DEF(Tutorial, state, entrance)

static bool isTeam(const Collective* collective) {
  for (auto team : collective->getTeams().getAll())
    if (collective->getTeams().getMembers(team).size() >= 4)
      return true;
  return false;
}

bool Tutorial::canContinue(WConstGame game) const {
  auto collective = game->getPlayerCollective();
  Collective* villain = nullptr;
  for (auto c : game->getCollectives())
    if (c != collective && c->getTerritory().getAll()[0].getLevel() == c->getModel()->getGroundLevel()) {
      CHECK(!villain) << "Only one villain allowed in tutorial.";
      villain = c;
    }
  CHECK(villain) << "Villain not found in tutorial.";
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
    case State::CONTROLS2:
      return true;
    case State::GET_200_WOOD:
      return collective->numResource(CollectiveResourceId("WOOD")) >= 200;
    case State::DIG_ROOM:
      return getHighlightedSquaresHigh(game).empty();
    case State::BUILD_DOOR:
      return collective->getConstructions().getBuiltCount(FurnitureType("WOOD_DOOR"));
    case State::BUILD_LIBRARY:
      return collective->getConstructions().getBuiltCount(FurnitureType("BOOKCASE_WOOD")) >= 5;
    case State::DIG_2_ROOMS:
      return getHighlightedSquaresHigh(game).empty();
    case State::ACCEPT_IMMIGRANT:
      return collective->getCreatures(MinionTrait::FIGHTER).size() >= 1;
    case State::TORCHES:
      for (auto furniture : {FurnitureType("BOOKCASE_WOOD"), FurnitureType("TRAINING_WOOD")})
        for (auto pos : collective->getConstructions().getBuiltPositions(furniture))
          if (pos.getLightingEfficiency() < 0.99)
            return false;
      return true;
    case State::FLOORS:
      return getHighlightedSquaresLow(game).empty();
    case State::BUILD_WORKSHOP:
      return collective->getConstructions().getBuiltCount(FurnitureType("WORKSHOP")) >= 2 &&
          collective->getZones().getPositions(ZoneId::STORAGE_EQUIPMENT).size() >= 1;
    case State::SCHEDULE_WORKSHOP_ITEMS: {
      int numWeapons = collective->getNumItems(ItemIndex::WEAPON);
      for (auto& item : collective->getWorkshops().types.at(WorkshopType("WORKSHOP")).getQueued())
        if (item.item.type.type->getValueMaybe<CustomItemId>() == CustomItemId("Club"))
          ++numWeapons;
      return numWeapons >= 2; // the keeper already has one weapon
    }
    case State::ORDER_CRAFTING:
      return collective->getNumItems(ItemIndex::WEAPON) >= 2;
    case State::EQUIP_WEAPON:
      for (auto c : collective->getCreatures(MinionTrait::FIGHTER))
        if (!collective->hasTrait(c, MinionTrait::LEADER) && !c->getEquipment().getSlotItems(EquipmentSlot::WEAPON).empty())
          return true;
      return false;
    case State::ACCEPT_MORE_IMMIGRANTS:
      return collective->getCreatures(MinionTrait::FIGHTER).size() >= 4;
    case State::EQUIP_ALL_FIGHTERS:
      for (auto c : collective->getCreatures(MinionTrait::FIGHTER))
        if (!collective->hasTrait(c, MinionTrait::NO_EQUIPMENT) &&
            c->getEquipment().getSlotItems(EquipmentSlot::WEAPON).empty())
          return false;
      return true;
    case State::CREATE_TEAM:
      return isTeam(collective);
    case State::CONTROL_TEAM:
      return isTeam(collective) && !game->getPlayerControl()->getControlled().empty();
    case State::CONTROL_MODE_MOVEMENT:
      return true;
    case State::FULL_CONTROL:
      return true;
    case State::DISCOVER_VILLAGE:
      return collective->isKnownVillain(villain);
    case State::KILL_VILLAGE:
      return villain->isConquered();
    case State::LOOT_VILLAGE:
      for (auto pos : villain->getTerritory().getAll())
        if (!pos.getItems(CollectiveResourceId("GOLD")).empty())
          return false;
      return true;
    case State::LEAVE_CONTROL:
      return game->getPlayerControl()->getControlled().empty();
    case State::HELP_TAB:
    case State::MINIMAP_BUTTONS:
    case State::SUMMARY1:
    case State::SUMMARY2:
      return true;
    case State::RESEARCH:
      return collective->getDungeonLevel().numResearchAvailable() == 0;
    case State::FINISHED:
      return false;
  }
}

string Tutorial::getMessage() const {
  switch (state) {
    case State::WELCOME:
      return "Welcome to the KeeperRL tutorial!\n \n"
          "Together we will work to build a small dungeon, and assemble a military force. Nearby us lies "
          "a small village inhabited mostly by innocents, which we shall conquer.\n \n"
          "This should get you up to speed with the game!";
    case State::INTRO:
      return "Let's check out some things that you see on the map. The little wizard wearing a red robe is you, "
          "the Keeper.\n \n"
          "Remember that KeeperRL features perma-death, which means that you can't reload the game after a failure. "
          "If your Keeper dies, then the game is over, so be careful!";
    case State::INTRO2:
      return "The four little creatures are your imps. They are here to perform your orders. Try hovering the mouse "
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
    case State::CONTROLS2:
      return "Try zooming in and out of the map using the 'z' key or mouse wheel.";
    case State::GET_200_WOOD:
      return "Cut some more trees until you have gathered at least 200 wood.\n \n";
          //"You can mark things in bulk by dragging the mouse or by holding SHIFT and selecting a rectangle. Try it!";
    case State::DIG_ROOM:
      return "Time to strike the earth! Dig out a tunnel and a room, as shown by the yellow highlight.\n \n"
          "Hold down shift to select a rectangular area.";
    case State::BUILD_DOOR:
      return "Build a door at the entrance to your dungeon. This will slow down potential intruders, "
          "as they will need to destroy it before they can enter. "
          "Your minions can pass through doors freely, unless you lock a door by left clicking on it.\n \n"
          "Try locking and unlocking your new door.";
    case State::BUILD_LIBRARY:
      return "The first room that you need to build is a library. This is where the Keeper and other minions "
          "will learn spells, and train their spell damage attribute. Place 6 bookcases "
          "in the new room as highlighted. Remember that bookcases and other furniture block your minions' movement.";
    case State::DIG_2_ROOMS:
      return "Dig out some more rooms. "
          "If the library is blocking your tunnels, use the 'remove construction' order to get it out of the way.\n \n"
          "Change the speed of the game using the keys 1-4 or by clicking in the "
          "lower left corner if it's taking too long.";
    case State::ACCEPT_IMMIGRANT:
      return "Your dungeon has attracted a goblin warrior. "
          "Before your new minion joins you, you must fulfill a few requirements. "
          "Hover your mouse over the immigrant icon to see them. "
          "Once you are ready, accept the immigrant with a left click. Click on the '?' icon immediately to the left "
          "to learn more about immigration.";
    case State::TORCHES:
      return "We need to make sure that your minions have good working conditions when studying and training. Place some "
          "torches in your rooms to light them up. Hover your mouse over the book shelves and training dummies "
          "and look in the lower right corner to check if they have enough light.";
    case State::FLOORS:
      return "Minions are also more efficient if there is a nice floor where they are working. For now you can only "
          "afford wooden floors, but they should do.\n \n"
          "Make sure you have enough wood!";
    case State::BUILD_WORKSHOP:
      return "Your minions will need equipment, such as weapons, armor, and consumables, to be more deadly in "
          "combat.\n \n"
          "Build at least 2 workshop stands in your dungeon. It's best to dig out a dedicated room for them. "
          "You will also need a storage area for equipment. Place it somewhere near your workshop.";
    case State::SCHEDULE_WORKSHOP_ITEMS:
      return "Weapons are the most important piece of equipment, because unarmed, your minions have little chance "
          "against an enemy. Open the workshop menu by clicking on any of your workshop stands and schedule the "
          "production of a club.";
    case State::ORDER_CRAFTING:
      return "To have your item produced, order your goblin to pick up crafting. "
          "Click and drag him onto a workshop stand. "
          "Pausing the game will make it a bit easier. Also, make sure you have enough wood!\n \n"
          "You can check the progress of production when you click on the workshop.";
    case State::EQUIP_WEAPON:
      return "Your minions will automatically pick up weapons and other equipment from the storage, "
          "but you can also control it manually. Click on your goblin, and on his weapon slot to assign him the "
          "club that he has just produced.\n \n"
          "This way you will order him to go and pick it up.\n \n";
    case State::ACCEPT_MORE_IMMIGRANTS:
      return "You are ready to grow your military force. Three more goblin warrior immigrants should do.\n \n"
          "You can also invite goblin artificers, which don't fight, but are excellent craftsmen.";
    case State::EQUIP_ALL_FIGHTERS:
      return "Craft clubs for all of your warriors, and have them equipped. They will be needed soon.";
    case State::CREATE_TEAM:
      return "Your tiny army is ready! Assemble a team by dragging your warriors onto the [new team] button. You can "
          "drag them straight from the map or from the minion menu.\n \n"
          "Create a team of 4 goblin warriors.";
    case State::CONTROL_TEAM:
      return "Time to take control over your team. Select the team, and one of the team members as the leader, "
          "and click [Control].";
    case State::CONTROL_MODE_MOVEMENT:
      return "You are now in control of a minion, and the game has become turn-based. Try moving around using "
          "the arrow keys or by left-clicking on the map. You can scroll the map by dragging it with the right "
          "mouse button.\n \n"
          "Notice the rest of your team following you.";
    case State::FULL_CONTROL:
      return "You can take control over all team members in a tactical situation. To do this click on the [Control mode] "
          "button in the upper left corner or press [G]. Clicking again will go back to controlling only the team "
          "leader.";
    case State::DISCOVER_VILLAGE:
      return "It's time to discover the whereabouts of the nearby human village. Click on the minimap in the upper "
          "right corner. The approximate location of the village is marked by a '?'. Take your team there.";
    case State::KILL_VILLAGE:
      return "Your team has arrived at the village. The only right thing to do in this situation is to find and "
          "exterminate all inhabitants!\n \n"
          "Remember to break into every house by destroying the door.";
    case State::LOOT_VILLAGE:
      return "There is a nice pile of treasure in one of the houses. Pick it all up by entering the tiles containing "
          "the loot, and clicking in the menu in the upper left corner.";
    case State::LEAVE_CONTROL:
      return "To relinquish control of your team, click the [Exit control mode] button in the upper left corner.";
    case State::SUMMARY1:
      return "You are back in the real-time mode. Your minions will now return to base and resume their normal routine. "
          "Once they are back, they will drop all the loot for the imps to take care of.";
    case State::RESEARCH:
      return "You have increased your malevolence level. This is the main meter of your progress in the game and allows "
          "you to research new technologies.\n \n"
          "Click on the technology tab and research something.";
    case State::HELP_TAB:
      return "KeeperRL features how-to pages for some more advanced features. Click on the help tab icon to see them. "
          "They're likely to be useful in your future playthroughs.";
    case State::MINIMAP_BUTTONS:
      return "As the last objective, familiarize yourself with the two buttons under the minimap in the top-right corner. "
          "The first one opens the world map window, which you can use to travel to other sites when in control mode.\n \n"
          "The second one centers the map on your Keeper.";
    case State::SUMMARY2:
      return "Thank you for completing the tutorial! We hope that we have made it a bit easier for you to get into "
          "KeeperRL. We would love to hear your comments, so please drop by on the forums on Steam or at keeperrl.com "
          "some time!";
    case State::FINISHED:
      return "You should go and start a new game now, as this one exists only for the purpose of the tutorial.\n \n"
          "Press Escape and abandon this game. ";
  }
}

EnumSet<TutorialHighlight> Tutorial::getHighlights(WConstGame game) const {
  /*if (canContinue(game))
    return {};*/
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
    case State::BUILD_WORKSHOP:
      return {TutorialHighlight::EQUIPMENT_STORAGE, TutorialHighlight::BUILD_WORKSHOP};
    case State::SCHEDULE_WORKSHOP_ITEMS:
      return {TutorialHighlight::SCHEDULE_CLUB};
    case State::EQUIP_WEAPON:
      return {TutorialHighlight::EQUIPMENT_SLOT_WEAPON};
    case State::CREATE_TEAM:
      return {TutorialHighlight::NEW_TEAM};
    case State::FULL_CONTROL:
      return {TutorialHighlight::FULL_CONTROL};
    case State::CONTROL_TEAM:
      return {TutorialHighlight::CONTROL_TEAM};
    case State::LEAVE_CONTROL:
      return {TutorialHighlight::LEAVE_CONTROL};
    case State::MINIMAP_BUTTONS:
      return {TutorialHighlight::MINIMAP_BUTTONS};
    case State::RESEARCH:
      return {TutorialHighlight::RESEARCH};
    case State::HELP_TAB:
      return {TutorialHighlight::HELP_TAB};
    default:
      return {};
  }
}

bool Tutorial::blockAutoEquipment() const {
  return state <= State::EQUIP_WEAPON;
}

static void clearDugOutSquares(WConstGame game, vector<Vec2>& highlights) {
  for (auto elem : Iter(highlights)) {
    if (auto furniture = Position(*elem, game->getPlayerCollective()->getModel()->getGroundLevel())
        .getFurniture(FurnitureLayer::MIDDLE))
      if (furniture->canDestroy(DestroyAction::Type::DIG))
        continue;
    elem.markToErase();
  }
}

static const int corridorLength = 6;
static const int roomWidth = 5;

vector<Vec2> Tutorial::getHighlightedSquaresHigh(WConstGame game) const {
  auto collective = game->getPlayerCollective();
  const Vec2 firstRoom(entrance - Vec2(0, corridorLength + roomWidth / 2));
  switch (state) {
    case State::DIG_ROOM: {
      vector<Vec2> ret;
      for (int i : Range(0, corridorLength))
        ret.push_back(entrance - Vec2(0, 1) * i);
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
    case State::BUILD_LIBRARY: {
      const Vec2 roomCenter(entrance - Vec2(0, corridorLength + roomWidth / 2));
      vector<Vec2> ret;
      for (Vec2 v : roomCenter.neighbors8())
        if (v.y != roomCenter.y && !collective->getConstructions().containsFurniture(
              Position(v, collective->getModel()->getGroundLevel()), FurnitureLayer::MIDDLE))
          ret.push_back(v);
      return ret;
    }
    case State::FLOORS: {
      vector<Vec2> ret;
      for (auto furniture : {FurnitureType("BOOKCASE_WOOD"), FurnitureType("TRAINING_WOOD")})
        for (auto pos : collective->getConstructions().getBuiltPositions(furniture))
          for (auto floorPos : concat({pos}, pos.neighbors8()))
            if (floorPos.canConstruct(FurnitureType("FLOOR_WOOD1")) && !ret.contains(floorPos.getCoord()))
              ret.push_back(floorPos.getCoord());
      return ret;
    }
    case State::SCHEDULE_WORKSHOP_ITEMS:
      return collective->getConstructions().getBuiltPositions(FurnitureType("WORKSHOP")).transform(
          [](const Position& pos) { return pos.getCoord(); });
    default:
      return {};
  }
}

Tutorial::Tutorial() : state(State::WELCOME) {

}

optional<string> Tutorial::getWarning(WConstGame game) const {
  switch (state) {
    case State::CONTROL_TEAM:
    case State::CONTROL_MODE_MOVEMENT:
    case State::DISCOVER_VILLAGE:
    case State::KILL_VILLAGE:
    case State::LOOT_VILLAGE:
    case State::LEAVE_CONTROL:
      return none;
    default:
      /*if (!game->getPlayerCreatures().empty())
        return "Press [U] to leave control mode."_s;
      else*/
        return none;
  }
}

void Tutorial::refreshInfo(WConstGame game, optional<TutorialInfo>& info) const {
  info = TutorialInfo {
      getMessage(),
      getWarning(game),
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

Tutorial::State Tutorial::getState() const {
  return state;
}

void Tutorial::createTutorial(Game& game, const ContentFactory* factory) {
  auto tutorial = make_shared<Tutorial>();
  game.getPlayerControl()->setTutorial(tutorial);
  auto collective = game.getPlayerCollective();
  bool foundEntrance = false;
  for (auto pos : collective->getModel()->getGroundLevel()->getAllPositions())
    if (auto f = pos.getFurniture(FurnitureLayer::CEILING))
      if (f->getType() == FurnitureType("TUTORIAL_ENTRANCE")) {
        tutorial->entrance = pos.getCoord() - Vec2(0, 1);
        CHECK(!foundEntrance);
        foundEntrance = true;
      }
  CHECK(foundEntrance);
  for (auto l : collective->getLeaders())
    collective->setTrait(l, MinionTrait::NO_AUTO_EQUIPMENT);
  collective->getWarnings().disable();
  collective->init(CollectiveConfig::keeper(50_visible, 10, "population", true, ConquerCondition::KILL_LEADER));
  auto immigrants = factory->immigrantsData.at("tutorial");
  CollectiveConfig::addBedRequirementToImmigrants(immigrants, game.getContentFactory());
  collective->setImmigration(makeOwner<Immigration>(collective, std::move(immigrants)));
}
