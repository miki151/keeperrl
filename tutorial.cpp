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

bool Tutorial::canContinue(const Game* game) const {
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

EnumSet<TutorialHighlight> Tutorial::getHighlights(const Game* game) const {
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

static void clearDugOutSquares(const Game* game, vector<Vec2>& highlights) {
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

vector<Vec2> Tutorial::getHighlightedSquaresHigh(const Game* game) const {
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

vector<Vec2> Tutorial::getHighlightedSquaresLow(const Game* game) const {
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

optional<string> Tutorial::getWarning(const Game* game) const {
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

void Tutorial::refreshInfo(const Game* game, optional<TutorialInfo>& info) const {
  info = TutorialInfo {
      state,
      getWarning(game),
      canContinue(game),
      (int) state > 0,
      getHighlights(game),
      getHighlightedSquaresHigh(game),
      getHighlightedSquaresLow(game)
  };
}

void Tutorial::onNewState(const Game* game) {
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

void Tutorial::continueTutorial(const Game* game) {
  if (canContinue(game))
    state = (State)((int) state + 1);
  onNewState(game);
  if (state == TutorialState::FINISHED)
    game->achieve(AchievementId("tutorial_done"));
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
  collective->init(CollectiveConfig::keeper(50_visible, 10, "population", true, ConquerCondition::KILL_LEADER, true));
  auto immigrants = factory->immigrantsData.at("tutorial");
  CollectiveConfig::addBedRequirementToImmigrants(immigrants, game.getContentFactory());
  collective->setImmigration(makeOwner<Immigration>(collective, std::move(immigrants)));
}
