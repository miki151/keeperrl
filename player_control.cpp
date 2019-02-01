/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "stdafx.h"

#include "player_control.h"
#include "level.h"
#include "task.h"
#include "model.h"
#include "statistics.h"
#include "options.h"
#include "technology.h"
#include "village_control.h"
#include "item.h"
#include "item_factory.h"
#include "creature.h"
#include "square.h"
#include "view_id.h"
#include "collective.h"
#include "effect.h"
#include "music.h"
#include "encyclopedia.h"
#include "map_memory.h"
#include "item_action.h"
#include "equipment.h"
#include "collective_teams.h"
#include "minion_equipment.h"
#include "task_map.h"
#include "construction_map.h"
#include "minion_activity_map.h"
#include "spell.h"
#include "tribe.h"
#include "visibility_map.h"
#include "creature_name.h"
#include "view.h"
#include "view_index.h"
#include "collective_attack.h"
#include "territory.h"
#include "sound.h"
#include "game.h"
#include "collective_name.h"
#include "creature_attributes.h"
#include "collective_config.h"
#include "villain_type.h"
#include "workshops.h"
#include "attack_trigger.h"
#include "view_object.h"
#include "body.h"
#include "furniture.h"
#include "furniture_type.h"
#include "furniture_factory.h"
#include "known_tiles.h"
#include "zones.h"
#include "inventory.h"
#include "immigration.h"
#include "scroll_position.h"
#include "tutorial.h"
#include "tutorial_highlight.h"
#include "container_range.h"
#include "trap_type.h"
#include "collective_warning.h"
#include "furniture_usage.h"
#include "message_generator.h"
#include "message_buffer.h"
#include "minion_controller.h"
#include "build_info.h"
#include "vision.h"
#include "external_enemies.h"
#include "resource_info.h"
#include "workshop_item.h"
#include "time_queue.h"
#include "quarters.h"
#include "unknown_locations.h"
#include "furniture_click.h"
#include "game_config.h"
#include "campaign.h"
#include "game_event.h"
#include "view_object_action.h"

template <class Archive>
void PlayerControl::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(CollectiveControl) & SUBCLASS(EventListener);
  ar(memory, introText, lastControlKeeperQuestion, keeperCreatureInfo);
  ar(newAttacks, ransomAttacks, notifiedAttacks, messages, hints, visibleEnemies);
  ar(visibilityMap, unknownLocations, dismissedVillageInfos, buildInfo);
  ar(messageHistory, tutorial, controlModeMessages, stunnedCreatures);
}

SERIALIZABLE(PlayerControl)

SERIALIZATION_CONSTRUCTOR_IMPL(PlayerControl)

using ResourceId = Collective::ResourceId;

const LocalTime hintFrequency = LocalTime(700);
static vector<string> getHints() {
  return {
    "Morale affects minion productivity and chances of fleeing from battle.",
 //   "You can turn these hints off in the settings (F2).",
    "Killing a leader greatly lowers the morale of his tribe and stops immigration.",
//    "Your minions' morale is boosted when they are commanded by the Keeper.",
  };
}

PlayerControl::PlayerControl(Private, WCollective col, KeeperCreatureInfo keeperCreatureInfo)
      : CollectiveControl(col), hints(getHints()), keeperCreatureInfo(keeperCreatureInfo) {
  controlModeMessages = make_shared<MessageBuffer>();
  visibilityMap = make_shared<VisibilityMap>();
  unknownLocations = make_shared<UnknownLocations>();
  memory.reset(new MapMemory());
  for (auto pos : col->getModel()->getTopLevel()->getAllPositions())
    if (auto f = pos.getFurniture(FurnitureLayer::MIDDLE))
      if (f->isClearFogOfWar())
        addToMemory(pos);
}

PPlayerControl PlayerControl::create(WCollective col, vector<string> introText,
    KeeperCreatureInfo keeperInfo) {
  auto ret = makeOwner<PlayerControl>(Private{}, col, keeperInfo);
  ret->subscribeTo(col->getModel());
  ret->introText = introText;
  return ret;
}

PlayerControl::~PlayerControl() {
}

template <typename T>
static T readData(View* view, const GameConfig* gameConfig, GameConfigId id) {
  T data;
  while (1) {
    if (auto error = gameConfig->readObject(data, id))
      view->presentText("Error", *error);
    else
      break;
  }
  return data;
}

void PlayerControl::reloadBuildingMenu() {
  auto gameConfig = getGame()->getGameConfig();
  vector<BuildInfo> buildInfoTmp;
  auto allData = readData<vector<pair<string, vector<BuildInfo>>>>(getView(), gameConfig, GameConfigId::BUILD_MENU);
  set<string> allDataGroups;
  for (auto& group : allData) {
    allDataGroups.insert(group.first);
    if (keeperCreatureInfo.buildingGroups.contains(group.first))
      buildInfoTmp.append(group.second);
  }
  for (auto& group : keeperCreatureInfo.buildingGroups)
    if (!allDataGroups.count(group)) {
      getView()->presentText("Error", "Building menu group \"" + group + "\" not found");
      return;
    }
  bool hotkeys[128] = {0};
  for (auto& info : buildInfoTmp) {
    if (info.hotkey != '\0') {
      if (hotkeys[int(info.hotkey)]) {
        getView()->presentText("Error",
            "Hotkey \'" + string(1, info.hotkey) + "\' is used more than once in building menu");
        return;
      }
      hotkeys[int(info.hotkey)] = true;
    }
  }
  for (auto& info : buildInfoTmp)
    for (auto& requirement : info.requirements)
      if (auto tech = requirement.getReferenceMaybe<TechId>())
        if (!collective->getTechnology().techs.count(*tech)) {
          getView()->presentText("Error",
              "Technology prerequisite \"" + *tech + "\" of build item \"" + info.name + "\" is not available");
          return;
        }
  buildInfo = buildInfoTmp;
  for (auto& info : buildInfo)
    if (auto furniture = info.type.getReferenceMaybe<BuildInfo::Furniture>()) {
      for (auto type : furniture->types) {
        double luxury = Furniture::getLuxuryInfo(type).luxury;
        if (luxury > 0) {
          info.help += " Increases luxury by " + toString(luxury) + ".";
          break;
        }
      }
      if (auto increase = Furniture::getPopulationIncreaseDescription(furniture->types[0]))
        info.help += " " + *increase;
      for (auto expType : ENUM_ALL(ExperienceType))
        if (auto increase = CollectiveConfig::getTrainingMaxLevel(expType, furniture->types[0]))
          info.help += " Adds up to " + toString(*increase) + " " + toLower(getName(expType)) + " levels.";
    }
}

static optional<string> checkGroupCounts(const map<string, vector<ImmigrantInfo>>& immigrants) {
  for (auto& group : immigrants)
    for (auto& elem : group.second)
      if (elem.getGroupSize().isEmpty())
        return "Bad immigrant group size: " + toString(elem.getGroupSize()) +
            ". Lower bound is inclusive, upper bound exclusive.";
  return none;
}

optional<string> PlayerControl::reloadImmigrationAndWorkshops(const GameConfig* gameConfig,
    const CreatureFactory* creatureFactory) {
  Technology technology;
  if (auto error = gameConfig->readObject(technology, GameConfigId::TECHNOLOGY))
    return error;
  for (auto& tech : copyOf(technology.techs))
    if (!keeperCreatureInfo.technology.contains(tech.first))
      technology.techs.erase(tech.first);
  for (auto& tech : technology.techs)
    for (auto& preq : tech.second.prerequisites)
      if (!technology.techs.count(preq))
        return "Technology prerequisite \"" + preq + "\" of \"" + tech.first + "\" is not available";
  using WorkshopArray = std::array<vector<WorkshopItemCfg>, EnumInfo<WorkshopType>::size>;
  vector<pair<string, WorkshopArray>> workshopGroups;
  if (auto error = gameConfig->readObject(workshopGroups, GameConfigId::WORKSHOPS_MENU))
    return error;
  WorkshopArray merged;
  set<string> allWorkshopGroups;
  for (auto& group : workshopGroups) {
    allWorkshopGroups.insert(group.first);
    if (keeperCreatureInfo.workshopGroups.contains(group.first))
      for (int i : Range(EnumInfo<WorkshopType>::size))
        merged[i].append(group.second[i]);
  }
  for (auto& group : keeperCreatureInfo.workshopGroups)
    if (!allWorkshopGroups.count(group))
      return "Workshop menu group \"" + group + "\" not found";
  for (auto& elem : merged)
    for (auto& item : elem)
      if (item.tech && !technology.techs.count(*item.tech))
        return "Technology prerequisite \"" + *item.tech + "\" of workshop item \"" + item.item.get()->getName()
            + "\" is not available";
  collective->setWorkshops(unique<Workshops>(std::move(merged)));
  map<string, vector<ImmigrantInfo>> immigrantsData;
  if (auto error = gameConfig->readObject(immigrantsData, GameConfigId::IMMIGRATION))
    return error;
  if (auto error = checkGroupCounts(immigrantsData))
    return error;
  vector<ImmigrantInfo> immigrants;
  for (auto elem : keeperCreatureInfo.immigrantGroups)
    if (auto group = getReferenceMaybe(immigrantsData, elem))
      append(immigrants, *group);
    else
      return "Undefined immigrant group: \"" + elem + "\"";
  CollectiveConfig::addBedRequirementToImmigrants(immigrants, creatureFactory);
  collective->setImmigration(makeOwner<Immigration>(collective, std::move(immigrants)));
  collective->setTechnology(std::move(technology));
  return none;
}

void PlayerControl::reloadData() {
  auto gameConfig = getGame()->getGameConfig();
  reloadBuildingMenu();
  while (1) {
    if (auto error = reloadImmigrationAndWorkshops(gameConfig, getGame()->getCreatureFactory()))
      getView()->presentText("Error", *error);
    else
      break;
  }
}

const vector<Creature*>& PlayerControl::getControlled() const {
  return getGame()->getPlayerCreatures();
}

optional<TeamId> PlayerControl::getCurrentTeam() const {
  // try returning a non-persistent team first
  for (TeamId team : getTeams().getAllActive())
    if (!getTeams().isPersistent(team) && getTeams().getLeader(team)->isPlayer())
      return team;
  for (TeamId team : getTeams().getAllActive())
    if (getTeams().getLeader(team)->isPlayer())
      return team;
  return none;
}

void PlayerControl::onControlledKilled(const Creature* victim) {
  TeamId currentTeam = *getCurrentTeam();
  if (getTeams().getLeader(currentTeam) == victim && getGame()->getPlayerCreatures().size() == 1) {
    vector<CreatureInfo> team;
    for (auto c : getTeams().getMembers(currentTeam))
      if (c != victim)
        team.push_back(CreatureInfo(c));
    if (team.empty())
      return;
    optional<Creature::Id> newLeader;
    if (team.size() == 1)
      newLeader = team[0].uniqueId;
    else
      newLeader = getView()->chooseCreature("Choose new team leader:", team, "Order team back to base");
    if (newLeader) {
      if (Creature* c = getCreature(*newLeader)) {
        getTeams().setLeader(currentTeam, c);
        if (!c->isPlayer())
          c->pushController(createMinionController(c));
        return;
      }
    }
    leaveControl();
  }
}

void PlayerControl::onSunlightVisibilityChanged() {
  for (auto pos : collective->getConstructions().getBuiltPositions(FurnitureType::EYEBALL))
    visibilityMap->updateEyeball(pos);
}

void PlayerControl::setTutorial(STutorial t) {
  tutorial = t;
}

STutorial PlayerControl::getTutorial() const {
  return tutorial;
}

bool PlayerControl::canControlSingle(const Creature* c) const {
  return !collective->hasTrait(c, MinionTrait::PRISONER);
}

bool PlayerControl::canControlInTeam(const Creature* c) const {
  return collective->hasTrait(c, MinionTrait::FIGHTER) || c == collective->getLeader();
}

void PlayerControl::addToCurrentTeam(Creature* c) {
  collective->freeTeamMembers({c});
  if (auto teamId = getCurrentTeam()) {
    getTeams().add(*teamId, c);
    if (getGame()->getPlayerCreatures().size() > 1)
      c->pushController(createMinionController(c));
  }
}

void PlayerControl::teamMemberAction(TeamMemberAction action, Creature::Id id) {
  if (Creature* c = getCreature(id))
    switch (action) {
      case TeamMemberAction::MOVE_NOW:
        c->getPosition().getModel()->getTimeQueue().moveNow(c);
        break;
      case TeamMemberAction::CHANGE_LEADER:
        if (auto teamId = getCurrentTeam())
          if (getTeams().getMembers(*teamId).size() > 1 && canControlInTeam(c)) {
            auto controlled = getControlled();
            if (controlled.size() == 1) {
              getTeams().getLeader(*teamId)->popController();
              getTeams().setLeader(*teamId, c);
              c->pushController(createMinionController(c));
            }
          }
        break;
      case TeamMemberAction::REMOVE_MEMBER:
        if (auto teamId = getCurrentTeam())
          if (getTeams().getMembers(*teamId).size() > 1) {
            getTeams().remove(*teamId, c);
            if (c->isPlayer()) {
              c->popController();
              auto newLeader = getTeams().getLeader(*teamId);
              if (!newLeader->isPlayer())
                newLeader->pushController(createMinionController(newLeader));
            }
          }
        break;
    }
}

void PlayerControl::leaveControl() {
  set<TeamId> allTeams;
  for (auto controlled : copyOf(getControlled())) {
    if (controlled == getKeeper())
      lastControlKeeperQuestion = collective->getGlobalTime();
    if (!controlled->getPosition().isSameLevel(getLevel()))
      getView()->setScrollPos(getPosition());
    controlled->popController();
    for (TeamId team : getTeams().getActive(controlled))
      allTeams.insert(team);
  }
  for (auto team : allTeams) {
    // a creature may die when landing and be removed from the team so copy the members vector
    for (Creature* c : copyOf(getTeams().getMembers(team)))
//      if (getGame()->canTransferCreature(c, collective->getLevel()->getModel()))
        getGame()->transferCreature(c, getModel());
    if (!getTeams().isPersistent(team)) {
      if (getTeams().getMembers(team).size() == 1)
        getTeams().cancel(team);
      else
        getTeams().deactivate(team);
      break;
    }
  }
  getView()->stopClock();
}

void PlayerControl::render(View* view) {
  if (firstRender) {
    firstRender = false;
    reloadBuildingMenu();
    for (Creature* c : getCreatures())
      updateMinionVisibility(c);
  }
  if (getControlled().empty()) {
    view->updateView(this, false);
  }
  if (!introText.empty() && getGame()->getOptions()->getBoolValue(OptionId::HINTS)) {
    view->updateView(this, false);
    for (auto& msg : introText)
      view->presentText("", msg);
    introText.clear();
  }
}

bool PlayerControl::isTurnBased() {
  return !getControlled().empty();
}

void PlayerControl::addConsumableItem(Creature* creature) {
  ScrollPosition scrollPos;
  while (1) {
    Item* chosenItem = chooseEquipmentItem(creature, {}, [&](const Item* it) {
        return !collective->getMinionEquipment().isOwner(it, creature)
            && !it->canEquip()
            && collective->getMinionEquipment().needsItem(creature, it, true); }, &scrollPos);
    if (chosenItem) {
      creature->removeEffect(LastingEffect::SLEEP);
      CHECK(collective->getMinionEquipment().tryToOwn(creature, chosenItem));
    } else
      break;
  }
}

void PlayerControl::addEquipment(Creature* creature, EquipmentSlot slot) {
  vector<Item*> currentItems = creature->getEquipment().getSlotItems(slot);
  Item* chosenItem = chooseEquipmentItem(creature, currentItems, [&](const Item* it) {
      return !collective->getMinionEquipment().isOwner(it, creature)
      && creature->canEquipIfEmptySlot(it, nullptr) && it->getEquipmentSlot() == slot; });
  if (chosenItem) {
    if (auto creatureId = collective->getMinionEquipment().getOwner(chosenItem))
      if (Creature* c = getCreature(*creatureId))
        c->removeEffect(LastingEffect::SLEEP);
    creature->removeEffect(LastingEffect::SLEEP);
    CHECK(collective->getMinionEquipment().tryToOwn(creature, chosenItem));
  }
}

void PlayerControl::minionEquipmentAction(const EquipmentActionInfo& action) {
  if (auto creature = getCreature(action.creature))
    switch (action.action) {
      case ItemAction::DROP:
        for (auto id : action.ids)
          collective->getMinionEquipment().discard(id);
        break;
      case ItemAction::REPLACE:
        if (action.slot)
          addEquipment(creature, *action.slot);
        else
          addConsumableItem(creature);
        break;
      case ItemAction::LOCK:
        for (auto id : action.ids)
          collective->getMinionEquipment().setLocked(creature, id, true);
        break;
      case ItemAction::UNLOCK:
        for (auto id : action.ids)
          collective->getMinionEquipment().setLocked(creature, id, false);
        break;
      default:
        break;
    }
}

void PlayerControl::minionTaskAction(const TaskActionInfo& action) {
  if (auto c = getCreature(action.creature)) {
    if (action.switchTo)
      collective->setMinionActivity(c, *action.switchTo);
    for (MinionActivity task : action.lock)
      c->getAttributes().getMinionActivities().toggleLock(task);
  }
}

static ItemInfo getItemInfo(const vector<Item*>& stack, bool equiped, bool pending, bool locked,
    optional<ItemInfo::Type> type = none) {
  return CONSTRUCT(ItemInfo,
    c.name = stack[0]->getShortName(nullptr, stack.size() > 1);
    c.fullName = stack[0]->getNameAndModifiers(false);
    c.description = stack[0]->getDescription();
    c.number = stack.size();
    if (stack[0]->canEquip())
      c.slot = stack[0]->getEquipmentSlot();
    c.viewId = stack[0]->getViewObject().id();
    for (auto it : stack)
      c.ids.insert(it->getUniqueId());
    c.actions = {ItemAction::DROP};
    c.equiped = equiped;
    c.locked = locked;
    if (type)
      c.type = *type;
    c.pending = pending;);
}

static ViewId getSlotViewId(EquipmentSlot slot) {
  switch (slot) {
    case EquipmentSlot::BOOTS: return ViewId::LEATHER_BOOTS;
    case EquipmentSlot::WEAPON: return ViewId::SWORD;
    case EquipmentSlot::RINGS: return ViewId::FIRE_RESIST_RING;
    case EquipmentSlot::HELMET: return ViewId::LEATHER_HELM;
    case EquipmentSlot::RANGED_WEAPON: return ViewId::BOW;
    case EquipmentSlot::GLOVES: return ViewId::LEATHER_GLOVES;
    case EquipmentSlot::BODY_ARMOR: return ViewId::LEATHER_ARMOR;
    case EquipmentSlot::AMULET: return ViewId::AMULET1;
  }
}

static ItemInfo getEmptySlotItem(EquipmentSlot slot) {
  return CONSTRUCT(ItemInfo,
    c.name = "";
    c.fullName = "";
    c.description = "";
    c.slot = slot;
    c.number = 1;
    c.viewId = getSlotViewId(slot);
    c.actions = {ItemAction::REPLACE};
    c.equiped = false;
    c.pending = false;);
}

static ItemInfo getTradeItemInfo(const vector<Item*>& stack, int budget) {
  return CONSTRUCT(ItemInfo,
    c.name = stack[0]->getShortName(nullptr, stack.size() > 1);
    c.price = make_pair(ViewId::GOLD, stack[0]->getPrice());
    c.fullName = stack[0]->getNameAndModifiers(false);
    c.description = stack[0]->getDescription();
    c.number = stack.size();
    c.viewId = stack[0]->getViewObject().id();
    for (auto it : stack)
      c.ids.insert(it->getUniqueId());
    c.unavailable = c.price->second > budget;);
}

void PlayerControl::fillEquipment(Creature* creature, PlayerInfo& info) const {
  info.inventory.clear();
  if (!creature->getBody().isHumanoid())
    return;
  vector<EquipmentSlot> slots;
  for (auto slot : Equipment::slotTitles)
    slots.push_back(slot.first);
  vector<Item*> ownedItems = collective->getMinionEquipment().getItemsOwnedBy(creature);
  vector<Item*> slotItems;
  vector<EquipmentSlot> slotIndex;
  for (auto slot : slots) {
    vector<Item*> items;
    for (Item* it : ownedItems)
      if (it->canEquip() && it->getEquipmentSlot() == slot)
        items.push_back(it);
    for (int i = creature->getEquipment().getMaxItems(slot); i < items.size(); ++i)
      // a rare occurence that minion owns too many items of the same slot,
      //should happen only when an item leaves the fortress and then is braught back
      if (!collective->getMinionEquipment().isLocked(creature, items[i]->getUniqueId()))
        collective->getMinionEquipment().discard(items[i]);
    append(slotItems, items);
    append(slotIndex, vector<EquipmentSlot>(items.size(), slot));
    for (Item* item : items) {
      ownedItems.removeElement(item);
      bool equiped = creature->getEquipment().isEquipped(item);
      bool locked = collective->getMinionEquipment().isLocked(creature, item->getUniqueId());
      info.inventory.push_back(getItemInfo({item}, equiped, !equiped, locked, ItemInfo::EQUIPMENT));
      info.inventory.back().actions.push_back(locked ? ItemAction::UNLOCK : ItemAction::LOCK);
    }
    if (creature->getEquipment().getMaxItems(slot) > items.size()) {
      info.inventory.push_back(getEmptySlotItem(slot));
      slotIndex.push_back(slot);
      slotItems.push_back(nullptr);
    }
    if (slot == EquipmentSlot::WEAPON && tutorial &&
        tutorial->getHighlights(getGame()).contains(TutorialHighlight::EQUIPMENT_SLOT_WEAPON))
      info.inventory.back().tutorialHighlight = true;
  }
  vector<vector<Item*>> consumables = Item::stackItems(ownedItems,
      [&](const Item* it) { if (!creature->getEquipment().hasItem(it)) return " (pending)"; else return ""; } );
  for (auto& stack : consumables)
    info.inventory.push_back(getItemInfo(stack, false,
          !creature->getEquipment().hasItem(stack[0]), false, ItemInfo::CONSUMABLE));
  for (Item* item : creature->getEquipment().getItems())
    if (!collective->getMinionEquipment().isItemUseful(item))
      info.inventory.push_back(getItemInfo({item}, false, false, false, ItemInfo::OTHER));
}

Item* PlayerControl::chooseEquipmentItem(Creature* creature, vector<Item*> currentItems, ItemPredicate predicate,
    ScrollPosition* scrollPos) {
  vector<Item*> availableItems;
  vector<Item*> usedItems;
  vector<Item*> allItems = collective->getAllItems(predicate);
  collective->getMinionEquipment().sortByEquipmentValue(creature, allItems);
  for (Item* item : allItems)
    if (!currentItems.contains(item)) {
      auto owner = collective->getMinionEquipment().getOwner(item);
      if (owner && getCreature(*owner))
        usedItems.push_back(item);
      else
        availableItems.push_back(item);
    }
  if (currentItems.empty() && availableItems.empty() && usedItems.empty())
    return nullptr;
  vector<vector<Item*>> usedStacks = Item::stackItems(usedItems,
      [&](const Item* it) {
        const Creature* c = getCreature(*collective->getMinionEquipment().getOwner(it));
        return c->getName().bare() + toString<int>(c->getBestAttack().value);});
  vector<Item*> allStacked;
  vector<ItemInfo> options;
  for (Item* it : currentItems)
    options.push_back(getItemInfo({it}, true, false, false));
  for (auto& stack : concat(Item::stackItems(availableItems), usedStacks)) {
    options.emplace_back(getItemInfo(stack, false, false, false));
    if (auto creatureId = collective->getMinionEquipment().getOwner(stack[0]))
      if (const Creature* c = getCreature(*creatureId))
        options.back().owner = CreatureInfo(c);
    allStacked.push_back(stack.front());
  }
  auto index = getView()->chooseItem(options, scrollPos);
  if (!index)
    return nullptr;
  return concat(currentItems, allStacked)[*index];
}

int PlayerControl::getNumMinions() const {
  return (int) collective->getCreatures(MinionTrait::FIGHTER).size();
}

typedef CollectiveInfo::Button Button;

static optional<pair<ViewId, int>> getCostObjWithZero(CostInfo cost) {
  auto& resourceInfo = CollectiveConfig::getResourceInfo(cost.id);
  if (!resourceInfo.dontDisplay)
    return make_pair(resourceInfo.viewId, (int) cost.value);
  else
    return none;
}

static optional<pair<ViewId, int>> getCostObj(CostInfo cost) {
  auto& resourceInfo = CollectiveConfig::getResourceInfo(cost.id);
  if (cost.value > 0 && !resourceInfo.dontDisplay)
    return make_pair(resourceInfo.viewId, (int) cost.value);
  else
    return none;
}

static optional<pair<ViewId, int>> getCostObj(const optional<CostInfo>& cost) {
  if (cost)
    return getCostObj(*cost);
  else
    return none;
}

string PlayerControl::getMinionName(CreatureId id) const {
  static map<CreatureId, string> names;
  if (!names.count(id))
    names[id] = getGame()->getCreatureFactory()->fromId(id, TribeId::getMonster())->getName().bare();
  return names.at(id);
}

static ViewId getFurnitureViewId(FurnitureType type) {
  static EnumMap<FurnitureType, optional<ViewId>> ids;
  if (!ids[type])
    ids[type] = FurnitureFactory::get(type, TribeId::getMonster())->getViewObject()->id();
  return *ids[type];
}

vector<Button> PlayerControl::fillButtons(const vector<BuildInfo>& buildInfo) const {
  vector<Button> buttons;
  EnumMap<ResourceId, int> numResource([this](ResourceId id) { return collective->numResource(id);});
  for (auto& button : buildInfo) {
    button.type.visit(
        [&](const BuildInfo::Furniture& elem) {
          ViewId viewId = getFurnitureViewId(elem.types[0]);
          string description;
          if (elem.cost.value > 0) {
            int num = 0;
            for (auto type : elem.types)
              num += collective->getConstructions().getBuiltCount(type);
            if (num > 0)
              description = "[" + toString(num) + "]";
          }
          int availableNow = !elem.cost.value ? 1 : numResource[elem.cost.id] / elem.cost.value;
          if (CollectiveConfig::getResourceInfo(elem.cost.id).dontDisplay && availableNow)
            description += " (" + toString(availableNow) + " available)";
          buttons.push_back({viewId, button.name,
              getCostObj(elem.cost),
              description,
              (elem.noCredit && !availableNow) ?
                 CollectiveInfo::Button::GRAY_CLICKABLE : CollectiveInfo::Button::ACTIVE });
          },
        [&](const BuildInfo::Dig&) {
          buttons.push_back({ViewId::DIG_ICON, button.name, none, "", CollectiveInfo::Button::ACTIVE});
        },
        [&](ZoneId zone) {
          buttons.push_back({getViewId(zone), button.name, none, "", CollectiveInfo::Button::ACTIVE});
        },
        [&](BuildInfo::ClaimTile) {
          buttons.push_back({ViewId::KEEPER_FLOOR, button.name, none, "", CollectiveInfo::Button::ACTIVE});
        },
        [&](BuildInfo::Dispatch) {
          buttons.push_back({ViewId::IMP, button.name, none, "", CollectiveInfo::Button::ACTIVE});
        },
        [&](const BuildInfo::Trap& elem) {
          buttons.push_back({elem.viewId, button.name, none});
        },
        [&](const BuildInfo::DestroyLayers&) {
           buttons.push_back({ViewId::DESTROY_BUTTON, button.name, none, "",
               CollectiveInfo::Button::ACTIVE});
        },
        [&](BuildInfo::ForbidZone) {
          buttons.push_back({ViewId::FORBID_ZONE, button.name, none, "", CollectiveInfo::Button::ACTIVE});
        }
    );
    vector<string> unmetReqText;
    for (auto& req : button.requirements)
      if (!BuildInfo::meetsRequirement(collective, req)) {
        unmetReqText.push_back("Requires " + BuildInfo::getRequirementText(req) + ".");
        buttons.back().state = CollectiveInfo::Button::INACTIVE;
      }
    if (unmetReqText.empty())
      buttons.back().help = button.help;
    else
      buttons.back().help = combineSentences(concat({button.help}, unmetReqText));
    buttons.back().hotkey = button.hotkey;
    buttons.back().groupName = button.groupName;
    buttons.back().hotkeyOpensGroup = button.hotkeyOpensGroup;
    buttons.back().tutorialHighlight = button.tutorialHighlight;
  }
  return buttons;
}

string PlayerControl::getTriggerLabel(const AttackTrigger& trigger) const {
  switch (trigger.getId()) {
    case AttackTriggerId::SELF_VICTIMS: return "killed tribe members";
    case AttackTriggerId::GOLD: return "gold";
    case AttackTriggerId::STOLEN_ITEMS: return "item theft";
    case AttackTriggerId::ROOM_BUILT: {
      auto type = trigger.get<RoomTriggerInfo>().type;
      auto myCount = collective->getConstructions().getBuiltCount(type);
      return Furniture::getName(type, myCount);
    }
    case AttackTriggerId::POWER: return "your power";
    case AttackTriggerId::FINISH_OFF: return "finishing you off";
    case AttackTriggerId::ENEMY_POPULATION: return "population";
    case AttackTriggerId::TIMER: return "your evil";
    case AttackTriggerId::NUM_CONQUERED: return "your aggression";
    case AttackTriggerId::MINING_IN_PROXIMITY: return "breach of territory";
    case AttackTriggerId::PROXIMITY: return "proximity";
  }
}

VillageInfo::Village PlayerControl::getVillageInfo(WConstCollective col) const {
  VillageInfo::Village info;
  info.name = col->getName()->shortened;
  info.id = col->getUniqueId();
  info.tribeName = col->getName()->race;
  info.viewId = col->getName()->viewId;
  info.triggers.clear();
  info.type = col->getVillainType();
  info.attacking = false;
  for (auto& attack : notifiedAttacks)
    if (attack.getAttacker() == col && attack.isOngoing())
        info.attacking = true;
  auto addTriggers = [&] {
    for (auto& trigger : col->getTriggers(collective))
//#ifdef RELEASE
      if (trigger.value > 0)
//#endif
        info.triggers.push_back({getTriggerLabel(trigger.trigger), trigger.value});
  };
  if (col->getModel() == getModel()) {
    if (!collective->isKnownVillainLocation(col) && !getGame()->getOptions()->getBoolValue(OptionId::SHOW_MAP))
      info.access = VillageInfo::Village::NO_LOCATION;
    else
      info.access = VillageInfo::Village::LOCATION;
    addTriggers();
  } else if (!getGame()->isVillainActive(col))
    info.access = VillageInfo::Village::INACTIVE;
  else {
    info.access = VillageInfo::Village::ACTIVE;
    addTriggers();
  }
  if ((info.isConquered = col->isConquered())) {
    info.triggers.clear();
    if (canPillage(col))
      info.actions.push_back({VillageAction::PILLAGE, none});
  } else if (!col->getTribe()->isEnemy(collective->getTribe())) {
    if (collective->isKnownVillainLocation(col)) {
      if (col->hasTradeItems())
        info.actions.push_back({VillageAction::TRADE, none});
    } else if (getGame()->isVillainActive(col)) {
      if (col->hasTradeItems())
        info.actions.push_back({VillageAction::TRADE, string("You must discover the location of the ally in order to trade.")});
    }
  }
  return info;
}

void PlayerControl::handleTrading(WCollective ally) {
  ScrollPosition scrollPos;
  auto& storage = collective->getZones().getPositions(ZoneId::STORAGE_EQUIPMENT);
  if (storage.empty()) {
    getView()->presentText("Information", "You need a storage room for equipment in order to trade.");
    return;
  }
  while (1) {
    vector<Item*> available = ally->getTradeItems();
    vector<vector<Item*>> items = Item::stackItems(available);
    if (items.empty())
      break;
    int budget = collective->numResource(ResourceId::GOLD);
    vector<ItemInfo> itemInfo = items.transform(
        [budget] (const vector<Item*>& it) { return getTradeItemInfo(it, budget); });
    auto index = getView()->chooseTradeItem("Trade with " + ally->getName()->full,
        {ViewId::GOLD, collective->numResource(ResourceId::GOLD)}, itemInfo, &scrollPos);
    if (!index)
      break;
    for (Item* it : available)
      if (it->getUniqueId() == *index && it->getPrice() <= budget) {
        collective->takeResource({ResourceId::GOLD, it->getPrice()});
        Random.choose(storage).dropItem(ally->buyItem(it));
      }
    getView()->updateView(this, true);
  }
}

static ItemInfo getPillageItemInfo(const vector<Item*>& stack, bool noStorage) {
  return CONSTRUCT(ItemInfo,
    c.name = stack[0]->getShortName(nullptr, stack.size() > 1);
    c.fullName = stack[0]->getNameAndModifiers(false);
    c.description = stack[0]->getDescription();
    c.number = stack.size();
    c.viewId = stack[0]->getViewObject().id();
    for (auto it : stack)
      c.ids.insert(it->getUniqueId());
    c.unavailable = noStorage;
    c.unavailableReason = noStorage ? "No storage is available for this item." : "";
  );
}

vector<PItem> PlayerControl::retrievePillageItems(WCollective col, vector<Item*> items) {
  vector<PItem> ret;
  EntitySet<Item> index(items);
  for (auto pos : col->getTerritory().getAll()) {
    bool update = false;
    for (auto item : copyOf(pos.getInventory().getItems()))
      if (index.contains(item)) {
        ret.push_back(pos.removeItem(item));
        update = true;
      }
    if (update)
      addToMemory(pos);
  }
  return ret;
}

vector<Item*> PlayerControl::getPillagedItems(WCollective col) const {
  vector<Item*> ret;
  for (Position v : col->getTerritory().getAll())
    if (!collective->getTerritory().contains(v))
      append(ret, v.getItems());
  return ret;
}

bool PlayerControl::canPillage(WConstCollective col) const {
  for (Position v : col->getTerritory().getAll())
    if (!collective->getTerritory().contains(v) && !v.getItems().empty())
      return true;
  return false;
}

void PlayerControl::handlePillage(WCollective col) {
  ScrollPosition scrollPos;
  while (1) {
    struct PillageOption {
      vector<Item*> items;
      PositionSet storage;
    };
    vector<PillageOption> options;
    for (auto& elem : Item::stackItems(getPillagedItems(col)))
      options.push_back({elem, collective->getStorageForPillagedItem(elem.front())});
    if (options.empty())
      return;
    vector<ItemInfo> itemInfo = options.transform([] (const PillageOption& it) {
            return getPillageItemInfo(it.items, it.storage.empty());});
    auto index = getView()->choosePillageItem("Pillage " + col->getName()->full, itemInfo, &scrollPos);
    if (!index)
      break;
    CHECK(!options[*index].storage.empty());
    Random.choose(options[*index].storage).dropItems(retrievePillageItems(col, options[*index].items));
    getView()->updateView(this, true);
  }
}

void PlayerControl::handleRansom(bool pay) {
  if (ransomAttacks.empty())
    return;
  auto& ransom = ransomAttacks.front();
  int amount = *ransom.getRansom();
  if (pay && collective->hasResource({ResourceId::GOLD, amount})) {
    collective->takeResource({ResourceId::GOLD, amount});
    ransom.getAttacker()->onRansomPaid();
  }
  ransomAttacks.removeIndex(0);
}

vector<WCollective> PlayerControl::getKnownVillains() const {
  auto showAll = getGame()->getOptions()->getBoolValue(OptionId::SHOW_MAP);
  return getGame()->getCollectives().filter([&](WCollective c) {
      return showAll || collective->isKnownVillain(c) || !c->getTriggers(collective).empty();});
}

string PlayerControl::getMinionGroupName(Creature* c) const {
  if (collective->hasTrait(c, MinionTrait::PRISONER)) {
    return "prisoner";
  } else
    return c->getName().stack();
}

ViewId PlayerControl::getMinionGroupViewId(Creature* c) const {
  if (collective->hasTrait(c, MinionTrait::PRISONER)) {
    return ViewId::PRISONER;
  } else
    return c->getViewObject().id();
}

vector<Creature*> PlayerControl::getMinionsLike(Creature* like) const {
  vector<Creature*> minions;
  for (Creature* c : getCreatures())
    if (getMinionGroupName(c) == getMinionGroupName(like))
      minions.push_back(c);
  return minions;
}

void PlayerControl::sortMinionsForUI(vector<Creature*>& minions) const {
  std::sort(minions.begin(), minions.end(), [] (const Creature* c1, const Creature* c2) {
      auto l1 = (int) max(c1->getAttr(AttrType::DAMAGE), c1->getAttr(AttrType::SPELL_DAMAGE));
      auto l2 = (int) max(c2->getAttr(AttrType::DAMAGE), c2->getAttr(AttrType::SPELL_DAMAGE));
      return l1 > l2 || (l1 == l2 && c1->getUniqueId() > c2->getUniqueId());
      });
}

vector<PlayerInfo> PlayerControl::getPlayerInfos(vector<Creature*> creatures, UniqueEntity<Creature>::Id chosenId) const {
  PROFILE;
  sortMinionsForUI(creatures);
  vector<PlayerInfo> minions;
  for (Creature* c : creatures) {
    minions.emplace_back(c);
    auto& minionInfo = minions.back();
    // only fill equipment for the chosen minion to avoid lag
    if (c->getUniqueId() == chosenId) {
      for (auto expType : ENUM_ALL(ExperienceType))
        if (auto requiredDummy = collective->getMissingTrainingFurniture(c, expType))
          minionInfo.levelInfo.warning[expType] =
              "Requires " + Furniture::getName(*requiredDummy) + " to train further.";
      for (MinionActivity t : ENUM_ALL(MinionActivity))
        if (c->getAttributes().getMinionActivities().isAvailable(collective, c, t, true)) {
          minionInfo.minionTasks.push_back({t,
              !collective->isActivityGood(c, t, true),
              collective->getCurrentActivity(c).activity == t,
              c->getAttributes().getMinionActivities().isLocked(t)});
        }
      if (collective->usesEquipment(c))
        fillEquipment(c, minionInfo);
      if (canControlSingle(c))
        minionInfo.actions.push_back(PlayerInfo::CONTROL);
      if (!collective->hasTrait(c, MinionTrait::PRISONER)) {
        minionInfo.actions.push_back(PlayerInfo::RENAME);
      } else
        minionInfo.levelInfo.limit.clear();
      if (c != collective->getLeader())
        minionInfo.actions.push_back(PlayerInfo::BANISH);
      if (!collective->hasTrait(c, MinionTrait::WORKER)) {
        minionInfo.canAssignQuarters = true;
        auto& quarters = collective->getQuarters();
        if (auto index = quarters.getAssigned(c->getUniqueId()))
          minionInfo.quarters = quarters.getAllQuarters()[*index].viewId;
        else
          minionInfo.quarters = none;
      } else
        minionInfo.canAssignQuarters = false;
      if (c->isAffected(LastingEffect::CONSUMPTION_SKILL))
        minionInfo.actions.push_back(PlayerInfo::CONSUME);
    }
  }
  return minions;
}

vector<CollectiveInfo::CreatureGroup> PlayerControl::getCreatureGroups(vector<Creature*> v) const {
  sortMinionsForUI(v);
  map<string, CollectiveInfo::CreatureGroup> groups;
  for (Creature* c : v) {
    auto groupName = getMinionGroupName(c);
    auto viewId = getMinionGroupViewId(c);
    if (!groups.count(groupName))
      groups[groupName] = { c->getUniqueId(), groupName, viewId, 0};
    ++groups[groupName].count;
    if (chosenCreature == c->getUniqueId() && !getChosenTeam())
      groups[groupName].highlight = true;
  }
  return getValues(groups);
}

vector<CollectiveInfo::CreatureGroup> PlayerControl::getEnemyGroups() const {
  vector<Creature*> enemies;
  for (Vec2 v : getVisibleEnemies())
    if (Creature* c = Position(v, getCurrentLevel()).getCreature())
      enemies.push_back(c);
  return getCreatureGroups(enemies);
}

void PlayerControl::fillMinions(CollectiveInfo& info) const {
  vector<Creature*> minions;
  for (auto trait : {MinionTrait::FIGHTER, MinionTrait::PRISONER, MinionTrait::WORKER})
    for (Creature* c : collective->getCreatures(trait))
      if (!minions.contains(c))
        minions.push_back(c);
  if (auto leader = collective->getLeader())
    minions.push_back(leader);
  info.minionGroups = getCreatureGroups(minions);
  info.minions = minions.transform([](const Creature* c) { return CreatureInfo(c) ;});
  info.minionCount = collective->getPopulationSize();
  info.minionLimit = collective->getMaxPopulation();
}

ItemInfo PlayerControl::getWorkshopItem(const WorkshopItem& option) const {
  return CONSTRUCT(ItemInfo,
      c.number = option.number * option.batchSize;
      c.name = c.number == 1 ? option.name : toString(c.number) + " " + option.pluralName;
      c.viewId = option.viewId;
      c.price = getCostObj(option.cost * option.number);
      if (option.techId && !collective->getTechnology().researched.count(*option.techId)) {
        c.unavailable = true;
        c.unavailableReason = "Requires technology: " + *option.techId;
      }
      c.description = option.description;
      c.productionState = option.state.value_or(0);
      c.actions = LIST(ItemAction::REMOVE, ItemAction::CHANGE_NUMBER);
      c.tutorialHighlight = tutorial && option.tutorialHighlight &&
          tutorial->getHighlights(getGame()).contains(*option.tutorialHighlight);
    );
}

static const ViewObject& getConstructionObject(FurnitureType type) {
  static EnumMap<FurnitureType, optional<ViewObject>> objects;
  if (!objects[type]) {
    if (auto obj = FurnitureFactory::get(type, TribeId::getMonster())->getViewObject())
      objects[type] = *obj;
    objects[type]->setModifier(ViewObject::Modifier::PLANNED);
  }
  return *objects[type];
}

void PlayerControl::acquireTech(TechId tech) {
  auto techs = collective->getTechnology().getNextTechs();
  if (techs.contains(tech)) {
    if (collective->getDungeonLevel().numResearchAvailable() > 0) {
      collective->acquireTech(tech, true);
    }
  }
}

void PlayerControl::fillLibraryInfo(CollectiveInfo& collectiveInfo) const {
  if (chosenLibrary) {
    collectiveInfo.libraryInfo.emplace();
    auto& info = *collectiveInfo.libraryInfo;
    auto& dungeonLevel = collective->getDungeonLevel();
    if (dungeonLevel.numResearchAvailable() == 0)
      info.warning = "Conquer some villains to advance your level."_s;
    info.totalProgress = 100 * dungeonLevel.getNecessaryProgress(dungeonLevel.level);
    info.currentProgress = int(100 * dungeonLevel.progress * dungeonLevel.getNecessaryProgress(dungeonLevel.level));
    auto& technology = collective->getTechnology();
    auto techs = technology.getNextTechs();
    for (auto& tech : techs) {
      info.available.emplace_back();
      auto& techInfo = info.available.back();
      techInfo.name = tech;
      //techInfo.tutorialHighlight = tech->getTutorialHighlight();
      techInfo.active = !info.warning && dungeonLevel.numResearchAvailable() > 0;
      techInfo.description = technology.techs.at(tech).description;
    }
    for (auto& tech : collective->getTechnology().researched) {
      info.researched.emplace_back();
      auto& techInfo = info.researched.back();
      techInfo.name = tech;
      techInfo.description = technology.techs.at(tech).description;
    }
  }
}

void PlayerControl::fillWorkshopInfo(CollectiveInfo& info) const {
  info.workshopButtons.clear();
  int index = 0;
  int i = 0;
  for (auto workshopType : ENUM_ALL(WorkshopType)) {
    auto& workshopInfo = CollectiveConfig::getWorkshopInfo(workshopType);
    bool unavailable = collective->getConstructions().getBuiltPositions(workshopInfo.furniture).empty();
    info.workshopButtons.push_back({capitalFirst(workshopInfo.taskName),
        getConstructionObject(workshopInfo.furniture).id(), false, unavailable});
    if (chosenWorkshop == workshopType) {
      index = i;
      info.workshopButtons.back().active = true;
    }
    ++i;
  }
  if (chosenWorkshop) {
    auto transFun = [this](const WorkshopItem& item) { return getWorkshopItem(item); };
    info.chosenWorkshop = CollectiveInfo::ChosenWorkshopInfo {
        collective->getWorkshops().get(*chosenWorkshop).getOptions().transform(transFun),
        collective->getWorkshops().get(*chosenWorkshop).getQueued().transform(transFun),
        index
    };
  }
}

void PlayerControl::acceptPrisoner(int index) {
  index = -index - 1;
  auto immigrants = getPrisonerImmigrantStack();
  if (index < immigrants.size() && !immigrants[index].collective) {
    auto victim = immigrants[index].creatures[0];
    victim->removeEffect(LastingEffect::STUNNED);
    // to make sure prisoners don't die around dead bodies
    victim->addPermanentEffect(LastingEffect::POISON_RESISTANT);
    victim->getAttributes().getSkills().setValue(SkillId::DIGGING,
        victim->isAffected(LastingEffect::NAVIGATION_DIGGING_SKILL) ? 1 : 0.2);
    victim->removePermanentEffect(LastingEffect::NAVIGATION_DIGGING_SKILL);
    victim->removePermanentEffect(LastingEffect::BRIDGE_BUILDING_SKILL);
    collective->addCreature(victim, {MinionTrait::WORKER, MinionTrait::PRISONER, MinionTrait::NO_LIMIT});
    addMessage(PlayerMessage("You enslave " + victim->getName().a()).setPosition(victim->getPosition()));
    for (auto& elem : copyOf(stunnedCreatures))
      if (elem.first == victim)
        stunnedCreatures.removeElement(elem);
  }
}

void PlayerControl::rejectPrisoner(int index) {
  index = -index - 1;
  auto immigrants = getPrisonerImmigrantStack();
  if (index < immigrants.size()) {
    auto victim = immigrants[index].creatures[0];
    victim->dieWithLastAttacker();
  }
}

vector<PlayerControl::StunnedInfo> PlayerControl::getPrisonerImmigrantStack() const {
  vector<StunnedInfo> ret;
  vector<Creature*> outside;
  unordered_map<WCollective, vector<Creature*>, CustomHash<WCollective>> inside;
  for (auto stunned : stunnedCreatures)
    if (stunned.first->isAffected(LastingEffect::STUNNED) && !stunned.first->isDead()) {
      if (auto villain = stunned.second) {
        if (villain->isConquered() || !villain->getTerritory().contains(stunned.first->getPosition()))
          outside.push_back(stunned.first);
        else
          inside[villain].push_back(stunned.first);
      } else
        outside.push_back(stunned.first);
    }
  for (auto& elem : inside)
    for (auto& stack : Creature::stack(elem.second))
      ret.push_back(StunnedInfo{stack, elem.first});
  for (auto& stack : Creature::stack(outside))
    ret.push_back(StunnedInfo{stack, nullptr});
  return ret;
}

vector<ImmigrantDataInfo> PlayerControl::getPrisonerImmigrantData() const {
  vector<ImmigrantDataInfo> ret;
  int index = -1;
  for (auto stack : getPrisonerImmigrantStack()) {
    auto c = stack.creatures[0];
    const int numFreePrisoners = 4;
    const int requiredPrisonSize = 2;
    const int numPrisoners = collective->getCreatures(MinionTrait::PRISONER).size() - numFreePrisoners;
    const int prisonSize = collective->getConstructions().getBuiltCount(FurnitureType::PRISON);
    vector<string> requirements;
    const int missingSize = (numPrisoners + 1) * requiredPrisonSize - prisonSize;
    if (missingSize > 0) {
      if (prisonSize == 0)
        requirements.push_back("Requires a prison.");
      else
        requirements.push_back("Requires " + toString(missingSize) + " more prison tiles.");
    }
    if (stack.collective)
      requirements.push_back("Requires conquering " + stack.collective->getName()->full);
    ret.push_back(ImmigrantDataInfo {
        requirements,
        {},
        {},
        none,
        c->getName().bare() + " (prisoner)",
        c->getViewObject().id(),
        AttributeInfo::fromCreature(c),
        stack.creatures.size() == 1 ? none : optional<int>(stack.creatures.size()),
        c->getTimeRemaining(LastingEffect::STUNNED),
        index,
        none,
        none,
        none,
        none
    });
    --index;
  }
  return ret;
}

void PlayerControl::fillImmigration(CollectiveInfo& info) const {
  PROFILE;
  info.immigration.clear();
  auto& immigration = collective->getImmigration();
  info.immigration.append(getPrisonerImmigrantData());
  for (auto& elem : immigration.getAvailable()) {
    const auto& candidate = elem.second.get();
    if (candidate.getInfo().isInvisible())
      continue;
    const int count = (int) candidate.getCreatures().size();
    optional<TimeInterval> timeRemaining;
    if (auto time = candidate.getEndTime())
      timeRemaining = *time - getGame()->getGlobalTime();
    vector<string> infoLines;
    candidate.getInfo().visitRequirements(makeVisitor(
        [&](const Pregnancy&) {
          optional<TimeInterval> maxT;
          for (Creature* c : collective->getCreatures())
            if (c->isAffected(LastingEffect::PREGNANT))
              if (auto remaining = c->getTimeRemaining(LastingEffect::PREGNANT))
                if (!maxT || *remaining > *maxT)
                  maxT = *remaining;
          if (maxT && (!timeRemaining || *maxT > *timeRemaining))
            timeRemaining = *maxT;
        },
        [&](const RecruitmentInfo& info) {
          infoLines.push_back(
              toString(info.getAvailableRecruits(getGame(), candidate.getInfo().getId(0)).size()) +
              " recruits available");
        },
        [&](const auto&) {}
    ));
    Creature* c = candidate.getCreatures()[0];
    string name = count > 1 ? c->getName().groupOf(count) : c->getName().title();
    if (auto& s = c->getName().stackOnly())
      name += " (" + *s + ")";
    if (count > 1)
      infoLines.push_back("The entire group takes up one population spot");
    for (auto trait : candidate.getInfo().getTraits())
      if (auto desc = getImmigrantDescription(trait))
        infoLines.push_back(desc);
    info.immigration.push_back(ImmigrantDataInfo {
        immigration.getMissingRequirements(candidate),
        infoLines,
        candidate.getSpecialTraits(),
        getCostObj(candidate.getCost()),
        name,
        c->getViewObject().id(),
        AttributeInfo::fromCreature(c),
        count == 1 ? none : optional<int>(count),
        timeRemaining,
        elem.first,
        none,
        candidate.getCreatedTime(),
        candidate.getInfo().getKeybinding(),
        candidate.getInfo().getTutorialHighlight()
    });
  }
  sort(info.immigration.begin(), info.immigration.end(),
      [](const ImmigrantDataInfo& i1, const ImmigrantDataInfo& i2) {
        return (i1.timeLeft && (!i2.timeLeft || *i1.timeLeft > *i2.timeLeft)) ||
            (!i1.timeLeft && !i2.timeLeft && i1.id > i2.id);
      });
}

void PlayerControl::fillImmigrationHelp(CollectiveInfo& info) const {
  info.allImmigration.clear();
  static map<CreatureId, PCreature> creatureStats;
  auto getStats = [&](CreatureId id) -> Creature* {
    if (!creatureStats[id]) {
      creatureStats[id] = getGame()->getCreatureFactory()->fromId(id, TribeId::getDarkKeeper());
    }
    return creatureStats[id].get();
  };
  for (auto elem : Iter(collective->getImmigration().getImmigrants())) {
    if (elem->isHiddenInHelp())
      continue;
    auto creatureId = elem->getNonRandomId(0);
    Creature* c = getStats(creatureId);
    optional<pair<ViewId, int>> costObj;
    vector<string> requirements;
    vector<string> infoLines;
    elem->visitRequirements(makeVisitor(
        [&](const AttractionInfo& attraction) {
          int required = attraction.amountClaimed;
          requirements.push_back("Requires " + toString(required) + " " +
              combineWithOr(attraction.types.transform(
                  [&](const AttractionType& type) { return AttractionInfo::getAttractionName(type, required); })));
        },
        [&](const TechId& techId) {
          requirements.push_back("Requires technology: " + techId);
        },
        [&](const SunlightState& state) {
          requirements.push_back("Will only join during the "_s + SunlightInfo::getText(state));
        },
        [&](const FurnitureType& type) {
          requirements.push_back("Requires at least one " + Furniture::getName(type));
        },
        [&](const CostInfo& cost) {
          costObj = getCostObj(cost);
        },
        [&](const ExponentialCost& cost) {
          auto& resourceInfo = CollectiveConfig::getResourceInfo(cost.base.id);
          costObj = make_pair(resourceInfo.viewId, (int) cost.base.value);
          infoLines.push_back("Cost doubles for every " + toString(cost.numToDoubleCost) + " "
              + c->getName().plural());
          if (cost.numFree > 0)
            infoLines.push_back("First " + toString(cost.numFree) + " " + c->getName().plural() + " are free");
        },
        [&](const Pregnancy&) {
          requirements.push_back("Requires a pregnant succubus");
        },
        [&](const RecruitmentInfo& info) {
          if (info.findEnemy(getGame()))
            requirements.push_back("Ally must be discovered and have recruits available");
          else
            requirements.push_back("Recruit is not available in this game");
        },
        [&](const TutorialRequirement&) {
        },
        [&](const MinTurnRequirement&) {
        }
    ));
    if (auto limit = elem->getLimit())
      infoLines.push_back("Limited to " + toString(*limit) + " creatures");
    for (auto trait : elem->getTraits())
      if (auto desc = getImmigrantDescription(trait))
        infoLines.push_back(desc);
    info.allImmigration.push_back(ImmigrantDataInfo {
        requirements,
        infoLines,
        {},
        costObj,
        c->getName().stack(),
        c->getViewObject().id(),
        AttributeInfo::fromCreature(c),
        0,
        none,
        elem.index(),
        collective->getImmigration().getAutoState(elem.index())
    });
  }
  info.allImmigration.push_back(ImmigrantDataInfo {
      {"Requires 2 prison tiles", "Requires knocking out a hostile creature"},
      {"Supplies your imp force", "Can be converted to your side using torture"},
      {},
      none,
      "prisoner",
      ViewId::PRISONER,
      {},
      0,
      none,
      -1,
  });
}

static optional<CollectiveInfo::RebellionChance> getRebellionChance(double prob) {
  if (prob > 0.6)
    return CollectiveInfo::RebellionChance::HIGH;
  if (prob > 0.2)
    return CollectiveInfo::RebellionChance::MEDIUM;
  if (prob > 0)
    return CollectiveInfo::RebellionChance::LOW;
  return none;
}

void PlayerControl::fillCurrentLevelInfo(GameInfo& gameInfo) const {
  auto& levels = getModel()->getMainLevels();
  int index = *levels.findElement(getCurrentLevel());
  gameInfo.currentLevel = CurrentLevelInfo {
    "Level " + toString(index),
    index > 0,
    index < levels.size() - 1
  };
}

void PlayerControl::refreshGameInfo(GameInfo& gameInfo) const {
  fillCurrentLevelInfo(gameInfo);
  if (tutorial)
    tutorial->refreshInfo(getGame(), gameInfo.tutorial);
  gameInfo.singleModel = getGame()->isSingleModel();
  gameInfo.villageInfo.villages.clear();
  gameInfo.villageInfo.numMainVillains = gameInfo.villageInfo.numConqueredMainVillains = 0;
  for (auto& col : getGame()->getVillains(VillainType::MAIN)) {
    ++gameInfo.villageInfo.numMainVillains;
    if (col->isConquered())
      ++gameInfo.villageInfo.numConqueredMainVillains;
  }
  for (auto& col : getKnownVillains())
    if (col->getName() && col->isDiscoverable())
      gameInfo.villageInfo.villages.push_back(getVillageInfo(col));
  gameInfo.villageInfo.dismissedInfos = dismissedVillageInfos;
  std::stable_sort(gameInfo.villageInfo.villages.begin(), gameInfo.villageInfo.villages.end(),
       [](const auto& v1, const auto& v2) { return (int) v1.type < (int) v2.type; });
  SunlightInfo sunlightInfo = getGame()->getSunlightInfo();
  gameInfo.sunlightInfo = { sunlightInfo.getText(), sunlightInfo.getTimeRemaining() };
  gameInfo.infoType = GameInfo::InfoType::BAND;
  gameInfo.playerInfo = CollectiveInfo();
  auto& info = *gameInfo.playerInfo.getReferenceMaybe<CollectiveInfo>();
  info.buildings = fillButtons(buildInfo);
  fillMinions(info);
  fillImmigration(info);
  fillImmigrationHelp(info);
  info.chosenCreature.reset();
  if (chosenCreature)
    if (Creature* c = getCreature(*chosenCreature)) {
      if (!getChosenTeam())
        info.chosenCreature = CollectiveInfo::ChosenCreatureInfo {
            *chosenCreature, getPlayerInfos(getMinionsLike(c), *chosenCreature)};
      else
        info.chosenCreature = CollectiveInfo::ChosenCreatureInfo {
            *chosenCreature, getPlayerInfos(getTeams().getMembers(*getChosenTeam()), *chosenCreature), *getChosenTeam()};
    }
  fillWorkshopInfo(info);
  fillLibraryInfo(info);
  info.monsterHeader = "Minions: " + toString(info.minionCount) + " / " + toString(info.minionLimit);
  info.enemyGroups = getEnemyGroups();
  info.numResource.clear();
  const auto dungeonLevel = collective->getDungeonLevel();
  info.dungeonLevel = dungeonLevel.level + 1;
  info.dungeonLevelViewId = collective->getLeader()->getViewObject().id();
  info.leaderTitle = collective->getLeader()->getName().title();
  info.dungeonLevelProgress = dungeonLevel.progress;
  info.numResearchAvailable = dungeonLevel.numResearchAvailable();
  for (auto resourceId : ENUM_ALL(CollectiveResourceId)) {
    auto& elem = CollectiveConfig::getResourceInfo(resourceId);
    if (!elem.dontDisplay)
      info.numResource.push_back(
          {elem.viewId, collective->numResourcePlusDebt(resourceId), elem.name, elem.tutorialHighlight});
  }
  info.warning = "";
  gameInfo.time = collective->getGame()->getGlobalTime();
  gameInfo.modifiedSquares = gameInfo.totalSquares = 0;
  gameInfo.modifiedSquares += getCurrentLevel()->getNumGeneratedSquares();
  gameInfo.totalSquares += getCurrentLevel()->getNumTotalSquares();
  info.teams.clear();
  for (int i : All(getTeams().getAll())) {
    TeamId team = getTeams().getAll()[i];
    info.teams.emplace_back();
    for (Creature* c : getTeams().getMembers(team))
      info.teams.back().members.push_back(c->getUniqueId());
    info.teams.back().active = getTeams().isActive(team);
    info.teams.back().id = team;
    if (getChosenTeam() == team)
      info.teams.back().highlight = true;
  }
  gameInfo.messageBuffer = messages;
  info.taskMap.clear();
  for (WConstTask task : collective->getTaskMap().getAllTasks()) {
    optional<UniqueEntity<Creature>::Id> creature;
    if (auto c = collective->getTaskMap().getOwner(task))
      creature = c->getUniqueId();
    info.taskMap.push_back(CollectiveInfo::Task{task->getDescription(), creature, collective->getTaskMap().isPriorityTask(task)});
  }
  for (auto& elem : ransomAttacks) {
    info.ransom = CollectiveInfo::Ransom {make_pair(ViewId::GOLD, *elem.getRansom()), elem.getAttackerName(),
        collective->hasResource({ResourceId::GOLD, *elem.getRansom()})};
    break;
  }
  const auto maxEnemyCountdown = 500_visible;
  if (auto& enemies = getModel()->getExternalEnemies())
    if (auto nextWave = enemies->getNextWave()) {
      if (nextWave->enemy.behaviour.getId() != AttackBehaviourId::HALLOWEEN_KIDS) {
        auto countDown = nextWave->attackTime - getModel()->getLocalTime();
        auto index = enemies->getNextWaveIndex();
        auto name = nextWave->enemy.name;
        auto viewId = nextWave->viewId;
        if (index % 6 == 5) {
          name = "Unknown";
          viewId = ViewId::UNKNOWN_MONSTER;
        }
        if (!dismissedNextWaves.count(index) && countDown < maxEnemyCountdown)
          info.nextWave = CollectiveInfo::NextWave {
            viewId,
            name,
            countDown
          };
      }
    }
  if (auto rebellionWarning = getRebellionChance(collective->getRebellionProbability()))
    if (!lastWarningDismiss || getModel()->getLocalTime() > *lastWarningDismiss + 1000_visible)
      info.rebellionChance = *rebellionWarning;
  info.allQuarters = collective->getQuarters().getAllQuarters().transform(
      [](const auto& info) { return info.viewId; });
}

void PlayerControl::addMessage(const PlayerMessage& msg) {
  messages.push_back(msg);
  messageHistory.push_back(msg);
  if (msg.getPriority() == MessagePriority::CRITICAL) {
    getView()->stopClock();
    for (auto c : getControlled()) {
      c->privateMessage(msg);
      break;
    }
  }
}

void PlayerControl::updateMinionVisibility(const Creature* c) {
  auto visibleTiles = c->getVisibleTiles();
  visibilityMap->update(c, visibleTiles);
  for (Position pos : visibleTiles) {
    if (collective->addKnownTile(pos))
      updateKnownLocations(pos);
    addToMemory(pos);
  }
}

void PlayerControl::onEvent(const GameEvent& event) {
  using namespace EventInfo;
  event.visit(
      [&](const Projectile& info) {
        if (getControlled().empty() && (canSee(info.begin) || canSee(info.end)))
          getView()->animateObject(info.begin.getCoord(), info.end.getCoord(), info.viewId, info.fx);
      },
      [&](const CreatureEvent& info) {
        if (collective->getCreatures().contains(info.creature))
          addMessage(PlayerMessage(info.message).setCreature(info.creature->getUniqueId()));
      },
      [&](const VisibilityChanged& info) {
        visibilityMap->onVisibilityChanged(info.pos);
      },
      [&](const CreatureMoved& info) {
        if (getCreatures().contains(info.creature))
          updateMinionVisibility(info.creature);
      },
      [&](const ItemsEquipped& info) {
        if (info.creature->isPlayer() &&
            !collective->getMinionEquipment().tryToOwn(info.creature, info.items.getOnlyElement()))
          getView()->presentText("", "Item won't be permanently assigned to creature because the equipment slot is locked.");
      },
      [&](const WonGame&) {
        if (auto keeper = getKeeper()) { // Check if keeper is alive just in case. If he's not then game over has already happened
          getGame()->conquered(keeper->getName().firstOrBare(), collective->getKills().getSize(),
              (int) collective->getDangerLevel() + collective->getPoints());
          getView()->presentText("", "When you are ready, retire your dungeon and share it online. "
            "Other players will be able to invade it as adventurers. To do this, press Escape and choose \'retire\'.");
        }
      },
      [&](const RetiredGame&) {
        if (auto keeper = getKeeper()) // Check if keeper is alive just in case. If he's not then game over has already happened
          if (getGame()->getVillains(VillainType::MAIN).empty())
            // No victory condition in this game, so we generate a highscore when retiring.
            getGame()->retired(keeper->getName().firstOrBare(), collective->getKills().getSize(),
                (int) collective->getDangerLevel() + collective->getPoints());
      },
      [&](const TechbookRead& info) {
        auto tech = info.technology;
        vector<TechId> nextTechs = collective->getTechnology().getNextTechs();
        if (!collective->getTechnology().researched.count(tech)) {
          if (!nextTechs.contains(tech))
            getView()->presentText("Information", "The tome describes the knowledge of " + tech
                + ", but you do not comprehend it.");
          else {
            getView()->presentText("Information", "You have acquired the knowledge of " + tech);
            collective->acquireTech(tech, false);
          }
        } else {
          getView()->presentText("Information", "The tome describes the knowledge of " + tech
              + ", which you already possess.");
        }
      },
      [&](const CreatureStunned& info) {
        for (auto villain : getGame()->getCollectives())
          if (villain->getCreatures().contains(info.victim)) {
            stunnedCreatures.push_back({info.victim, villain});
            return;
          }
        stunnedCreatures.push_back({info.victim, nullptr});
      },
      [&](const CreatureKilled& info) {
        auto pos = info.victim->getPosition();
        if (canSee(pos))
          if (auto anim = info.victim->getBody().getDeathAnimation())
            getView()->animation(pos.getCoord(), *anim);
      },
      [&](const CreatureAttacked& info) {
        auto pos = info.victim->getPosition();
        if (canSee(pos)) {
          auto dir = info.attacker->getPosition().getDir(pos);
          if (dir.length8() == 1) {
            auto orientation = dir.getCardinalDir();
            getView()->animation(pos.getCoord(), AnimationId::ATTACK, orientation);
          }
        }
      },
      [&](const FurnitureDestroyed& info) {
        if (info.type == FurnitureType::EYEBALL)
          visibilityMap->removeEyeball(info.position);
        if (info.type == FurnitureType::PIT && collective->getKnownTiles().isKnown(info.position))
          addToMemory(info.position);
      },
      [&](const FX& info) {
        if (getControlled().empty() && canSee(info.position) && info.position.isSameLevel(getCurrentLevel()))
          getView()->animation(FXSpawnInfo(info.fx, info.position.getCoord(), info.direction.value_or(Vec2(0, 0))));
      },
      [&](const auto&) {}
  );
}

void PlayerControl::updateKnownLocations(const Position& pos) {
  /*if (pos.getModel() == getModel())
    if (const Location* loc = pos.getLocation())
      if (!knownLocations.count(loc)) {
        knownLocations.insert(loc);
        if (auto name = loc->getName())
          addMessage(PlayerMessage("Your minions discover the location of " + *name, MessagePriority::HIGH)
              .setLocation(loc));
        else if (loc->isMarkedAsSurprise())
          addMessage(PlayerMessage("Your minions discover a new location.").setLocation(loc));
      }*/
  if (getGame()) // check in case this method is called before Game is constructed
    for (WConstCollective col : getGame()->getCollectives())
      if (col != collective && col->getTerritory().contains(pos)) {
        collective->addKnownVillain(col);
        if (!collective->isKnownVillainLocation(col)) {
          collective->addKnownVillainLocation(col);
          if (col->isDiscoverable())
            if (auto& name = col->getName())
              addMessage(PlayerMessage("Your minions discover the location of " + name->full,
                  MessagePriority::HIGH).setPosition(pos));
        }
      }
}


const MapMemory& PlayerControl::getMemory() const {
  return *memory;
}

ViewObject PlayerControl::getTrapObject(TrapType type, bool armed) const {
  for (auto& info : buildInfo)
    if (auto trap = info.type.getReferenceMaybe<BuildInfo::Trap>())
      if (trap->type == type) {
        if (!armed)
          return ViewObject(trap->viewId, ViewLayer::FLOOR, "Unarmed " + getTrapName(type) + " trap")
            .setModifier(ViewObject::Modifier::PLANNED);
        else
          return ViewObject(trap->viewId, ViewLayer::FLOOR, getTrapName(type) + " trap");
      }
  FATAL << "trap not found" << int(type);
  return ViewObject(ViewId::EMPTY, ViewLayer::FLOOR);
}

void PlayerControl::getSquareViewIndex(Position pos, bool canSee, ViewIndex& index) const {
  // use the leader as a generic viewer
  auto leader = collective->getLeader();
  if (!leader) { // if no leader try any creature, else bail out
    auto& creatures = collective->getCreatures();
    if (!creatures.empty())
      leader = creatures[0];
    else
      return;
  }
  if (canSee)
    pos.getViewIndex(index, leader);
  else
    index.setHiddenId(pos.getTopViewId());
  if (const Creature* c = pos.getCreature())
    if (canSee) {
      index.modEquipmentCounts() = c->getEquipment().getCounts();
      index.insert(c->getViewObject());
      auto& object = index.getObject(ViewLayer::CREATURE);
      if (isEnemy(c)) {
        object.setModifier(ViewObject::Modifier::HOSTILE);
        if (c->canBeCaptured())
          object.setClickAction(c->isCaptureOrdered() ?
              ViewObjectAction::CANCEL_CAPTURE_ORDER : ViewObjectAction::ORDER_CAPTURE);
      } else
        object.getCreatureStatus().intersectWith(getDisplayedOnMinions());
    }
}

void PlayerControl::getViewIndex(Vec2 pos, ViewIndex& index) const {
  PROFILE;
  Position position(pos, getCurrentLevel());
  if (!position.isValid())
    return;
  bool canSeePos = canSee(position);
  getSquareViewIndex(position, canSeePos, index);
  if (!canSeePos)
    if (auto memIndex = getMemory().getViewIndex(position))
      index.mergeFromMemory(*memIndex);
  if (collective->getTerritory().contains(position)) {
    if (auto furniture = position.getFurniture(FurnitureLayer::MIDDLE)) {
      if (auto clickType = furniture->getClickType())
        if (auto& obj = furniture->getViewObject())
          if (index.hasObject(obj->layer()))
            index.getObject(obj->layer()).setClickAction(FurnitureClick::getClickAction(*clickType, position, furniture));
      if (furniture->getUsageType() == FurnitureUsageType::STUDY || CollectiveConfig::getWorkshopType(furniture->getType()))
        index.setHighlight(HighlightType::CLICKABLE_FURNITURE);
      if ((chosenWorkshop && chosenWorkshop == CollectiveConfig::getWorkshopType(furniture->getType())) ||
          (chosenLibrary && furniture->getUsageType() == FurnitureUsageType::STUDY))
        index.setHighlight(HighlightType::CLICKED_FURNITURE);
      if (draggedCreature)
        if (Creature* c = getCreature(*draggedCreature))
          if (auto task = MinionActivities::getActivityFor(collective, c, furniture->getType()))
            if (collective->isActivityGood(c, *task, true))
              index.setHighlight(HighlightType::CREATURE_DROP);
      if (CollectiveConfig::requiresLighting(furniture->getType()) && position.getLightingEfficiency() < 0.99)
        index.setHighlight(HighlightType::INSUFFICIENT_LIGHT);
    }
    for (auto furniture : position.getFurniture())
      if (furniture->getLuxuryInfo().luxury > 0)
        if (auto obj = furniture->getViewObject())
          if (index.hasObject(obj->layer()))
            index.getObject(obj->layer()).setAttribute(ViewObject::Attribute::LUXURY, furniture->getLuxuryInfo().luxury);
  }
  if (collective->isMarked(position))
    index.setHighlight(collective->getMarkHighlight(position));
  if (collective->hasPriorityTasks(position))
    index.setHighlight(HighlightType::PRIORITY_TASK);
  if (!index.hasObject(ViewLayer::CREATURE))
    for (auto task : collective->getTaskMap().getTasks(position))
      if (auto viewId = task->getViewId())
          index.insert(ViewObject(*viewId, ViewLayer::CREATURE));
  if (position.isTribeForbidden(getTribeId()))
    index.setHighlight(HighlightType::FORBIDDEN_ZONE);
  collective->getZones().setHighlights(position, index);
  if (rectSelection
      && pos.inRectangle(Rectangle::boundingBox({rectSelection->corner1, rectSelection->corner2})))
    index.setHighlight(rectSelection->deselect ? HighlightType::RECT_DESELECTION : HighlightType::RECT_SELECTION);
  if (getGame()->getOptions()->getBoolValue(OptionId::SHOW_MAP))
    for (auto col : getGame()->getCollectives())
      if (col->getTerritory().contains(position))
        index.setHighlight(HighlightType::RECT_SELECTION);
  const ConstructionMap& constructions = collective->getConstructions();
  if (auto trap = constructions.getTrap(position))
    index.insert(getTrapObject(trap->getType(), trap->isArmed()));
  for (auto layer : ENUM_ALL(FurnitureLayer))
    if (auto f = constructions.getFurniture(position, layer))
      if (!f->isBuilt(position))
        index.insert(getConstructionObject(f->getFurnitureType()));
  if (unknownLocations->contains(position))
    index.insert(ViewObject(ViewId::UNKNOWN_MONSTER, ViewLayer::TORCH2, "Surprise"));
}

Vec2 PlayerControl::getPosition() const {
  Vec2 topLeft(100000, 100000);
  Vec2 bottomRight(-100000, -100000);
  auto currentLevel = getCurrentLevel();
  for (auto& pos : collective->getTerritory().getAll())
    if (pos.isSameLevel(currentLevel)) {
      auto coord = pos.getCoord();
      topLeft.x = min(coord.x, topLeft.x);
      topLeft.y = min(coord.y, topLeft.y);
      bottomRight.x = max(coord.x, bottomRight.x);
      bottomRight.y = max(coord.y, bottomRight.y);
    }
  if (topLeft.x < 100000)
    return (topLeft + bottomRight) / 2;
  else if (getKeeper()->getPosition().isSameLevel(currentLevel))
    return getKeeper()->getPosition().getCoord();
  return currentLevel->getBounds().middle();
}

static enum Selection { SELECT, DESELECT, NONE } selection = NONE;

void PlayerControl::controlSingle(Creature* c) {
  CHECK(getCreatures().contains(c));
  CHECK(!c->isDead());
  commandTeam(getTeams().create({c}));
}

Creature* PlayerControl::getCreature(UniqueEntity<Creature>::Id id) const {
  for (Creature* c : getCreatures())
    if (c->getUniqueId() == id)
      return c;
  return nullptr;
}

vector<Creature*> PlayerControl::getTeam(const Creature* c) {
  vector<Creature*> ret;
  for (auto team : getTeams().getActive(c))
    append(ret, getTeams().getMembers(team));
  return ret;
}

void PlayerControl::commandTeam(TeamId team) {
  if (!getControlled().empty())
    leaveControl();
  auto c = getTeams().getLeader(team);
  c->pushController(createMinionController(c));
  getTeams().activate(team);
  collective->freeTeamMembers(getTeams().getMembers(team));
  getView()->resetCenter();
}

void PlayerControl::toggleControlAllTeamMembers() {
  if (auto teamId = getCurrentTeam()) {
    auto members = getTeams().getMembers(*teamId);
    if (members.size() > 1) {
      if (getControlled().size() == 1) {
        for (auto c : members)
          if (!c->isPlayer() && canControlInTeam(c))
            c->pushController(createMinionController(c));
      } else
        for (auto c : members)
          if (c->isPlayer() && c != getTeams().getLeader(*teamId))
            c->popController();
    }
  }
}

optional<PlayerMessage&> PlayerControl::findMessage(PlayerMessage::Id id){
  for (auto& elem : messages)
    if (elem.getUniqueId() == id)
      return elem;
  return none;
}

CollectiveTeams& PlayerControl::getTeams() {
  return collective->getTeams();
}

const CollectiveTeams& PlayerControl::getTeams() const {
  return collective->getTeams();
}

void PlayerControl::setScrollPos(Position pos) {
  if (pos.isSameLevel(getLevel()))
    getView()->setScrollPos(pos.getCoord());
  else if (auto stairs = getLevel()->getStairsTo(pos.getLevel()))
    getView()->setScrollPos(stairs->getCoord());
}

void PlayerControl::scrollToMiddle(const vector<Position>& pos) {
  vector<Vec2> visible;
  for (Position v : pos)
    if (collective->getKnownTiles().isKnown(v))
      visible.push_back(v.getCoord());
  CHECK(!visible.empty());
  getView()->setScrollPos(Rectangle::boundingBox(visible).middle());
}

WCollective PlayerControl::getVillain(UniqueEntity<Collective>::Id id) {
  for (auto col : getGame()->getCollectives())
    if (col->getUniqueId() == id)
      return col;
  return nullptr;
}

optional<TeamId> PlayerControl::getChosenTeam() const {
  if (chosenTeam && getTeams().exists(*chosenTeam))
    return chosenTeam;
  else
    return none;
}

void PlayerControl::setChosenCreature(optional<UniqueEntity<Creature>::Id> id) {
  clearChosenInfo();
  chosenCreature = id;
}

void PlayerControl::setChosenTeam(optional<TeamId> team, optional<UniqueEntity<Creature>::Id> creature) {
  clearChosenInfo();
  chosenTeam = team;
  chosenCreature = creature;
}

void PlayerControl::clearChosenInfo() {
  setChosenWorkshop(none);
  setChosenLibrary(false);
  chosenCreature = none;
  chosenTeam = none;
}

void PlayerControl::setChosenLibrary(bool state) {
  if (state)
    clearChosenInfo();
  chosenLibrary = state;
}

void PlayerControl::setChosenWorkshop(optional<WorkshopType> type) {
  auto refreshHighlights = [&] {
    if (chosenWorkshop)
      for (auto pos : collective->getConstructions().getBuiltPositions(
             CollectiveConfig::getWorkshopInfo(*chosenWorkshop).furniture))
        pos.setNeedsRenderUpdate(true);
  };
  refreshHighlights();
  if (type)
    clearChosenInfo();
  chosenWorkshop = type;
  refreshHighlights();
}

void PlayerControl::minionDragAndDrop(const CreatureDropInfo& info) {
  PROFILE;
  Position pos(info.pos, getLevel());
  if (Creature* c = getCreature(info.creatureId)) {
    c->removeEffect(LastingEffect::TIED_UP);
    c->removeEffect(LastingEffect::SLEEP);
    if (auto furniture = collective->getConstructions().getFurniture(pos, FurnitureLayer::MIDDLE))
      if (auto task = MinionActivities::getActivityFor(collective, c, furniture->getFurnitureType())) {
        if (collective->isActivityGood(c, *task, true)) {
          collective->setMinionActivity(c, *task);
          collective->setTask(c, Task::goTo(pos));
          return;
        }
      }
    PTask task = Task::goToAndWait(pos, 15_visible);
    task->setViewId(ViewId::GUARD_POST);
    collective->setTask(c, std::move(task));
    pos.setNeedsRenderUpdate(true);
  }
}

void PlayerControl::processInput(View* view, UserInput input) {
  switch (input.getId()) {
    case UserInputId::MESSAGE_INFO:
      if (auto message = findMessage(input.get<PlayerMessage::Id>())) {
        if (auto pos = message->getPosition())
          setScrollPos(*pos);
        else if (auto id = message->getCreature()) {
          if (const Creature* c = getCreature(*id))
            setScrollPos(c->getPosition());
        }/* else if (auto loc = message->getLocation()) {
          if (loc->getMiddle().isSameLevel(getLevel()))
            scrollToMiddle(loc->getAllSquares());
          else
            setScrollPos(loc->getMiddle());
        }*/
      }
      break;
    case UserInputId::DISMISS_VILLAGE_INFO: {
      auto& info = input.get<DismissVillageInfo>();
      dismissedVillageInfos.insert({info.collectiveId, info.infoText});
      break;
    }
    case UserInputId::CREATE_TEAM:
      if (Creature* c = getCreature(input.get<Creature::Id>()))
        if (collective->hasTrait(c, MinionTrait::FIGHTER) || c == collective->getLeader())
          getTeams().create({c});
      break;
    case UserInputId::CREATE_TEAM_FROM_GROUP:
      if (Creature* creature = getCreature(input.get<Creature::Id>())) {
        vector<Creature*> group = getMinionsLike(creature);
        optional<TeamId> team;
        for (Creature* c : group)
          if (collective->hasTrait(c, MinionTrait::FIGHTER) || c == collective->getLeader()) {
            if (!team)
              team = getTeams().create({c});
            else
              getTeams().add(*team, c);
          }
      }
      break;
    case UserInputId::CREATURE_DRAG:
      draggedCreature = input.get<Creature::Id>();
      for (auto task : ENUM_ALL(MinionActivity))
        for (auto& pos : MinionActivities::getAllPositions(collective, nullptr, task))
          pos.setNeedsRenderUpdate(true);
      break;
    case UserInputId::CREATURE_DRAG_DROP:
      minionDragAndDrop(input.get<CreatureDropInfo>());
      draggedCreature = none;
      break;
    case UserInputId::TEAM_DRAG_DROP: {
      auto& info = input.get<TeamDropInfo>();
      Position pos = Position(info.pos, getLevel());
      if (getTeams().exists(info.teamId))
        for (Creature* c : getTeams().getMembers(info.teamId)) {
          c->removeEffect(LastingEffect::TIED_UP);
          c->removeEffect(LastingEffect::SLEEP);
          PTask task = Task::goToAndWait(pos, 15_visible);
          task->setViewId(ViewId::GUARD_POST);
          collective->setTask(c, std::move(task));
          pos.setNeedsRenderUpdate(true);
        }
      break;
    }
    case UserInputId::CANCEL_TEAM:
      if (getChosenTeam() == input.get<TeamId>()) {
        setChosenTeam(none);
        chosenCreature = none;
      }
      getTeams().cancel(input.get<TeamId>());
      break;
    case UserInputId::SELECT_TEAM: {
      auto teamId = input.get<TeamId>();
      if (getChosenTeam() == teamId) {
        setChosenTeam(none);
        chosenCreature = none;
      } else
        setChosenTeam(teamId, getTeams().getLeader(teamId)->getUniqueId());
      break;
    }
    case UserInputId::ACTIVATE_TEAM:
      if (!getTeams().isActive(input.get<TeamId>()))
        getTeams().activate(input.get<TeamId>());
      else
        getTeams().deactivate(input.get<TeamId>());
      break;
    case UserInputId::TILE_CLICK: {
      Vec2 pos = input.get<Vec2>();
      if (pos.inRectangle(getLevel()->getBounds()))
        onSquareClick(Position(pos, getLevel()));
      break;
    }
    case UserInputId::DRAW_LEVEL_MAP: view->drawLevelMap(this); break;
    case UserInputId::DRAW_WORLD_MAP: getGame()->presentWorldmap(); break;
    case UserInputId::TECHNOLOGY: setChosenLibrary(!chosenLibrary); break;
    case UserInputId::KEEPEROPEDIA: Encyclopedia(buildInfo, collective->getTechnology()).present(view); break;
    case UserInputId::WORKSHOP: {
      int index = input.get<int>();
      if (index < 0 || index >= EnumInfo<WorkshopType>::size)
        setChosenWorkshop(none);
      else {
        WorkshopType type = (WorkshopType) index;
        if (chosenWorkshop == type)
          setChosenWorkshop(none);
        else
          setChosenWorkshop(type);
      }
      break;
    }
    case UserInputId::WORKSHOP_ADD:
      if (chosenWorkshop) {
        collective->getWorkshops().get(*chosenWorkshop).queue(input.get<int>());
        collective->getWorkshops().scheduleItems(collective);
        collective->updateResourceProduction();
      }
      break;
    case UserInputId::LIBRARY_ADD:
      acquireTech(input.get<TechId>());
      break;
    case UserInputId::LIBRARY_CLOSE:
      setChosenLibrary(false);
      break;
    case UserInputId::WORKSHOP_ITEM_ACTION: {
      auto& info = input.get<WorkshopQueuedActionInfo>();
      if (chosenWorkshop) {
        auto& workshop = collective->getWorkshops().get(*chosenWorkshop);
        if (info.itemIndex < workshop.getQueued().size()) {
          switch (info.action) {
            case ItemAction::REMOVE:
              workshop.unqueue(info.itemIndex);
              break;
            case ItemAction::CHANGE_NUMBER: {
              int batchSize = workshop.getQueued()[info.itemIndex].batchSize;
              if (auto number = getView()->getNumber("Change the number of items:",
                  Range(0, 50 * batchSize), batchSize, batchSize)) {
                if (*number > 0)
                  workshop.changeNumber(info.itemIndex, *number / batchSize);
                else
                  workshop.unqueue(info.itemIndex);
              }
              break;
            }
            default:
              break;
          }
          collective->getWorkshops().scheduleItems(collective);
          collective->updateResourceProduction();
        }
      }
      break;
    }
    case UserInputId::CREATURE_GROUP_BUTTON:
      if (Creature* c = getCreature(input.get<Creature::Id>()))
        if (!chosenCreature || getChosenTeam() || !getCreature(*chosenCreature) ||
            getCreature(*chosenCreature)->getName().stack() != c->getName().stack()) {
          setChosenTeam(none);
          setChosenCreature(input.get<Creature::Id>());
          break;
        }
      setChosenTeam(none);
      chosenCreature = none;
      break;
    case UserInputId::CREATURE_MAP_CLICK: {
      if (Creature* c = Position(input.get<Vec2>(), getLevel()).getCreature()) {
        if (getCreatures().contains(c)) {
          if (!getChosenTeam() || !getTeams().contains(*getChosenTeam(), c))
            setChosenCreature(c->getUniqueId());
          else
            setChosenTeam(*chosenTeam, c->getUniqueId());
        } else
          c->toggleCaptureOrder();
      }
      break;
    }
    case UserInputId::CREATURE_BUTTON: {
      auto chosenId = input.get<Creature::Id>();
      if (Creature* c = getCreature(chosenId)) {
        if (!getChosenTeam() || !getTeams().contains(*getChosenTeam(), c))
          setChosenCreature(chosenId);
        else
          setChosenTeam(*chosenTeam, chosenId);
      }
      else
        setChosenTeam(none);
      break;
    }
    case UserInputId::CREATURE_TASK_ACTION:
      minionTaskAction(input.get<TaskActionInfo>());
      break;
    case UserInputId::CREATURE_EQUIPMENT_ACTION:
      minionEquipmentAction(input.get<EquipmentActionInfo>());
      break;
    case UserInputId::CREATURE_CONTROL:
      if (Creature* c = getCreature(input.get<Creature::Id>())) {
        if (getChosenTeam() && getTeams().exists(*getChosenTeam())) {
          getTeams().setLeader(*getChosenTeam(), c);
          commandTeam(*getChosenTeam());
        } else
          controlSingle(c);
        chosenCreature = none;
        setChosenTeam(none);
      }
      break;
    case UserInputId::CREATURE_RENAME:
      if (Creature* c = getCreature(input.get<RenameActionInfo>().creature))
        c->getName().setFirst(input.get<RenameActionInfo>().name);
      break;
    case UserInputId::CREATURE_CONSUME:
      if (Creature* c = getCreature(input.get<Creature::Id>())) {
        if (auto creatureId = getView()->chooseCreature("Choose minion to absorb",
            getConsumptionTargets(c).transform(
                [] (const Creature* c) { return CreatureInfo(c);}), "cancel"))
          if (Creature* consumed = getCreature(*creatureId))
            collective->setTask(c, Task::consume(consumed));
      }
      break;
    case UserInputId::CREATURE_BANISH:
      if (Creature* c = getCreature(input.get<Creature::Id>()))
        if (getView()->yesOrNoPrompt("Do you want to banish " + c->getName().the() + " forever? "
            "Banishing has a negative impact on morale of other minions.")) {
          vector<Creature*> like = getMinionsLike(c);
          sortMinionsForUI(like);
          if (like.size() > 1)
            for (int i : All(like))
              if (like[i] == c) {
                if (i < like.size() - 1)
                  setChosenCreature(like[i + 1]->getUniqueId());
                else
                  setChosenCreature(like[like.size() - 2]->getUniqueId());
                break;
              }
          collective->banishCreature(c);
        }
      break;
    case UserInputId::GO_TO_ENEMY:
      for (Vec2 v : getVisibleEnemies())
        if (Creature* c = Position(v, getCurrentLevel()).getCreature())
          setScrollPos(c->getPosition());
      break;
    case UserInputId::ADD_GROUP_TO_TEAM: {
      auto info = input.get<TeamCreatureInfo>();
      if (Creature* creature = getCreature(info.creatureId)) {
        vector<Creature*> group = getMinionsLike(creature);
        for (Creature* c : group)
          if (getTeams().exists(info.team) && !getTeams().contains(info.team, c) &&
              (collective->hasTrait(c, MinionTrait::FIGHTER) || c == collective->getLeader()))
            getTeams().add(info.team, c);
      }
      break;
    }
    case UserInputId::ADD_TO_TEAM: {
      auto info = input.get<TeamCreatureInfo>();
      if (Creature* c = getCreature(info.creatureId))
        if (getTeams().exists(info.team) && !getTeams().contains(info.team, c) && canControlInTeam(c))
          getTeams().add(info.team, c);
      break;
    }
    case UserInputId::REMOVE_FROM_TEAM: {
      auto info = input.get<TeamCreatureInfo>();
      if (Creature* c = getCreature(info.creatureId))
        if (getTeams().exists(info.team) && getTeams().contains(info.team, c)) {
          getTeams().remove(info.team, c);
          if (getTeams().exists(info.team)) {
            if (chosenCreature == info.creatureId)
              setChosenTeam(info.team, getTeams().getLeader(info.team)->getUniqueId());
          } else
            chosenCreature = none;
        }
      break;
    }
    case UserInputId::ASSIGN_QUARTERS: {
      auto& info = input.get<AssignQuartersInfo>();
      collective->getQuarters().assign(info.index, info.minionId);
      break;
    }
    case UserInputId::IMMIGRANT_ACCEPT: {
      int index = input.get<int>();
      if (index < 0)
        acceptPrisoner(index);
      else {
        auto available = collective->getImmigration().getAvailable();
        if (auto info = getReferenceMaybe(available, index))
          if (auto sound = info->get().getInfo().getSound())
            getView()->addSound(*sound);
        collective->getImmigration().accept(index);
      }
      break;
    }
    case UserInputId::IMMIGRANT_REJECT: {
      int index = input.get<int>();
      if (index < 0)
        rejectPrisoner(index);
      else {
        collective->getImmigration().rejectIfNonPersistent(index);
      }
      break;
    }
    case UserInputId::IMMIGRANT_AUTO_ACCEPT: {
      int id = input.get<int>();
      if (id >= 0) { // Otherwise the player has clicked the dummy prisoner element
        if (!!collective->getImmigration().getAutoState(id))
          collective->getImmigration().setAutoState(id, none);
        else
          collective->getImmigration().setAutoState(id, ImmigrantAutoState::AUTO_ACCEPT);
      }
      break;
    }
    case UserInputId::IMMIGRANT_AUTO_REJECT: {
      int id = input.get<int>();
      if (id >= 0) { // Otherwise the player has clicked the dummy prisoner element
        if (!!collective->getImmigration().getAutoState(id))
          collective->getImmigration().setAutoState(id, none);
        else
          collective->getImmigration().setAutoState(id, ImmigrantAutoState::AUTO_REJECT);
      }
      break;
    }
    case UserInputId::RECT_SELECTION: {
      auto& info = input.get<BuildingInfo>();
      if (buildInfo[info.building].canSelectRectangle()) {
        updateSelectionSquares();
        if (rectSelection) {
          rectSelection->corner2 = info.pos;
        } else
          rectSelection = CONSTRUCT(SelectionInfo, c.corner1 = c.corner2 = info.pos;);
        updateSelectionSquares();
      } else
        handleSelection(info.pos, buildInfo[info.building], false);
      break;
    }
    case UserInputId::RECT_DESELECTION:
      updateSelectionSquares();
      if (rectSelection) {
        rectSelection->corner2 = input.get<Vec2>();
      } else
        rectSelection = CONSTRUCT(SelectionInfo, c.corner1 = c.corner2 = input.get<Vec2>(); c.deselect = true;);
      updateSelectionSquares();
      break;
    case UserInputId::BUILD: {
      auto& info = input.get<BuildingInfo>();
      handleSelection(info.pos, buildInfo[info.building], false);
      break;
    }
    case UserInputId::VILLAGE_ACTION: {
      auto& info = input.get<VillageActionInfo>();
      if (WCollective village = getVillain(info.id))
        switch (info.action) {
          case VillageAction::TRADE:
            handleTrading(village);
            break;
          case VillageAction::PILLAGE:
            handlePillage(village);
            break;
        }
      break;
    }
    case UserInputId::PAY_RANSOM:
      handleRansom(true);
      break;
    case UserInputId::IGNORE_RANSOM:
      handleRansom(false);
      break;
    case UserInputId::SHOW_HISTORY:
      PlayerMessage::presentMessages(getView(), messageHistory);
      break;
    case UserInputId::CHEAT_ATTRIBUTES:
      for (auto resource : ENUM_ALL(CollectiveResourceId))
        collective->returnResource(CostInfo(resource, 1000));
      collective->getDungeonLevel().increaseLevel();
      break;
    case UserInputId::RELOAD_DATA:
      reloadData();
      break;
    case UserInputId::TUTORIAL_CONTINUE:
      if (tutorial)
        tutorial->continueTutorial(getGame());
      break;
    case UserInputId::TUTORIAL_GO_BACK:
      if (tutorial)
        tutorial->goBack();
      break;
    case UserInputId::RECT_CONFIRM:
      if (rectSelection) {
        selection = rectSelection->deselect ? DESELECT : SELECT;
        for (Vec2 v : Rectangle::boundingBox({rectSelection->corner1, rectSelection->corner2}))
          handleSelection(v, buildInfo[input.get<BuildingInfo>().building], true, rectSelection->deselect);
      }
      FALLTHROUGH;
    case UserInputId::RECT_CANCEL:
      updateSelectionSquares();
      rectSelection = none;
      selection = NONE;
      break;
    case UserInputId::EXIT: getGame()->exitAction(); return;
    case UserInputId::IDLE: break;
    case UserInputId::DISMISS_NEXT_WAVE:
      if (auto& enemies = getModel()->getExternalEnemies())
        if (auto nextWave = enemies->getNextWave())
          dismissedNextWaves.insert(enemies->getNextWaveIndex());
      break;
    case UserInputId::DISMISS_WARNING_WINDOW:
      lastWarningDismiss = getModel()->getLocalTime();
      break;
    case UserInputId::SCROLL_TO_HOME:
      getView()->setScrollPos(getPosition());
      break;
    case UserInputId::SCROLL_DOWN_STAIRS:
      scrollStairs(false);
      break;
    case UserInputId::SCROLL_UP_STAIRS:
      scrollStairs(true);
      break;
    default:
      break;
  }
}

void PlayerControl::scrollStairs(bool up) {
  if (!currentLevel)
    currentLevel = getModel()->getTopLevel();
  auto& levels = getModel()->getMainLevels();
  int index = *levels.findElement(currentLevel);
  index += up ? -1 : 1;
  if (index < 0 || index >= levels.size())
    return;
  currentLevel = levels[index];
  getView()->updateView(this, false);
  CHECK(currentLevel);
}

vector<Creature*> PlayerControl::getConsumptionTargets(Creature* consumer) const {
  vector<Creature*> ret;
  for (Creature* c : getCreatures())
    if (consumer->canConsume(c) && c != collective->getLeader())
      ret.push_back(c);
  return ret;
}

void PlayerControl::updateSelectionSquares() {
  if (rectSelection)
    for (Vec2 v : Rectangle::boundingBox({rectSelection->corner1, rectSelection->corner2}))
      Position(v, getLevel()).setNeedsRenderUpdate(true);
}

void PlayerControl::handleSelection(Vec2 pos, const BuildInfo& building, bool rectangle, bool deselectOnly) {
  PROFILE;
  Position position(pos, getLevel());
  for (auto& req : building.requirements)
    if (!BuildInfo::meetsRequirement(collective, req))
      return;
  if (position.isUnavailable())
    return;
  if (!deselectOnly && rectangle && !building.canSelectRectangle())
    return;
  building.type.visit(
    [&](const BuildInfo::Trap& trap) {
      if (collective->getConstructions().getTrap(position) && selection != SELECT) {
        collective->removeTrap(position);
        getView()->addSound(SoundId::DIG_UNMARK);
        selection = DESELECT;
        // Does this mean I can remove the order if the trap physically exists?
      } else
      if (position.canEnterEmpty({MovementTrait::WALK}) &&
          collective->getTerritory().contains(position) &&
          !collective->getConstructions().getTrap(position) &&
          !collective->getConstructions().getFurniture(position, FurnitureLayer::MIDDLE) &&
          selection != DESELECT) {
        collective->addTrap(position, trap.type);
        getView()->addSound(SoundId::ADD_CONSTRUCTION);
        selection = SELECT;
      }
    },
    [&](const BuildInfo::DestroyLayers& layers) {
      for (auto layer : layers) {
        auto f = collective->getConstructions().getFurniture(position, layer);
        if (f && !f->isBuilt(position)) {
          collective->removeFurniture(position, layer);
          getView()->addSound(SoundId::DIG_UNMARK);
          selection = SELECT;
        } else
        if (collective->getKnownTiles().isKnown(position) && !position.isBurning()) {
          selection = SELECT;
          collective->destroyOrder(position, layer);
          if (auto f = position.getFurniture(layer))
            if (f->getType() == FurnitureType::TREE_TRUNK)
              position.removeFurniture(f);
          getView()->addSound(SoundId::REMOVE_CONSTRUCTION);
          updateSquareMemory(position);
        }
      }
    },
    [&](const BuildInfo::ForbidZone) {
      if (position.isTribeForbidden(getTribeId()) && selection != SELECT) {
        position.allowMovementForTribe(getTribeId());
        selection = DESELECT;
      }
      else if (!position.isTribeForbidden(getTribeId()) && selection != DESELECT) {
        position.forbidMovementForTribe(getTribeId());
        selection = SELECT;
      }
    },
    [&](const BuildInfo::Dig&) {
      bool markedToDig = collective->isMarked(position) &&
          (collective->getMarkHighlight(position) == HighlightType::DIG ||
           collective->getMarkHighlight(position) == HighlightType::CUT_TREE);
      if (markedToDig && selection != SELECT) {
        collective->cancelMarkedTask(position);
        getView()->addSound(SoundId::DIG_UNMARK);
        selection = DESELECT;
      } else
      if (!markedToDig && selection != DESELECT) {
        if (auto furniture = position.getFurniture(FurnitureLayer::MIDDLE))
          for (auto type : {DestroyAction::Type::CUT, DestroyAction::Type::DIG})
            if (furniture->canDestroy(type)) {
              collective->orderDestruction(position, type);
              getView()->addSound(SoundId::DIG_MARK);
              selection = SELECT;
              break;
            }
      }
    },
    [&](ZoneId zone) {
      auto& zones = collective->getZones();
      if (zones.isZone(position, zone) && selection != SELECT) {
        zones.eraseZone(position, zone);
        selection = DESELECT;
      } else if (selection != DESELECT && !zones.isZone(position, zone) &&
          collective->getKnownTiles().isKnown(position) &&
          zones.canSet(position, zone, collective)) {
        zones.setZone(position, zone);
        selection = SELECT;
      }
    },
    [&](BuildInfo::ClaimTile) {
      if (collective->canClaimSquare(position))
        collective->claimSquare(position);
    },
    [&](BuildInfo::Dispatch) {
      collective->setPriorityTasks(position);
    },
    [&](const BuildInfo::Furniture& info) {
      auto layer = Furniture::getLayer(info.types[0]);
      auto currentPlanned = collective->getConstructions().getFurniture(position, layer);
      if (currentPlanned && currentPlanned->isBuilt(position))
        currentPlanned = none;
      int nextIndex = 0;
      if (currentPlanned) {
        if (auto currentIndex = info.types.findElement(currentPlanned->getFurnitureType()))
          nextIndex = *currentIndex + 1;
        else
          return;
      }
      bool removed = false;
      if (!!currentPlanned && selection != SELECT) {
        collective->removeFurniture(position, layer);
        removed = true;
      }
      while (nextIndex < info.types.size() && !collective->canAddFurniture(position, info.types[nextIndex]))
        ++nextIndex;
      int totalCount = 0;
      for (auto type : info.types)
        totalCount += collective->getConstructions().getTotalCount(type);
      if (nextIndex < info.types.size() && selection != DESELECT &&
          (!info.limit || *info.limit > totalCount)) {
        collective->addFurniture(position, info.types[nextIndex], info.cost, info.noCredit);
        collective->updateResourceProduction();
        selection = SELECT;
        getView()->addSound(SoundId::ADD_CONSTRUCTION);
      } else if (removed) {
        selection = DESELECT;
        getView()->addSound(SoundId::DIG_UNMARK);
      }
    }
  );
}

void PlayerControl::onSquareClick(Position pos) {
  if (collective->getTerritory().contains(pos))
    if (auto furniture = pos.getFurniture(FurnitureLayer::MIDDLE)) {
      if (furniture->getClickType()) {
        furniture->click(pos); // this can remove the furniture
        updateSquareMemory(pos);
      } else {
        if (auto workshopType = CollectiveConfig::getWorkshopType(furniture->getType()))
          setChosenWorkshop(*workshopType);
        if (furniture->getUsageType() == FurnitureUsageType::STUDY)
          setChosenLibrary(!chosenLibrary);
      }
    }
}

double PlayerControl::getAnimationTime() const {
  return getModel()->getLocalTimeDouble();
}

PlayerControl::CenterType PlayerControl::getCenterType() const {
  return CenterType::NONE;
}

const vector<Vec2>& PlayerControl::getUnknownLocations(WConstLevel) const {
  PROFILE;
  return unknownLocations->getOnLevel(getLevel());
}

const Creature* PlayerControl::getKeeper() const {
  return collective->getLeader();
}

Creature* PlayerControl::getKeeper() {
  return collective->getLeader();
}

void PlayerControl::addToMemory(Position pos) {
  if (!pos.needsMemoryUpdate())
    return;
  pos.setNeedsMemoryUpdate(false);
  ViewIndex index;
  getSquareViewIndex(pos, true, index);
  memory->update(pos, index);
}

void PlayerControl::checkKeeperDanger() {
  PROFILE;
  auto controlled = getControlled();
  if (auto keeper = getKeeper()) {
    auto prompt = [&] (const string& reason) {
        return getView()->yesOrNoPrompt(reason + ". Do you want to control " +
            him(keeper->getAttributes().getGender()) + "?");
    };
    if (!keeper->isDead() && !controlled.contains(keeper) &&
        lastControlKeeperQuestion < collective->getGlobalTime() - 50_visible) {
      if (auto lastCombatIntent = keeper->getLastCombatIntent())
        if (lastCombatIntent->time > getGame()->getGlobalTime() - 5_visible) {
          lastControlKeeperQuestion = collective->getGlobalTime();
          if (prompt("The Keeper is engaged in a fight with " + lastCombatIntent->attacker->getName().a())) {
            controlSingle(keeper);
            return;
          }
        }
      auto prompt2 = [&](const string& reason) {
        lastControlKeeperQuestion = collective->getGlobalTime();
        if (prompt(reason)) {
          controlSingle(keeper);
          return;
        }
      };
      if (keeper->isAffected(LastingEffect::POISON))
        prompt2("The Keeper is suffering from poisoning");
      else if (keeper->isAffected(LastingEffect::BLEEDING))
        prompt2("The Keeper is bleeding");
      else if (keeper->getBody().isWounded())
        prompt2("The Keeper is wounded");
    }
  }
}

void PlayerControl::onNoEnemies() {
  getGame()->setCurrentMusic(MusicType::PEACEFUL, false);
}

void PlayerControl::considerNightfallMessage() {
  /*if (getGame()->getSunlightInfo().getState() == SunlightState::NIGHT) {
    if (!isNight) {
      addMessage(PlayerMessage("Night is falling. Killing enemies in their sleep yields double mana.",
            MessagePriority::HIGH));
      isNight = true;
    }
  } else
    isNight = false;*/
}

void PlayerControl::update(bool currentlyActive) {
  updateVisibleCreatures();
  vector<Creature*> addedCreatures;
  vector<WLevel> currentLevels {getLevel()};
  for (auto c : getControlled())
    if (!currentLevels.contains(c->getLevel()))
      currentLevels.push_back(c->getLevel());
  for (WLevel l : currentLevels)
    for (Creature* c : l->getAllCreatures())
      if (!getCreatures().contains(c) && c->getTribeId() == getTribeId() && canSee(c) && !isEnemy(c)) {
        if (!collective->wasBanished(c) && !c->getBody().isMinionFood() && c->getAttributes().getCanJoinCollective()) {
          addedCreatures.push_back(c);
          collective->addCreature(c, {MinionTrait::FIGHTER});
          for (auto controlled : getControlled())
            if ((collective->hasTrait(controlled, MinionTrait::FIGHTER)
                  || controlled == collective->getLeader())
                && c->getPosition().isSameLevel(controlled->getPosition())
                && canControlInTeam(c)
                && !collective->hasTrait(c, MinionTrait::SUMMONED)) {
              addToCurrentTeam(c);
              controlled->privateMessage(PlayerMessage(c->getName().a() + " joins your team.",
                  MessagePriority::HIGH));
              break;
            }
        } else
          if (c->getBody().isMinionFood())
            collective->addCreature(c, {MinionTrait::FARM_ANIMAL, MinionTrait::NO_LIMIT});
      }
  if (!addedCreatures.empty()) {
    collective->addNewCreatureMessage(addedCreatures);
  }
}

WLevel PlayerControl::getCurrentLevel() const {
  if (!currentLevel)
    return getModel()->getTopLevel();
  else
    return currentLevel;
}

bool PlayerControl::isConsideredAttacking(const Creature* c, WConstCollective enemy) {
  if (enemy && enemy->getModel() == getModel())
    return canSee(c) && (collective->getTerritory().contains(c->getPosition()) ||
        collective->getTerritory().getStandardExtended().contains(c->getPosition()));
  else
    return canSee(c) && c->getLevel() == getLevel();
}

const double messageTimeout = 80;

void PlayerControl::updateUnknownLocations() {
  vector<Position> locations;
  for (auto col : getGame()->getCollectives())
    if (!collective->isKnownVillainLocation(col) && !col->isConquered())
      if (auto& pos = col->getTerritory().getCentralPoint())
        locations.push_back(*pos);
  unknownLocations->update(locations);
}

void PlayerControl::considerTransferingLostMinions() {
  if (getGame()->getCurrentModel() == getModel())
    for (auto c : copyOf(getCreatures()))
      if (c->getPosition().getModel() != getModel())
        getGame()->transferCreature(c, getModel());
}

void PlayerControl::tick() {
  PROFILE_BLOCK("PlayerControl::tick");
  updateUnknownLocations();
  considerTransferingLostMinions();
  for (auto& elem : messages)
    elem.setFreshness(max(0.0, elem.getFreshness() - 1.0 / messageTimeout));
  messages = messages.filter([&] (const PlayerMessage& msg) {
      return msg.getFreshness() > 0; });
  considerNightfallMessage();
  if (auto msg = collective->getWarnings().getNextWarning(getModel()->getLocalTime()))
    addMessage(PlayerMessage(*msg, MessagePriority::HIGH));
  checkKeeperDanger();
  for (auto attack : copyOf(ransomAttacks))
    for (const Creature* c : attack.getCreatures())
      if (collective->getTerritory().contains(c->getPosition())) {
        ransomAttacks.removeElement(attack);
        break;
      }
  for (auto attack : copyOf(newAttacks))
    for (const Creature* c : attack.getCreatures())
      if (isConsideredAttacking(c, attack.getAttacker())) {
        addMessage(PlayerMessage("You are under attack by " + attack.getAttackerName() + "!",
            MessagePriority::CRITICAL).setPosition(c->getPosition()));
        getGame()->setCurrentMusic(MusicType::BATTLE, true);
        newAttacks.removeElement(attack);
        if (auto attacker = attack.getAttacker())
          collective->addKnownVillain(attacker);
        notifiedAttacks.push_back(attack);
        if (attack.getRansom())
          ransomAttacks.push_back(attack);
        break;
      }
  auto time = collective->getLocalTime();
  if (getGame()->getOptions()->getBoolValue(OptionId::HINTS) && time > hintFrequency) {
    int numHint = time.getDouble() / hintFrequency.getDouble() - 1;
    if (numHint < hints.size() && !hints[numHint].empty()) {
      addMessage(PlayerMessage(hints[numHint], MessagePriority::HIGH));
      hints[numHint] = "";
    }
  }
}

bool PlayerControl::canSee(const Creature* c) const {
  return canSee(c->getPosition());
}

bool PlayerControl::canSee(Position pos) const {
  return getGame()->getOptions()->getBoolValue(OptionId::SHOW_MAP) || visibilityMap->isVisible(pos);
}

TribeId PlayerControl::getTribeId() const {
  return collective->getTribeId();
}

bool PlayerControl::isEnemy(const Creature* c) const {
  auto keeper = getKeeper();
  return c->getTribeId() != getTribeId() && keeper && keeper->isEnemy(c);
}

void PlayerControl::onMemberKilled(const Creature* victim, const Creature* killer) {
  if (victim->isPlayer() && victim != getKeeper())
    onControlledKilled(victim);
  visibilityMap->remove(victim);
  if (victim == getKeeper() && !getGame()->isGameOver()) {
    getGame()->gameOver(victim, collective->getKills().getSize(), "enemies",
        collective->getDangerLevel() + collective->getPoints());
  }
}

void PlayerControl::onMemberAdded(Creature* c) {
  updateMinionVisibility(c);
  auto team = getControlled();
  if (collective->hasTrait(c, MinionTrait::PRISONER) && !team.empty() &&
      team[0]->getPosition().isSameLevel(c->getPosition()))
    addToCurrentTeam(c);
}

WLevel PlayerControl::getLevel() const {
  return getCurrentLevel();
}

WModel PlayerControl::getModel() const {
  return collective->getModel();
}

WGame PlayerControl::getGame() const {
  return collective->getGame();
}

View* PlayerControl::getView() const {
  return getGame()->getView();
}

void PlayerControl::addAttack(const CollectiveAttack& attack) {
  newAttacks.push_back(attack);
}

void PlayerControl::updateSquareMemory(Position pos) {
  ViewIndex index;
  pos.getViewIndex(index, collective->getLeader()); // use the leader as a generic viewer
  memory->update(pos, index);
}

void PlayerControl::onConstructed(Position pos, FurnitureType type) {
  addToMemory(pos);
  if (type == FurnitureType::EYEBALL)
    visibilityMap->updateEyeball(pos);
}

PController PlayerControl::createMinionController(Creature* c) {
  return ::getMinionController(c, memory, this, controlModeMessages, visibilityMap, unknownLocations, tutorial);
}

static void considerAddingKeeperFloor(Position pos) {
  if (NOTNULL(pos.getFurniture(FurnitureLayer::GROUND))->getViewObject()->id() == ViewId::FLOOR)
    pos.modFurniture(FurnitureLayer::GROUND)->getViewObject()->setId(ViewId::KEEPER_FLOOR);
}

void PlayerControl::onClaimedSquare(Position position) {
  considerAddingKeeperFloor(position);
  position.setNeedsRenderUpdate(true);
  updateSquareMemory(position);
}

void PlayerControl::onDestructed(Position pos, FurnitureType type, const DestroyAction& action) {
  if (action.getType() == DestroyAction::Type::DIG) {
    Vec2 visRadius(3, 3);
    for (Position v : pos.getRectangle(Rectangle(-visRadius, visRadius + Vec2(1, 1)))) {
      collective->addKnownTile(v);
      updateSquareMemory(v);
    }
    considerAddingKeeperFloor(pos);
    pos.setNeedsRenderUpdate(true);
  }
}

void PlayerControl::updateVisibleCreatures() {
  visibleEnemies.clear();
  for (const Creature* c : getCurrentLevel()->getAllCreatures())
    if (canSee(c) && isEnemy(c))
      visibleEnemies.push_back(c->getPosition().getCoord());
}

vector<Vec2> PlayerControl::getVisibleEnemies() const {
  return visibleEnemies;
}

REGISTER_TYPE(ListenerTemplate<PlayerControl>)
