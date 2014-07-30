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
#include "message_buffer.h"
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
#include "monster.h"
#include "collective.h"

enum class MinionTask { SLEEP,
  GRAVE,
  TRAIN,
  WORKSHOP,
  STUDY,
  LABORATORY,
  PRISON,
  TORTURE,
  SACRIFICE,
  WORSHIP,
  LAIR,
  EXPLORE,
};

RICH_ENUM(MinionType,
  IMP,
  NORMAL,
  UNDEAD,
  GOLEM,
  BEAST,
  KEEPER,
  PRISONER,
);

template <class Archive> 
void PlayerControl::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(CreatureView)
    & SUBCLASS(EventListener)
    & SUBCLASS(Task::Callback)
    & SUBCLASS(CollectiveControl)
    & SVAR(credit)
    & SVAR(technologies)
    & SVAR(numFreeTech)
    & SVAR(minions)
    & SVAR(minionByType)
    & SVAR(markedItems)
    & SVAR(taskMap)
    & SVAR(traps)
    & SVAR(constructions)
    & SVAR(minionTasks)
    & SVAR(minionTaskStrings)
    & SVAR(mySquares)
    & SVAR(myTiles)
    & SVAR(memory)
    & SVAR(knownTiles)
    & SVAR(borderTiles)
    & SVAR(gatheringTeam)
    & SVAR(team)
    & SVAR(teamLevelChanges)
    & SVAR(levelChangeHistory)
    & SVAR(possessed)
    & SVAR(minionEquipment)
    & SVAR(guardPosts)
    & SVAR(mana)
    & SVAR(points)
    & SVAR(model)
    & SVAR(kills)
    & SVAR(showWelcomeMsg)
    & SVAR(delayedPos)
    & SVAR(lastCombat)
    & SVAR(lastControlKeeperQuestion)
    & SVAR(startImpNum)
    & SVAR(retired)
    & SVAR(alarmInfo)
    & SVAR(prisonerInfo)
    & SVAR(executions)
    & SVAR(flyingSectors)
    & SVAR(sectors)
    & SVAR(squareEfficiency)
    & SVAR(surprises)
  CHECK_SERIAL;
}

SERIALIZABLE(PlayerControl);

SERIALIZATION_CONSTRUCTOR_IMPL(PlayerControl);

template <class Archive>
void PlayerControl::TaskMap::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(Task::Mapping)
    & BOOST_SERIALIZATION_NVP(marked)
    & BOOST_SERIALIZATION_NVP(lockedTasks)
    & BOOST_SERIALIZATION_NVP(completionCost)
    & BOOST_SERIALIZATION_NVP(priorityTasks)
    & BOOST_SERIALIZATION_NVP(delayedTasks);
}

SERIALIZABLE(PlayerControl::TaskMap);

template <class Archive>
void PlayerControl::AlarmInfo::serialize(Archive& ar, const unsigned int version) {
   ar& BOOST_SERIALIZATION_NVP(finishTime)
     & BOOST_SERIALIZATION_NVP(position);
}

SERIALIZABLE(PlayerControl::AlarmInfo);

template <class Archive>
void PlayerControl::TrapInfo::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(type)
    & BOOST_SERIALIZATION_NVP(armed)
    & BOOST_SERIALIZATION_NVP(marked);
}

SERIALIZABLE(PlayerControl::TrapInfo);

template <class Archive>
void PlayerControl::ConstructionInfo::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(cost)
    & BOOST_SERIALIZATION_NVP(built)
    & BOOST_SERIALIZATION_NVP(marked)
    & BOOST_SERIALIZATION_NVP(type)
    & BOOST_SERIALIZATION_NVP(task);
}

SERIALIZABLE(PlayerControl::ConstructionInfo);

template <class Archive>
void PlayerControl::GuardPostInfo::serialize(Archive& ar, const unsigned int version) {
  ar & BOOST_SERIALIZATION_NVP(attender);
}

SERIALIZABLE(PlayerControl::GuardPostInfo);

template <class Archive>
void PlayerControl::CostInfo::serialize(Archive& ar, const unsigned int version) {
  ar & BOOST_SERIALIZATION_NVP(id) & BOOST_SERIALIZATION_NVP(value);
}

SERIALIZABLE(PlayerControl::CostInfo);

template <class Archive>
void PlayerControl::PrisonerInfo::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(state)
    & BOOST_SERIALIZATION_NVP(marked);
}

SERIALIZABLE(PlayerControl::PrisonerInfo);

PlayerControl::BuildInfo::BuildInfo(SquareInfo info, Optional<TechId> id, const string& h, char key, string group)
    : squareInfo(info), buildType(SQUARE), techId(id), help(h), hotkey(key), groupName(group) {}

PlayerControl::BuildInfo::BuildInfo(TrapInfo info, Optional<TechId> id, const string& h, char key, string group)
    : trapInfo(info), buildType(TRAP), techId(id), help(h), hotkey(key), groupName(group) {}

PlayerControl::BuildInfo::BuildInfo(DeityHabitat habitat, CostInfo cost, const string& group, const string& h,
    char key) : squareInfo({SquareType({habitat}), cost, "To " + Deity::getDeity(habitat)->getName(), false}),
    buildType(SQUARE), help(h), hotkey(key), groupName(group) {}

PlayerControl::BuildInfo::BuildInfo(const Creature* c, CostInfo cost, const string& group, const string& h, char key)
    : squareInfo({SquareType({c}), cost, "To " + c->getName(), false}),
    buildType(SQUARE), help(h), hotkey(key), groupName(group) {}

PlayerControl::BuildInfo::BuildInfo(BuildType type, const string& h, char key, string group)
    : buildType(type), help(h), hotkey(key), groupName(group) {
  CHECK(contains({DIG, IMP, GUARD_POST, DESTROY, FETCH, DISPATCH}, type));
}

PlayerControl::BuildInfo::BuildInfo(BuildType type, SquareInfo info, const string& h, char key, string group) 
  : squareInfo(info), buildType(type), help(h), hotkey(key), groupName(group) {
  CHECK(type == IMPALED_HEAD);
}

vector<PlayerControl::BuildInfo> PlayerControl::getBuildInfo() const {
  return getBuildInfo(getLevel());
}

vector<PlayerControl::BuildInfo> PlayerControl::getBuildInfo(const Level* level) {
  const CostInfo altarCost {ResourceId::STONE, 30};
  vector<BuildInfo> buildInfo {
    BuildInfo(BuildInfo::DIG, "", 'd'),
    BuildInfo({SquareType::STOCKPILE, {ResourceId::GOLD, 0}, "Everything", true}, Nothing(), "", 's', "Storage"),
    BuildInfo({SquareType::STOCKPILE_EQUIP, {ResourceId::GOLD, 0}, "Equipment", true}, Nothing(), "", 0, "Storage"),
    BuildInfo({SquareType::STOCKPILE_RES, {ResourceId::GOLD, 0}, "Resources", true}, Nothing(), "", 0, "Storage"),
    BuildInfo({SquareType::TREASURE_CHEST, {ResourceId::WOOD, 5}, "Treasure room"}, Nothing(), ""),
    BuildInfo({SquareType::DORM, {ResourceId::WOOD, 10}, "Dormitory"}, Nothing(), "", 'm'),
    BuildInfo({SquareType::TRAINING_ROOM, {ResourceId::IRON, 20}, "Training room"}, Nothing(), "", 't'),
    BuildInfo({SquareType::LIBRARY, {ResourceId::WOOD, 20}, "Library"}, Nothing(), "", 'y'),
    BuildInfo({SquareType::LABORATORY, {ResourceId::STONE, 15}, "Laboratory"}, TechId::ALCHEMY, "", 'r', "Workshops"),
    BuildInfo({SquareType::WORKSHOP, {ResourceId::IRON, 15}, "Forge"}, TechId::CRAFTING, "", 'f', "Workshops"),
    BuildInfo({SquareType::BEAST_LAIR, {ResourceId::WOOD, 12}, "Beast lair"}, Nothing(), ""),
    BuildInfo({SquareType::CEMETERY, {ResourceId::STONE, 20}, "Graveyard"}, Nothing(), "", 'v'),
    BuildInfo({SquareType::PRISON, {ResourceId::IRON, 20}, "Prison"}, Nothing(), "", 'p'),
    BuildInfo({SquareType::TORTURE_TABLE, {ResourceId::IRON, 20}, "Torture room"}, Nothing(), "", 'u')};
  for (Deity* deity : Deity::getDeities())
    buildInfo.push_back(BuildInfo(deity->getHabitat(), altarCost, "Shrines",
          deity->getGender().god() + " of " + deity->getEpithetsString(), 0));
  if (level)
    for (const Creature* c : level->getAllCreatures())
      if (c->isWorshipped())
        buildInfo.push_back(BuildInfo(c, altarCost, "Shrines", c->getSpeciesName(), 0));
  append(buildInfo, {
    BuildInfo({SquareType::BRIDGE, {ResourceId::WOOD, 20}, "Bridge"}, Nothing(), ""),
    BuildInfo(BuildInfo::GUARD_POST, "Place it anywhere to send a minion.", 0, "Orders"),
    BuildInfo(BuildInfo::FETCH, "Order imps to fetch items from outside the dungeon.", 0, "Orders"),
    BuildInfo(BuildInfo::DISPATCH, "Order imps to prioritize the tasks at location.", 'a', "Orders"),
    BuildInfo(BuildInfo::DESTROY, "", 'e', "Orders"),
    BuildInfo({SquareType::TRIBE_DOOR, {ResourceId::WOOD, 5}, "Door"}, TechId::CRAFTING,
        "Click on a built door to lock it.", 'o', "Installations"),
    BuildInfo({SquareType::BARRICADE, {ResourceId::WOOD, 20}, "Barricade"}, TechId::CRAFTING, "", 0, "Installations"),
    BuildInfo({SquareType::TORCH, {ResourceId::WOOD, 1}, "Torch"}, Nothing(), "", 'c', "Installations"),
    BuildInfo(BuildInfo::IMPALED_HEAD, {SquareType::IMPALED_HEAD, {ResourceId::GOLD, 0}, "Prisoner head"},
        "Impaled head of an executed prisoner. Aggravates enemies.", 0, "Installations"),
    BuildInfo({TrapType::TERROR, "Terror trap", ViewId::TERROR_TRAP}, TechId::TRAPS,
        "Causes the trespasser to panic.", 0, "Traps"),
    BuildInfo({TrapType::POISON_GAS, "Gas trap", ViewId::GAS_TRAP}, TechId::TRAPS,
        "Releases a cloud of poisonous gas.", 0, "Traps"),
    BuildInfo({TrapType::ALARM, "Alarm trap", ViewId::ALARM_TRAP}, TechId::TRAPS,
        "Summons all minions", 0, "Traps"),
    BuildInfo({TrapType::WEB, "Web trap", ViewId::WEB_TRAP}, TechId::TRAPS,
        "Immobilises the trespasser for some time.", 0, "Traps"),
    BuildInfo({TrapType::BOULDER, "Boulder trap", ViewId::BOULDER}, TechId::TRAPS,
        "Causes a huge boulder to roll towards the enemy.", 0, "Traps"),
    BuildInfo({TrapType::SURPRISE, "Surprise trap", ViewId::SURPRISE_TRAP}, TechId::TRAPS,
        "Teleports nearby minions to deal with the trespasser.", 0, "Traps"),
  });
  return buildInfo;
}

vector<PlayerControl::BuildInfo> PlayerControl::workshopInfo {
};

Optional<SquareType> getSecondarySquare(SquareType type) {
  switch (type.id) {
    case SquareType::DORM: return SquareType(SquareType::BED);
    case SquareType::BEAST_LAIR: return SquareType(SquareType::BEAST_CAGE);
    case SquareType::CEMETERY: return SquareType(SquareType::GRAVE);
    default: return Nothing();
  }
}

vector<PlayerControl::BuildInfo> PlayerControl::libraryInfo {
  BuildInfo(BuildInfo::IMP, "", 'i'),
};

vector<PlayerControl::BuildInfo> PlayerControl::minionsInfo {
};

vector<PlayerControl::RoomInfo> PlayerControl::getRoomInfo() {
  vector<RoomInfo> ret;
  for (BuildInfo bInfo : getBuildInfo(nullptr))
    if (bInfo.buildType == BuildInfo::SQUARE)
      ret.push_back({bInfo.squareInfo.name, bInfo.help, bInfo.techId});
  return ret;
}

vector<PlayerControl::RoomInfo> PlayerControl::getWorkshopInfo() {
  vector<RoomInfo> ret;
  for (BuildInfo bInfo : workshopInfo)
    if (bInfo.buildType == BuildInfo::TRAP) {
      BuildInfo::TrapInfo info = bInfo.trapInfo;
      ret.push_back({info.name, bInfo.help, bInfo.techId});
    }
    else if (bInfo.buildType == BuildInfo::IMPALED_HEAD)
      ret.push_back({"Prisoner head", bInfo.help, Nothing()});
  return ret;
}

vector<MinionType> minionTypes {
  MinionType::KEEPER,
  MinionType::IMP,
  MinionType::NORMAL,
  MinionType::UNDEAD,
  MinionType::GOLEM,
  MinionType::BEAST,
  MinionType::PRISONER,
};


RICH_ENUM(PlayerControl::Warning, DIGGING, STORAGE, WOOD, IRON, STONE, GOLD, LIBRARY, MINIONS, BEDS, TRAINING, WORKSHOP, LABORATORY, NO_WEAPONS, GRAVES, CHESTS, NO_PRISON, LARGER_PRISON, TORTURE_ROOM, ALTAR, MORE_CHESTS, MANA);

string PlayerControl::getWarningText(Warning w) {
  switch (w) {
    case Warning::DIGGING: return "Start digging into the mountain to build a dungeon.";
    case Warning::STORAGE: return "You need to build a storage room.";
    case Warning::WOOD: return "Cut down some trees for wood.";
    case Warning::IRON: return "You need to mine more iron.";
    case Warning::STONE: return "You need to mine more stone.";
    case Warning::GOLD: return "You need to mine more gold.";
    case Warning::LIBRARY: return "Build a library to start research.";
    case Warning::MINIONS: return "Use the library tab in the top-right to summon some minions.";
    case Warning::BEDS: return "You need to build beds for your minions.";
    case Warning::TRAINING: return "Build a training room for your minions.";
    case Warning::WORKSHOP: return "Build a workshop to produce equipment and traps.";
    case Warning::LABORATORY: return "Build a laboratory to produce potions.";
    case Warning::NO_WEAPONS: return "You need weapons for your minions.";
    case Warning::GRAVES: return "You need a graveyard to collect corpses";
    case Warning::CHESTS: return "You need to build a treasure room.";
    case Warning::NO_PRISON: return "You need to build a prison.";
    case Warning::LARGER_PRISON: return "You need a larger prison.";
    case Warning::TORTURE_ROOM: return "You need to build a torture room.";
    case Warning::ALTAR: return "You need to build a shrine to sacrifice.";
    case Warning::MORE_CHESTS: return "You need a larger treasure room.";
    case Warning::MANA: return "Kill or torture some innocent beings for more mana.";
  }
  return "";
}

void PlayerControl::setWarning(Warning w, bool state) {
  warnings[w] = state;
}

const static vector<SquareType> resourceStorage {SquareType::STOCKPILE, SquareType::STOCKPILE_RES};
const static vector<SquareType> equipmentStorage {SquareType::STOCKPILE, SquareType::STOCKPILE_EQUIP};

const map<PlayerControl::ResourceId, PlayerControl::ResourceInfo> PlayerControl::resourceInfo {
  {ResourceId::GOLD, { {SquareType::TREASURE_CHEST}, Item::typePredicate(ItemType::GOLD), ItemId::GOLD_PIECE, "gold",
                       PlayerControl::Warning::GOLD}},
  {ResourceId::WOOD, { resourceStorage, Item::namePredicate("wood plank"),
                       ItemId::WOOD_PLANK, "wood", PlayerControl::Warning::WOOD}},
  {ResourceId::IRON, { resourceStorage, Item::namePredicate("iron ore"),
                       ItemId::IRON_ORE, "iron", PlayerControl::Warning::IRON}},
  {ResourceId::STONE, { resourceStorage, Item::namePredicate("rock"), ItemId::ROCK, "stone",
                       PlayerControl::Warning::STONE}},
};

vector<PlayerControl::ItemFetchInfo> PlayerControl::getFetchInfo() const {
  return {
    {unMarkedItems(ItemType::CORPSE), {SquareType::CEMETERY}, true, {}, Warning::GRAVES},
    {unMarkedItems(ItemType::GOLD), {SquareType::TREASURE_CHEST}, false, {}, Warning::CHESTS},
    {[this](const Item* it) {
        return minionEquipment.isItemUseful(it) && !isItemMarked(it);
      }, equipmentStorage, false, {}, Warning::STORAGE},
    {[this](const Item* it) {
        return it->getName() == "wood plank" && !isItemMarked(it); },
    resourceStorage, false, {SquareType::TREE_TRUNK}, Warning::STORAGE},
    {[this](const Item* it) {
        return it->getName() == "iron ore" && !isItemMarked(it); }, resourceStorage, false, {}, Warning::STORAGE},
    {[this](const Item* it) {
        return it->getName() == "rock" && !isItemMarked(it); }, resourceStorage, false, {}, Warning::STORAGE},
  };
}

PlayerControl::MinionTaskInfo::MinionTaskInfo(vector<SquareType> s, const string& desc, Optional<Warning> w, bool c) 
    : type(APPLY_SQUARE), squares(s), description(desc), warning(w), centerOnly(c) {
}

PlayerControl::MinionTaskInfo::MinionTaskInfo(Type t, const string& desc) : type(t), description(desc) {
  CHECK(type == EXPLORE);
}

map<MinionTask, PlayerControl::MinionTaskInfo> PlayerControl::getTaskInfo() const {
  map<MinionTask, MinionTaskInfo> ret {
    {MinionTask::LABORATORY, {{SquareType::LABORATORY}, "lab", PlayerControl::Warning::LABORATORY}},
    {MinionTask::TRAIN, {{SquareType::TRAINING_ROOM}, "training", PlayerControl::Warning::TRAINING}},
    {MinionTask::WORKSHOP, {{SquareType::WORKSHOP}, "crafting", PlayerControl::Warning::WORKSHOP}},
    {MinionTask::SLEEP, {{SquareType::BED}, "sleeping", PlayerControl::Warning::BEDS}},
    {MinionTask::GRAVE, {{SquareType::GRAVE}, "sleeping", PlayerControl::Warning::GRAVES}},
    {MinionTask::LAIR, {{SquareType::BEAST_CAGE}, "sleeping"}},
    {MinionTask::STUDY, {{SquareType::LIBRARY}, "studying", PlayerControl::Warning::LIBRARY}},
    {MinionTask::PRISON, {{SquareType::PRISON}, "prison", PlayerControl::Warning::NO_PRISON}},
    {MinionTask::TORTURE, {{SquareType::TORTURE_TABLE}, "tortured", PlayerControl::Warning::TORTURE_ROOM, true}},
    {MinionTask::EXPLORE, {MinionTaskInfo::EXPLORE, "exploring"}},
    {MinionTask::SACRIFICE, {{}, "sacrificed", PlayerControl::Warning::ALTAR}},
    {MinionTask::WORSHIP, {{}, "worship", Nothing()}}};
  for (auto elem : mySquares)
    if (contains({SquareType::ALTAR, SquareType::CREATURE_ALTAR}, elem.first.id) && !elem.second.empty()) {
      ret.at(MinionTask::WORSHIP).squares.push_back(elem.first);
      ret.at(MinionTask::SACRIFICE).squares.push_back(elem.first);
    }
  return ret;
};

PlayerControl::PlayerControl(Collective* col, Model* m, Level* level) : CollectiveControl(col),
  mana(200), model(m), sectors(new Sectors(level->getBounds())), flyingSectors(new Sectors(level->getBounds())) {
  bool hotkeys[128] = {0};
  for (BuildInfo info : concat(getBuildInfo(level), workshopInfo)) {
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
  memory.reset(new map<UniqueId, MapMemory>);
  // init the map so the values can be safely read with .at()
  mySquares[SquareType::TREE_TRUNK].clear();
  mySquares[SquareType::IMPALED_HEAD].clear();
  mySquares[SquareType::FLOOR].clear();
  mySquares[SquareType::TRIBE_DOOR].clear();
  for (BuildInfo info : concat(getBuildInfo(level), workshopInfo))
    if (info.buildType == BuildInfo::SQUARE) {
      mySquares[info.squareInfo.type].clear();
      if (auto t = getSecondarySquare(info.squareInfo.type))
        mySquares[*t].clear();
    }
  credit = {
    {ResourceId::GOLD, 0},
    {ResourceId::WOOD, 0},
    {ResourceId::IRON, 0},
    {ResourceId::STONE, 0},
  };
  for (MinionType t : minionTypes)
    minionByType[t].clear();
  for (Vec2 v : level->getBounds()) {
    if (level->getSquare(v)->canEnterEmpty(Creature::getDefaultMinion()))
      sectors->add(v);
    if (level->getSquare(v)->canEnterEmpty(Creature::getDefaultMinionFlyer()))
      flyingSectors->add(v);
    if (contains({"gold ore", "iron ore", "stone"}, level->getSquare(v)->getName()))
      getMemory(level).addObject(v, level->getSquare(v)->getViewObject());
  }
  for(const Location* loc : level->getAllLocations())
    if (loc->isMarkedAsSurprise())
      surprises.insert(loc->getBounds().middle());
  knownTiles = Table<bool>(level->getBounds(), false);
}

const int basicImpCost = 20;
const int minionLimit = 40;

void PlayerControl::unpossess() {
  if (possessed == getKeeper())
    lastControlKeeperQuestion = getTime();
  CHECK(possessed);
  if (possessed->isPlayer())
    possessed->popController();
  ViewObject::setHallu(false);
  possessed = nullptr;
 /* team.clear();
  gatheringTeam = false;*/
  teamLevelChanges.clear();
}

void PlayerControl::render(View* view) {
  if (retired)
    return;
  if (possessed && (!possessed->isPlayer() || possessed->isDead())) {
 /*   if (contains(team, possessed))
      removeElement(team, possessed);*/
    if ((possessed->isDead() || possessed->isAffected(LastingEffect::SLEEP)) && !team.empty()) {
      possess(team.front(), view);
    } else {
      view->setTimeMilli(possessed->getTime() * 300);
      view->clearMessages();
      if (possessed->getLevel() != getLevel())
        view->resetCenter();
      unpossess();
    }
  }
  if (!possessed) {
    view->updateView(this);
  } else
    view->stopClock();
  if (showWelcomeMsg && Options::getValue(OptionId::HINTS)) {
    view->updateView(this);
    showWelcomeMsg = false;
    view->presentText("Welcome", "In short: you are a warlock who has been banished from the lawful world for practicing black magic. You are going to claim the land of " + NameGenerator::get(NameGeneratorId::WORLD)->getNext() + " and make it your domain. The best way to achieve this is to kill everyone.\n \n"
"Use the mouse to dig into the mountain. You can select rectangular areas using the shift key. You will need access to trees, iron and gold ore. Build rooms and traps and prepare for war. You can control a minion at any time by clicking on them in the minions tab or on the map.\n \n You can turn these messages off in the options (press F2).");
  }
}

bool PlayerControl::isTurnBased() {
  return retired || (possessed != nullptr && possessed->isPlayer());
}

void PlayerControl::retire() {
  if (possessed)
    unpossess();
  retired = true;
}

vector<pair<Item*, Vec2>> PlayerControl::getTrapItems(TrapType type, set<Vec2> squares) const {
  vector<pair<Item*, Vec2>> ret;
  if (squares.empty())
    squares = mySquares.at(SquareType::WORKSHOP);
  for (Vec2 pos : squares) {
    vector<Item*> v = getLevel()->getSquare(pos)->getItems([type, this](Item* it) {
        return it->getTrapType() == type && !isItemMarked(it); });
    for (Item* it : v)
      ret.emplace_back(it, pos);
  }
  return ret;
}

ViewObject PlayerControl::getResourceViewObject(ResourceId id) const {
  return ItemFactory::fromId(resourceInfo.at(id).itemId)->getViewObject();
}

static vector<ItemId> marketItems {
  ItemId::HEALING_POTION,
  ItemId::SLEEP_POTION,
  ItemId::BLINDNESS_POTION,
  ItemId::INVISIBLE_POTION,
  ItemId::POISON_POTION,
  ItemId::POISON_RESIST_POTION,
  ItemId::LEVITATION_POTION,
  ItemId::SLOW_POTION,
  ItemId::SPEED_POTION,
  ItemId::WARNING_AMULET,
  ItemId::HEALING_AMULET,
  ItemId::DEFENSE_AMULET,
  ItemId::FRIENDLY_ANIMALS_AMULET,
  ItemId::FIRE_RESIST_RING,
  ItemId::POISON_RESIST_RING,
};

MinionType PlayerControl::getMinionType(const Creature* c) const {
  for (auto elem : minionTypes)
    if (contains(minionByType[elem], c))
      return elem;
  FAIL << "Minion type not found " << c->getName();
  return MinionType(0);
}

void PlayerControl::setMinionType(Creature* c, MinionType type) {
  removeElement(minionByType[getMinionType(c)], c);
  minionByType[type].push_back(c);
  if (!contains({MinionType::IMP}, type))
    minionTasks.at(c->getUniqueId()) = getTasksForMinion(c);
}

enum class PlayerControl::MinionOption { POSSESS, EQUIPMENT, INFO, WAKE_UP, PRISON, TORTURE, SACRIFICE, EXECUTE,
  LABOR, TRAINING, WORKSHOP, LAB, STUDY, WORSHIP };

struct TaskOption {
  MinionTask task;
  PlayerControl::MinionOption option;
  string description;
};

vector<TaskOption> taskOptions { 
  {MinionTask::TRAIN, PlayerControl::MinionOption::TRAINING, "Training"},
  {MinionTask::WORKSHOP, PlayerControl::MinionOption::WORKSHOP, "Workshop"},
  {MinionTask::LABORATORY, PlayerControl::MinionOption::LAB, "Lab"},
  {MinionTask::STUDY, PlayerControl::MinionOption::STUDY, "Study"},
  {MinionTask::WORSHIP, PlayerControl::MinionOption::WORSHIP, "Worship"},
};

static string getMoraleString(double morale) {
  return convertToString(morale);
 /* if (morale < -0.33)
    return "low";
  if (morale > 0.33)
    return "high";
  return "normal";*/
}

void PlayerControl::getMinionOptions(Creature* c, vector<MinionOption>& mOpt, vector<View::ListElem>& lOpt) {
  switch (getMinionType(c)) {
    case MinionType::IMP:
      mOpt = {MinionOption::PRISON, MinionOption::TORTURE, MinionOption::SACRIFICE, MinionOption::EXECUTE};
      lOpt = {"Send to prison", "Torture", "Sacrifice", "Execute"};
      break;
    case MinionType::PRISONER:
      switch (prisonerInfo.at(c).state) {
        case PrisonerInfo::EXECUTE:
          lOpt = {View::ListElem("Execution ordered", View::TITLE)};
          break;
        case PrisonerInfo::SACRIFICE:
          lOpt = {View::ListElem("Sacrifice ordered", View::TITLE)};
          break;
        case PrisonerInfo::TORTURE:
        case PrisonerInfo::PRISON:
          mOpt = {MinionOption::PRISON, MinionOption::TORTURE, MinionOption::EXECUTE, MinionOption::SACRIFICE,
            MinionOption::LABOR };
          lOpt = {"Send to prison", "Torture", "Execute", "Sacrifice", "Send to labor" };
          break;
        case PrisonerInfo::SURRENDER: FAIL << "state not handled: " << int(prisonerInfo.at(c).state);
      }
      break;
    case MinionType::BEAST:
      mOpt = {MinionOption::POSSESS, MinionOption::INFO };
      lOpt = {"Possess", "Description" };
      break;
    default:
      mOpt = {MinionOption::POSSESS, MinionOption::EQUIPMENT, MinionOption::INFO };
      lOpt = {"Possess", "Equipment", "Description" };
      lOpt.emplace_back("Order task:", View::TITLE);
      for (auto elem : taskOptions)
        if (minionTasks.at(c->getUniqueId()).containsState(elem.task)) {
          lOpt.push_back(elem.description);
          mOpt.push_back(elem.option);
        }
      if (c->isAffected(LastingEffect::SLEEP)) {
        mOpt.push_back(MinionOption::WAKE_UP);
        lOpt.push_back("Wake up");
      }
      break;
  }
  lOpt.emplace_back("", View::INACTIVE);
  vector<Skill*> skills = c->getSkills();
  if (!skills.empty()) {
    lOpt.emplace_back("Skills:", View::TITLE);
    for (Skill* skill : skills)
      lOpt.emplace_back(skill->getName(), View::INACTIVE);
    lOpt.emplace_back("", View::INACTIVE);
  }
  vector<SpellInfo> spells = c->getSpells();
  if (!spells.empty()) {
    lOpt.emplace_back("Spells:", View::TITLE);
    for (SpellInfo elem : spells)
      lOpt.emplace_back(elem.name, View::INACTIVE);
    lOpt.emplace_back("", View::INACTIVE);
  }
  for (string s : c->getAdjectives())
    lOpt.emplace_back(s, View::INACTIVE);
}

void PlayerControl::setMinionTask(Creature* c, MinionTask task) {
  minionTasks.at(c->getUniqueId()).setState(task);
}

MinionTask PlayerControl::getMinionTask(Creature* c) const {
  return minionTasks.at(c->getUniqueId()).getState();
}

void PlayerControl::minionView(View* view, Creature* creature, int prevIndex) {
  vector<MinionOption> mOpt;
  vector<View::ListElem> lOpt;
  getMinionOptions(creature, mOpt, lOpt);
  auto index = view->chooseFromList(capitalFirst(creature->getNameAndTitle()) 
      + ", level: " + convertToString(creature->getExpLevel()) + ", kills: "
      + convertToString(creature->getKills().size())
      + ", morale: " + getMoraleString(creature->getMorale()),
      lOpt, prevIndex, View::MINION_MENU);
  if (!index)
    return;
  switch (mOpt[*index]) {
    case MinionOption::POSSESS: possess(creature, view); return;
    case MinionOption::EQUIPMENT: handleEquipment(view, creature); break;
    case MinionOption::INFO: messageBuffer.addMessage(MessageBuffer::important(creature->getDescription())); break;
    case MinionOption::WAKE_UP: creature->removeEffect(LastingEffect::SLEEP); return;
    case MinionOption::PRISON:
      setMinionTask(creature, MinionTask::PRISON);
      setMinionType(creature, MinionType::PRISONER);
      return;
    case MinionOption::TORTURE:
      setMinionType(creature, MinionType::PRISONER);
      prisonerInfo.at(creature) = {PrisonerInfo::TORTURE, false};
      setMinionTask(creature, MinionTask::TORTURE);
      return;
    case MinionOption::SACRIFICE:
      setMinionType(creature, MinionType::PRISONER);
      prisonerInfo.at(creature) = {PrisonerInfo::SACRIFICE, false};
      setMinionTask(creature, MinionTask::SACRIFICE);
      return;
    case MinionOption::EXECUTE:
      setMinionType(creature, MinionType::PRISONER);
      prisonerInfo.at(creature) = {PrisonerInfo::EXECUTE, false};
      minionTaskStrings[creature->getUniqueId()] = "execution";
      break;
    case MinionOption::LABOR:
      setMinionType(creature, MinionType::IMP);
      minionTaskStrings[creature->getUniqueId()] = "labor";
      break;
    default:
      for (auto elem : taskOptions)
        if (mOpt[*index] == elem.option) {
          setMinionTask(creature, elem.task);
          creature->removeEffect(LastingEffect::SLEEP);
          return;
        }
      FAIL << "Unknown option " << int(mOpt[*index]);
      break;
  }
  minionView(view, creature, *index);
}

bool PlayerControl::usesEquipment(const Creature* c) const {
  return c->isHumanoid() && getMinionType(c) != MinionType::PRISONER && c->getName() != "gnome";
}

Item* PlayerControl::getWorstItem(vector<Item*> items) const {
  Item* ret = nullptr;
  for (Item* it : items)
    if (ret == nullptr || minionEquipment.getItemValue(it) < minionEquipment.getItemValue(ret))
      ret = it;
  return ret;
}

void PlayerControl::autoEquipment(Creature* creature, bool replace) {
  map<EquipmentSlot, vector<Item*>> slots;
  vector<Item*> myItems = getAllItems([&](const Item* it) {
      return minionEquipment.getOwner(it) == creature && it->canEquip(); });
  for (Item* it : myItems) {
    EquipmentSlot slot = it->getEquipmentSlot();
    if (slots[slot].size() < creature->getEquipment().getMaxItems(slot)) {
      slots[slot].push_back(it);
    } else  // a rare occurence that minion owns too many items of the same slot,
          //should happen only when an item leaves the fortress and then is braught back
      minionEquipment.discard(it);
  }
  for (Item* it : getAllItems([&](const Item* it) {
      return minionEquipment.needs(creature, it, false, replace) && !minionEquipment.getOwner(it); }, false)) {
    if (!it->canEquip()
        || slots[it->getEquipmentSlot()].size() < creature->getEquipment().getMaxItems(it->getEquipmentSlot())
        || minionEquipment.getItemValue(getWorstItem(slots[it->getEquipmentSlot()]))
            < minionEquipment.getItemValue(it)) {
      minionEquipment.own(creature, it);
      if (it->canEquip()
        && slots[it->getEquipmentSlot()].size() == creature->getEquipment().getMaxItems(it->getEquipmentSlot())) {
        Item* removed = getWorstItem(slots[it->getEquipmentSlot()]);
        minionEquipment.discard(removed);
        removeElement(slots[it->getEquipmentSlot()], removed); 
      }
      if (it->canEquip())
        slots[it->getEquipmentSlot()].push_back(it);
      if (it->getType() != ItemType::AMMO)
        break;
    }
  }
}

void PlayerControl::handleEquipment(View* view, Creature* creature, int prevItem) {
  if (!creature->isHumanoid()) {
    view->presentText("", creature->getTheName() + " can't use any equipment.");
    return;
  }
  vector<EquipmentSlot> slots;
  for (auto slot : Equipment::slotTitles)
    slots.push_back(slot.first);
  vector<View::ListElem> list;
  vector<Item*> ownedItems = getAllItems([this, creature](const Item* it) {
      return minionEquipment.getOwner(it) == creature; });
  vector<Item*> slotItems;
  vector<EquipmentSlot> slotIndex;
  for (auto slot : slots) {
    list.emplace_back(Equipment::slotTitles.at(slot), View::TITLE);
    vector<Item*> items;
    for (Item* it : ownedItems)
      if (it->canEquip() && it->getEquipmentSlot() == slot)
        items.push_back(it);
    for (int i = creature->getEquipment().getMaxItems(slot); i < items.size(); ++i)
      // a rare occurence that minion owns too many items of the same slot,
      //should happen only when an item leaves the fortress and then is braught back
      minionEquipment.discard(items[i]);
    append(slotItems, items);
    append(slotIndex, vector<EquipmentSlot>(items.size(), slot));
    for (Item* item : items) {
      removeElement(ownedItems, item);
      list.push_back(item->getNameAndModifiers() + (creature->getEquipment().isEquiped(item) 
            ? " (equiped)" : " (pending)"));
    }
    if (creature->getEquipment().getMaxItems(slot) > items.size()) {
      list.push_back("[Equip item]");
      slotIndex.push_back(slot);
      slotItems.push_back(nullptr);
    }
  }
  list.emplace_back(View::ListElem("Consumables", View::TITLE));
  vector<pair<string, vector<Item*>>> consumables = Item::stackItems(ownedItems,
      [&](const Item* it) { if (!creature->getEquipment().hasItem(it)) return " (pending)"; else return ""; } );
  for (auto elem : consumables)
    list.push_back(elem.first);
  list.push_back("[Add item]");
  Optional<int> newIndex = model->getView()->chooseFromList(creature->getName() + "'s equipment", list, prevItem);
  if (!newIndex)
    return;
  int index = *newIndex;
  if (index == slotItems.size() + consumables.size()) { // [Add item]
    int itIndex = 0;
    double scrollPos = 0;
    while (1) {
      const Item* chosenItem = chooseEquipmentItem(view, {}, [&](const Item* it) {
          return minionEquipment.getOwner(it) != creature && !it->canEquip()
              && minionEquipment.needs(creature, it, true); }, &itIndex, &scrollPos);
      if (chosenItem)
        minionEquipment.own(creature, chosenItem);
      else
        break;
    }
  } else
  if (index >= slotItems.size()) {  // discard a consumable item
    minionEquipment.discard(consumables[index - slotItems.size()].second[0]);
  } else
  if (Item* item = slotItems[index])  // discard equipment
    minionEquipment.discard(item);
  else { // add new equipment
    vector<Item*> currentItems = creature->getEquipment().getItem(slotIndex[index]);
    const Item* chosenItem = chooseEquipmentItem(view, currentItems, [&](const Item* it) {
        return minionEquipment.getOwner(it) != creature
            && creature->canEquipIfEmptySlot(it, nullptr) && it->getEquipmentSlot() == slotIndex[index]; });
    if (chosenItem) {
      if (Creature* c = const_cast<Creature*>(minionEquipment.getOwner(chosenItem)))
        c->removeEffect(LastingEffect::SLEEP);
      if (chosenItem->getEquipmentSlot() != EquipmentSlot::WEAPON
          || chosenItem->getMinStrength() <= creature->getAttr(AttrType::STRENGTH)
          || view->yesOrNoPrompt(chosenItem->getTheName() + " is too heavy for " + creature->getTheName() 
            + ", and will incur an accuracy penaulty.\n Do you want to continue?"))
        minionEquipment.own(creature, chosenItem);
    }
  }
  handleEquipment(view, creature, index);
}

vector<Item*> PlayerControl::getAllItems(ItemPredicate predicate, bool includeMinions) const {
  vector<Item*> allItems;
  for (Vec2 v : myTiles)
    append(allItems, getLevel()->getSquare(v)->getItems(predicate));
  if (includeMinions)
    for (Creature* c : getCreatures())
      append(allItems, c->getEquipment().getItems(predicate));
  sort(allItems.begin(), allItems.end(), [this](const Item* it1, const Item* it2) {
      int diff = minionEquipment.getItemValue(it1) - minionEquipment.getItemValue(it2);
      if (diff == 0)
        return it1->getUniqueId() < it2->getUniqueId();
      else
        return diff > 0;
    });
  return allItems;
}

Item* PlayerControl::chooseEquipmentItem(View* view, vector<Item*> currentItems, ItemPredicate predicate,
    int* prevIndex, double* scrollPos) const {
  vector<View::ListElem> options;
  vector<Item*> currentItemVec;
  if (!currentItems.empty())
    options.push_back(View::ListElem("Currently equiped: ", View::TITLE));
  for (Item* it : currentItems) {
    currentItemVec.push_back(it);
    options.push_back(it->getNameAndModifiers());
  }
  options.emplace_back("Free:", View::TITLE);
  vector<Item*> availableItems;
  vector<Item*> usedItems;
  for (Item* item : getAllItems(predicate))
    if (!contains(currentItems, item)) {
      if (minionEquipment.getOwner(item))
        usedItems.push_back(item);
      else
        availableItems.push_back(item);
    }
  if (currentItems.empty() && availableItems.empty() && usedItems.empty())
    return nullptr;
  vector<pair<string, vector<Item*>>> usedStacks = Item::stackItems(usedItems,
      [&](const Item* it) {
        const Creature* c = NOTNULL(minionEquipment.getOwner(it));
        return " owned by " + c->getAName() + " (level " + convertToString(c->getExpLevel()) + ")";});
  vector<Item*> allStacked;
  for (auto elem : concat(Item::stackItems(availableItems), usedStacks)) {
    if (!usedStacks.empty() && elem == usedStacks.front())
      options.emplace_back("Used:", View::TITLE);
    options.emplace_back(elem.first);
    allStacked.push_back(elem.second.front());
  }
  auto index = view->chooseFromList("Choose an item to equip: ", options, prevIndex ? *prevIndex : 0,
      View::NORMAL_MENU, scrollPos);
  if (!index)
    return nullptr;
  if (prevIndex)
    *prevIndex = *index;
  return concat(currentItemVec, allStacked)[*index];
}

vector<Vec2> PlayerControl::getAllSquares(const vector<SquareType>& types, bool centerOnly) const {
  vector<Vec2> ret;
  for (SquareType type : types)
    append(ret, mySquares.at(type));
  if (centerOnly)
    ret = filter(ret, [this] (Vec2 pos) { return squareEfficiency.count(pos) && squareEfficiency.at(pos) == 8;});
  return ret;
}

void PlayerControl::handleMarket(View* view, int prevItem) {
  vector<Vec2> storage = getAllSquares(equipmentStorage);
  if (storage.empty()) {
    view->presentText("Information", "You need a storage room to use the market.");
    return;
  }
  vector<View::ListElem> options;
  vector<PItem> items;
  for (ItemId id : marketItems) {
    items.push_back(ItemFactory::fromId(id));
    options.emplace_back(items.back()->getName() + "    $" + convertToString(items.back()->getPrice()),
        items.back()->getPrice() > numResource(ResourceId::GOLD) ? View::INACTIVE : View::NORMAL);
  }
  auto index = view->chooseFromList("Buy items", options, prevItem);
  if (!index)
    return;
  Vec2 dest = chooseRandom(storage);
  takeResource({ResourceId::GOLD, items[*index]->getPrice()});
  getLevel()->getSquare(dest)->dropItem(std::move(items[*index]));
  view->updateView(this);
  handleMarket(view, *index);
}

struct SpellLearningInfo {
  SpellId id;
  TechId techId;
};

vector<SpellLearningInfo> spellLearning {
    { SpellId::HEALING, TechId::SPELLS },
    { SpellId::SUMMON_INSECTS, TechId::SPELLS},
    { SpellId::DECEPTION, TechId::SPELLS},
    { SpellId::SPEED_SELF, TechId::SPELLS},
    { SpellId::STR_BONUS, TechId::SPELLS_ADV},
    { SpellId::DEX_BONUS, TechId::SPELLS_ADV},
    { SpellId::FIRE_SPHERE_PET, TechId::SPELLS_ADV},
    { SpellId::TELEPORT, TechId::SPELLS_ADV},
    { SpellId::INVISIBILITY, TechId::SPELLS_MAS},
    { SpellId::WORD_OF_POWER, TechId::SPELLS_MAS},
    { SpellId::PORTAL, TechId::SPELLS_MAS},
};

vector<SpellInfo> PlayerControl::getSpellLearning(const Technology* tech) {
  vector<SpellInfo> ret;
  for (auto elem : spellLearning)
    if (Technology::get(elem.techId) == tech)
      ret.push_back(Creature::getSpell(elem.id));
  return ret;
}

static string requires(TechId id) {
  return " (requires: " + Technology::get(id)->getName() + ")";
}

void PlayerControl::handlePersonalSpells(View* view) {
  vector<View::ListElem> options {
      View::ListElem("The Keeper can learn spells for use in combat and other situations. ", View::TITLE),
      View::ListElem("You can cast them with 's' when you are in control of the Keeper.", View::TITLE)};
  vector<SpellId> knownSpells;
  for (SpellInfo spell : getKeeper()->getSpells())
    knownSpells.push_back(spell.id);
  for (auto elem : spellLearning) {
    SpellInfo spell = Creature::getSpell(elem.id);
    View::ElemMod mod = View::NORMAL;
    string suff;
    if (!contains(knownSpells, spell.id)) {
      mod = View::INACTIVE;
      suff = requires(elem.techId);
    }
    options.emplace_back(spell.name + suff, mod);
  }
  view->presentList("Sorcery", options);
}

vector<PlayerControl::SpawnInfo> animationInfo {
  {CreatureId::STONE_GOLEM, 30, TechId::GOLEM},
  {CreatureId::IRON_GOLEM, 80, TechId::GOLEM_ADV},
  {CreatureId::LAVA_GOLEM, 150, TechId::GOLEM_MAS},
};


void PlayerControl::handleMatterAnimation(View* view) {
  handleSpawning(view, SquareType::LABORATORY,
      "You need to build a laboratory to animate golems.", "You need a larger laboratory.", "Golem animation",
      MinionType::GOLEM, animationInfo, 1);
}

vector<PlayerControl::SpawnInfo> tamingInfo {
  {CreatureId::RAVEN, 20, Nothing()},
  {CreatureId::WOLF, 40, TechId::BEAST},
  {CreatureId::CAVE_BEAR, 80, TechId::BEAST},
  {CreatureId::SPECIAL_MONSTER_KEEPER, 150, TechId::BEAST_MUT},
};

void PlayerControl::handleBeastTaming(View* view) {
  handleSpawning(view, SquareType::BEAST_LAIR,
      "You need to build a beast lair to trap beasts.", "You need a larger lair.", "Beast taming",
      MinionType::BEAST, tamingInfo, getCollective()->getBeastMultiplier());
}

vector<PlayerControl::SpawnInfo> breedingInfo {
  {CreatureId::GNOME, 30, Nothing()},
  {CreatureId::GOBLIN, 50, TechId::GOBLIN},
  {CreatureId::OGRE, 80, TechId::OGRE},
  {CreatureId::SPECIAL_HUMANOID, 150, TechId::HUMANOID_MUT},
};

void PlayerControl::handleHumanoidBreeding(View* view) {
  handleSpawning(view, SquareType::DORM,
      "You need to build a dormitory to breed humanoids.", "You need a larger dormitory.", "Humanoid breeding",
      MinionType::NORMAL, breedingInfo, 1);
}

vector<PlayerControl::SpawnInfo> raisingInfo {
  {CreatureId::ZOMBIE, 30, TechId::NECRO},
  {CreatureId::MUMMY, 50, TechId::NECRO},
  {CreatureId::VAMPIRE, 80, TechId::VAMPIRE},
  {CreatureId::VAMPIRE_LORD, 150, TechId::VAMPIRE_ADV},
};

void PlayerControl::handleNecromancy(View* view) {
  vector<pair<Vec2, Item*>> corpses;
  for (Vec2 pos : mySquares.at(SquareType::CEMETERY)) {
    for (Item* it : getLevel()->getSquare(pos)->getItems([](const Item* it) {
        return it->getType() == ItemType::CORPSE && it->getCorpseInfo()->canBeRevived; }))
      corpses.push_back({pos, it});
  }
  handleSpawning(view, SquareType::CEMETERY, "You need to build a graveyard and collect corpses to raise undead.",
      "You need a larger graveyard", "Necromancy ", MinionType::UNDEAD, raisingInfo,
      getCollective()->getUndeadMultiplier(), corpses, "corpses available",
      "You need to collect some corpses to raise undead.");
}

vector<CreatureId> PlayerControl::getSpawnInfo(const Technology* tech) {
  vector<CreatureId> ret;
  for (SpawnInfo elem : concat<SpawnInfo>({breedingInfo, tamingInfo, animationInfo, raisingInfo}))
    if (elem.techId && Technology::get(*elem.techId) == tech)
      ret.push_back(elem.id);
  return ret;
}

int PlayerControl::getNumMinions() const {
  return minions.size() - minionByType[MinionType::PRISONER].size();
}

static int countNeighbor(Vec2 pos, const set<Vec2>& squares) {
  int num = 0;
  for (Vec2 v : pos.neighbors8())
    num += squares.count(v);
  return num;
}

static Optional<Vec2> chooseBedPos(const set<Vec2>& lair, const set<Vec2>& beds) {
  vector<Vec2> res;
  for (Vec2 v : lair) {
    if (countNeighbor(v, beds) > 2)
      continue;
    bool bad = false;
    for (Vec2 n : v.neighbors8())
      if (beds.count(n) && countNeighbor(n, beds) >= 2) {
        bad = true;
        break;
      }
    if (!bad)
      res.push_back(v);
  }
  if (!res.empty())
    return chooseRandom(res);
  else
    return Nothing();
}

void PlayerControl::handleSpawning(View* view, SquareType spawnSquare, const string& info1, const string& info2,
    const string& titleBase, MinionType minionType, vector<SpawnInfo> spawnInfo, double multiplier,
    Optional<vector<pair<Vec2, Item*>>> genItems, string genItemsInfo, string info3) {
  Optional<SquareType> replacement = getSecondarySquare(spawnSquare);
  int prevItem = 0;
  bool allInactive = false;
  while (1) {
    set<Vec2> lairSquares = mySquares.at(spawnSquare);
    vector<View::ListElem> options;
    Optional<Vec2> bedPos;
    if (!replacement && !lairSquares.empty())
      bedPos = chooseRandom(lairSquares);
    else if (minionByType[minionType].size() < mySquares.at(*replacement).size())
      bedPos = chooseRandom(mySquares.at(*replacement));
    else
      bedPos = chooseBedPos(lairSquares, mySquares.at(*replacement));
    if (getNumMinions() >= minionLimit) {
      allInactive = true;
      options.emplace_back("You have reached the limit of the number of minions.", View::TITLE);
    } else
    if (lairSquares.empty()) {
      allInactive = true;
      options.emplace_back(info1, View::TITLE);
    } else
    if (!bedPos) {
      allInactive = true;
      options.emplace_back(info2, View::TITLE);
    } else
    if (genItems && genItems->empty()) {
      options.emplace_back(info3, View::TITLE);
      allInactive = true;
    }
    vector<pair<PCreature, int>> availableCreatures;
    for (SpawnInfo info : spawnInfo) {
      int cost = multiplier * info.manaCost;
      availableCreatures.push_back({CreatureFactory::fromId(info.id, getTribe(),
            MonsterAIFactory::collective(getCollective())), cost});
      bool isTech = !info.techId || hasTech(*info.techId);
      string suf;
      if (!isTech)
        suf = requires(*info.techId);
      options.emplace_back(availableCreatures.back().first->getSpeciesName()
          + "  mana: " + convertToString(cost) + suf,
          allInactive || !isTech || cost > mana ? View::INACTIVE : View::NORMAL);
    }
    string title = titleBase;
    if (genItems)
      title += convertToString(genItems->size()) + " " + genItemsInfo;
    auto index = view->chooseFromList(title, options, prevItem);
    if (!index)
      return;
    Vec2 pos = *bedPos;
    PCreature& creature = availableCreatures[*index].first;
    mana -= availableCreatures[*index].second;
    for (Vec2 v : concat({pos}, pos.neighbors8(true)))
      if (getLevel()->inBounds(v) && getLevel()->getSquare(v)->canEnter(creature.get())) {
        if (genItems) {
          pair<Vec2, Item*> item = chooseRandom(*genItems);
          getLevel()->getSquare(item.first)->removeItems({item.second});
          removeElement(*genItems, item);
        }
        addCreature(std::move(creature), v, minionType);
        break;
      }
    if (creature)
      messageBuffer.addMessage(MessageBuffer::important("The spell failed."));
    else if (replacement) {
      getLevel()->replaceSquare(pos, SquareFactory::get(*replacement));
      mySquares.at(spawnSquare).erase(pos);
      mySquares.at(*replacement).insert(pos);
    }
    view->updateView(this);
    prevItem = *index;
  }
}

bool PlayerControl::hasTech(TechId id) const {
  return contains(technologies, Technology::get(id));
}

int PlayerControl::getMinLibrarySize() const {
  return 2 * technologies.size();
}

double PlayerControl::getTechCost() {
  int numTech = technologies.size() - numFreeTech;
  int numDouble = 4;
  return getCollective()->getTechCostMultiplier() * pow(2, double(numTech) / numDouble);
}

void PlayerControl::handleLibrary(View* view) {
  if (mySquares.at(SquareType::LIBRARY).empty()) {
    view->presentText("", "You need to build a library to start research.");
    return;
  }
  vector<View::ListElem> options;
  bool allInactive = false;
  if (mySquares.at(SquareType::LIBRARY).size() <= getMinLibrarySize()) {
    allInactive = true;
    options.emplace_back("You need a larger library to continue research.", View::TITLE);
  }
  int neededPoints = 50 * getTechCost();
  options.push_back(View::ListElem("You have " + convertToString(int(mana)) + " mana. " +
        convertToString(neededPoints) + " needed to advance.", View::TITLE));
  vector<Technology*> techs = filter(Technology::getNextTechs(technologies),
      [](const Technology* tech) { return tech->canResearch(); });
  for (Technology* tech : techs) {
    string text = tech->getName();
    options.emplace_back(text, neededPoints <= mana && !allInactive ? View::NORMAL : View::INACTIVE);
  }
  options.emplace_back("Researched:", View::TITLE);
  for (Technology* tech : technologies)
    options.emplace_back(tech->getName(), View::INACTIVE);
  auto index = view->chooseFromList("Library", options);
  if (!index)
    return;
  Technology* tech = techs[*index];
  mana -= neededPoints;
  acquireTech(tech);
  view->updateView(this);
  handleLibrary(view);
}

void PlayerControl::acquireTech(Technology* tech, bool free) {
  technologies.push_back(tech);
  if (free)
    ++numFreeTech;
  for (auto elem : spellLearning)
    if (Technology::get(elem.techId) == tech)
      getKeeper()->addSpell(elem.id);
  if (Skill* skill = tech->getSkill())
    for (Creature* c : minions)
      c->addSkill(skill);
}

typedef GameInfo::BandInfo::Button Button;

Optional<pair<ViewObject, int>> PlayerControl::getCostObj(CostInfo cost) const {
  if (cost.value > 0)
    return make_pair(getResourceViewObject(cost.id), cost.value);
  else
    return Nothing();
}

vector<Button> PlayerControl::fillButtons(const vector<BuildInfo>& buildInfo) const {
  vector<Button> buttons;
  for (BuildInfo button : buildInfo) {
    bool isTech = (!button.techId || hasTech(*button.techId));
    switch (button.buildType) {
      case BuildInfo::SQUARE: {
           BuildInfo::SquareInfo& elem = button.squareInfo;
           ViewObject viewObj(SquareFactory::get(elem.type)->getViewObject());
           if (getSecondarySquare(elem.type))
             viewObj = SquareFactory::get(*getSecondarySquare(elem.type))->getViewObject();
           buttons.push_back({
               viewObj,
               elem.name,
               getCostObj(elem.cost),
               (elem.cost.value > 0 ? "[" + convertToString(mySquares.at(elem.type).size()) + "]" : ""),
               isTech ? "" : "Requires " + Technology::get(*button.techId)->getName() });
           }
           break;
      case BuildInfo::DIG: {
             buttons.push_back({
                 ViewObject(ViewId::DIG_ICON, ViewLayer::LARGE_ITEM, ""),
                 "Dig or cut tree", Nothing(), "", ""});
           }
           break;
      case BuildInfo::IMPALED_HEAD: {
           BuildInfo::SquareInfo elem = button.squareInfo;
             buttons.push_back({
                 ViewObject(ViewId::IMPALED_HEAD, ViewLayer::LARGE_ITEM, ""),
                 elem.name, Nothing(), "(" + convertToString(executions) + " ready)",
                 executions > 0  ? "" : "inactive"});
           }
           break;
      case BuildInfo::FETCH: {
             buttons.push_back({
                 ViewObject(ViewId::FETCH_ICON, ViewLayer::LARGE_ITEM, ""),
                 "Fetch items", Nothing(), "", ""});
           }
           break;
      case BuildInfo::DISPATCH: {
             buttons.push_back({
                 ViewObject(ViewId::IMP, ViewLayer::LARGE_ITEM, ""),
                 "Dispatch imp", Nothing(), "", ""});
           }
           break;
      case BuildInfo::TRAP: {
             BuildInfo::TrapInfo& elem = button.trapInfo;
             int numTraps = getTrapItems(elem.type).size();
             buttons.push_back({
                 ViewObject(elem.viewId, ViewLayer::LARGE_ITEM, ""),
                 elem.name,
                 Nothing(),
                 "(" + convertToString(numTraps) + " ready)",
                 isTech ? "" : "Requires " + Technology::get(*button.techId)->getName() });
           }
           break;
      case BuildInfo::IMP: {
           pair<ViewObject, int> cost = {ViewObject::mana(), getImpCost()};
           buttons.push_back({
               ViewObject(ViewId::IMP, ViewLayer::CREATURE, ""),
               "Summon imp",
               cost,
               "[" + convertToString(minionByType[MinionType::IMP].size()) + "]",
               getImpCost() <= mana ? "" : "inactive"});
           break; }
      case BuildInfo::DESTROY:
           buttons.push_back({
               ViewObject(ViewId::DESTROY_BUTTON, ViewLayer::CREATURE, ""), "Remove construction", Nothing(), "",
                   ""});
           break;
      case BuildInfo::GUARD_POST:
           buttons.push_back({
               ViewObject(ViewId::GUARD_POST, ViewLayer::CREATURE, ""), "Guard post", Nothing(), "", ""});
           break;
    }
    if (!isTech)
      buttons.back().help = "Requires " + Technology::get(*button.techId)->getName();
    if (buttons.back().help.empty())
      buttons.back().help = button.help;
    buttons.back().hotkey = button.hotkey;
    buttons.back().groupName = button.groupName;
  }
  return buttons;
}

vector<PlayerControl::TechInfo> PlayerControl::getTechInfo() const {
  vector<TechInfo> ret;
  ret.push_back({{ViewId::OGRE, "Humanoids", 'h'},
      [this](PlayerControl* c, View* view) { c->handleHumanoidBreeding(view); }});
  ret.push_back({{ViewId::BEAR, "Beasts", 'b'},
      [this](PlayerControl* c, View* view) { c->handleBeastTaming(view); }}); 
  if (hasTech(TechId::GOLEM))
    ret.push_back({{ViewId::IRON_GOLEM, "Golems", 'g'},
      [this](PlayerControl* c, View* view) { c->handleMatterAnimation(view); }});      
  if (hasTech(TechId::NECRO))
    ret.push_back({{ViewId::VAMPIRE, "Necromancy", 'n'},
      [this](PlayerControl* c, View* view) { c->handleNecromancy(view); }});
  ret.push_back({{ViewId::MANA, "Sorcery"}, [this](PlayerControl* c, View* view) {c->handlePersonalSpells(view);}});
  ret.push_back({{ViewId::EMPTY, ""}, [](PlayerControl* c, View*) {}});
  ret.push_back({{ViewId::LIBRARY, "Library", 'l'},
      [this](PlayerControl* c, View* view) { c->handleLibrary(view); }});
  ret.push_back({{ViewId::GOLD, "Black market"},
      [this](PlayerControl* c, View* view) { c->handleMarket(view); }});
  ret.push_back({{ViewId::EMPTY, ""}, [](PlayerControl* c, View*) {}});
  ret.push_back({{ViewId::BOOK, "Keeperopedia"},
      [this](PlayerControl* c, View* view) { model->keeperopedia.present(view); }});
  return ret;
}

void PlayerControl::refreshGameInfo(GameInfo& gameInfo) const {
  gameInfo.bandInfo.deities.clear();
  for (Deity* deity : Deity::getDeities())
    gameInfo.bandInfo.deities.push_back({deity->getName(), getCollective()->getStanding(deity)});
  gameInfo.villageInfo.villages.clear();
  bool attacking = false;
  for (VillageControl* c : model->getVillageControls())
    if (!c->isAnonymous()) {
      gameInfo.villageInfo.villages.push_back(c->getVillageInfo());
      if (c->currentlyAttacking())
        attacking = true;
    }
  if (attacking)
    model->getView()->getJukebox()->setCurrent(Jukebox::BATTLE);
  Model::SunlightInfo sunlightInfo = model->getSunlightInfo();
  gameInfo.sunlightInfo = { sunlightInfo.getText(), (int)sunlightInfo.timeRemaining };
  gameInfo.infoType = GameInfo::InfoType::BAND;
  GameInfo::BandInfo& info = gameInfo.bandInfo;
  info.buildings = fillButtons(getBuildInfo());
  info.workshop = fillButtons(workshopInfo);
  info.libraryButtons = fillButtons(libraryInfo);
  info.tasks = minionTaskStrings;
  for (Creature* c : minions)
    if (isInCombat(c))
      info.tasks[c->getUniqueId()] = "fighting";
  info.monsterHeader = "Monsters: " + convertToString(getNumMinions()) + " / " + convertToString(minionLimit);
  info.creatures.clear();
  for (Creature* c : minions)
    info.creatures.push_back(c);
  info.enemies.clear();
  for (Vec2 v : myTiles)
    if (const Creature* c = getLevel()->getSquare(v)->getCreature())
      if (c->getTribe() != getTribe())
        info.enemies.push_back(c);
  info.numResource.clear();
  for (auto elem : resourceInfo)
    info.numResource.push_back({getResourceViewObject(elem.first), numResource(elem.first), elem.second.name});
  info.numResource.push_back({ViewObject::mana(), int(mana), "mana"});
  if (attacking) {
    if (info.warning.empty())
      info.warning = NameGenerator::get(NameGeneratorId::INSULTS)->getNext();
  } else
    info.warning = "";
  for (Warning w : ENUM_ALL(Warning))
    if (warnings[w]) {
      info.warning = getWarningText(w);
      break;
    }
  info.time = getTime();
  info.gatheringTeam = gatheringTeam;
  info.team.clear();
  for (Creature* c : team)
    info.team.push_back(c->getUniqueId());
  info.techButtons.clear();
  for (TechInfo tech : getTechInfo())
    info.techButtons.push_back(tech.button);
}

bool PlayerControl::isItemMarked(const Item* it) const {
  return markedItems.contains(it->getUniqueId());
}

void PlayerControl::markItem(const Item* it) {
  markedItems.insert(it->getUniqueId());
}

void PlayerControl::unmarkItem(UniqueId id) {
  markedItems.erase(id);
}

void PlayerControl::updateMemory() {
  for (Vec2 v : getLevel()->getBounds())
    if (knownTiles[v])
      addToMemory(v);
}

const MapMemory& PlayerControl::getMemory() const {
  return (*memory.get())[getLevel()->getUniqueId()];
}

MapMemory& PlayerControl::getMemory(Level* l) {
  return (*memory.get())[l->getUniqueId()];
}

ViewObject PlayerControl::getTrapObject(TrapType type) {
  for (const PlayerControl::BuildInfo& info : concat(workshopInfo, getBuildInfo(nullptr)))
    if (info.buildType == BuildInfo::TRAP && info.trapInfo.type == type)
      return ViewObject(info.trapInfo.viewId, ViewLayer::LARGE_ITEM, "Unarmed trap")
        .setModifier(ViewObject::Modifier::PLANNED);
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

ViewIndex PlayerControl::getViewIndex(Vec2 pos) const {
  ViewIndex index = getLevel()->getSquare(pos)->getViewIndex(this);
  if (const Creature* c = getLevel()->getSquare(pos)->getCreature())
    if (contains(team, c) && gatheringTeam)
      index.getObject(ViewLayer::CREATURE).setModifier(ViewObject::Modifier::TEAM_HIGHLIGHT);
  if (taskMap.getMarked(pos))
    index.addHighlight(HighlightType::BUILD);
  else if (rectSelectCorner && rectSelectCorner2
      && pos.inRectangle(Rectangle::boundingBox({*rectSelectCorner, *rectSelectCorner2})))
    index.addHighlight(HighlightType::RECT_SELECTION);
  if (!index.hasObject(ViewLayer::LARGE_ITEM)) {
    if (traps.count(pos))
      index.insert(getTrapObject(traps.at(pos).type));
    if (guardPosts.count(pos))
      index.insert(ViewObject(ViewId::GUARD_POST, ViewLayer::LARGE_ITEM, "Guard post"));
    if (constructions.count(pos) && !constructions.at(pos).built)
      index.insert(getConstructionObject(constructions.at(pos).type));
  }
  if (surprises.count(pos) && !knownPos(pos))
    index.insert(ViewObject(ViewId::UNKNOWN_MONSTER, ViewLayer::CREATURE, "Surprise"));
  if (hasEfficiency(pos)) {
    double efficiency = getEfficiency(pos);
    //index.addHighlight(HighlightType::EFFICIENCY, efficiency);
    index.getObject(ViewLayer::FLOOR).setAttribute(ViewObject::Attribute::EFFICIENCY, efficiency);
  }
  return index;
}

bool PlayerControl::staticPosition() const {
  return false;
}

Vec2 PlayerControl::getPosition() const {
  return getKeeper()->getPosition();
}

enum Selection { SELECT, DESELECT, NONE } selection = NONE;

Task* PlayerControl::TaskMap::addTaskCost(PTask task, Vec2 position, CostInfo cost) {
  completionCost[task.get()] = cost;
  return Task::Mapping::addTask(std::move(task), position);
}

PlayerControl::CostInfo PlayerControl::TaskMap::removeTask(Task* task) {
  CostInfo cost {ResourceId::GOLD, 0};
  if (completionCost.count(task)) {
    cost = completionCost.at(task);
    completionCost.erase(task);
  }
  if (auto pos = getPosition(task))
    if (marked.count(*pos))
      marked.erase(*pos);
  Task::Mapping::removeTask(task);
  return cost;
}

PlayerControl::CostInfo PlayerControl::TaskMap::removeTask(UniqueId id) {
  for (PTask& task : tasks)
    if (task->getUniqueId() == id) {
      return removeTask(task.get());
    }
  return {ResourceId::GOLD, 0};
}

bool PlayerControl::TaskMap::isLocked(const Creature* c, const Task* t) const {
  return lockedTasks.count({c, t->getUniqueId()});
}

void PlayerControl::TaskMap::lock(const Creature* c, const Task* t) {
  lockedTasks.insert({c, t->getUniqueId()});
}

void PlayerControl::TaskMap::clearAllLocked() {
  lockedTasks.clear();
}

Task* PlayerControl::TaskMap::getMarked(Vec2 pos) const {
  if (marked.count(pos))
    return marked.at(pos);
  else
    return nullptr;
}

void PlayerControl::TaskMap::markSquare(Vec2 pos, PTask task) {
  marked[pos] = task.get();
  addTask(std::move(task), pos);
}

void PlayerControl::TaskMap::unmarkSquare(Vec2 pos) {
  Task* task = marked.at(pos);
  marked.erase(pos);
  removeTask(task);
}

int PlayerControl::numResource(ResourceId id) const {
  int ret = credit.at(id);
  for (SquareType type : resourceInfo.at(id).storageType)
    for (Vec2 pos : mySquares.at(type))
      ret += getLevel()->getSquare(pos)->getItems(resourceInfo.at(id).predicate).size();
  return ret;
}

void PlayerControl::takeResource(CostInfo cost) {
  int num = cost.value;
  if (num == 0)
    return;
  CHECK(num > 0);
  if (credit.at(cost.id)) {
    if (credit.at(cost.id) >= num) {
      credit[cost.id] -= num;
      return;
    } else {
      num -= credit.at(cost.id);
      credit[cost.id] = 0;
    }
  }
  for (Vec2 pos : randomPermutation(getAllSquares(resourceInfo.at(cost.id).storageType))) {
    vector<Item*> goldHere = getLevel()->getSquare(pos)->getItems(resourceInfo.at(cost.id).predicate);
    for (Item* it : goldHere) {
      getLevel()->getSquare(pos)->removeItem(it);
      if (--num == 0)
        return;
    }
  }
  FAIL << "Didn't have enough gold";
}

void PlayerControl::returnResource(CostInfo amount) {
  if (amount.value == 0)
    return;
  CHECK(amount.value > 0);
  vector<Vec2> destination = getAllSquares(resourceInfo.at(amount.id).storageType);
  if (!destination.empty()) {
    getLevel()->getSquare(chooseRandom(destination))->
      dropItems(ItemFactory::fromId(resourceInfo.at(amount.id).itemId, amount.value));
  } else
    credit[amount.id] += amount.value;
}

int PlayerControl::getImpCost() const {
  int numImps = 0;
  for (Creature* c : minionByType[MinionType::IMP])
    if (!contains(minions, c)) // see if it's not a prisoner
      ++numImps;
  if (numImps < startImpNum)
    return 0;
  return basicImpCost * pow(2, double(numImps - startImpNum) / 5);
}

class MinionController : public Player {
  public:
  MinionController(Creature* c, Model* m, map<UniqueId, MapMemory>* memory) : Player(c, m, false, memory) {}

  virtual void onKilled(const Creature* attacker) override {
    if (model->getView()->yesOrNoPrompt("Would you like to see the last messages?"))
      messageBuffer.showHistory();
    creature->popController();
  }

  virtual bool unpossess() override {
    return true;
  }

  virtual void onFellAsleep() {
    if (model->getView()->yesOrNoPrompt("You fell asleep. Do you want to leave your minion?"))
      creature->popController();
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Player)
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(MinionController);
};

void PlayerControl::possess(const Creature* cr, View* view) {
  if (possessed && possessed != cr && possessed->isPlayer())
    possessed->popController();
  view->stopClock();
  CHECK(contains(getCreatures(), cr));
  CHECK(!cr->isDead());
  Creature* c = const_cast<Creature*>(cr);
  if (c->isAffected(LastingEffect::SLEEP))
    c->removeEffect(LastingEffect::SLEEP);
  freeFromGuardPost(c);
  updateMemory();
  c->pushController(PController(new MinionController(c, model, memory.get())));
  possessed = c;
}

bool PlayerControl::canBuildDoor(Vec2 pos) const {
  if (!getLevel()->getSquare(pos)->canConstruct(SquareType::TRIBE_DOOR))
    return false;
  Rectangle innerRect = getLevel()->getBounds().minusMargin(1);
  auto wallFun = [=](Vec2 pos) {
      return getLevel()->getSquare(pos)->canConstruct(SquareType::FLOOR) ||
          !pos.inRectangle(innerRect); };
  return !traps.count(pos) && pos.inRectangle(innerRect) && 
      ((wallFun(pos - Vec2(0, 1)) && wallFun(pos - Vec2(0, -1))) ||
       (wallFun(pos - Vec2(1, 0)) && wallFun(pos - Vec2(-1, 0))));
}

bool PlayerControl::canPlacePost(Vec2 pos) const {
  return !guardPosts.count(pos) && !traps.count(pos) &&
      getLevel()->getSquare(pos)->canEnterEmpty(Creature::getDefault()) && knownPos(pos);
}
  
void PlayerControl::freeFromGuardPost(const Creature* c) {
  for (auto& elem : guardPosts)
    if (elem.second.attender == c)
      elem.second.attender = nullptr;
}

Creature* PlayerControl::getCreature(UniqueId id) {
  for (Creature* c : getCreatures())
    if (c->getUniqueId() == id)
      return c;
  FAIL << "Creature not found " << id;
  return nullptr;
}

void PlayerControl::handleCreatureButton(Creature* c, View* view) {
  if (!gatheringTeam)
    minionView(view, c);
  else if (contains(minions, c) && getMinionType(c) != MinionType::PRISONER) {
    if (contains(team, c))
      removeElement(team, c);
    else
      team.push_back(c);
  }
}

void PlayerControl::processInput(View* view, UserInput input) {
  if (retired)
    return;
  switch (input.type) {
    case UserInput::EDIT_TEAM:
        CHECK(!team.empty());
        CHECK(!gatheringTeam);
        gatheringTeam = true;
      break;
    case UserInput::GATHER_TEAM:
        if (gatheringTeam && !team.empty()) {
          gatheringTeam = false;
          break;
        } 
        if (!gatheringTeam && !team.empty()) {
          possess(team[0], view);
 //         gatheringTeam = false;
          for (Creature* c : team) {
            freeFromGuardPost(c);
            if (c->isAffected(LastingEffect::SLEEP))
              c->removeEffect(LastingEffect::SLEEP);
          }
        } else
          gatheringTeam = true;
        break;
    case UserInput::DRAW_LEVEL_MAP: view->drawLevelMap(this); break;
    case UserInput::CANCEL_TEAM: gatheringTeam = false; team.clear(); break;
    case UserInput::MARKET: handleMarket(view); break;
    case UserInput::DEITIES: model->keeperopedia.deity(view, Deity::getDeities()[input.getNum()]); break;
    case UserInput::TECHNOLOGY: {
        vector<TechInfo> techInfo = getTechInfo();
        techInfo[input.getNum()].butFun(this, view);
        break;}
    case UserInput::CREATURE_BUTTON: 
        handleCreatureButton(getCreature(input.getNum()), view);
        break;
    case UserInput::POSSESS: {
        Vec2 pos = input.getPosition();
        if (!pos.inRectangle(getLevel()->getBounds()))
          return;
        if (Creature* c = getLevel()->getSquare(pos)->getCreature())
          if (contains(minions, c))
            handleCreatureButton(c, view);
        break;
        }
    case UserInput::RECT_SELECTION:
        if (rectSelectCorner) {
          rectSelectCorner2 = input.getPosition();
        } else
          rectSelectCorner = input.getPosition();
        break;
    case UserInput::BUILD:
        handleSelection(input.getPosition(), getBuildInfo()[input.getBuildInfo().building], false);
        break;
    case UserInput::WORKSHOP:
        handleSelection(input.getPosition(), workshopInfo[input.getBuildInfo().building], false);
        break;
    case UserInput::LIBRARY:
        handleSelection(input.getPosition(), libraryInfo[input.getBuildInfo().building], false);
        break;
    case UserInput::BUTTON_RELEASE:
        selection = NONE;
        if (rectSelectCorner && rectSelectCorner2) {
          handleSelection(*rectSelectCorner, getBuildInfo()[input.getBuildInfo().building], true);
          for (Vec2 v : Rectangle::boundingBox({*rectSelectCorner, *rectSelectCorner2}))
            handleSelection(v, getBuildInfo()[input.getBuildInfo().building], true);
        }
        rectSelectCorner = Nothing();
        rectSelectCorner2 = Nothing();
        selection = NONE;
        break;
    case UserInput::EXIT: model->exitAction(); break;
    case UserInput::IDLE: break;
    default: break;
  }
}

void PlayerControl::handleSelection(Vec2 pos, const BuildInfo& building, bool rectangle) {
  if (building.techId && !hasTech(*building.techId))
    return;
  if (!pos.inRectangle(getLevel()->getBounds()))
    return;
  switch (building.buildType) {
    case BuildInfo::IMP:
        if (mana >= getImpCost() && selection == NONE && !rectangle) {
          selection = SELECT;
          PCreature imp = CreatureFactory::fromId(CreatureId::IMP, getTribe(),
              MonsterAIFactory::collective(getCollective()));
          for (Vec2 v : pos.neighbors8(true))
            if (v.inRectangle(getLevel()->getBounds()) && getLevel()->getSquare(v)->canEnter(imp.get()) 
                && canSee(v)) {
              mana -= getImpCost();
              addCreature(std::move(imp), v, MinionType::IMP);
              break;
            }
        }
        break;
    case BuildInfo::TRAP: {
        TrapType trapType = building.trapInfo.type;
        if (traps.count(pos)) {
          traps.erase(pos);
        } else
        if (canPlacePost(pos) && myTiles.count(pos)) {
          traps[pos] = {trapType, false, 0};
          updateConstructions();
        }
      }
      break;
    case BuildInfo::DESTROY:
        selection = SELECT;
        if (getLevel()->getSquare(pos)->canDestroy() && myTiles.count(pos))
          getLevel()->getSquare(pos)->destroy();
        getLevel()->getSquare(pos)->removeTriggers();
        if (Creature* c = getLevel()->getSquare(pos)->getCreature())
          if (c->getName() == "boulder")
            c->die(nullptr, false);
        if (traps.count(pos)) {
          traps.erase(pos);
        }
        if (constructions.count(pos)) {
          returnResource(taskMap.removeTask(constructions.at(pos).task));
          constructions.erase(pos);
        }
        break;
    case BuildInfo::GUARD_POST:
        if (guardPosts.count(pos) && selection != SELECT) {
          guardPosts.erase(pos);
          selection = DESELECT;
        }
        else if (canPlacePost(pos) && guardPosts.size() < getNumMinions() && selection != DESELECT) {
          guardPosts[pos] = {nullptr};
          selection = SELECT;
        }
        break;
    case BuildInfo::DIG:
        if (!tryLockingDoor(pos)) {
          if (taskMap.getMarked(pos) && selection != SELECT) {
            taskMap.unmarkSquare(pos);
            selection = DESELECT;
          } else
            if (!taskMap.getMarked(pos) && selection != DESELECT) {
              if (getLevel()->getSquare(pos)->canConstruct(SquareType::TREE_TRUNK)) {
                taskMap.markSquare(pos, Task::construction(this, pos, SquareType::TREE_TRUNK));
                selection = SELECT;
              } else
                if (getLevel()->getSquare(pos)->canConstruct(SquareType::FLOOR) || !knownPos(pos)) {
                  taskMap.markSquare(pos, Task::construction(this, pos, SquareType::FLOOR));
                  selection = SELECT;
                }
            }
        }
        break;
    case BuildInfo::FETCH:
        for (ItemFetchInfo elem : getFetchInfo())
          fetchItems(pos, elem, true);
        break;
    case BuildInfo::DISPATCH:
        taskMap.setPriorityTasks(pos);
        break;
    case BuildInfo::IMPALED_HEAD:
    case BuildInfo::SQUARE:
        if (!tryLockingDoor(pos)) {
          if (constructions.count(pos)) {
            if (selection != SELECT) {
              returnResource(taskMap.removeTask(constructions.at(pos).task));
              if (constructions.at(pos).type == SquareType::IMPALED_HEAD)
                ++executions;
              constructions.erase(pos);
              selection = DESELECT;
            }
          } else {
            BuildInfo::SquareInfo info = building.squareInfo;
            if (knownPos(pos) && getLevel()->getSquare(pos)->canConstruct(info.type) && !traps.count(pos)
                && (info.type != SquareType::TRIBE_DOOR || canBuildDoor(pos)) && selection != DESELECT) {
              if (info.buildImmediatly) {
                while (!getLevel()->getSquare(pos)->construct(info.type)) {}
                onConstructed(pos, info.type);
              } else {
                if (info.type == SquareType::IMPALED_HEAD) {
                  if (executions <= 0)
                    break;
                  else
                    --executions;
                }
                constructions[pos] = {info.cost, false, 0, info.type, -1};
                selection = SELECT;
                updateConstructions();
              }
            }
          }
        }
        break;
  }
}

bool PlayerControl::tryLockingDoor(Vec2 pos) {
  if (constructions.count(pos)) {
    Square* square = getLevel()->getSquare(pos);
    if (square->canLock()) {
      if (selection != DESELECT && !square->isLocked()) {
        square->lock();
        sectors->remove(pos);
        flyingSectors->remove(pos);
        selection = SELECT;
      } else if (selection != SELECT && square->isLocked()) {
        square->lock();
        sectors->add(pos);
        flyingSectors->add(pos);
        selection = DESELECT;
      }
      return true;
    }
  }
  return false;
}

void PlayerControl::addKnownTile(Vec2 pos) {
  if (!knownTiles[pos]) {
    borderTiles.erase(pos);
    knownTiles[pos] = true;
    for (Vec2 v : pos.neighbors4())
      if (getLevel()->inBounds(v) && !knownTiles[v])
        borderTiles.insert(v);
    if (Task* task = taskMap.getMarked(pos))
      if (task->isImpossible(getLevel()))
        taskMap.removeTask(task);
  }
}

const static unordered_set<SquareType> efficiencySquares {
  SquareType::TRAINING_ROOM,
  SquareType::WORKSHOP,
  SquareType::LIBRARY,
  SquareType::LABORATORY,
};

void PlayerControl::onConstructed(Vec2 pos, SquareType type) {
  if (!contains({SquareType::TREE_TRUNK}, type))
    myTiles.insert(pos);
  CHECK(!mySquares[type].count(pos));
  mySquares[type].insert(pos);
  if (efficiencySquares.count(type))
    updateEfficiency(pos, type);
  if (contains({SquareType::FLOOR, SquareType::BRIDGE}, type))
    taskMap.clearAllLocked();
  if (taskMap.getMarked(pos))
    taskMap.unmarkSquare(pos);
  if (constructions.count(pos)) {
    constructions.at(pos).built = true;
    constructions.at(pos).marked = 0;
    constructions.at(pos).task = -1;
  }
  if (getLevel()->getSquare(pos)->canEnterEmpty(Creature::getDefaultMinion()))
    sectors->add(pos);
  else
    sectors->remove(pos);
  if (getLevel()->getSquare(pos)->canEnterEmpty(Creature::getDefaultMinionFlyer()))
    flyingSectors->add(pos);
  else
    flyingSectors->remove(pos);
}

void PlayerControl::onPickedUp(Vec2 pos, EntitySet items) {
  for (UniqueId id : items)
    unmarkItem(id);
}
  
void PlayerControl::onCantPickItem(EntitySet items) {
  for (UniqueId id : items)
    unmarkItem(id);
}

void PlayerControl::onBrought(Vec2 pos, vector<Item*> items) {
}

void PlayerControl::onAppliedItem(Vec2 pos, Item* item) {
  CHECK(item->getTrapType());
  if (traps.count(pos)) {
    traps[pos].marked = 0;
    traps[pos].armed = true;
  }
}

set<TrapType> PlayerControl::getNeededTraps() const {
  set<TrapType> ret;
  for (auto elem : traps)
    if (!elem.second.marked && !elem.second.armed)
      ret.insert(elem.second.type);
  return ret;
}

void PlayerControl::onAppliedSquare(Vec2 pos) {
  Creature* c = NOTNULL(getLevel()->getSquare(pos)->getCreature());
  double efficiency = getCollective()->getEfficiency(c);
  if (mySquares.at(SquareType::LIBRARY).count(pos)) {
    if (c == getKeeper())
      mana += efficiency * getEfficiency(pos) * (0.3 + max(0., 2 - (mana + double(getDangerLevel(false))) / 700));
    else {
      if (Random.rollD(60.0 / (getEfficiency(pos) * efficiency)))
        c->addSpell(chooseRandom(getKeeper()->getSpells()).id);
    }
  }
  if (mySquares.at(SquareType::TRAINING_ROOM).count(pos)) {
    double lev1 = 0.03;
    double lev10 = 0.01;
    c->increaseExpLevel(
        efficiency * getEfficiency(pos) * (lev1 - double(c->getExpLevel() - 1) * (lev1 - lev10) / 9.0  ));
  }
  if (mySquares.at(SquareType::LABORATORY).count(pos))
    if (Random.rollD(30.0 / (getCollective()->getCraftingMultiplier() * getEfficiency(pos) * efficiency))) {
      getLevel()->getSquare(pos)->dropItems(ItemFactory::laboratory(technologies).random());
      Statistics::add(StatId::POTION_PRODUCED);
    }
  if (mySquares.at(SquareType::WORKSHOP).count(pos))
    if (Random.rollD(40.0 / (getCollective()->getCraftingMultiplier() * getEfficiency(pos) * efficiency))) {
      set<TrapType> neededTraps = getNeededTraps();
      vector<PItem> items;
      for (int i : Range(10)) {
        items = ItemFactory::workshop(technologies).random();
        if (neededTraps.empty() || (items[0]->getTrapType() && neededTraps.count(*items[0]->getTrapType())))
          break;
      }
      if (items[0]->getType() == ItemType::WEAPON)
        Statistics::add(StatId::WEAPON_PRODUCED);
      if (items[0]->getType() == ItemType::ARMOR)
        Statistics::add(StatId::ARMOR_PRODUCED);
      getLevel()->getSquare(pos)->dropItems(std::move(items));
    }
}

void PlayerControl::onAppliedItemCancel(Vec2 pos) {
  if (traps.count(pos))
    traps.at(pos).marked = 0;
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

double PlayerControl::getWarLevel() const {
  double ret = 0;
  for (const Creature* c : minions)
    ret += c->getDifficultyPoints();
  ret += mySquares.at(SquareType::IMPALED_HEAD).size() * 150;
  return ret * getCollective()->getWarMultiplier();
}

double PlayerControl::getDangerLevel(bool includeExecutions) const {
  double ret = 0;
  for (const Creature* c : minions)
    ret += c->getDifficultyPoints();
  if (includeExecutions)
    ret += mySquares.at(SquareType::IMPALED_HEAD).size() * 150;
  return ret;
}

ItemPredicate PlayerControl::unMarkedItems(ItemType type) const {
  return [this, type](const Item* it) {
      return it->getType() == type && !isItemMarked(it); };
}

void PlayerControl::addToMemory(Vec2 pos) {
  getMemory(getLevel()).update(pos, getLevel()->getSquare(pos)->getViewIndex(this));
}

void PlayerControl::update(Creature* c) {
  if (!retired && possessed != getKeeper()) { 
    if ((isInCombat(getKeeper()) || getKeeper()->getHealth() < 1) && lastControlKeeperQuestion < getTime() - 50) {
      lastControlKeeperQuestion = getTime();
      if (model->getView()->yesOrNoPrompt("The keeper is in trouble. Do you want to control him?")) {
        possess(getKeeper(), model->getView());
        return;
      }
    }
    if (getKeeper()->isAffected(LastingEffect::POISON) && lastControlKeeperQuestion < getTime() - 5) {
      lastControlKeeperQuestion = getTime();
      if (model->getView()->yesOrNoPrompt("The keeper is in trouble. Do you want to control him?")) {
        possess(getKeeper(), model->getView());
        return;
      }
    }
  }
  if (!contains(getCreatures(), c) || c->getLevel() != getLevel())
    return;
  for (Vec2 pos : getLevel()->getVisibleTiles(c))
    addKnownTile(pos);
}

bool PlayerControl::isDownstairsVisible() const {
  vector<Vec2> v = getLevel()->getLandingSquares(StairDirection::DOWN, StairKey::DWARF);
  return v.size() == 1 && knownPos(v[0]);
}

// after this time applying trap or building door is rescheduled (imp death, etc).
const static int timeToBuild = 50;

void PlayerControl::updateConstructions() {
  map<TrapType, vector<pair<Item*, Vec2>>> trapItems;
  for (const BuildInfo& info : concat(workshopInfo, getBuildInfo()))
    if (info.buildType == BuildInfo::TRAP)
      trapItems[info.trapInfo.type] = getTrapItems(info.trapInfo.type, myTiles);
  for (auto elem : traps)
    if (!isDelayed(elem.first)) {
      vector<pair<Item*, Vec2>>& items = trapItems.at(elem.second.type);
      if (!items.empty()) {
        if (!elem.second.armed && elem.second.marked <= getTime()) {
          Vec2 pos = items.back().second;
          taskMap.addTask(Task::applyItem(this, pos, items.back().first, elem.first), pos);
          markItem(items.back().first);
          items.pop_back();
          traps[elem.first].marked = getTime() + timeToBuild;
        }
      }
    }
  for (auto& elem : constructions)
    if (!isDelayed(elem.first) && elem.second.marked <= getTime() && !elem.second.built) {
      if ((warnings[resourceInfo.at(elem.second.cost.id).warning]
          = (numResource(elem.second.cost.id) < elem.second.cost.value)))
        continue;
      elem.second.task = taskMap.addTaskCost(Task::construction(this, elem.first, elem.second.type), elem.first,
          elem.second.cost)->getUniqueId();
      elem.second.marked = getTime() + timeToBuild;
      takeResource(elem.second.cost);
    }
}

double PlayerControl::getTime() const {
  return getKeeper()->getTime();
}

void PlayerControl::delayDangerousTasks(const vector<Vec2>& enemyPos, double delayTime) {
  int infinity = 1000000;
  int radius = 10;
  Table<int> dist(Rectangle::boundingBox(enemyPos)
      .minusMargin(-radius)
      .intersection(getLevel()->getBounds()), infinity);
  queue<Vec2> q;
  for (Vec2 v : enemyPos) {
    dist[v] = 0;
    q.push(v);
  }
  while (!q.empty()) {
    Vec2 pos = q.front();
    q.pop();
    delayedPos[pos] = delayTime;
    if (dist[pos] >= radius)
      continue;
    for (Vec2 v : pos.neighbors8())
      if (v.inRectangle(dist.getBounds()) && dist[v] == infinity &&
          /*level->getSquare(v)->canEnterEmpty(Creature::getDefault()) &&*/ myTiles.count(v)) {
        dist[v] = dist[pos] + 1;
        q.push(v);
      }
  }
}

bool PlayerControl::isDelayed(Vec2 pos) {
  return delayedPos.count(pos) && delayedPos.at(pos) > getTime();
}

void PlayerControl::addDeityServant(Deity* deity, Vec2 deityPos, Vec2 victimPos) {
  PTask task = Task::chain(this,
      Task::destroySquare(this, victimPos),
      Task::disappear(this));
  PCreature creature = CreatureFactory::fromId(deity->getServant(), Tribe::get(TribeId::KILL_EVERYONE),
      MonsterAIFactory::singleTask(task));
  for (Vec2 v : concat({deityPos}, deityPos.neighbors8(true)))
    if (getLevel()->getSquare(v)->canEnter(creature.get())) {
      getLevel()->addCreature(v, std::move(creature));
      break;
    }
}

void PlayerControl::considerDeityFight() {
  vector<pair<Vec2, SquareType>> altars;
  vector<pair<Vec2, SquareType>> deityAltars;
  vector<SquareType> altarTypes;
  for (auto& elem : mySquares) 
    if (!elem.second.empty() && contains({SquareType::ALTAR, SquareType::CREATURE_ALTAR}, elem.first.id)) {
      altarTypes.push_back(elem.first);
      for (Vec2 v : elem.second) {
        altars.push_back({v, elem.first});
        if (elem.first.id == SquareType::ALTAR)
          deityAltars.push_back({v, elem.first});
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
    } while (victim.second.id == SquareType::ALTAR &&
        victim.second.altarInfo.habitat == attacking.second.altarInfo.habitat);
    addDeityServant(Deity::getDeity(attacking.second.altarInfo.habitat), attacking.first, victim.first);
    if (victim.second.id == SquareType::ALTAR)
      addDeityServant(Deity::getDeity(victim.second.altarInfo.habitat), victim.first, attacking.first);
  }
}

void PlayerControl::tick(double time) {
  considerDeityFight();
  model->getView()->getJukebox()->update();
  if (retired) {
    if (const Creature* c = getLevel()->getPlayer())
      if (Random.roll(30) && !myTiles.count(c->getPosition()))
        c->playerMessage("You sense horrible evil in the " + 
            getCardinalName((getKeeper()->getPosition() - c->getPosition()).getBearing().getCardinalDir()));
  }
  updateVisibleCreatures();
  setWarning(Warning::MANA, mana < 100);
  setWarning(Warning::WOOD, numResource(ResourceId::WOOD) == 0);
  setWarning(Warning::DIGGING, mySquares.at(SquareType::FLOOR).empty());
  setWarning(Warning::MINIONS, getNumMinions() <= 1);
  for (auto elem : getTaskInfo())
    if (!getAllSquares(elem.second.squares).empty() && elem.second.warning)
      setWarning(*elem.second.warning, false);
  setWarning(Warning::NO_WEAPONS, false);
  PItem genWeapon = ItemFactory::fromId(ItemId::SWORD);
  vector<Item*> freeWeapons = getAllItems([&](const Item* it) {
      return it->getType() == ItemType::WEAPON && !minionEquipment.getOwner(it);
    }, false);
  for (Creature* c : minions) {
    if (usesEquipment(c) && c->equip(genWeapon.get()) 
        && filter(freeWeapons, [&] (const Item* it) { return minionEquipment.needs(c, it); }).empty()) {
      setWarning(Warning::NO_WEAPONS, true);
      break;
    }
  }

  map<Vec2, int> extendedTiles;
  queue<Vec2> extendedQueue;
  vector<Vec2> enemyPos;
  for (Vec2 pos : myTiles) {
    if (Creature* c = getLevel()->getSquare(pos)->getCreature()) {
      if (c->getTribe() != getTribe())
        enemyPos.push_back(c->getPosition());
    }
    for (Vec2 v : pos.neighbors8())
      if (v.inRectangle(getLevel()->getBounds()) && !myTiles.count(v) && !extendedTiles.count(v) 
          && getLevel()->getSquare(v)->canEnterEmpty(Creature::getDefault())) {
        extendedTiles[v] = 1;
        extendedQueue.push(v);
      }
  }
  for (const Creature* c1 : getVisibleFriends()) {
    Creature* c = const_cast<Creature*>(c1);
    if (c->canBeMinion() && !contains(getCreatures(), c))
      importCreature(c, MinionType::NORMAL);
  }
  const int maxRadius = 10;
  while (!extendedQueue.empty()) {
    Vec2 pos = extendedQueue.front();
    extendedQueue.pop();
    if (Creature* c = getLevel()->getSquare(pos)->getCreature())
      if (c->getTribe() != getTribe())
        enemyPos.push_back(c->getPosition());
    for (Vec2 v : pos.neighbors8())
      if (v.inRectangle(getLevel()->getBounds()) && !myTiles.count(v) && !extendedTiles.count(v) 
          && getLevel()->getSquare(v)->canEnterEmpty(Creature::getDefault())) {
        int a = extendedTiles[v] = extendedTiles[pos] + 1;
        if (a < maxRadius)
          extendedQueue.push(v);
      }

  }
  if (!enemyPos.empty())
    delayDangerousTasks(enemyPos, getTime() + 20);
  else
    alarmInfo.finishTime = -1000;
  bool allSurrender = !retired;
  for (Vec2 v : enemyPos)
    if (!prisonerInfo.count(NOTNULL(getLevel()->getSquare(v)->getCreature()))) {
      allSurrender = false;
      break;
    }
  if (allSurrender) {
    for (auto elem : copyOf(prisonerInfo))
      if (elem.second.state == PrisonerInfo::SURRENDER) {
        Creature* c = elem.first;
        Vec2 pos = c->getPosition();
        if (myTiles.count(pos) && !c->isDead()) {
          getLevel()->globalMessage(pos, c->getTheName() + " surrenders.");
          c->die(nullptr, true, false);
          addCreature(CreatureFactory::fromId(CreatureId::PRISONER, getTribe(),
                MonsterAIFactory::collective(getCollective())), pos, MinionType::PRISONER);
        }
        prisonerInfo.erase(c);
      }
  }
  updateConstructions();
  for (ItemFetchInfo elem : getFetchInfo()) {
    for (Vec2 pos : myTiles)
      fetchItems(pos, elem);
    for (SquareType type : elem.additionalPos)
      for (Vec2 pos : mySquares.at(type))
        fetchItems(pos, elem);
  }
}

void PlayerControl::fetchItems(Vec2 pos, ItemFetchInfo elem, bool ignoreDelayed) {
  if ((isDelayed(pos) && !ignoreDelayed) 
      || (traps.count(pos) && traps.at(pos).type == TrapType::BOULDER && traps.at(pos).armed == true))
    return;
  for (SquareType type : elem.destination)
    if (mySquares[type].count(pos))
      return;
  vector<Item*> equipment = getLevel()->getSquare(pos)->getItems(elem.predicate);
  if (!equipment.empty()) {
    vector<Vec2> destination = getAllSquares(elem.destination);
    if (!destination.empty()) {
      setWarning(elem.warning, false);
      if (elem.oneAtATime)
        equipment = {equipment[0]};
      taskMap.addTask(Task::bringItem(this, pos, equipment, destination), pos);
      for (Item* it : equipment)
        markItem(it);
    } else
      setWarning(elem.warning, true);
  }
}

bool PlayerControl::canSee(const Creature* c) const {
  return canSee(c->getPosition());
}

bool PlayerControl::canSee(Vec2 position) const {
  return knownPos(position);
}

bool PlayerControl::knownPos(Vec2 position) const {
  return knownTiles[position];
}

vector<const Creature*> PlayerControl::getUnknownAttacker() const {
  return {};
}

const Tribe* PlayerControl::getTribe() const {
  return getCollective()->getTribe();
}

Tribe* PlayerControl::getTribe() {
  return getCollective()->getTribe();
}

bool PlayerControl::isEnemy(const Creature* c) const {
  return getKeeper()->isEnemy(c);
}

void PlayerControl::onChangeLevelEvent(const Creature* c, const Level* from, Vec2 pos, const Level* to, Vec2 toPos) {
  if (c == possessed) { 
    teamLevelChanges[from] = pos;
    if (!levelChangeHistory.count(to))
      levelChangeHistory[to] = toPos;
  }
}

static const int alarmTime = 100;

void PlayerControl::onAlarmEvent(const Level* l, Vec2 pos) {
  if (l == getLevel()) {
    alarmInfo.finishTime = getTime() + alarmTime;
    alarmInfo.position = pos;
    for (Creature* c : minions)
      if (c->isAffected(LastingEffect::SLEEP))
        c->removeEffect(LastingEffect::SLEEP);
  }
}

void PlayerControl::onTechBookEvent(Technology* tech) {
  if (retired) {
    messageBuffer.addImportantMessage("The tome describes the knowledge of " + tech->getName()
        + ", but you do not comprehend it.");
    return;   
  }
  vector<Technology*> nextTechs = Technology::getNextTechs(technologies);
  if (tech == nullptr) {
    if (!nextTechs.empty())
      tech = chooseRandom(nextTechs);
    else
      tech = chooseRandom(Technology::getAll());
  }
  if (!contains(technologies, tech)) {
    if (!contains(nextTechs, tech))
      messageBuffer.addImportantMessage("The tome describes the knowledge of " + tech->getName()
          + ", but you do not comprehend it.");
    else {
      messageBuffer.addImportantMessage("You have acquired the knowledge of " + tech->getName());
      acquireTech(tech, true);
    }
  } else {
    messageBuffer.addImportantMessage("The tome describes the knowledge of " + tech->getName()
        + ", which you already possess.");
  }
}

void PlayerControl::onEquipEvent(const Creature* c, const Item* it) {
  if (possessed == c)
    minionEquipment.own(c, it);
}

void PlayerControl::onPickupEvent(const Creature* c, const vector<Item*>& items) {
  if (possessed == c)
    for (Item* it : items)
      if (minionEquipment.isItemUseful(it))
        minionEquipment.own(c, it);
}

void PlayerControl::onSurrenderEvent(Creature* who, const Creature* to) {
  if (contains(getCreatures(), to) && !contains(getCreatures(), who) && !prisonerInfo.count(who) && !who->isAnimal())
    prisonerInfo[who] = {PrisonerInfo::SURRENDER, false};
}

void PlayerControl::onTortureEvent(Creature* who, const Creature* torturer) {
  mana += 1;
}

MoveInfo PlayerControl::getGuardPostMove(Creature* c) {
  if (contains({MinionType::BEAST, MinionType::PRISONER, MinionType::KEEPER}, getMinionType(c)))
    return NoMove;
  vector<Vec2> pos = getKeys(guardPosts);
  for (Vec2 v : pos)
    if (guardPosts.at(v).attender == c) {
      pos = {v};
      break;
    }
  for (Vec2 v : pos) {
    GuardPostInfo& info = guardPosts.at(v);
    if (!info.attender || info.attender == c) {
      info.attender = c;
      minionTaskStrings[c->getUniqueId()] = "guarding";
      if (c->getPosition().dist8(v) > 1)
        if (auto action = c->moveTowards(v))
          return {1.0, action};
      break;
    }
  }
  return NoMove;
}

MoveInfo PlayerControl::getPossessedMove(Creature* c) {
  Optional<Vec2> v;
  if (possessed->getLevel() != c->getLevel()) {
    if (teamLevelChanges.count(c->getLevel())) {
      v = teamLevelChanges.at(c->getLevel());
      if (v == c->getPosition() && c->applySquare())
        return {1.0, c->applySquare()};
    }
  } else 
    v = possessed->getPosition();
  if (v)
    if (auto action = c->moveTowards(*v))
      return {1.0, action};
  return NoMove;
}

MoveInfo PlayerControl::getBacktrackMove(Creature* c) {
  if (!levelChangeHistory.count(c->getLevel()))
    return NoMove;
  Vec2 target = levelChangeHistory.at(c->getLevel());
  if (c->getPosition() == target && c->applySquare())
    return {1.0, c->applySquare()};
  else if (auto action = c->moveTowards(target))
    return {1.0, action};
  else
    return NoMove;
}

MoveInfo PlayerControl::getAlarmMove(Creature* c) {
  if (alarmInfo.finishTime > c->getTime())
    if (auto action = c->moveTowards(alarmInfo.position))
      return {1.0, action};
  return NoMove;
}

MoveInfo PlayerControl::getDropItems(Creature *c) {
  if (myTiles.count(c->getPosition())) {
    vector<Item*> items = c->getEquipment().getItems([this, c](const Item* item) {
        return minionEquipment.isItemUseful(item) && minionEquipment.getOwner(item) != c; });
    if (!items.empty() && c->drop(items))
      return {1.0, c->drop(items)};
  }
  return NoMove;
}

PTask PlayerControl::getPrisonerTask(Creature* prisoner) {
  switch (prisonerInfo.at(prisoner).state) {
    case PrisonerInfo::EXECUTE: return Task::kill(this, prisoner);
    case PrisonerInfo::TORTURE: return Task::torture(this, prisoner);
    case PrisonerInfo::SACRIFICE: return Task::sacrifice(this, prisoner);
    default: return nullptr;
  }
}

void PlayerControl::onKillCancelled(Creature* c) {
  if (prisonerInfo.count(c))
    prisonerInfo.at(c).marked = false;
}

MoveInfo PlayerControl::getMinionMove(Creature* c) {
  if (possessed && contains(team, c))
    return getPossessedMove(c);
  if (c->getLevel() != getLevel())
    return getBacktrackMove(c);
  if (!contains({MinionType::KEEPER, MinionType::PRISONER}, getMinionType(c)))
    if (MoveInfo alarmMove = getAlarmMove(c))
      return alarmMove;
  if (MoveInfo dropMove = getDropItems(c))
    return dropMove;
  if (MoveInfo guardPostMove = getGuardPostMove(c))
    return guardPostMove;
  if (Task* task = taskMap.getTask(c)) {
    if (task->isDone()) {
      taskMap.removeTask(task);
    } else
      return task->getMove(c);
  }
  if (usesEquipment(c))
    autoEquipment(c, Random.roll(10));
  if (c != getKeeper() || !underAttack())
    for (Vec2 v : getAllSquares(equipmentStorage))
      for (Item* it : getLevel()->getSquare(v)->getItems([this, c] (const Item* it) {
            return minionEquipment.getOwner(it) == c; })) {
        PTask t;
        if (c->equip(it))
          t = Task::equipItem(this, v, it);
        else
          t = Task::pickItem(this, v, {it});
        if (t->getMove(c)) {
          taskMap.addTask(std::move(t), c);
          return taskMap.getTask(c)->getMove(c);
        }
      }
  if (!contains({MinionType::BEAST, MinionType::PRISONER, MinionType::KEEPER}, getMinionType(c)))
    for (auto& elem : prisonerInfo)
      if (!elem.second.marked) {
        if (PTask t = getPrisonerTask(elem.first)) {
          taskMap.addTask(std::move(t), c);
          elem.second.marked = true;
          return taskMap.getTask(c)->getMove(c);
        }
      }
  minionTasks.at(c->getUniqueId()).update();
  if (c->getHealth() < 1 && c->canSleep() && !c->isAffected(LastingEffect::POISON))
    for (MinionTask t : {MinionTask::SLEEP, MinionTask::GRAVE, MinionTask::LAIR})
      if (minionTasks.at(c->getUniqueId()).containsState(t)) {
        minionTasks.at(c->getUniqueId()).setState(t);
        break;
      }
  if (c == getKeeper() && !myTiles.empty() && !myTiles.count(c->getPosition()))
    if (auto action = c->moveTowards(chooseRandom(myTiles)))
      return {1.0, action};
  MinionTaskInfo info = getTaskInfo().at(minionTasks.at(c->getUniqueId()).getState());
  switch (info.type) {
    case MinionTaskInfo::APPLY_SQUARE:
      if (getAllSquares(info.squares, info.centerOnly).empty()) {
        minionTasks.at(c->getUniqueId()).updateToNext();
        if (info.warning)
          setWarning(*info.warning, true);
        return NoMove;
      }
      taskMap.addTask(Task::applySquare(this, getAllSquares(info.squares, info.centerOnly)), c);
      break;
    case MinionTaskInfo::EXPLORE:
      taskMap.addTask(Task::explore(this, chooseRandom(borderTiles)), c);
      break;
  }
  if (info.warning)
    setWarning(*info.warning, false);
  minionTaskStrings[c->getUniqueId()] = info.description;
  return taskMap.getTask(c)->getMove(c);
}

bool PlayerControl::underAttack() const {
  for (Vec2 v : myTiles)
    if (const Creature* c = getLevel()->getSquare(v)->getCreature())
      if (c->getTribe() != getTribe())
        return true;
  return false;
}

Task* PlayerControl::TaskMap::getTaskForImp(Creature* c) {
  Task* closest = nullptr;
  for (PTask& task : tasks) {
    if (auto pos = getPosition(task.get())) {
      double dist = (*pos - c->getPosition()).length8();
      const Creature* owner = getOwner(task.get());
      if ((!owner || (task->canTransfer() && (*pos - owner->getPosition()).length8() > dist))
          && (!closest || dist < (*getPosition(closest) - c->getPosition()).length8()
              || priorityTasks.contains(task.get()))
          && !isLocked(c, task.get())
          && (!delayedTasks.count(task->getUniqueId()) || delayedTasks.at(task->getUniqueId()) < c->getTime())) {
        bool valid = task->getMove(c);
        if (valid)
          closest = task.get();
        else
          lock(c, task.get());
      }
    }
  }
  return closest;
}

MoveInfo PlayerControl::getMove(Creature* c) {
  CHECK(contains(getCreatures(), c));
  if (!contains(minionByType[MinionType::IMP], c)) {
    CHECK(contains(minions, c));
    return getMinionMove(c);
  }
  if (c->getLevel() != getLevel())
    return NoMove;
  if (startImpNum == -1)
    startImpNum = minionByType[MinionType::IMP].size();
  if (Task* task = taskMap.getTask(c)) {
    if (task->isDone()) {
      taskMap.removeTask(task);
    } else
      return task->getMove(c);
  }
  if (Task* closest = taskMap.getTaskForImp(c)) {
    taskMap.takeTask(c, closest);
    return closest->getMove(c);
  } else {
    if (!myTiles.count(c->getPosition()) && getKeeper()->getLevel() == c->getLevel()) {
      Vec2 keeperPos = getKeeper()->getPosition();
      if (keeperPos.dist8(c->getPosition()) < 3)
        return NoMove;
      if (auto action = c->moveTowards(keeperPos))
        return {1.0, action};
      else
        return NoMove;
    } else
      return NoMove;
  }
}

MarkovChain<MinionTask> PlayerControl::getTasksForMinion(Creature* c) {
  const double changeFreq = 0.01;
  switch (getMinionType(c)) {
    case MinionType::KEEPER:
      return MarkovChain<MinionTask>(MinionTask::STUDY, {
          {MinionTask::LABORATORY, {}},
          {MinionTask::WORSHIP, {{MinionTask::STUDY, changeFreq}}},
          {MinionTask::STUDY, {}},
          {MinionTask::SLEEP, {{ MinionTask::STUDY, 1}}}});
    case MinionType::PRISONER:
      return MarkovChain<MinionTask>(MinionTask::PRISON, {
          {MinionTask::PRISON, {}},
          {MinionTask::SACRIFICE, {}},
          {MinionTask::TORTURE, {{ MinionTask::PRISON, 0.001}}}});
    case MinionType::GOLEM:
      return MarkovChain<MinionTask>(MinionTask::TRAIN, {{MinionTask::TRAIN, {}}});
    case MinionType::BEAST:
        return MarkovChain<MinionTask>(MinionTask::LAIR, {
            {MinionTask::LAIR, {{ MinionTask::EXPLORE, 0.5}}},
            {MinionTask::EXPLORE, {{ MinionTask::LAIR, 0.05}}}});
   case MinionType::UNDEAD:
      if (c->getName() != "vampire lord")
        return MarkovChain<MinionTask>(MinionTask::GRAVE, {
            {MinionTask::GRAVE, {{ MinionTask::TRAIN, 0.5}}},
            {MinionTask::TRAIN, {{ MinionTask::GRAVE, 0.005}}}});
      else
        return MarkovChain<MinionTask>(MinionTask::GRAVE, {
            {MinionTask::GRAVE, {{ MinionTask::TRAIN, 0.5}, { MinionTask::STUDY, 0.1}}},
            {MinionTask::STUDY, {{ MinionTask::GRAVE, 0.005}}},
            {MinionTask::WORSHIP, {{MinionTask::GRAVE, changeFreq}}},
            {MinionTask::TRAIN, {{ MinionTask::GRAVE, 0.005}}}});
    case MinionType::NORMAL: {
      double worshipTime = 0.05;
      double workshopTime = (c->getName() == "gnome" ? 0.60 : 0.2);
      double labTime = (c->getName() == "gnome" ? 0.30 : 0.1);
      double trainTime = 1 - workshopTime - labTime - worshipTime;
      return MarkovChain<MinionTask>(MinionTask::SLEEP, {
          {MinionTask::SLEEP, {
              { MinionTask::TRAIN, trainTime},
              { MinionTask::WORKSHOP, workshopTime},
              { MinionTask::WORSHIP, worshipTime},
              { MinionTask::LABORATORY, labTime}}},
          {MinionTask::WORSHIP, {{MinionTask::SLEEP, changeFreq}}},
          {MinionTask::WORKSHOP, {{ MinionTask::SLEEP, changeFreq}}},
          {MinionTask::LABORATORY, {{ MinionTask::SLEEP, changeFreq}}},
          {MinionTask::TRAIN, {{ MinionTask::SLEEP, changeFreq}}}});
      }
    case MinionType::IMP: FAIL << "Not handled by this method";
  }
  FAIL <<"pokewf";
  return MarkovChain<MinionTask>(MinionTask::TRAIN, {{MinionTask::TRAIN, {}}});
}

void PlayerControl::importCreature(Creature* c, MinionType type) {
  c->setController(PController(new Monster(c, MonsterAIFactory::collective(getCollective()))));
  addCreature(c, type);
}

Creature* PlayerControl::addCreature(PCreature creature, Vec2 v, MinionType type) {
  // strip all spawned minions
  creature->getEquipment().removeAllItems();
  addCreature(creature.get(), type);
  Creature* ret = creature.get();
  getLevel()->addCreature(v, std::move(creature));
  return ret;
}

void PlayerControl::addCreature(Creature* c) {
  if (c->hasSkill(Skill::get(SkillId::CONSTRUCTION)))
    addCreature(c, MinionType::IMP);
  else
    addCreature(c, MinionType::NORMAL);
}

class KeeperControlOverride : public Creature::MoraleOverride {
  public:
  KeeperControlOverride(PlayerControl* ctrl, Creature* c) : control(ctrl), creature(c) {}
  virtual Optional<double> getMorale() override {
    if (contains(control->team, creature) && control->possessed == control->getKeeper())
      return 1;
    else
      return Nothing();
  }

  SERIALIZATION_CONSTRUCTOR(KeeperControlOverride);

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Creature::MoraleOverride) & SVAR(control) & SVAR(creature);
    CHECK_SERIAL;
  }

  private:
  PlayerControl* SERIAL(control);
  Creature* (creature);
};

void PlayerControl::addCreature(Creature* c, MinionType type) {
  getCollective()->addCreature(c);
  if (!getKeeper()) {
    getCollective()->setLeader(c);
    type = MinionType::KEEPER;
    minionByType[type].push_back(c);
    bool seeTerrain = Options::getValue(OptionId::SHOW_MAP); 
    Vec2 radius = seeTerrain ? Vec2(400, 400) : Vec2(30, 30);
    auto pred = [&](Vec2 pos) {
      return getLevel()->getSquare(pos)->canEnterEmpty(Creature::getDefault())
            || (seeTerrain && getLevel()->getSquare(pos)->canEnterEmpty(Creature::getDefaultMinionFlyer()));
    };
    for (Vec2 pos : Rectangle(c->getPosition() - radius, c->getPosition() + radius))
      if (pos.distD(c->getPosition()) <= radius.x && pos.inRectangle(getLevel()->getBounds()) && pred(pos))
        for (Vec2 v : concat({pos}, pos.neighbors8()))
          if (v.inRectangle(getLevel()->getBounds()))
            addKnownTile(v);
  }
  if (!c->isAffected(LastingEffect::FLYING))
    c->addSectors(sectors.get());
  else
    c->addSectors(flyingSectors.get());
  minionByType[type].push_back(c);
  if (!contains({MinionType::IMP}, type)) {
    minions.push_back(c);
    c->addMoraleOverride(Creature::PMoraleOverride(new KeeperControlOverride(this, c)));
    for (Technology* t : technologies)
      if (Skill* skill = t->getSkill())
        c->addSkill(skill);
  }
  if (!contains({MinionType::IMP}, type))
    minionTasks.insert(make_pair(c->getUniqueId(), getTasksForMinion(c)));
  for (const Item* item : c->getEquipment().getItems())
    minionEquipment.own(c, item);
  if (type == MinionType::PRISONER)
    prisonerInfo[c] = {PrisonerInfo::PRISON, false};
}

// actually only called when square is destroyed
void PlayerControl::onSquareReplacedEvent(const Level* l, Vec2 pos) {
  if (l == getLevel()) {
    for (auto& elem : mySquares)
      if (elem.second.count(pos)) {
        elem.second.erase(pos);
        if (efficiencySquares.count(elem.first))
          updateEfficiency(pos, elem.first);
      }
    if (constructions.count(pos)) {
      ConstructionInfo& info = constructions.at(pos);
      info.marked = getTime() + 10; // wait a little before considering rebuilding
      info.built = false;
      info.task = -1;
    }
    sectors->add(pos);
    flyingSectors->add(pos);
  }
}

void PlayerControl::onTriggerEvent(const Level* l, Vec2 pos) {
  if (traps.count(pos) && l == getLevel()) {
    traps.at(pos).armed = false;
    if (traps.at(pos).type == TrapType::SURPRISE)
      handleSurprise(pos);
  }
}

void PlayerControl::handleSurprise(Vec2 pos) {
  Vec2 rad(8, 8);
  bool wasMsg = false;
  Creature* c = NOTNULL(getLevel()->getSquare(pos)->getCreature());
  for (Vec2 v : randomPermutation(Rectangle(pos - rad, pos + rad).getAllSquares()))
    if (getLevel()->inBounds(v))
      if (Creature* other = getLevel()->getSquare(v)->getCreature())
        if (contains(minions, other)
            && !contains({MinionType::IMP, MinionType::KEEPER, MinionType::PRISONER}, getMinionType(other))
            && v.dist8(pos) > 1) {
          for (Vec2 dest : pos.neighbors8(true))
            if (getLevel()->canMoveCreature(other, dest - v)) {
              getLevel()->moveCreature(other, dest - v);
              other->playerMessage("Surprise!");
              if (!wasMsg) {
                c->playerMessage("Surprise!");
                wasMsg = true;
              }
              break;
            }
        }
}

void PlayerControl::onConqueredLand(const string& name) {
  if (retired)
    return;
  model->conquered(*getKeeper()->getFirstName() + " the Keeper", name, kills, getDangerLevel() + points);
}

void PlayerControl::onCombatEvent(const Creature* c) {
  CHECK(c != nullptr);
  if (contains(minions, c))
    lastCombat[c] = c->getTime();
}

bool PlayerControl::isInCombat(const Creature* c) const {
  double timeout = 5;
  return lastCombat.count(c) && lastCombat.at(c) > c->getTime() -5;
}

void PlayerControl::TaskMap::freeTaskDelay(Task* t, double d) {
  freeTask(t);
  delayedTasks[t->getUniqueId()] = d;
}

void PlayerControl::TaskMap::setPriorityTasks(Vec2 pos) {
  for (Task* t : getTasks(pos))
    priorityTasks.insert(t);
}

void PlayerControl::onCreatureKilled(const Creature* victim, const Creature* killer) {
  if (victim == getKeeper() && !retired) {
    model->gameOver(getKeeper(), kills.size(), "enemies", getDangerLevel() + points);
  }
  Creature* c = const_cast<Creature*>(victim);
  if (getMinionType(victim) == MinionType::PRISONER && killer && contains(getCreatures(), killer))
    ++executions;
  prisonerInfo.erase(c);
  freeFromGuardPost(c);
  if (contains(team, c)) {
    removeElement(team, c);
    if (team.empty())
      gatheringTeam = false;
  }
  if (Task* task = taskMap.getTask(c)) {
    if (!task->canTransfer()) {
      task->cancel();
      returnResource(taskMap.removeTask(task));
    } else
      taskMap.freeTaskDelay(task, getTime() + 50);
  }
  if (contains(minions, c))
    removeElement(minions, c);
  removeElement(minionByType[getMinionType(c)], c);
}

void PlayerControl::onKillEvent(const Creature* victim, const Creature* killer) {
  if (victim->getTribe() != getTribe() && (!killer || contains(getCreatures(), killer))) {
    double incMana = victim->getDifficultyPoints() / 3;
    mana += incMana;
    kills.push_back(victim);
    points += victim->getDifficultyPoints();
    Debug() << "Mana increase " << incMana << " from " << victim->getName();
    getKeeper()->increaseExpLevel(double(victim->getDifficultyPoints()) / 200);
  }
}
  
const Level* PlayerControl::getViewLevel() const {
  return getLevel();
}

bool PlayerControl::hasEfficiency(Vec2 pos) const {
  return squareEfficiency.count(pos);
}

const double lightBase = 0.5;
const double flattenVal = 0.9;

double PlayerControl::getEfficiency(Vec2 pos) const {
  double base = squareEfficiency.at(pos) == 8 ? 1 : 0.5;
  return base * min(1.0, (lightBase + getLevel()->getLight(pos) * (1 - lightBase)) / flattenVal);
}

const double sizeBase = 0.5;

void PlayerControl::updateEfficiency(Vec2 pos, SquareType type) {
  if (mySquares.at(type).count(pos)) {
    squareEfficiency[pos] = 0;
    for (Vec2 v : pos.neighbors8())
      if (mySquares.at(type).count(v)) {
        ++squareEfficiency[pos];
        ++squareEfficiency[v];
        CHECK(squareEfficiency[v] <= 8);
        CHECK(squareEfficiency[pos] <= 8);
      }
  } else {
    squareEfficiency.erase(pos);
    for (Vec2 v : pos.neighbors8())
      if (mySquares.at(type).count(v)) {
        --squareEfficiency[v];
        CHECK(squareEfficiency[v] >=0);
      }
  }
}

void PlayerControl::uncoverRandomLocation() {
  const Location* location = nullptr;
  for (auto loc : randomPermutation(getLevel()->getAllLocations()))
    if (!knownPos(loc->getBounds().middle())) {
      location = loc;
      break;
    }
  double radius = 8.5;
  if (location)
    for (Vec2 v : location->getBounds().middle().circle(radius))
      if (getLevel()->inBounds(v))
        addKnownTile(v);
}

void PlayerControl::onWorshipEvent(Creature* who, const Deity* to, WorshipType type) {
  if (type == WorshipType::DESTROY_ALTAR)
    return;
  if (!contains(getCreatures(), who))
    return;
  for (EpithetId id : to->getEpithets())
    switch (id) {
      case EpithetId::DEATH:
        if (!who->isUndead() && Random.roll(500))
          who->makeUndead();
        if (type == WorshipType::SACRIFICE && Random.roll(10))
          getKeeper()->makeUndead();
        break;
      case EpithetId::SECRETS:
        if (Random.roll(200) || type == WorshipType::SACRIFICE)
          uncoverRandomLocation();
        break;
      default: break;
    }
}

template <class Archive>
void PlayerControl::registerTypes(Archive& ar) {
  REGISTER_TYPE(ar, MinionController);
  REGISTER_TYPE(ar, KeeperControlOverride);
}

REGISTER_TYPES(PlayerControl);

