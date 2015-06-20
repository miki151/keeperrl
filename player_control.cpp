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
#include "player.h"
#include "model.h"
#include "statistics.h"
#include "options.h"
#include "technology.h"
#include "music.h"
#include "village_control.h"
#include "pantheon.h"
#include "item.h"
#include "item_factory.h"
#include "creature.h"
#include "square.h"
#include "view_id.h"
#include "collective.h"
#include "effect.h"
#include "trigger.h"
#include "music.h"
#include "location.h"
#include "model_builder.h"

template <class Archive> 
void PlayerControl::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(CollectiveControl)
    & SVAR(memory)
    & SVAR(model)
    & SVAR(showWelcomeMsg)
    & SVAR(lastControlKeeperQuestion)
    & SVAR(startImpNum)
    & SVAR(retired)
    & SVAR(payoutWarning)
    & SVAR(surprises)
    & SVAR(assaultNotifications)
    & SVAR(messages)
    & SVAR(currentWarning)
    & SVAR(hints)
    & SVAR(visibleEnemies)
    & SVAR(visibleFriends)
    & SVAR(notifiedConquered)
    & SVAR(visibilityMap);
}

SERIALIZABLE(PlayerControl);

SERIALIZATION_CONSTRUCTOR_IMPL(PlayerControl);

typedef Collective::ResourceId ResourceId;

PlayerControl::BuildInfo::BuildInfo(SquareInfo info, vector<Requirement> req, const string& h, char key, string group)
    : squareInfo(info), buildType(SQUARE), requirements(req), help(h), hotkey(key), groupName(group) {}

PlayerControl::BuildInfo::BuildInfo(TrapInfo info, vector<Requirement> req, const string& h, char key, string group)
    : trapInfo(info), buildType(TRAP), requirements(req), help(h), hotkey(key), groupName(group) {}

PlayerControl::BuildInfo::BuildInfo(DeityHabitat habitat, CostInfo cost, const string& group, const string& h,
    char key) : squareInfo({SquareType(SquareId::ALTAR, habitat), cost, "To " + Deity::getDeity(habitat)->getName(), false}),
    buildType(SQUARE), help(h), hotkey(key), groupName(group) {}

PlayerControl::BuildInfo::BuildInfo(const Creature* c, CostInfo cost, const string& group, const string& h, char key)
    : squareInfo({SquareType(SquareId::CREATURE_ALTAR, c), cost, "To " + c->getName().bare(), false}),
    buildType(SQUARE), help(h), hotkey(key), groupName(group) {}

PlayerControl::BuildInfo::BuildInfo(BuildType type, const string& h, char key, string group)
    : buildType(type), help(h), hotkey(key), groupName(group) {
  CHECK(contains({DIG, IMP, GUARD_POST, DESTROY, FORBID_ZONE, FETCH, DISPATCH, CLAIM_TILE, TORCH}, type));
}

vector<PlayerControl::BuildInfo> PlayerControl::getBuildInfo() const {
  return getBuildInfo(getLevel(), getTribe());
}

vector<PlayerControl::BuildInfo> PlayerControl::getBuildInfo(const Level* level, const Tribe* tribe) {
  const CostInfo altarCost {ResourceId::STONE, 30};
  const string workshop = "Manufactories";
  vector<BuildInfo> buildInfo {
    BuildInfo(BuildInfo::DIG, "", 'd'),
    BuildInfo({SquareId::STOCKPILE, {ResourceId::GOLD, 0}, "Everything", true}, {},
        "All possible items in your dungeon can be stored here.", 's', "Storage"),
    BuildInfo({SquareId::STOCKPILE_EQUIP, {ResourceId::GOLD, 0}, "Equipment", true}, {},
        "All equipment for your minions can be stored here.", 0, "Storage"),
    BuildInfo({SquareId::STOCKPILE_RES, {ResourceId::GOLD, 0}, "Resources", true}, {},
        "Only wood, iron and granite can be stored here.", 0, "Storage"),
    BuildInfo({SquareId::LIBRARY, {ResourceId::WOOD, 20}, "Library"}, {},
        "Mana is regenerated here.", 'y'),
    BuildInfo({SquareId::THRONE, {ResourceId::GOLD, 800}, "Throne", false, false, 0, 1}, {{RequirementId::VILLAGE_CONQUERED}},
        "Increases population limit by " + toString(ModelBuilder::getThronePopulationIncrease())),
    BuildInfo({SquareId::TREASURE_CHEST, {ResourceId::WOOD, 5}, "Treasure room"}, {},
        "Stores gold."),
    BuildInfo({Collective::getHatcheryType(const_cast<Tribe*>(tribe)),
        {ResourceId::WOOD, 20}, "Pigsty"}, {{RequirementId::TECHNOLOGY, TechId::PIGSTY}}, "Increases minion population limit by up to " +
            toString(ModelBuilder::getPigstyPopulationIncrease()) + ".", 'p'),
    BuildInfo({SquareId::DORM, {ResourceId::WOOD, 10}, "Dormitory"}, {},
        "Humanoid minions place their beds here.", 'm'),
    BuildInfo({SquareId::TRAINING_ROOM, {ResourceId::IRON, 20}, "Training room"}, {},
        "Used to level up your minions.", 't'),
    BuildInfo({SquareId::WORKSHOP, {ResourceId::WOOD, 20}, "Workshop"}, {{RequirementId::TECHNOLOGY, TechId::CRAFTING}},
        "Produces leather equipment, traps, first-aid kits and other.", 'w', workshop),
    BuildInfo({SquareId::FORGE, {ResourceId::IRON, 15}, "Forge"}, {{RequirementId::TECHNOLOGY, TechId::IRON_WORKING}},
        "Produces iron weapons and armor.", 'f', workshop),
    BuildInfo({SquareId::LABORATORY, {ResourceId::STONE, 15}, "Laboratory"}, {{RequirementId::TECHNOLOGY, TechId::ALCHEMY}},
        "Produces magical potions.", 'r', workshop),
    BuildInfo({SquareId::JEWELER, {ResourceId::WOOD, 20}, "Jeweler"}, {{RequirementId::TECHNOLOGY, TechId::JEWELLERY}},
        "Produces magical rings and amulets.", 'j', workshop),
    BuildInfo({SquareId::RITUAL_ROOM, {ResourceId::MANA, 15}, "Ritual room"}, {},
        "Summons various demons to your dungeon."),
    BuildInfo({SquareId::BEAST_LAIR, {ResourceId::WOOD, 12}, "Beast lair"}, {},
        "Beasts have their cages here."),
    BuildInfo({SquareId::CEMETERY, {ResourceId::STONE, 20}, "Graveyard"}, {},
        "Corpses are dragged here. Sometimes an undead creature makes their home here.", 'v'),
    BuildInfo({SquareId::PRISON, {ResourceId::IRON, 20}, "Prison"}, {},
        "Captured enemies are kept here.", 0),
    BuildInfo({SquareId::TORTURE_TABLE, {ResourceId::IRON, 20}, "Torture room"}, {},
        "Can be used to torture prisoners.", 'u')};
/*  for (Deity* deity : Deity::getDeities())
    buildInfo.push_back(BuildInfo(deity->getHabitat(), altarCost, "Shrines",
          deity->getGender().god() + " of " + deity->getEpithetsString(), 0));
  if (level)
    for (const Creature* c : level->getAllCreatures())
      if (c->isWorshipped())
        buildInfo.push_back(BuildInfo(c, altarCost, "Shrines", c->getSpeciesName(), 0));*/
  append(buildInfo, {
 //   BuildInfo(BuildInfo::GUARD_POST, "Place it anywhere to send a minion.", 'p', "Orders"),
    BuildInfo(BuildInfo::CLAIM_TILE, "Claim a tile. Building anything has the same effect.", 0, "Orders"),
    BuildInfo(BuildInfo::FETCH, "Order imps to fetch items from outside the dungeon.", 0, "Orders"),
    BuildInfo(BuildInfo::DISPATCH, "Click on an existing task to give it a high priority.", 'a', "Orders"),
    BuildInfo(BuildInfo::DESTROY, "", 'e', "Orders"),
    BuildInfo(BuildInfo::FORBID_ZONE, "Mark tiles to keep minions from entering.", 'b', "Orders"),
    BuildInfo({{SquareId::TRIBE_DOOR, tribe}, {ResourceId::WOOD, 5}, "Door"},
        {{RequirementId::TECHNOLOGY, TechId::CRAFTING}}, "Click on a built door to lock it.", 'o', "Installations"),
    BuildInfo({SquareId::BRIDGE, {ResourceId::WOOD, 20}, "Bridge"}, {},
      "Build it to pass over water or lava.", 0, "Installations"),
    BuildInfo({{SquareId::BARRICADE, tribe}, {ResourceId::WOOD, 20}, "Barricade"},
      {{RequirementId::TECHNOLOGY, TechId::CRAFTING}}, "", 0, "Installations"),
    BuildInfo(BuildInfo::TORCH, "Place it on tiles next to a wall.", 'c', "Installations"),
    BuildInfo({SquareId::EYEBALL, {ResourceId::MANA, 10}, "Eyeball"}, {},
      "Makes the area around it visible.", 0, "Installations"),
    BuildInfo({SquareId::MINION_STATUE, {ResourceId::GOLD, 300}, "Statue", false, false}, {},
      "Increases minion population limit by " +
            toString(ModelBuilder::getStatuePopulationIncrease()) + ".", 0, "Installations"),
    BuildInfo({SquareId::IMPALED_HEAD, {ResourceId::PRISONER_HEAD, 1}, "Prisoner head", false, true}, {},
        "Impaled head of an executed prisoner. Aggravates enemies.", 0, "Installations"),
    BuildInfo({TrapType::TERROR, "Terror trap", ViewId::TERROR_TRAP}, {{RequirementId::TECHNOLOGY, TechId::TRAPS}},
        "Causes the trespasser to panic.", 0, "Traps"),
    BuildInfo({TrapType::POISON_GAS, "Gas trap", ViewId::GAS_TRAP}, {{RequirementId::TECHNOLOGY, TechId::TRAPS}},
        "Releases a cloud of poisonous gas.", 0, "Traps"),
    BuildInfo({TrapType::ALARM, "Alarm trap", ViewId::ALARM_TRAP}, {{RequirementId::TECHNOLOGY, TechId::TRAPS}},
        "Summons all minions", 0, "Traps"),
    BuildInfo({TrapType::WEB, "Web trap", ViewId::WEB_TRAP}, {{RequirementId::TECHNOLOGY, TechId::TRAPS}},
        "Immobilises the trespasser for some time.", 0, "Traps"),
    BuildInfo({TrapType::BOULDER, "Boulder trap", ViewId::BOULDER}, {{RequirementId::TECHNOLOGY, TechId::TRAPS}},
        "Causes a huge boulder to roll towards the enemy.", 0, "Traps"),
    BuildInfo({TrapType::SURPRISE, "Surprise trap", ViewId::SURPRISE_TRAP}, {{RequirementId::TECHNOLOGY, TechId::TRAPS}},
        "Teleports nearby minions to deal with the trespasser.", 0, "Traps"),
  });
  return buildInfo;
}

vector<PlayerControl::BuildInfo> PlayerControl::libraryInfo {
  BuildInfo(BuildInfo::IMP, "", 'i'),
};

vector<PlayerControl::BuildInfo> PlayerControl::minionsInfo {
};

vector<PlayerControl::RoomInfo> PlayerControl::getRoomInfo() {
  vector<RoomInfo> ret;
  for (BuildInfo bInfo : getBuildInfo(nullptr, nullptr))
    if (bInfo.buildType == BuildInfo::SQUARE)
      ret.push_back({bInfo.squareInfo.name, bInfo.help, bInfo.requirements});
  return ret;
}

typedef Collective::Warning Warning;

string PlayerControl::getWarningText(Collective::Warning w) {
  switch (w) {
    case Warning::DIGGING: return "Dig into the mountain and start building a dungeon.";
    case Warning::RESOURCE_STORAGE: return "Storage room for resources is needed.";
    case Warning::EQUIPMENT_STORAGE: return "Storage room for equipment is needed.";
    case Warning::LIBRARY: return "Build a library to start research.";
    case Warning::BEDS: return "You need to build a dormitory for your minions.";
    case Warning::TRAINING: return "Build a training room for your minions.";
    case Warning::NO_HATCHERY: return "You need to build a pigsty.";
    case Warning::WORKSHOP: return "Build a workshop to produce equipment and traps.";
    case Warning::NO_WEAPONS: return "You need weapons for your minions.";
    case Warning::GRAVES: return "You need a graveyard to collect corpses";
    case Warning::CHESTS: return "You need to build a treasure room.";
    case Warning::NO_PRISON: return "You need to build a prison.";
    case Warning::LARGER_PRISON: return "You need a larger prison.";
    case Warning::TORTURE_ROOM: return "You need to build a torture room.";
//    case Warning::ALTAR: return "You need to build a shrine to sacrifice.";
    case Warning::MORE_CHESTS: return "You need a larger treasure room.";
    case Warning::MANA: return "Kill or torture some innocent beings for more mana.";
    case Warning::MORE_LIGHTS: return "Place some torches to light up your dungeon.";
  }
  return "";
}

static bool seeEverything = false;

const int hintFrequency = 700;
static vector<string> getHints() {
  return {
    "Right click on minions to control them or show information.",
 //   "You can turn these hints off in the settings (F2).",
    "Killing a leader greatly lowers the morale of his tribe and stops immigration.",
    "Your minions' morale is boosted when they are commanded by the Keeper.",
  };
}

PlayerControl::PlayerControl(Collective* col, Model* m, Level* level) : CollectiveControl(col), model(m),
    hints(getHints()), visibilityMap(level->getBounds()) {
  bool hotkeys[128] = {0};
  for (BuildInfo info : getBuildInfo(level, nullptr)) {
    if (info.hotkey) {
      CHECK(!hotkeys[int(info.hotkey)]);
      hotkeys[int(info.hotkey)] = true;
    }
  }
  for (TechInfo info : getTechInfo()) {
    if (info.button.hotkey) {
      CHECK(!hotkeys[int(info.button.hotkey)]);
      hotkeys[int(info.button.hotkey)] = true;
    }
  }
  memory.reset(new map<UniqueEntity<Level>::Id, MapMemory>);
  for (Vec2 v : level->getBounds())
    if (contains({"gold ore", "iron ore", "granite"}, level->getSafeSquare(v)->getName()))
      getMemory(level).addObject(v, level->getSafeSquare(v)->getViewObject());
  for(const Location* loc : level->getAllLocations())
    if (loc->isMarkedAsSurprise())
      surprises.insert(loc->getMiddle());
  m->getOptions()->addTrigger(OptionId::SHOW_MAP, [&] (bool val) { seeEverything = val; });
}

const int basicImpCost = 20;

Creature* PlayerControl::getControlled() {
  for (TeamId team : getTeams().getActiveTeams()) {
    if (getTeams().getLeader(team)->isPlayer())
      return getTeams().getLeader(team);
  }
  return nullptr;
}

void PlayerControl::leaveControl() {
  Creature* controlled = getControlled();
  if (controlled == getKeeper())
    lastControlKeeperQuestion = getCollective()->getTime();
  CHECK(controlled);
  if (controlled->getLevel() != getLevel())
    model->getView()->resetCenter();
  if (controlled->isPlayer())
    controlled->popController();
  for (TeamId team : getTeams().getActiveTeams(controlled))
    if (!getTeams().isPersistent(team)) {
      if (getTeams().getMembers(team).size() == 1)
        getTeams().cancel(team);
      else
        getTeams().deactivate(team);
      break;
    }
  model->getView()->stopClock();
}

void PlayerControl::render(View* view) {
  if (retired)
    return;
  if (firstRender) {
    firstRender = false;
    initialize();
  }
  if (!getControlled()) {
    ViewObject::setHallu(false);
    view->updateView(this, false);
  }
  if (showWelcomeMsg && model->getOptions()->getBoolValue(OptionId::HINTS)) {
    view->updateView(this, false);
    showWelcomeMsg = false;
    view->presentText("", "So warlock,\n \nYou were dabbling in the Dark Arts, a tad, I see.\n \n "
        "Welcome to the valley of " + model->getWorldName() + ", where you'll have to do "
        "what you can to KEEP yourself together. Build rooms, storage units and workshops to endorse your "
        "minions. The only way to go forward in this world is to destroy the ones who oppose you.\n \n"
"Use the mouse to dig into the mountain. You can select rectangular areas using the shift key. You will need access to trees, iron, stone and gold ore. Build rooms and traps and prepare for war. You can control a minion at any time by clicking on them in the minions tab or on the map.\n \n You can turn these messages off in the settings (press F2).");
  }
}

bool PlayerControl::isTurnBased() {
  return retired || getControlled();
}

void PlayerControl::retire() {
  if (getControlled())
    leaveControl();
  retired = true;
}

ViewId PlayerControl::getResourceViewId(ResourceId id) const {
  switch (id) {
    case ResourceId::CORPSE:
    case ResourceId::PRISONER_HEAD:
    case ResourceId::MANA: return ViewId::MANA;
    case ResourceId::GOLD: return ViewId::GOLD;
    case ResourceId::WOOD: return ViewId::WOOD_PLANK;
    case ResourceId::IRON: return ViewId::IRON_ROCK;
    case ResourceId::STONE: return ViewId::ROCK;
  }
}

static vector<ItemType> marketItems {
  {ItemId::POTION, EffectId::HEAL},
  {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SLEEP)},
  {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::BLIND)},
  {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::INVISIBLE)},
  {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::POISON)},
  {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::POISON_RESISTANT)},
  {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::FLYING)},
  {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SLOWED)},
  {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SPEED)},
  ItemId::WARNING_AMULET,
  ItemId::HEALING_AMULET,
  ItemId::DEFENSE_AMULET,
  {ItemId::RING, LastingEffect::FIRE_RESISTANT},
  {ItemId::RING, LastingEffect::POISON_RESISTANT},
};

Creature* PlayerControl::getConsumptionTarget(View* view, Creature* consumer) {
  vector<Creature*> res;
  vector<View::ListElem> opt;
  for (Creature* c : getCollective()->getConsumptionTargets(consumer)) {
    res.push_back(c);
    opt.emplace_back(c->getName().bare() + ", level " + toString(c->getExpLevel()));
  }
  if (auto index = view->chooseFromList("Choose minion to absorb:", opt))
    return res[*index];
  return nullptr;
}

void PlayerControl::addConsumableItem(Creature* creature) {
  double scrollPos = 0;
  while (1) {
    const Item* chosenItem = chooseEquipmentItem(creature, {}, [&](const Item* it) {
        return getCollective()->getMinionEquipment().getOwner(it) != creature && !it->canEquip()
        && getCollective()->getMinionEquipment().needs(creature, it, true); }, &scrollPos);
    if (chosenItem)
      getCollective()->getMinionEquipment().own(creature, chosenItem);
    else
      break;
  }
}

void PlayerControl::addEquipment(Creature* creature, EquipmentSlot slot) {
  vector<Item*> currentItems = creature->getEquipment().getItem(slot);
  const Item* chosenItem = chooseEquipmentItem(creature, currentItems, [&](const Item* it) {
      return getCollective()->getMinionEquipment().getOwner(it) != creature
      && creature->canEquipIfEmptySlot(it, nullptr) && it->getEquipmentSlot() == slot; });
  if (chosenItem) {
    if (Creature* c = const_cast<Creature*>(getCollective()->getMinionEquipment().getOwner(chosenItem)))
      c->removeEffect(LastingEffect::SLEEP);
    if (chosenItem->getEquipmentSlot() != EquipmentSlot::WEAPON
        || creature->isEquipmentAppropriate(chosenItem)
        || model->getView()->yesOrNoPrompt(chosenItem->getTheName() + " is too heavy for " +
          creature->getName().the() + ", and will incur an accuracy penaulty.\n Do you want to continue?"))
      getCollective()->getMinionEquipment().own(creature, chosenItem);
  }
}

void PlayerControl::minionEquipmentAction(Creature* creature, const View::MinionAction::ItemAction& action) {
  switch (action.action) {
    case GameInfo::ItemInfo::Action::DROP:
      for (auto id : action.ids)
        getCollective()->getMinionEquipment().discard(id);
      break;
    case GameInfo::ItemInfo::Action::REPLACE:
      if (action.slot)
        addEquipment(creature, *action.slot);
      else
        addConsumableItem(creature);
    case GameInfo::ItemInfo::Action::LOCK:
      for (auto id : action.ids)
        getCollective()->getMinionEquipment().setLocked(creature, id, true);
      break;
    case GameInfo::ItemInfo::Action::UNLOCK:
      for (auto id : action.ids)
        getCollective()->getMinionEquipment().setLocked(creature, id, false);
      break;
    default: 
      break;
  }
}

vector<GameInfo::PlayerInfo> PlayerControl::getMinionGroup(Creature* like) {
  vector<GameInfo::PlayerInfo> minions;
  for (Creature* c : getCreatures())
    if (c->getSpeciesName() == like->getSpeciesName()) {
      minions.emplace_back();
      minions.back().readFrom(c);
      for (MinionTask t : c->getMinionTasks().getAll()) {
        minions.back().minionTasks.push_back({t,
            !getCollective()->isMinionTaskPossible(c, t),
            getCollective()->getMinionTask(c) == t});
      }
      minions.back().creatureId = c->getUniqueId();
      if (getCollective()->usesEquipment(c))
        fillEquipment(c, minions.back());
      minions.back().actions = { GameInfo::PlayerInfo::CONTROL, GameInfo::PlayerInfo::RENAME };
      if (!getCollective()->hasTrait(c, MinionTrait::LEADER))
        minions.back().actions.push_back(GameInfo::PlayerInfo::BANISH);
    }
  sort(minions.begin(), minions.end(), [] (const GameInfo::PlayerInfo& m1, const GameInfo::PlayerInfo& m2) {
        return m1.level > m2.level;
      });
  return minions;
}

void PlayerControl::minionView(Creature* creature) {
  UniqueEntity<Creature>::Id currentId = creature->getUniqueId();
  while (1) {
    vector<GameInfo::PlayerInfo> group = getMinionGroup(creature);
    if (group.empty())
      return;
    bool isId = false;
    for (auto& elem : group)
      if (elem.creatureId == currentId) {
        isId = true;
        break;
      }
    if (!isId)
      currentId = group[0].creatureId;
    auto actionInfo = model->getView()->getMinionAction(group, currentId);
    if (!actionInfo)
      return;
    if (Creature* c = getCreature(currentId))
      switch (actionInfo->action.which()) {
        case 0: 
          getCollective()->setMinionTask(c, boost::get<MinionTask>(actionInfo->action));
          break;
        case 1:
          minionEquipmentAction(c, boost::get<View::MinionAction::ItemAction>(actionInfo->action));
          break;
        case 2:
          controlSingle(c);
          return;
        case 3:
          c->setFirstName(boost::get<View::MinionAction::RenameAction>(actionInfo->action).newName);
          break;
        case 4:
          if (model->getView()->yesOrNoPrompt("Do you want to banish " + c->getName().the() + " forever? "
                "Banishing has a negative impact on morale of other minions."))
            getCollective()->banishCreature(c);
          break;
    }
  }
}

static GameInfo::ItemInfo getItemInfo(const vector<Item*>& stack, bool equiped, bool pending, bool locked) {
  return CONSTRUCT(GameInfo::ItemInfo,
    c.name = stack[0]->getShortName(true);
    c.fullName = stack[0]->getNameAndModifiers(false);
    c.description = stack[0]->getDescription();
    c.number = stack.size();
    if (stack[0]->canEquip())
      c.slot = stack[0]->getEquipmentSlot();
    c.viewId = stack[0]->getViewObject().id();
    c.ids = transform2<UniqueEntity<Item>::Id>(stack, [](const Item* it) { return it->getUniqueId();});
    c.actions = {GameInfo::ItemInfo::DROP};
    c.equiped = equiped;
    c.locked = locked;
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
    case EquipmentSlot::AMULET: return ViewId::AMBER_AMULET;
  }
}

static GameInfo::ItemInfo getEmptySlotItem(EquipmentSlot slot) {
  return CONSTRUCT(GameInfo::ItemInfo,
    c.name = "";
    c.fullName = "";
    c.description = "";
    c.slot = slot;
    c.number = 1;
    c.viewId = getSlotViewId(slot);
    c.actions = {GameInfo::ItemInfo::REPLACE};
    c.equiped = false;
    c.pending = false;);
}

void PlayerControl::fillEquipment(Creature* creature, GameInfo::PlayerInfo& info) {
  if (!creature->isHumanoid())
    return;
  int index = 0;
  double scrollPos = 0;
  vector<EquipmentSlot> slots;
  for (auto slot : Equipment::slotTitles)
    slots.push_back(slot.first);
  vector<Item*> ownedItems = getCollective()->getAllItems([this, creature](const Item* it) {
      return getCollective()->getMinionEquipment().getOwner(it) == creature; });
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
      if (!getCollective()->getMinionEquipment().isLocked(creature, items[i]->getUniqueId()))
        getCollective()->getMinionEquipment().discard(items[i]);
    append(slotItems, items);
    append(slotIndex, vector<EquipmentSlot>(items.size(), slot));
    for (Item* item : items) {
      removeElement(ownedItems, item);
      bool equiped = creature->getEquipment().isEquiped(item);
      bool locked = getCollective()->getMinionEquipment().isLocked(creature, item->getUniqueId());
      info.inventory.push_back(getItemInfo({item}, equiped, !equiped, locked));
      info.inventory.back().actions.push_back(locked ? GameInfo::ItemInfo::UNLOCK : GameInfo::ItemInfo::LOCK);
    }
    if (creature->getEquipment().getMaxItems(slot) > items.size()) {
      info.inventory.push_back(getEmptySlotItem(slot));
      slotIndex.push_back(slot);
      slotItems.push_back(nullptr);
    }
  }
  vector<pair<string, vector<Item*>>> consumables = Item::stackItems(ownedItems,
      [&](const Item* it) { if (!creature->getEquipment().hasItem(it)) return " (pending)"; else return ""; } );
  for (auto elem : consumables)
    info.inventory.push_back(getItemInfo(elem.second, false,
          !creature->getEquipment().hasItem(elem.second.at(0)), false));
}

Item* PlayerControl::chooseEquipmentItem(Creature* creature, vector<Item*> currentItems, ItemPredicate predicate,
    double* scrollPos) {
  vector<Item*> availableItems;
  vector<Item*> usedItems;
  vector<Item*> allItems = getCollective()->getAllItems(predicate);
  getCollective()->sortByEquipmentValue(allItems);
  for (Item* item : allItems)
    if (!contains(currentItems, item)) {
      if (getCollective()->getMinionEquipment().getOwner(item))
        usedItems.push_back(item);
      else
        availableItems.push_back(item);
    }
  if (currentItems.empty() && availableItems.empty() && usedItems.empty())
    return nullptr;
  vector<pair<string, vector<Item*>>> usedStacks = Item::stackItems(usedItems,
      [&](const Item* it) {
        const Creature* c = NOTNULL(getCollective()->getMinionEquipment().getOwner(it));
        return " owned by " + c->getName().a() + " (level " + toString(c->getExpLevel()) + ")";});
  vector<Item*> allStacked;
  vector<GameInfo::ItemInfo> options;
  for (Item* it : currentItems)
    options.push_back(getItemInfo({it}, true, false, false));
  for (auto elem : concat(Item::stackItems(availableItems), usedStacks)) {
    options.emplace_back(getItemInfo(elem.second, false, false, false));
    if (const Creature* c = getCollective()->getMinionEquipment().getOwner(elem.second.at(0)))
      options.back().owner = GameInfo::CreatureInfo(c);
    allStacked.push_back(elem.second.front());
  }
  auto creatureId = creature->getUniqueId();
  auto index = model->getView()->chooseItem(getMinionGroup(creature), creatureId, options, scrollPos);
  if (!index)
    return nullptr;
  return concat(currentItems, allStacked)[*index];
}

void PlayerControl::handleMarket(View* view, int prevItem) {
  vector<Vec2> storage = getCollective()->getAllSquares(getCollective()->getEquipmentStorageSquares());
  if (storage.empty()) {
    view->presentText("Information", "You need a storage room to use the market.");
    return;
  }
  vector<View::ListElem> options;
  vector<PItem> items;
  for (ItemType id : marketItems) {
    items.push_back(ItemFactory::fromId(id));
    options.emplace_back(items.back()->getName() + "    $" + toString(items.back()->getPrice()),
        items.back()->getPrice() > getCollective()->numResource(ResourceId::GOLD) ? View::INACTIVE : View::NORMAL);
  }
  auto index = view->chooseFromList("Buy items", options, prevItem);
  if (!index)
    return;
  Vec2 dest = chooseRandom(storage);
  getCollective()->takeResource({ResourceId::GOLD, items[*index]->getPrice()});
  getLevel()->getSafeSquare(dest)->dropItem(std::move(items[*index]));
  view->updateView(this, true);
  handleMarket(view, *index);
}

static string requires(TechId id) {
  return " (requires: " + Technology::get(id)->getName() + ")";
}

void PlayerControl::handlePersonalSpells(View* view) {
  vector<View::ListElem> options {
      View::ListElem("The Keeper can learn spells for use in combat and other situations. ", View::TITLE),
      View::ListElem("You can cast them with 's' when you are in control of the Keeper.", View::TITLE)};
  vector<Spell*> knownSpells = getCollective()->getAvailableSpells();
  for (auto spell : getCollective()->getAllSpells()) {
    View::ElemMod mod = View::NORMAL;
    string suff;
    if (!contains(knownSpells, spell)) {
      mod = View::INACTIVE;
      suff = requires(getCollective()->getNeededTech(spell));
    }
    options.push_back(View::ListElem(spell->getName() + suff, mod).setTip(spell->getDescription()));
  }
  view->presentList("Sorcery", options);
}

int PlayerControl::getNumMinions() const {
  return getCollective()->getCreatures(MinionTrait::FIGHTER).size();
}

int PlayerControl::getMinLibrarySize() const {
  return 2 * getCollective()->getTechnologies().size();
}

void PlayerControl::handleLibrary(View* view) {
  if (getCollective()->getSquares(SquareId::LIBRARY).empty()) {
    view->presentText("", "You need to build a library to start research.");
    return;
  }
  vector<View::ListElem> options;
  bool allInactive = false;
  if (getCollective()->getSquares(SquareId::LIBRARY).size() <= getMinLibrarySize()) {
    allInactive = true;
    options.emplace_back("You need a larger library to continue research.", View::TITLE);
  }
  options.push_back(View::ListElem("You have " 
        + toString(int(getCollective()->numResource(ResourceId::MANA))) + " mana. ", View::TITLE));
  vector<Technology*> techs = filter(Technology::getNextTechs(getCollective()->getTechnologies()),
      [](const Technology* tech) { return tech->canResearch(); });
  for (Technology* tech : techs) {
    int cost = getCollective()->getTechCost(tech);
    string text = tech->getName() + " (" + toString(cost) + " mana)";
    options.push_back(View::ListElem(text, cost <= int(getCollective()->numResource(ResourceId::MANA))
        && !allInactive ? View::NORMAL : View::INACTIVE).setTip(tech->getDescription()));
  }
  options.emplace_back("Researched:", View::TITLE);
  for (Technology* tech : getCollective()->getTechnologies())
    options.push_back(View::ListElem(tech->getName(), View::INACTIVE).setTip(tech->getDescription()));
  auto index = view->chooseFromList("Library", options);
  if (!index)
    return;
  Technology* tech = techs[*index];
  getCollective()->takeResource({ResourceId::MANA, int(getCollective()->getTechCost(tech))});
  getCollective()->acquireTech(tech);
  view->updateView(this, true);
  handleLibrary(view);
}

typedef GameInfo::BandInfo::Button Button;

optional<pair<ViewId, int>> PlayerControl::getCostObj(CostInfo cost) const {
  if (cost.value() > 0 && !Collective::resourceInfo.at(cost.id()).dontDisplay)
    return make_pair(getResourceViewId(cost.id()), cost.value());
  else
    return none;
}

string PlayerControl::getMinionName(CreatureId id) const {
  static map<CreatureId, string> names;
  if (!names.count(id))
    names[id] = CreatureFactory::fromId(id, nullptr)->getName().bare();
  return names.at(id);
}

bool PlayerControl::meetsRequirement(Requirement req) const {
  switch (req.getId()) {
    case RequirementId::TECHNOLOGY:
      return getCollective()->hasTech(req.get<TechId>());
    case RequirementId::VILLAGE_CONQUERED:
      for (const Collective* c : model->getMainVillains())
        if (c->isConquered())
          return true;
      return false;
  }
}

string PlayerControl::getRequirementText(Requirement req) {
  switch (req.getId()) {
    case RequirementId::TECHNOLOGY:
      return Technology::get(req.get<TechId>())->getName();
    case PlayerControl::RequirementId::VILLAGE_CONQUERED:
      return "that at least one enemy village is conquered";
  }
}

vector<Button> PlayerControl::fillButtons(const vector<BuildInfo>& buildInfo) const {
  vector<Button> buttons;
  for (BuildInfo button : buildInfo) {
    switch (button.buildType) {
      case BuildInfo::SQUARE: {
           BuildInfo::SquareInfo& elem = button.squareInfo;
           ViewId viewId = SquareFactory::get(elem.type)->getViewObject().id();
           string description;
           if (elem.cost.value() > 0)
             description = "[" + toString(getCollective()->getSquares(elem.type).size()) + "]";
           int availableNow = !elem.cost.value() ? 1 : getCollective()->numResource(elem.cost.id()) / elem.cost.value();
           if (Collective::resourceInfo.at(elem.cost.id()).dontDisplay && availableNow)
             description += " (" + toString(availableNow) + " available)";
           if (Collective::getSecondarySquare(elem.type))
             viewId = SquareFactory::get(*Collective::getSecondarySquare(elem.type))->getViewObject().id();
           buttons.push_back({viewId, elem.name,
               getCostObj(getRoomCost(elem.type, elem.cost, elem.costExponent)),
               description,
               (elem.noCredit && !availableNow) ?
                  GameInfo::BandInfo::Button::GRAY_CLICKABLE : GameInfo::BandInfo::Button::ACTIVE });
           }
           break;
      case BuildInfo::DIG:
           buttons.push_back({ViewId::DIG_ICON, "Dig or cut tree", none, "", GameInfo::BandInfo::Button::ACTIVE});
           break;
      case BuildInfo::FETCH:
           buttons.push_back({ViewId::FETCH_ICON, "Fetch items", none, "", GameInfo::BandInfo::Button::ACTIVE});
           break;
      case BuildInfo::CLAIM_TILE:
           buttons.push_back({ViewId::KEEPER_FLOOR, "Claim tile", none, "", GameInfo::BandInfo::Button::ACTIVE});
           break;
      case BuildInfo::DISPATCH:
           buttons.push_back({ViewId::IMP, "Prioritize task", none, "", GameInfo::BandInfo::Button::ACTIVE});
           break;
      case BuildInfo::TRAP: {
             BuildInfo::TrapInfo& elem = button.trapInfo;
             int numTraps = getCollective()->getTrapItems(elem.type).size();
             buttons.push_back({elem.viewId, elem.name, none, "(" + toString(numTraps) + " ready)" });
           }
           break;
      case BuildInfo::IMP: {
           buttons.push_back({ViewId::IMP, "Summon imp", make_pair(ViewId::MANA, getImpCost()),
               "[" + toString(
                   getCollective()->getCreatures({MinionTrait::WORKER}, {MinionTrait::PRISONER}).size()) + "]",
               getImpCost() <= getCollective()->numResource(ResourceId::MANA) ?
                  GameInfo::BandInfo::Button::ACTIVE : GameInfo::BandInfo::Button::GRAY_CLICKABLE});
           break; }
      case BuildInfo::DESTROY:
           buttons.push_back({ViewId::DESTROY_BUTTON, "Remove construction", none, "",
                   GameInfo::BandInfo::Button::ACTIVE});
           break;
      case BuildInfo::FORBID_ZONE:
           buttons.push_back({ViewId::FORBID_ZONE, "Forbid zone", none, "", GameInfo::BandInfo::Button::ACTIVE});
           break;
      case BuildInfo::GUARD_POST:
           buttons.push_back({ViewId::GUARD_POST, "Guard post", none, "", GameInfo::BandInfo::Button::ACTIVE});
           break;
      case BuildInfo::TORCH:
           buttons.push_back({ViewId::TORCH, "Torch", none, "", GameInfo::BandInfo::Button::ACTIVE});
           break;
    }
    vector<string> unmetReqText;
    for (auto& req : button.requirements)
      if (!meetsRequirement(req)) {
        unmetReqText.push_back("Requires " + getRequirementText(req) + ".");
        buttons.back().state = GameInfo::BandInfo::Button::INACTIVE;
      }
    buttons.back().help = combineSentences(concat({button.help}, unmetReqText));
    buttons.back().hotkey = button.hotkey;
    buttons.back().groupName = button.groupName;
  }
  return buttons;
}

vector<PlayerControl::TechInfo> PlayerControl::getTechInfo() const {
  vector<TechInfo> ret;
  ret.push_back({{ViewId::MANA, "Sorcery"}, [this](PlayerControl* c, View* view) {c->handlePersonalSpells(view);}});
  ret.push_back({{ViewId::LIBRARY, "Library", 'l'},
      [this](PlayerControl* c, View* view) { c->handleLibrary(view); }});
  ret.push_back({{ViewId::BOOK, "Keeperopedia"},
      [this](PlayerControl* c, View* view) { model->keeperopedia.present(view); }});
  return ret;
}

void PlayerControl::refreshGameInfo(GameInfo& gameInfo) const {
 /* gameInfo.bandInfo.deities.clear();
  for (Deity* deity : Deity::getDeities())
    gameInfo.bandInfo.deities.push_back({deity->getName(), getCollective()->getStanding(deity)});*/
  gameInfo.villageInfo.villages.clear();
  for (const Collective* c : model->getMainVillains())
    gameInfo.villageInfo.villages.push_back(c->getVillageInfo());
  Model::SunlightInfo sunlightInfo = model->getSunlightInfo();
  gameInfo.sunlightInfo = { sunlightInfo.getText(), (int)sunlightInfo.timeRemaining };
  gameInfo.infoType = GameInfo::InfoType::BAND;
  GameInfo::BandInfo& info = gameInfo.bandInfo;
  info.buildings = fillButtons(getBuildInfo());
  info.libraryButtons = fillButtons(libraryInfo);
  //info.tasks = getCollective()->getMinionTaskStrings();
  info.minions.clear();
  info.payoutTimeRemaining = -1;
/*  int payoutTime = getCollective()->getNextPayoutTime();
  if (payoutTime > -1)
    info.payoutTimeRemaining = payoutTime - getCollective()->getTime();
  else
    info.payoutTimeRemaining = -1;
  info.nextPayout = getCollective()->getNextSalaries();*/
  for (Creature* c : getCollective()->getCreaturesAnyOf(
        {MinionTrait::LEADER, MinionTrait::FIGHTER, MinionTrait::PRISONER, MinionTrait::WORKER})) {
    info.minions.push_back(c);
  }
  info.minionCount = getCollective()->getPopulationSize();
  info.minionLimit = getCollective()->getMaxPopulation();
  info.monsterHeader = "Minions: " + toString(info.minionCount) + " / " + toString(info.minionLimit);
  info.enemies.clear();
  for (Vec2 v : getCollective()->getAllSquares())
    if (const Creature* c = getLevel()->getSafeSquare(v)->getCreature())
      if (c->getTribe() != getTribe() && canSee(c))
        info.enemies.push_back(c);
  info.numResource.clear();
  for (auto elem : getCollective()->resourceInfo)
    if (!elem.second.dontDisplay)
      info.numResource.push_back(
          {getResourceViewId(elem.first), getCollective()->numResourcePlusDebt(elem.first), elem.second.name});
  info.warning = "";
  gameInfo.time = getCollective()->getTime();
  info.currentTeam.reset();
  info.teams.clear();
  info.newTeam = newTeam;
  for (int i : All(getTeams().getAll())) {
    TeamId team = getTeams().getAll()[i];
    info.teams.emplace_back();
    for (Creature* c : getTeams().getMembers(team))
      info.teams.back().members.push_back(c->getUniqueId());
    info.teams.back().active = getTeams().isActive(team);
    info.teams.back().id = team;
    if (getCurrentTeam() == team)
      info.currentTeam = i;
  }
  info.techButtons.clear();
  for (TechInfo tech : getTechInfo())
    info.techButtons.push_back(tech.button);
  gameInfo.messageBuffer = messages;
  info.taskMap.clear();
  for (const Task* task : getCollective()->getTaskMap().getAllTasks()) {
    optional<UniqueEntity<Creature>::Id> creature;
    if (const Creature *c = getCollective()->getTaskMap().getOwner(task))
      creature = c->getUniqueId();
    info.taskMap.push_back({task->getDescription(), creature, getCollective()->getTaskMap().isPriorityTask(task)});
  }
}

void PlayerControl::addMessage(const PlayerMessage& msg) {
  messages.push_back(msg);
}

void PlayerControl::addImportantLongMessage(const string& msg, optional<Vec2> pos) {
  if (Creature* c = getControlled())
    c->playerMessage(PlayerMessage(msg, PlayerMessage::CRITICAL));
  for (string s : split(msg, {'.'})) {
    trim(s);
    auto msg = PlayerMessage(s, PlayerMessage::CRITICAL);
    if (pos)
      msg.setPosition(*pos);
    addMessage(msg);
    model->getView()->stopClock();
  }
}

void PlayerControl::initialize() {
  for (Creature* c : getCreatures())
    update(c);
}

void PlayerControl::update(Creature* c) {
  if (!retired && contains(getCreatures(), c)) {
    vector<Vec2> visibleTiles = getCollective()->getLevel()->getVisibleTiles(c);
    visibilityMap.update(c, visibleTiles);
    for (Vec2 pos : visibleTiles) {
      getCollective()->addKnownTile(pos);
      addToMemory(pos);
    }
  }
}

const MapMemory& PlayerControl::getMemory() const {
  return (*memory.get())[getLevel()->getUniqueId()];
}

MapMemory& PlayerControl::getMemory(Level* l) {
  return (*memory.get())[l->getUniqueId()];
}

ViewObject PlayerControl::getTrapObject(TrapType type, bool armed) {
  for (const PlayerControl::BuildInfo& info : getBuildInfo(nullptr, nullptr))
    if (info.buildType == BuildInfo::TRAP && info.trapInfo.type == type) {
      if (!armed)
        return ViewObject(info.trapInfo.viewId, ViewLayer::LARGE_ITEM, "Unarmed " + Item::getTrapName(type) + " trap")
          .setModifier(ViewObject::Modifier::PLANNED);
      else
        return ViewObject(info.trapInfo.viewId, ViewLayer::LARGE_ITEM, Item::getTrapName(type) + " trap");
    }
  FAIL << "trap not found" << int(type);
  return ViewObject(ViewId::EMPTY, ViewLayer::LARGE_ITEM, "Unarmed trap");
}

static const ViewObject& getConstructionObject(SquareType type) {
  static unordered_map<SquareType, ViewObject> objects;
  if (!objects.count(type)) {
    objects.insert(make_pair(type, SquareFactory::get(type)->getViewObject()));
    objects.at(type).setModifier(ViewObject::Modifier::PLANNED);
  }
  return objects.at(type);
}

void PlayerControl::setCurrentTeam(optional<TeamId> team) {
  currentTeam = team;
}

optional<TeamId> PlayerControl::getCurrentTeam() const {
  if (currentTeam && getTeams().exists(*currentTeam))
    return currentTeam;
  else
    return none;
}

void PlayerControl::getSquareViewIndex(const Square* square, bool canSee, ViewIndex& index) const {
  if (canSee)
    square->getViewIndex(index, getCollective()->getTribe());
  else
    index.setHiddenId(square->getViewObject().id());
  if (const Creature* c = square->getCreature())
    if (canSee)
      index.insert(c->getViewObject());
}

void PlayerControl::getViewIndex(Vec2 pos, ViewIndex& index) const {
  const Square* square = getLevel()->getSafeSquare(pos);
  bool canSeePos = canSee(pos);
  getSquareViewIndex(square, canSeePos, index);
  if (!canSeePos && getMemory().hasViewIndex(pos))
    index.mergeFromMemory(getMemory().getViewIndex(pos));
  if (getCollective()->getAllSquares().count(pos) 
      && index.hasObject(ViewLayer::FLOOR_BACKGROUND)
      && index.getObject(ViewLayer::FLOOR_BACKGROUND).id() == ViewId::FLOOR)
    index.getObject(ViewLayer::FLOOR_BACKGROUND).setId(ViewId::KEEPER_FLOOR);
  if (const Creature* c = square->getCreature())
    if (getCurrentTeam() && getTeams().contains(*getCurrentTeam(), c)
        && index.hasObject(ViewLayer::CREATURE))
      index.getObject(ViewLayer::CREATURE).setModifier(ViewObject::Modifier::TEAM_HIGHLIGHT);
  if (getCollective()->isMarked(pos))
    index.setHighlight(getCollective()->getMarkHighlight(pos));
  if (getCollective()->hasPriorityTasks(pos))
    index.setHighlight(HighlightType::PRIORITY_TASK);
  if (square->isTribeForbidden(getCollective()->getTribe()))
    index.setHighlight(HighlightType::FORBIDDEN_ZONE);
  if (rectSelection
      && pos.inRectangle(Rectangle::boundingBox({rectSelection->corner1, rectSelection->corner2})))
    index.setHighlight(rectSelection->deselect ? HighlightType::RECT_DESELECTION : HighlightType::RECT_SELECTION);
  const ConstructionMap& constructions = getCollective()->getConstructions();
  if (!index.hasObject(ViewLayer::LARGE_ITEM)) {
    if (constructions.containsTrap(pos))
      index.insert(getTrapObject(constructions.getTrap(pos).getType(), constructions.getTrap(pos).isArmed()));
    if (getCollective()->isGuardPost(pos))
      index.insert(ViewObject(ViewId::GUARD_POST, ViewLayer::LARGE_ITEM, "Guard post"));
    if (constructions.containsSquare(pos) && !constructions.getSquare(pos).isBuilt())
      index.insert(getConstructionObject(constructions.getSquare(pos).getSquareType()));
  }
  if (constructions.containsTorch(pos) && !constructions.getTorch(pos).isBuilt())
    index.insert(copyOf(Trigger::getTorchViewObject(constructions.getTorch(pos).getAttachmentDir()))
        .setModifier(ViewObject::Modifier::PLANNED));
  if (surprises.count(pos) && !getCollective()->isKnownSquare(pos))
    index.insert(ViewObject(ViewId::UNKNOWN_MONSTER, ViewLayer::CREATURE, "Surprise"));
  if (getCollective()->hasEfficiency(pos) && index.hasObject(ViewLayer::FLOOR))
    index.getObject(ViewLayer::FLOOR).setAttribute(
        ViewObject::Attribute::EFFICIENCY, getCollective()->getEfficiency(pos));
}

optional<Vec2> PlayerControl::getPosition(bool force) const {
  if (force) {
    if (const Creature* keeper = getKeeper())
      return keeper->getPosition();
    else
      return Vec2(0, 0);
  } else {
    if (!scrollPos.empty()) {
      Vec2 ret = scrollPos.front();
      scrollPos.pop();
      return ret;
    } else
      return none;
  }
}

optional<CreatureView::MovementInfo> PlayerControl::getMovementInfo() const {
  return none;
}

enum Selection { SELECT, DESELECT, NONE } selection = NONE;

CostInfo PlayerControl::getRoomCost(SquareType type, CostInfo baseCost, double exponent) const {
  return {baseCost.id(), int(baseCost.value() * pow(2, exponent * 
        getCollective()->getConstructions().getSquareCount(type)))};
}

int PlayerControl::getImpCost() const {
  int numImps = 0;
  for (Creature* c : getCollective()->getCreatures({MinionTrait::WORKER}, {MinionTrait::PRISONER}))
    ++numImps;
  if (numImps < startImpNum)
    return 0;
  return basicImpCost * pow(2, double(numImps - startImpNum) / 5);
}

class MinionController : public Player {
  public:
  MinionController(Creature* c, Model* m, map<UniqueEntity<Level>::Id, MapMemory>* memory, PlayerControl* ctrl,
      vector<Creature*> t) : Player(c, m, false, memory), control(ctrl), team(t) {}

  virtual void onKilled(const Creature* attacker) override {
    model->getView()->updateView(this, false);
    if (model->getView()->yesOrNoPrompt("Display message history?"))
      showHistory();
    //creature->popController(); this makes the controller crash if creature committed suicide
  }

  virtual bool unpossess() override {
    control->leaveControl();
    return true;
  }

  virtual void onFellAsleep() override {
    model->getView()->presentText("Important!", "You fall asleep. You loose control of your minion.");
    unpossess();
  }

  virtual const vector<Creature*> getTeam() const {
    return filter(team, [](const Creature* c) { return !c->isDead(); });
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Player)
      & SVAR(control)
      & SVAR(team);
  }

  SERIALIZATION_CONSTRUCTOR(MinionController);

  private:
  PlayerControl* SERIAL(control);
  vector<Creature*> SERIAL(team);
};

void PlayerControl::controlSingle(const Creature* cr) {
  CHECK(contains(getCreatures(), cr));
  CHECK(!cr->isDead());
  Creature* c = const_cast<Creature*>(cr);
  commandTeam(getTeams().create({c}));
}

bool PlayerControl::canBuildDoor(Vec2 pos) const {
  if (!getLevel()->getSafeSquare(pos)->canConstruct({SquareId::TRIBE_DOOR, getTribe()}))
    return false;
  Rectangle innerRect = getLevel()->getBounds().minusMargin(1);
  auto wallFun = [=](Vec2 pos) {
      return getLevel()->getSafeSquare(pos)->canConstruct(SquareId::FLOOR) ||
          !pos.inRectangle(innerRect); };
  return !getCollective()->getConstructions().containsTrap(pos) && pos.inRectangle(innerRect) && 
      ((wallFun(pos - Vec2(0, 1)) && wallFun(pos - Vec2(0, -1))) ||
       (wallFun(pos - Vec2(1, 0)) && wallFun(pos - Vec2(-1, 0))));
}

bool PlayerControl::canPlacePost(Vec2 pos) const {
  return !getCollective()->isGuardPost(pos) && !getCollective()->getConstructions().containsTrap(pos) &&
      getLevel()->getSafeSquare(pos)->canEnterEmpty({MovementTrait::WALK}) && getCollective()->isKnownSquare(pos);
}
  
Creature* PlayerControl::getCreature(UniqueEntity<Creature>::Id id) {
  for (Creature* c : getCreatures())
    if (c->getUniqueId() == id)
      return c;
  return nullptr;
}

void PlayerControl::handleCreatureButton(Creature* c, View* view) {
  if (!getCurrentTeam() && !newTeam)
    minionView(c);
  else if (getCollective()->hasAnyTrait(c, {MinionTrait::FIGHTER, MinionTrait::LEADER})) {
    if (newTeam) {
      setCurrentTeam(getTeams().create({c}));
      newTeam = false;
    }
    else if (getTeams().contains(*getCurrentTeam(), c)) {
      getTeams().remove(*getCurrentTeam(), c);
      if (!getCurrentTeam())  // team disappeared so it was the last minion in it
        newTeam = true;
    } else
      getTeams().add(*getCurrentTeam(), c);
  }
}

void PlayerControl::commandTeam(TeamId team) {
  if (getControlled())
    leaveControl();
  Creature* c = getTeams().getLeader(team);
  vector<Creature*> members = getTeams().getMembers(team);
  removeElement(members, c);
  c->pushController(PController(new MinionController(c, model, memory.get(), this, members)));
  getTeams().activate(team);
  getCollective()->freeTeamMembers(team);
}

optional<PlayerMessage> PlayerControl::findMessage(PlayerMessage::Id id){
  for (auto& elem : messages)
    if (elem.getUniqueId() == id)
      return elem;
  return none;
}

CollectiveTeams& PlayerControl::getTeams() {
  return getCollective()->getTeams();
}

const CollectiveTeams& PlayerControl::getTeams() const {
  return getCollective()->getTeams();
}

void PlayerControl::processInput(View* view, UserInput input) {
  if (retired)
    return;
  switch (input.getId()) {
    case UserInputId::MESSAGE_INFO:
        if (auto message = findMessage(input.get<int>())) {
          if (auto pos = message->getPosition())
            scrollPos.push(*pos);
          else if (auto id = message->getCreature()) {
            if (const Creature* c = getCreature(*id))
              scrollPos.push(c->getPosition());
          } else if (auto loc = message->getLocation()) {
            vector<Vec2> visible;
            for (Vec2 v : loc->getAllSquares())
              if (getCollective()->isKnownSquare(v))
                visible.push_back(v);
            scrollPos.push(Rectangle::boundingBox(visible).middle());
          }
        }
        break;
    case UserInputId::CONFIRM_TEAM:
        setCurrentTeam(none);
        break;
    case UserInputId::EDIT_TEAM:
        setCurrentTeam(input.get<TeamId>());
        newTeam = false;
        scrollPos.push(getTeams().getLeader(input.get<TeamId>())->getPosition());
        break;
    case UserInputId::CREATE_TEAM:
        newTeam = !newTeam;
        setCurrentTeam(none);
        break;
    case UserInputId::COMMAND_TEAM:
        if (!getCurrentTeam() || getTeams().getMembers(*getCurrentTeam()).empty())
          break;
        commandTeam(*getCurrentTeam());
        setCurrentTeam(none);
        break;
    case UserInputId::CANCEL_TEAM:
        getTeams().cancel(input.get<TeamId>());
        break;
    case UserInputId::ACTIVATE_TEAM:
        if (!getTeams().isActive(input.get<TeamId>()))
          getTeams().activate(input.get<TeamId>());
        else
          getTeams().deactivate(input.get<TeamId>());
        break;
    case UserInputId::SET_TEAM_LEADER:
        if (Creature* c = getCreature(input.get<TeamLeaderInfo>().creatureId()))
          getTeams().setLeader(input.get<TeamLeaderInfo>().team(), c);
        break;
    case UserInputId::MOVE_TO:
        if (currentTeam && getTeams().isActive(*currentTeam) && getCollective()->isKnownSquare(input.get<Vec2>())) {
          getCollective()->freeTeamMembers(*currentTeam);
          getCollective()->setTask(getTeams().getLeader(*currentTeam), Task::goTo(input.get<Vec2>()), true);
          view->continueClock();
        }
        break;
    case UserInputId::DRAW_LEVEL_MAP: view->drawLevelMap(this); break;
    case UserInputId::DEITIES: model->keeperopedia.deity(view, Deity::getDeities()[input.get<int>()]); break;
    case UserInputId::TECHNOLOGY: getTechInfo()[input.get<int>()].butFun(this, view); break;
    case UserInputId::CREATURE_BUTTON: 
        if (Creature* c = getCreature(input.get<int>()))
          handleCreatureButton(c, view);
        break;
    case UserInputId::POSSESS: {
        Vec2 pos = input.get<Vec2>();
        if (pos.inRectangle(getLevel()->getBounds()))
          tryLockingDoor(pos);
        break;
        }
    case UserInputId::RECT_SELECTION:
        if (rectSelection) {
          rectSelection->corner2 = input.get<Vec2>();
        } else
          rectSelection = CONSTRUCT(SelectionInfo, c.corner1 = c.corner2 = input.get<Vec2>(););
        break;
    case UserInputId::RECT_DESELECTION:
        if (rectSelection) {
          rectSelection->corner2 = input.get<Vec2>();
        } else
          rectSelection = CONSTRUCT(SelectionInfo, c.corner1 = c.corner2 = input.get<Vec2>(); c.deselect = true;);
        break;
    case UserInputId::BUILD:
        handleSelection(input.get<BuildingInfo>().pos(),
            getBuildInfo()[input.get<BuildingInfo>().building()], false);
        break;
    case UserInputId::LIBRARY:
        handleSelection(input.get<BuildingInfo>().pos(), libraryInfo[input.get<BuildingInfo>().building()], false);
        break;
    case UserInputId::BUTTON_RELEASE:
        if (rectSelection) {
          selection = rectSelection->deselect ? DESELECT : SELECT;
          for (Vec2 v : Rectangle::boundingBox({rectSelection->corner1, rectSelection->corner2}))
            handleSelection(v, getBuildInfo()[input.get<BuildingInfo>().building()], true, rectSelection->deselect);
        }
        rectSelection = none;
        selection = NONE;
        break;
    case UserInputId::EXIT: model->exitAction(); return;
    case UserInputId::IDLE: break;
    default: break;
  }
}

void PlayerControl::handleSelection(Vec2 pos, const BuildInfo& building, bool rectangle, bool deselectOnly) {
  for (auto& req : building.requirements)
    if (!meetsRequirement(req))
      return;
  if (!getLevel()->inBounds(pos))
    return;
  if (deselectOnly && !contains({BuildInfo::FORBID_ZONE, BuildInfo::DIG, BuildInfo::SQUARE}, building.buildType))
    return;
  switch (building.buildType) {
    case BuildInfo::IMP:
        if (getCollective()->numResource(ResourceId::MANA) >= getImpCost() && selection == NONE && !rectangle) {
          selection = SELECT;
          PCreature imp = CreatureFactory::fromId(CreatureId::IMP, getTribe(),
              MonsterAIFactory::collective(getCollective()));
          for (Square* square : getLevel()->getSquares(concat(pos.neighbors8(true), {pos})))
            if (square->canEnter(imp.get())
                && (canSee(square->getPosition()) || getCollective()->containsSquare(square->getPosition()))) {
              getCollective()->takeResource({ResourceId::MANA, getImpCost()});
              getCollective()->addCreature(std::move(imp), square->getPosition(),
                  {MinionTrait::WORKER, MinionTrait::NO_LIMIT, MinionTrait::NO_EQUIPMENT});
              break;
            }
        }
        break;
    case BuildInfo::TRAP:
        if (getCollective()->getConstructions().containsTrap(pos)) {
          getCollective()->removeTrap(pos);// Does this mean I can remove the order if the trap physically exists?
        } else
        if (canPlacePost(pos) && getCollective()->getSquares(SquareId::FLOOR).count(pos)) {
          getCollective()->addTrap(pos, building.trapInfo.type);
        }
      break;
    case BuildInfo::DESTROY:
        if (getCollective()->isKnownSquare(pos) && !getCollective()->getLevel()->getSafeSquare(pos)->isBurning()) {
          selection = SELECT;
          getCollective()->destroySquare(pos);
          updateSquareMemory(pos);
        }
        break;
    case BuildInfo::FORBID_ZONE:
        for (Square* square : getCollective()->getLevel()->getSquare(pos))
          if (square->isTribeForbidden(getCollective()->getTribe()) && selection != SELECT) {
            square->allowMovementForTribe(getCollective()->getTribe());
            selection = DESELECT;
          } 
          else if (!square->isTribeForbidden(getCollective()->getTribe()) && selection != DESELECT) {
            square->forbidMovementForTribe(getCollective()->getTribe());
            selection = SELECT;
          }
        break;
    case BuildInfo::GUARD_POST:
        if (getCollective()->isGuardPost(pos) && selection != SELECT) {
          getCollective()->removeGuardPost(pos);
          selection = DESELECT;
        }
        else if (canPlacePost(pos) && selection != DESELECT) {
          getCollective()->addGuardPost(pos);
          selection = SELECT;
        }
        break;
    case BuildInfo::TORCH:
        if (getCollective()->isPlannedTorch(pos) && selection != SELECT) {
          getCollective()->removeTorch(pos);
          selection = DESELECT;
        }
        else if (getCollective()->canPlaceTorch(pos) && selection != DESELECT) {
          getCollective()->addTorch(pos);
          selection = SELECT;
        }
        break;
    case BuildInfo::DIG: {
        bool markedToDig = getCollective()->isMarked(pos) &&
            (getCollective()->getMarkHighlight(pos) == HighlightType::DIG ||
             getCollective()->getMarkHighlight(pos) == HighlightType::CUT_TREE);
        if (markedToDig && selection != SELECT) {
          getCollective()->dontDig(pos);
          selection = DESELECT;
        } else
        if (!markedToDig && selection != DESELECT) {
          if (getLevel()->getSafeSquare(pos)->canConstruct(SquareId::TREE_TRUNK)) {
            getCollective()->cutTree(pos);
            selection = SELECT;
          } else
            if (getLevel()->getSafeSquare(pos)->canConstruct(SquareId::FLOOR)
                || !getCollective()->isKnownSquare(pos)) {
              getCollective()->dig(pos);
              selection = SELECT;
            }
        }
        break;
        }
    case BuildInfo::FETCH:
        getCollective()->fetchAllItems(pos);
        break;
    case BuildInfo::CLAIM_TILE:
        if (getCollective()->isKnownSquare(pos)
            && getLevel()->getSafeSquare(pos)->canConstruct(SquareId::STOCKPILE))
          getCollective()->claimSquare(pos);
        break;
    case BuildInfo::DISPATCH:
        getCollective()->setPriorityTasks(pos);
        break;
    case BuildInfo::SQUARE:
        if (getCollective()->getConstructions().containsSquare(pos)) {
          if (selection != SELECT) {
            getCollective()->removeConstruction(pos);
            selection = DESELECT;
          }
        } else {
          BuildInfo::SquareInfo info = building.squareInfo;
          if (getCollective()->isKnownSquare(pos) && getLevel()->getSafeSquare(pos)->canConstruct(info.type) 
              && !getCollective()->getConstructions().containsTrap(pos)
              && (info.type.getId() != SquareId::TRIBE_DOOR || canBuildDoor(pos)) && selection != DESELECT
              && (!info.maxNumber || *info.maxNumber > getCollective()->getSquares(info.type).size()
                  + getCollective()->getConstructions().getSquareCount(info.type))) {
            CostInfo cost = getRoomCost(info.type, info.cost, info.costExponent);
            getCollective()->addConstruction(pos, info.type, cost, info.buildImmediatly, info.noCredit);
            selection = SELECT;
          }
        }
        break;
  }
}

void PlayerControl::tryLockingDoor(Vec2 pos) {
  if (getCollective()->tryLockingDoor(pos))
    updateSquareMemory(pos);
}

double PlayerControl::getTime() const {
  return model->getTime();
}

bool PlayerControl::isRetired() const {
  return retired;
}

const Creature* PlayerControl::getKeeper() const {
  return getCollective()->getLeader();
}

Creature* PlayerControl::getKeeper() {
  return getCollective()->getLeader();
}

void PlayerControl::addToMemory(Vec2 pos) {
  Square* square = getLevel()->getSafeSquare(pos);
  if (!square->needsMemoryUpdate())
    return;
  square->setMemoryUpdated();
  ViewIndex index;
  getSquareViewIndex(square, true, index);
  getMemory(getLevel()).update(pos, index);
}

void PlayerControl::addDeityServant(Deity* deity, Vec2 deityPos, Vec2 victimPos) {
  PTask task = Task::chain(Task::destroySquare(victimPos), Task::disappear());
  PCreature creature = deity->getServant(getLevel()->getModel()->getKillEveryoneTribe()).random(
      MonsterAIFactory::singleTask(std::move(task)));
  for (Square* square : getLevel()->getSquares(concat({deityPos}, deityPos.neighbors8(true))))
    if (square->canEnter(creature.get())) {
      getLevel()->addCreature(square->getPosition(), std::move(creature));
      break;
    }
}

void PlayerControl::considerDeityFight() {
  vector<pair<Vec2, SquareType>> altars;
  vector<pair<Vec2, SquareType>> deityAltars;
  vector<SquareType> altarTypes;
  for (SquareType squareType : getCollective()->getSquareTypes()) {
    const set<Vec2>& squares = getCollective()->getSquares(squareType);
    if (!squares.empty() && contains({SquareId::ALTAR, SquareId::CREATURE_ALTAR}, squareType.getId())) {
      altarTypes.push_back(squareType);
      for (Vec2 v : squares) {
        altars.push_back({v, squareType});
        if (squareType.getId() == SquareId::ALTAR)
          deityAltars.push_back({v, squareType});
      }
    }
  }
  if (altarTypes.size() < 2 || deityAltars.size() == 0)
    return;
  double fightChance = 10000.0 / pow(5, altars.size() - 2);
  if (Random.rollD(fightChance)) {
    pair<Vec2, SquareType> attacking = chooseRandom(deityAltars);
    pair<Vec2, SquareType> victim;
    do {
      victim = chooseRandom(altars);
    } while (victim.second.getId() == SquareId::ALTAR &&
        victim.second == attacking.second);
    addDeityServant(Deity::getDeity(attacking.second.get<DeityHabitat>()), attacking.first, victim.first);
    if (victim.second.getId() == SquareId::ALTAR)
      addDeityServant(Deity::getDeity(victim.second.get<DeityHabitat>()), victim.first, attacking.first);
  }
}

void PlayerControl::checkKeeperDanger() {
  Creature* controlled = getControlled();
  if (!retired && getKeeper() && controlled != getKeeper()) { 
    if ((getKeeper()->wasInCombat(5) || getKeeper()->getHealth() < 1)
        && lastControlKeeperQuestion < getCollective()->getTime() - 50) {
      lastControlKeeperQuestion = getCollective()->getTime();
      if (model->getView()->yesOrNoPrompt("The keeper is in trouble. Do you want to control him?")) {
        controlSingle(getKeeper());
        return;
      }
    }
    if (getKeeper()->isAffected(LastingEffect::POISON)
        && lastControlKeeperQuestion < getCollective()->getTime() - 5) {
      lastControlKeeperQuestion = getCollective()->getTime();
      if (model->getView()->yesOrNoPrompt("The keeper is in trouble. Do you want to control him?")) {
        controlSingle(getKeeper());
        return;
      }
    }
  }
}

const double messageTimeout = 80;
const double warningFrequency = 200;

void PlayerControl::onNoEnemies() {
  model->setCurrentMusic(MusicType::PEACEFUL, false);
}

void PlayerControl::considerNightfall() {
  if (model->getSunlightInfo().state == Model::SunlightInfo::NIGHT) {
    if (!isNight) {
      addMessage(PlayerMessage("Night is falling. Killing enemies in their sleep yields double mana.",
            PlayerMessage::HIGH));
      isNight = true;
    }
  } else
    isNight = false;
}

void PlayerControl::tick(double time) {
  for (auto& elem : messages)
    elem.setFreshness(max(0.0, elem.getFreshness() - 1.0 / messageTimeout));
  messages = filter(messages, [&] (const PlayerMessage& msg) {
      return msg.getFreshness() > 0; });
  considerNightfall();
  for (Warning w : ENUM_ALL(Warning))
    if (getCollective()->isWarning(w)) {
      if (!currentWarning || currentWarning->warning() != w || currentWarning->lastView()+warningFrequency < time) {
        addMessage(PlayerMessage(getWarningText(w), PlayerMessage::HIGH));
        currentWarning = {w, time};
      }
      break;
    }
  if (startImpNum == -1)
    startImpNum = getCollective()->getCreatures(MinionTrait::WORKER).size();
  considerDeityFight();
  checkKeeperDanger();
  if (retired && getKeeper()) {
    if (const Creature* c = getLevel()->getPlayer())
      if (Random.roll(30) && !getCollective()->containsSquare(c->getPosition()))
        c->playerMessage("You sense horrible evil in the " + 
            getCardinalName((getKeeper()->getPosition() - c->getPosition()).getBearing().getCardinalDir()));
  }
  updateVisibleCreatures(getLevel()->getBounds());
  if (getCollective()->hasMinionDebt() && !retired && !payoutWarning) {
    model->getView()->presentText("Warning", "You don't have enough gold for salaries. "
        "Your minions will refuse to work if they are not paid.\n \n"
        "You can get more gold by mining or retrieve it from killed heroes and conquered villages.");
    payoutWarning = true;
  }
  vector<Creature*> addedCreatures;
  for (const Creature* c : visibleFriends)
    if (c->getSpawnType() && !contains(getCreatures(), c) && !getCollective()->wasBanished(c)) {
      addedCreatures.push_back(const_cast<Creature*>(c));
      getCollective()->addCreature(const_cast<Creature*>(c), {MinionTrait::FIGHTER});
    } else  
    if (c->isMinionFood() && !contains(getCreatures(), c))
      getCollective()->addCreature(const_cast<Creature*>(c), {MinionTrait::FARM_ANIMAL, MinionTrait::NO_LIMIT});
  if (!addedCreatures.empty()) {
    getCollective()->addNewCreatureMessage(addedCreatures);
  }
  for (auto assault : copyOf(assaultNotifications))
    for (const Creature* c : assault.second.creatures())
      if (canSee(c)) {
        addImportantLongMessage(assault.second.message(), c->getPosition());
        assaultNotifications.erase(assault.first);
        model->setCurrentMusic(MusicType::BATTLE, true);
        break;
      }
  if (model->getOptions()->getBoolValue(OptionId::HINTS) && time > hintFrequency) {
    int numHint = int(time) / hintFrequency - 1;
    if (numHint < hints.size() && !hints[numHint].empty()) {
      addMessage(PlayerMessage(hints[numHint], PlayerMessage::HIGH));
      hints[numHint] = "";
    }
  }
  for (const Collective* col : model->getMainVillains())
    if (col->isConquered() && !notifiedConquered.count(col)) {
      addImportantLongMessage("You have exterminated the armed forces of " + col->getName() + ". "
          "Make sure to plunder the village and retrieve any valuables.");
      notifiedConquered.insert(col);
    }
}

bool PlayerControl::canSee(const Creature* c) const {
  return canSee(c->getPosition());
}

bool PlayerControl::canSee(Vec2 position) const {
  if (seeEverything)
    return true;
  if (visibilityMap.isVisible(position))
      return true;
  for (Vec2 pos : getCollective()->getSquares(SquareId::EYEBALL))
    if (getLevel()->canSee(pos, position, VisionId::NORMAL))
      return true;
  return false;
}

const Tribe* PlayerControl::getTribe() const {
  return getCollective()->getTribe();
}

Tribe* PlayerControl::getTribe() {
  return getCollective()->getTribe();
}

bool PlayerControl::isEnemy(const Creature* c) const {
  return getKeeper() && getKeeper()->isEnemy(c);
}

void PlayerControl::onPickupEvent(const Creature* c, const vector<Item*>& items) {
  if (c == getControlled() && !getCollective()->hasTrait(c, MinionTrait::WORKER))
    getCollective()->ownItems(c, items);
}

void PlayerControl::onTechBookRead(Technology* tech) {
  if (retired) {
    model->getView()->presentText("Information", "The tome describes the knowledge of " + tech->getName()
        + ", but you do not comprehend it.");
    return;   
  }
  vector<Technology*> nextTechs = Technology::getNextTechs(getCollective()->getTechnologies());
  if (tech == nullptr) {
    if (!nextTechs.empty())
      tech = chooseRandom(nextTechs);
    else
      tech = chooseRandom(Technology::getAll());
  }
  if (!contains(getCollective()->getTechnologies(), tech)) {
    if (!contains(nextTechs, tech))
      model->getView()->presentText("Information", "The tome describes the knowledge of " + tech->getName()
          + ", but you do not comprehend it.");
    else {
      model->getView()->presentText("Information", "You have acquired the knowledge of " + tech->getName());
      getCollective()->acquireTech(tech, true);
    }
  } else {
    model->getView()->presentText("Information", "The tome describes the knowledge of " + tech->getName()
        + ", which you already possess.");
  }
}

MoveInfo PlayerControl::getMove(Creature* c) {
  if (c == getKeeper() && !getCollective()->getAllSquares().empty()
      && getCollective()->getSquares(SquareId::LIBRARY).empty() // after library is built let him wander
      && !getCollective()->containsSquare(c->getPosition()))
    return c->moveTowards(chooseRandom(getCollective()->getAllSquares()));
  else
    return NoMove;
}

void PlayerControl::addKeeper(Creature* c) {
  getCollective()->addCreature(c, {MinionTrait::LEADER});
}

void PlayerControl::addImp(Creature* c) {
  getCollective()->addCreature(c, {MinionTrait::WORKER, MinionTrait::NO_LIMIT, MinionTrait::NO_EQUIPMENT});
}

void PlayerControl::onConqueredLand() {
  if (retired || !getKeeper())
    return;
  model->conquered(*getKeeper()->getFirstName(), getCollective()->getKills(),
      getCollective()->getDangerLevel() + getCollective()->getPoints());
  model->getView()->presentText("", "When you are ready, retire your dungeon and share it online. "
      "Other players will be able to invade it as adventurers. To do this, press Escape and choose \'retire\'.");
}

void PlayerControl::onMemberKilled(const Creature* victim, const Creature* killer) {
  visibilityMap.remove(victim);
  if (!getKeeper() && !retired && !model->isGameOver()) {
    model->gameOver(victim, getCollective()->getKills().size(), "enemies",
        getCollective()->getDangerLevel() + getCollective()->getPoints());
  }
}

const Level* PlayerControl::getLevel() const {
  return getCollective()->getLevel();
}

Level* PlayerControl::getLevel() {
  return getCollective()->getLevel();
}
void PlayerControl::uncoverRandomLocation() {
  const Location* location = nullptr;
  for (auto loc : randomPermutation(getLevel()->getAllLocations()))
    if (!getCollective()->isKnownSquare(loc->getMiddle())) {
      location = loc;
      break;
    }
  double radius = 8.5;
  if (location)
    for (Vec2 v : location->getMiddle().circle(radius))
      if (getLevel()->inBounds(v))
        getCollective()->addKnownTile(v);
}

void PlayerControl::addAssaultNotification(const Collective* col, const vector<Creature*>& c, const string& message) {
  assaultNotifications[col].creatures() = c;
  assaultNotifications[col].message() = message;
}

void PlayerControl::removeAssaultNotification(const Collective* col) {
  assaultNotifications.erase(col);
}

void PlayerControl::onDiscoveredLocation(const Location* loc) {
  if (auto name = loc->getName())
    addMessage(PlayerMessage("Your minions discover the location of " + *name, PlayerMessage::HIGH)
        .setLocation(loc));
  else if (loc->isMarkedAsSurprise())
    addMessage(PlayerMessage("Your minions discover a new location.").setLocation(loc));
}

void PlayerControl::updateSquareMemory(Vec2 pos) {
  ViewIndex index;
  getLevel()->getSafeSquare(pos)->getViewIndex(index, getCollective()->getTribe());
  getMemory(getLevel()).update(pos, index);
}

void PlayerControl::onConstructed(Vec2 pos, SquareType type) {
  updateSquareMemory(pos);
  if (type == SquareId::FLOOR) {
    Vec2 visRadius(3, 3);
    for (Vec2 v : Rectangle(pos - visRadius, pos + visRadius + Vec2(1, 1)).intersection(getLevel()->getBounds())) {
      getCollective()->addKnownTile(v);
      updateSquareMemory(v);
    }
  }
}

void PlayerControl::updateVisibleCreatures(Rectangle range) {
  visibleEnemies.clear();
  visibleFriends.clear();
  for (const Creature* c : getLevel()->getAllCreatures(range)) 
    if (canSee(c)) {
      if (isEnemy(c))
        visibleEnemies.push_back(c);
      else if (c->getTribe() == getTribe())
        visibleFriends.push_back(c);
    }
}

vector<Vec2> PlayerControl::getVisibleEnemies() const {
  return transform2<Vec2>(visibleEnemies, [](const Creature* c) { return c->getPosition(); });
}

template <class Archive>
void PlayerControl::registerTypes(Archive& ar, int version) {
  REGISTER_TYPE(ar, MinionController);
}

REGISTER_TYPES(PlayerControl::registerTypes);

