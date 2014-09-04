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

#include "model.h"
#include "player_control.h"
#include "quest.h"
#include "player.h"
#include "village_control.h"
#include "message_buffer.h"
#include "statistics.h"
#include "options.h"
#include "technology.h"
#include "level.h"
#include "pantheon.h"
#include "name_generator.h"
#include "item_factory.h"
#include "creature.h"
#include "square.h"
#include "view_id.h"
#include "collective.h"
#include "collective_builder.h"

template <class Archive> 
void Model::serialize(Archive& ar, const unsigned int version) { 
  ar& SVAR(levels)
    & SVAR(collectives)
    & SVAR(villageControls)
    & SVAR(timeQueue)
    & SVAR(deadCreatures)
    & SVAR(lastTick)
    & SVAR(levelLinks)
    & SVAR(playerControl)
    & SVAR(won)
    & SVAR(addHero)
    & SVAR(adventurer)
    & SVAR(currentTime);
  CHECK_SERIAL;
  Skill::serializeAll(ar);
  Deity::serializeAll(ar);
  Quest::serializeAll(ar);
  Tribe::serializeAll(ar);
  Technology::serializeAll(ar);
  Vision::serializeAll(ar);
  Statistics::serialize(ar, version);
  updateSunlightInfo();
}

SERIALIZABLE(Model);
SERIALIZATION_CONSTRUCTOR_IMPL(Model);

bool Model::isTurnBased() {
  return !playerControl || playerControl->isTurnBased();
}

const double dayLength = 1500;
const double nightLength = 1500;

const double duskLength  = 180;

const Model::SunlightInfo& Model::getSunlightInfo() const {
  return sunlightInfo;
}

void Model::updateSunlightInfo() {
  double d = 0;
  if (Options::getValue(OptionId::START_WITH_NIGHT))
    d = -dayLength;
  while (1) {
    d += dayLength;
    if (d > currentTime) {
      sunlightInfo = {1, d - currentTime, SunlightInfo::DAY};
      return;
    }
    d += duskLength;
    if (d > currentTime) {
      sunlightInfo = {(d - currentTime) / duskLength, d + nightLength - duskLength - currentTime,
        SunlightInfo::NIGHT};
      return;
    }
    d += nightLength - 2 * duskLength;
    if (d > currentTime) {
      sunlightInfo = {0, d + duskLength - currentTime, SunlightInfo::NIGHT};
      return;
    }
    d += duskLength;
    if (d > currentTime) {
      sunlightInfo = {1 - (d - currentTime) / duskLength, d - currentTime, SunlightInfo::NIGHT};
      return;
    }
  }
}

const char* Model::SunlightInfo::getText() {
  switch (state) {
    case NIGHT: return "night";
    case DAY: return "day";
  }
  return "";
}

const vector<VillageControl*> Model::getVillageControls() const {
  return villageControls;
}

const Creature* Model::getPlayer() const {
  for (const PLevel& l : levels)
    if (l->getPlayer())
      return l->getPlayer();
  return nullptr;
}

void Model::update(double totalTime) {
  if (addHero) {
    CHECK(playerControl && playerControl->isRetired());
    landHeroPlayer();
    addHero = false;
  }
  int absoluteTime = view->getTimeMilliAbsolute();
  if (playerControl) {
    if (absoluteTime - lastUpdate > 50) {
      playerControl->render(view);
      lastUpdate = absoluteTime;
    }
  }
  do {
    Creature* creature = timeQueue.getNextCreature();
    CHECK(creature) << "No more creatures";
    Debug() << creature->getTheName() << " moving now " << creature->getTime();
    currentTime = creature->getTime();
    if (playerControl && !playerControl->isTurnBased()) {
      while (1) {
        UserInput input = view->getAction();
        if (input.type == UserInput::IDLE)
          break;
        else
          lastUpdate = -10;
        playerControl->processInput(view, input);
      }
    }
    if (currentTime > totalTime)
      return;
    if (currentTime >= lastTick + 1) {
      MEASURE({ tick(currentTime); }, "ticking time");
    }
    bool unpossessed = false;
    if (!creature->isDead()) {
      bool wasPlayer = creature->isPlayer();
#ifndef RELEASE
      CreatureAction::checkUsage(true);
      try {
#endif
      creature->makeMove();
#ifndef RELEASE
      } catch (GameOverException ex) {
        CreatureAction::checkUsage(false);
        throw ex;
      } catch (SaveGameException ex) {
        CreatureAction::checkUsage(false);
        throw ex;
      }
      CreatureAction::checkUsage(false);
#endif
      if (wasPlayer && !creature->isPlayer())
        unpossessed = true;
    }
    for (PCollective& c : collectives)
      c->update(creature);
    if (!creature->isDead()) {
      Level* level = creature->getLevel();
      CHECK(level->getSquare(creature->getPosition())->getCreature() == creature);
    }
    if (unpossessed) {
      lastUpdate = -10;
      break;
    }
  } while (1);
}

void Model::tick(double time) {
  updateSunlightInfo();
  Debug() << "Turn " << time;
  for (Creature* c : timeQueue.getAllCreatures()) {
    c->tick(time);
  }
  for (PLevel& l : levels)
    for (Square* square : l->getTickingSquares())
      square->tick(time);
  lastTick = time;
  if (playerControl) {
    if (!playerControl->isRetired()) {
      for (PCollective& col : collectives)
        col->tick(time);
      bool conquered = true;
      for (VillageControl* control : villageControls)
        conquered &= (control->isAnonymous() || control->isConquered());
      if (conquered && !won) {
        playerControl->onConqueredLand(NameGenerator::get(NameGeneratorId::WORLD)->getNext());
        won = true;
      }
    } else // temp fix to the player gets the location message
      playerControl->tick(time);
  }
}

void Model::addCreature(PCreature c) {
  c->setTime(timeQueue.getCurrentTime() + 1);
  timeQueue.addCreature(std::move(c));
}

void Model::removeCreature(Creature* c) {
  deadCreatures.push_back(timeQueue.removeCreature(c));
}

Level* Model::buildLevel(Level::Builder&& b, LevelMaker* maker) {
  Level::Builder builder(std::move(b));
  levels.push_back(builder.build(this, maker));
  return levels.back().get();
}

Model::Model(View* v) : view(v) {
  updateSunlightInfo();
}

Model::~Model() {
}

Level* Model::prepareTopLevel2(vector<SettlementInfo> settlements) {
  Level* top = buildLevel(
      Level::Builder(250, 250, "Wilderness", false),
      LevelMaker::topLevel2(CreatureFactory::forrest(Tribe::get(TribeId::WILDLIFE)), settlements));
  return top;
}

Level* Model::prepareTopLevel(vector<SettlementInfo> settlements) {
  pair<CreatureFactory, string> castleNem1 = chooseRandom<pair<CreatureFactory, string>>(
      {{CreatureFactory::singleType(Tribe::get(TribeId::CASTLE_CELLAR), CreatureId::GHOST),
          "The castle cellar is haunted. Go and kill the evil that is lurking there."},
      {CreatureFactory::insects(Tribe::get(TribeId::CASTLE_CELLAR)),
          "The castle cellar is infested by vermin. Go and clean it up."}}, {1, 1});
  pair<CreatureId, string> castleNem2 = chooseRandom<pair<CreatureId, string>>(
      {{CreatureId::RED_DRAGON, "dragon"}, {CreatureId::GREEN_DRAGON, "dragon"}, {CreatureId::CYCLOPS, "cyclops"}},
      {1, 1,1});
  Quest::set(QuestId::DRAGON, Quest::killTribeQuest(Tribe::get(TribeId::DRAGON), "A " + castleNem2.second + 
      " is harrasing our village. Kill it. It lives in a cave not far from here."));
  Quest::set(QuestId::CASTLE_CELLAR, Quest::killTribeQuest(Tribe::get(TribeId::CASTLE_CELLAR), castleNem1.second));
  Quest::set(QuestId::BANDITS, Quest::killTribeQuest(Tribe::get(TribeId::BANDIT),
        "There is a bandit camp nearby. Kill them all."));
  Quest::set(QuestId::DWARVES, Quest::killTribeQuest(Tribe::get(TribeId::DWARVEN),
        "Slay our enemy, the dwarf baron. I will reward you.", true));
  Quest::set(QuestId::GOBLINS, Quest::killTribeQuest(Tribe::get(TribeId::GOBLIN),
        "The goblin den is located deep under the earth. "
      "Slay the great goblin. I will reward you.", true));
  Level* top = buildLevel(
      Level::Builder(500, 500, "Wilderness", false),
      LevelMaker::topLevel(CreatureFactory::forrest(Tribe::get(TribeId::WILDLIFE)), settlements));
  Level* c1 = buildLevel(
      Level::Builder(30, 20, "Crypt"),
      LevelMaker::cryptLevel(
        CreatureFactory::crypt(Tribe::get(TribeId::MONSTER)),
        CreatureFactory::coffins(Tribe::get(TribeId::MONSTER)),
        {StairKey::CRYPT}, {}));
 /* Level* p1 = buildLevel(
      Level::Builder(13, 13, "Pyramid Level 2"),
      LevelMaker::pyramidLevel(CreatureFactory::pyramid(1), {StairKey::PYRAMID},  {StairKey::PYRAMID}));
  Level* p2 = buildLevel(
      Level::Builder(11, 11, "Pyramid Level 3"),
      LevelMaker::pyramidLevel(CreatureFactory::pyramid(2), {}, {StairKey::PYRAMID}));*/
  Level* cellar = buildLevel(
      Level::Builder(30, 20, "Cellar"),
      LevelMaker::cellarLevel(castleNem1.first,
          SquareId::LOW_ROCK_WALL, StairLook::CELLAR, {StairKey::CASTLE_CELLAR}, {}));
  Level* dragon = buildLevel(
      Level::Builder(40, 30, capitalFirst(castleNem2.second) + "'s Cave"),
      LevelMaker::cavernLevel(CreatureFactory::singleType(Tribe::get(TribeId::DRAGON), castleNem2.first),
          SquareId::MUD_WALL, SquareType(SquareId::MUD), StairLook::NORMAL, {StairKey::DRAGON}, {}));
  addLink(StairDirection::DOWN, StairKey::CRYPT, top, c1);
 // addLink(StairDirection::UP, StairKey::PYRAMID, top, p1);
 // addLink(StairDirection::UP, StairKey::PYRAMID, p1, p2);
  addLink(StairDirection::DOWN, StairKey::CASTLE_CELLAR, top, cellar);
  addLink(StairDirection::DOWN, StairKey::DRAGON, top, dragon); 

  return top;
}

static Location* getVillageLocation(bool markSurprise = false) {
  return new Location(NameGenerator::get(NameGeneratorId::TOWN)->getNext(), "", markSurprise);
}

static void setHandicap(Tribe* tribe, bool easy) {
  if (easy)
    tribe->setHandicap(5);
  else
    tribe->setHandicap(0);
}

Model* Model::heroModel(View* view) {
  Model* m = new Model(view);
  m->adventurer = true;
  Location* banditLocation = new Location("bandit hideout", "The bandits have robbed many travelers and townsfolk.");
  vector<SettlementInfo> settlements {
    {.type = SettlementType::CASTLE,
     .creatures = CreatureFactory::humanCastle(Tribe::get(TribeId::HUMAN)),
     .numCreatures = Random.getRandom(10, 20),
     .location = getVillageLocation(),
     .tribe = Tribe::get(TribeId::HUMAN),
     .buildingId = BuildingId::BRICK,
     .downStairs = {StairKey::CASTLE_CELLAR},
     .guardId = CreatureId::CASTLE_GUARD,
     .shopFactory = ItemFactory::villageShop()},
    {.type = SettlementType::VILLAGE,
     .creatures = CreatureFactory::lizardTown(Tribe::get(TribeId::LIZARD)),
     .numCreatures = Random.getRandom(5, 10),
     .location = getVillageLocation(),
     .tribe = Tribe::get(TribeId::LIZARD),
     .buildingId = BuildingId::MUD,
     .shopFactory = ItemFactory::mushrooms()},
    {.type = SettlementType::VILLAGE2,
     .creatures = CreatureFactory::elvenVillage(Tribe::get(TribeId::ELVEN)),
     .numCreatures = Random.getRandom(10, 20),
     .location = getVillageLocation(),
     .tribe = Tribe::get(TribeId::ELVEN),
     .buildingId = BuildingId::WOOD},
    {.type = SettlementType::WITCH_HOUSE,
     .creatures = CreatureFactory::singleType(Tribe::get(TribeId::MONSTER), CreatureId::WITCH),
     .numCreatures = 1,
     .location = new Location(),
     .tribe = Tribe::get(TribeId::MONSTER),
     .buildingId = BuildingId::WOOD},
    {.type = SettlementType::COTTAGE,
     .creatures = CreatureFactory::singleType(Tribe::get(TribeId::BANDIT), CreatureId::BANDIT),
     .numCreatures = Random.getRandom(4, 7),
     .location = banditLocation,
     .tribe = Tribe::get(TribeId::BANDIT),
     .buildingId = BuildingId::WOOD}};
  for (auto& elem : settlements)
    elem.collective = new CollectiveBuilder(CollectiveConfigId::VILLAGE, elem.tribe);
  Level* top = m->prepareTopLevel(settlements);
  for (auto& elem : settlements)
    m->collectives.push_back(elem.collective->build());
  Quest::get(QuestId::BANDITS)->setLocation(banditLocation);
  SettlementInfo dwarfSettlement {
    .type = SettlementType::MINETOWN,
    .creatures = CreatureFactory::dwarfTown(Tribe::get(TribeId::DWARVEN)),
    .numCreatures = Random.getRandom(10, 20),
    .location = getVillageLocation(),
    .tribe = Tribe::get(TribeId::DWARVEN),
    .buildingId = BuildingId::BRICK,
    .upStairs = {StairKey::DWARF},
    .downStairs = {StairKey::DWARF}, 
    .shopFactory = ItemFactory::dwarfShop()};
  dwarfSettlement.collective = new CollectiveBuilder(CollectiveConfigId::VILLAGE, dwarfSettlement.tribe);
  Level* d1 = m->buildLevel(
      Level::Builder(60, 35, "Dwarven Halls"),
      LevelMaker::mineTownLevel(dwarfSettlement));
  m->collectives.push_back(dwarfSettlement.collective->build());
  SettlementInfo goblinSettlement {
     .type = SettlementType::MINETOWN,
     .creatures = CreatureFactory::goblinTown(Tribe::get(TribeId::GOBLIN)),
     .numCreatures = Random.getRandom(10, 20),
     .location = getVillageLocation(),
     .tribe = Tribe::get(TribeId::GOBLIN),
     .buildingId = BuildingId::BRICK,
     .upStairs = {StairKey::DWARF},
     .shopFactory = ItemFactory::goblinShop()};
  goblinSettlement.collective = new CollectiveBuilder(CollectiveConfigId::VILLAGE, goblinSettlement.tribe);
  Level* g1 = m->buildLevel(
      Level::Builder(60, 35, "Goblin Den"),
      LevelMaker::mineTownLevel(goblinSettlement));
  m->collectives.push_back(goblinSettlement.collective->build());
  vector<Level*> gnomish;
  int numGnomLevels = 8;
 // int towerLinkIndex = Random.getRandom(1, numGnomLevels - 1);
  for (int i = 0; i < numGnomLevels; ++i) {
    vector<StairKey> upKeys {StairKey::DWARF};
 /*   if (i == towerLinkIndex)
      upKeys.push_back(StairKey::TOWER);*/
    gnomish.push_back(m->buildLevel(
          Level::Builder(60, 35, "Gnomish Mines Level " + convertToString(i + 1)),
          LevelMaker::roomLevel(CreatureFactory::level(i + 1,
              Tribe::get(TribeId::MONSTER), Tribe::get(TribeId::DWARVEN), Tribe::get(TribeId::PEST)),
            CreatureFactory::waterCreatures(Tribe::get(TribeId::MONSTER)),
            CreatureFactory::lavaCreatures(Tribe::get(TribeId::MONSTER)), upKeys, {StairKey::DWARF})));
  }
 /* vector<Level*> tower;
  int numTowerLevels = 5;
  for (int i = 0; i < numTowerLevels; ++i)
    tower.push_back(m->buildLevel(
          Level::Builder(4, 4, "Stone Tower " + convertToString(i + 2)),
          LevelMaker::towerLevel(StairKey::TOWER, StairKey::TOWER)));

  for (int i = 0; i < numTowerLevels - 1; ++i)
    m->addLink(StairDirection::DOWN, StairKey::TOWER, tower[i + 1], tower[i]);*/

 // m->addLink(StairDirection::DOWN, StairKey::TOWER, tower[0], gnomish[towerLinkIndex]);
 // m->addLink(StairDirection::DOWN, StairKey::TOWER, top, tower.back());

  for (int i = 0; i < numGnomLevels - 1; ++i)
    m->addLink(StairDirection::DOWN, StairKey::DWARF, gnomish[i], gnomish[i + 1]);

  m->addLink(StairDirection::DOWN, StairKey::DWARF, top, d1);
  m->addLink(StairDirection::DOWN, StairKey::DWARF, d1, gnomish[0]);
  m->addLink(StairDirection::UP, StairKey::DWARF, g1, gnomish.back());
  PCreature player = m->makePlayer();
  for (int i : Range(Random.getRandom(70, 131)))
    player->take(ItemFactory::fromId(ItemId::GOLD_PIECE));
  Tribe::get(TribeId::GOBLIN)->makeSlightEnemy(player.get());
  Level* start = top;
  start->landCreature(StairDirection::UP, StairKey::PLAYER_SPAWN, std::move(player));
  setHandicap(Tribe::get(TribeId::PLAYER), Options::getValue(OptionId::EASY_ADVENTURER));
  return m;
}

PCreature Model::makePlayer() {
  map<UniqueEntity<Level>::Id, MapMemory>* levelMemory = new map<UniqueEntity<Level>::Id, MapMemory>();
  PCreature player = CreatureFactory::addInventory(
      PCreature(new Creature(Tribe::get(TribeId::PLAYER),
      CATTR(
          c.viewId = ViewId::PLAYER;
          c.speed = 100;
          c.weight = 90;
          c.size = CreatureSize::LARGE;
          c.strength = 13;
          c.dexterity = 15;
          c.barehandedDamage = 5;
          c.humanoid = true;
          c.name = "Adventurer";
          c.firstName = NameGenerator::get(NameGeneratorId::FIRST)->getNext();
          c.maxLevel = 1;
          c.skills.insert(SkillId::ARCHERY);
          c.skills.insert(SkillId::AMBUSH);), Player::getFactory(this, levelMemory))), {
      ItemId::FIRST_AID_KIT,
      ItemId::SWORD,
      ItemId::KNIFE,
      ItemId::LEATHER_GLOVES,
      ItemId::LEATHER_ARMOR,
      ItemId::LEATHER_HELM});
  for (int i : Range(Random.getRandom(70, 131)))
    player->take(ItemFactory::fromId(ItemId::GOLD_PIECE));
  return player;
}

double Model::getTime() const {
  return currentTime;
}

void Model::exitAction() {
  enum Action { SAVE, RETIRE, OPTIONS, ABANDON, CANCEL };
  vector<View::ListElem> options { "Save the game", "Retire", "Change options", "Abandon the game", "Cancel" };
  auto ind = view->chooseFromList("Would you like to:", options);
  if (!ind)
    return;
  switch (Action(*ind)) {
    case RETIRE: {
      bool canRetire = true;
      for (VillageControl* c : villageControls)
        if (c->currentlyAttacking()) {
          view->presentText("", "You can't retire now as there is an ongoing attack.");
          canRetire = false;
          break;
        }
      if (canRetire && view->yesOrNoPrompt("Are you sure you want to retire your dungeon?")) {
        retireCollective();
        throw SaveGameException(GameType::RETIRED_KEEPER);
      }
      }
      break;
    case SAVE:
      if (!playerControl || playerControl->isRetired())
        throw SaveGameException(GameType::ADVENTURER);
      else
        throw SaveGameException(GameType::KEEPER);
    case ABANDON:
      if (view->yesOrNoPrompt("Are you sure you want to abandon your game?"))
        throw GameOverException();
      break;
    case OPTIONS: Options::handle(view, OptionSet::GENERAL); break;
    default: break;
  }
}

void Model::retireCollective() {
  CHECK(playerControl);
  Statistics::init();
  Tribe::get(TribeId::KEEPER)->setHandicap(0);
  playerControl->retire();
  won = false;
  addHero = true;
}

void Model::landHeroPlayer() {
  PCreature player = makePlayer();
  levels[0]->landCreature(StairDirection::UP, StairKey::HERO_SPAWN, std::move(player));
  auto handicap = view->getNumber("Choose handicap (strength and dexterity increase)", 0, 20, 5);
  if (handicap)
    Tribe::get(TribeId::PLAYER)->setHandicap(*handicap);
  adventurer = true;
}

string Model::getGameIdentifier() const {
  if (!adventurer)
    return *NOTNULL(playerControl)->getKeeper()->getFirstName();
  else
    return *NOTNULL(getPlayer())->getFirstName();
}

void Model::onKillEvent(const Creature* victim, const Creature* killer) {
  if (playerControl && playerControl->isRetired() && victim == playerControl->getKeeper()) {
    const Creature* c = getPlayer();
    killedKeeper(c->getNameAndTitle(), playerControl->getKeeper()->getNameAndTitle(), NameGenerator::get(NameGeneratorId::WORLD)->getNext(), c->getKills(), c->getPoints());
  }
}

struct EnemyInfo {
  SettlementInfo settlement;
  VillageControlInfo controlInfo;
};

static EnemyInfo getVault(SettlementType type, CreatureFactory factory, Tribe* tribe, int num,
    Optional<ItemFactory> itemFactory, VillageControlInfo controlInfo) {
  return {{
    .type = type,
    .creatures = factory,
    .numCreatures = num,
    .location = new Location(true),
    .tribe = tribe,
    .buildingId = BuildingId::DUNGEON,
    .shopFactory = itemFactory}, controlInfo};
}

static EnemyInfo getVault(SettlementType type, CreatureId id, Tribe* tribe, int num,
    Optional<ItemFactory> itemFactory, VillageControlInfo controlInfo) {
  return getVault(type, CreatureFactory::singleType(tribe, id), tribe, num, itemFactory, controlInfo);
}

struct FriendlyVault {
  CreatureId id;
  int min;
  int max;
};

static vector<FriendlyVault> friendlyVaults {
  {CreatureId::SPECIAL_HUMANOID, 1, 2},
  {CreatureId::GOBLIN, 3, 8},
  {CreatureId::CAVE_BEAR, 2, 5},
  {CreatureId::OGRE, 2, 5},
  {CreatureId::IRON_GOLEM, 2, 5},
  {CreatureId::VAMPIRE, 2, 5},
};

static vector<EnemyInfo> getVaults() {
  vector<EnemyInfo> ret {
    getVault(SettlementType::CAVE, chooseRandom({CreatureId::RED_DRAGON, CreatureId::GREEN_DRAGON}),
        Tribe::get(TribeId::DRAGON), 1, ItemFactory::dragonCave(), {
        Options::getValue(OptionId::AGGRESSIVE_HEROES) ? VillageControlInfo::DRAGON
        : VillageControlInfo::PEACEFUL}),
 /*   getVault(SettlementType::CAVE, CreatureId::GREEN_DRAGON, Tribe::get(TribeId::DRAGON), 1,
        ItemFactory::dragonCave(), {VillageControlInfo::DRAGON}),*/
    getVault(SettlementType::VAULT, CreatureFactory::insects(Tribe::get(TribeId::MONSTER)),
        Tribe::get(TribeId::MONSTER), Random.getRandom(6, 12), Nothing(), {VillageControlInfo::PEACEFUL}),
    getVault(SettlementType::VAULT, CreatureId::GOBLIN, Tribe::get(TribeId::KEEPER), Random.getRandom(3, 8),
        Nothing(), {VillageControlInfo::PEACEFUL}),
    getVault(SettlementType::VAULT, CreatureId::CYCLOPS, Tribe::get(TribeId::MONSTER), 1,
        ItemFactory::mushrooms(true), {VillageControlInfo::PEACEFUL}),
    getVault(SettlementType::VAULT, CreatureId::RAT, Tribe::get(TribeId::PEST), Random.getRandom(3, 8),
        ItemFactory::armory(), {VillageControlInfo::PEACEFUL})
  };
  for (int i : Range(Random.getRandom(1, 3))) {
    FriendlyVault v = chooseRandom(friendlyVaults);
    ret.push_back(getVault(SettlementType::VAULT, v.id, Tribe::get(TribeId::KEEPER), Random.getRandom(v.min, v.max),
          Nothing(), {VillageControlInfo::PEACEFUL}));
  }
  return ret;
}

static double getKilledCoeff() {
  return Random.getDouble(0.1, 1.2);
};

static double getPowerCoeff() {
  if (Options::getValue(OptionId::AGGRESSIVE_HEROES))
    return Random.getDouble(0.1, 0.4);
  else
    return 0.0;
}

vector<EnemyInfo> getEnemyInfo() {
  vector<EnemyInfo> ret;
  for (int i : Range(6, 12)) {
    ret.push_back({{
        .type = SettlementType::COTTAGE,
        .creatures = CreatureFactory::humanVillage(Tribe::get(TribeId::HUMAN)),
        .numCreatures = Random.getRandom(3, 7),
        .location = new Location(),
        .tribe = Tribe::get(TribeId::HUMAN),
        .buildingId = BuildingId::WOOD},
        {VillageControlInfo::PEACEFUL}});
  }
  append(ret, getVaults());
  append(ret, {
      {{.type = SettlementType::CASTLE2,
        .creatures = CreatureFactory::vikingTown(Tribe::get(TribeId::HUMAN)),
        .numCreatures = Random.getRandom(12, 16),
        .location = getVillageLocation(),
        .tribe = Tribe::get(TribeId::HUMAN),
        .buildingId = BuildingId::WOOD_CASTLE,
        .guardId = CreatureId::WARRIOR,
        .elderLoot = ItemType(ItemId::TECH_BOOK, TechId::BEAST_MUT)},
        {VillageControlInfo::POWER_BASED, getKilledCoeff(), getPowerCoeff()}},
      {{.type = SettlementType::VILLAGE,
        .creatures = CreatureFactory::lizardTown(Tribe::get(TribeId::LIZARD)),
        .numCreatures = Random.getRandom(8, 14),
        .location = getVillageLocation(),
        .tribe = Tribe::get(TribeId::LIZARD),
        .buildingId = BuildingId::MUD,
        .elderLoot = ItemType(ItemId::TECH_BOOK, TechId::HUMANOID_MUT),
        .shopFactory = ItemFactory::mushrooms()},
        {VillageControlInfo::POWER_BASED, getKilledCoeff(), getPowerCoeff()}},
      {{.type = SettlementType::VILLAGE2,
        .creatures = CreatureFactory::elvenVillage(Tribe::get(TribeId::ELVEN)),
        .numCreatures = Random.getRandom(11, 18),
        .location = getVillageLocation(),
        .tribe = Tribe::get(TribeId::ELVEN),
        .buildingId = BuildingId::WOOD,
        .elderLoot = ItemType(ItemId::TECH_BOOK, TechId::SPELLS_MAS)},
        {VillageControlInfo::PEACEFUL}},
      {{.type = SettlementType::MINETOWN,
        .creatures = CreatureFactory::dwarfTown(Tribe::get(TribeId::DWARVEN)),
        .numCreatures = Random.getRandom(9, 14),
        .location = getVillageLocation(true),
        .tribe = Tribe::get(TribeId::DWARVEN),
        .buildingId = BuildingId::DUNGEON,
        .shopFactory = ItemFactory::dwarfShop()},
        {VillageControlInfo::POWER_BASED_DISCOVER, getKilledCoeff(), getPowerCoeff()}},
      {{.type = SettlementType::CASTLE,
        .creatures = CreatureFactory::humanCastle(Tribe::get(TribeId::HUMAN)),
        .numCreatures = Random.getRandom(20, 26),
        .location = getVillageLocation(),
        .tribe = Tribe::get(TribeId::HUMAN),
        .buildingId = BuildingId::BRICK,
        .guardId = CreatureId::CASTLE_GUARD,
        .shopFactory = ItemFactory::villageShop()},
        {VillageControlInfo::FINAL_ATTACK}},
      {{.type = SettlementType::WITCH_HOUSE,
        .creatures = CreatureFactory::singleType(Tribe::get(TribeId::MONSTER), CreatureId::WITCH),
        .numCreatures = 1,
        .location = new Location(),
        .tribe = Tribe::get(TribeId::MONSTER),
        .buildingId = BuildingId::WOOD,
        .elderLoot = ItemType(ItemId::TECH_BOOK, TechId::ALCHEMY_ADV)},
        {VillageControlInfo::PEACEFUL}},
      {{.type = SettlementType::CAVE,
        .creatures = CreatureFactory::singleType(Tribe::get(TribeId::HUMAN), CreatureId::BANDIT),
        .numCreatures = Random.getRandom(4, 9),
        .location = new Location(),
        .tribe = Tribe::get(TribeId::HUMAN),
        .buildingId = BuildingId::DUNGEON},
        {VillageControlInfo::POWER_BASED, getKilledCoeff(), getPowerCoeff()}},
  });
  return ret;
}

Model* Model::collectiveModel(View* view) {
  Model* m = new Model(view);
  vector<EnemyInfo> enemyInfo = getEnemyInfo();
  vector<SettlementInfo> settlements;
  for (auto& elem : enemyInfo) {
    elem.settlement.collective = new CollectiveBuilder(CollectiveConfigId::VILLAGE, elem.settlement.tribe);
    settlements.push_back(elem.settlement);
  }
  Level* top = m->prepareTopLevel2(settlements);
  m->collectives.push_back(PCollective(
        new Collective(top, CollectiveConfigId::KEEPER, Tribe::get(TribeId::KEEPER))));
  Collective* keeperCollective = m->collectives.back().get();
  m->playerControl = new PlayerControl(keeperCollective, m, top);
  keeperCollective->setControl(PCollectiveControl(m->playerControl));
  PCreature c = CreatureFactory::fromId(CreatureId::KEEPER, Tribe::get(TribeId::KEEPER),
      MonsterAIFactory::collective(keeperCollective));
  Creature* ref = c.get();
  top->landCreature(StairDirection::UP, StairKey::PLAYER_SPAWN, c.get());
  m->addCreature(std::move(c));
  m->playerControl->addKeeper(ref);
  for (int i : Range(4)) {
    PCreature c = CreatureFactory::fromId(CreatureId::IMP, Tribe::get(TribeId::KEEPER),
        MonsterAIFactory::collective(keeperCollective));
    top->landCreature(StairDirection::UP, StairKey::PLAYER_SPAWN, c.get());
    m->playerControl->addImp(c.get());
    m->addCreature(std::move(c));
  }
  for (int i : All(enemyInfo)) {
    PCollective collective = enemyInfo[i].settlement.collective->build();
    PVillageControl control;
    if (enemyInfo[i].controlInfo.id != VillageControlInfo::FINAL_ATTACK)
      control = VillageControl::get(enemyInfo[i].controlInfo, collective.get(), keeperCollective,
          enemyInfo[i].settlement.location);
    else
      control = VillageControl::getFinalAttack(collective.get(), keeperCollective, enemyInfo[i].settlement.location,
          m->villageControls);
    m->villageControls.push_back(control.get());
    collective->setControl(std::move(control));
    m->collectives.push_back(std::move(collective));
  }
  setHandicap(Tribe::get(TribeId::KEEPER), Options::getValue(OptionId::EASY_KEEPER));
  return m;
}

View* Model::getView() {
  return view;
}

void Model::setView(View* v) {
  view = v;
  if (playerControl)
    v->setTimeMilli(playerControl->getKeeper()->getTime() * 300);
}

void Model::addLink(StairDirection dir, StairKey key, Level* l1, Level* l2) {
  levelLinks[make_tuple(dir, key, l1)] = l2;
  levelLinks[make_tuple(opposite(dir), key, l2)] = l1;
}

Vec2 Model::changeLevel(StairDirection dir, StairKey key, Creature* c) {
  Level* current = c->getLevel();
  Level* target = levelLinks[make_tuple(dir, key, current)];
  Vec2 newPos = target->landCreature(opposite(dir), key, c);
  if (c->isPlayer()) {
    current->updatePlayer();
    target->updatePlayer();
  }
  return newPos;
}

void Model::changeLevel(Level* target, Vec2 position, Creature* c) {
  Level* current = c->getLevel();
  target->landCreature({position}, c);
  if (c->isPlayer()) {
    current->updatePlayer();
    target->updatePlayer();
  }
}
  
void Model::conquered(const string& title, const string& land, vector<const Creature*> kills, int points) {
  string text= "You have conquered this land. You killed " + convertToString(kills.size()) +
      " innocent beings and scored " + convertToString(points) +
      " points. Thank you for playing KeeperRL alpha.\n \n";
  for (string stat : Statistics::getText())
    text += stat + "\n";
  view->presentText("Victory", text);
  ofstream("highscore.txt", std::ofstream::out | std::ofstream::app)
    << title << "," << "conquered the land of " + land + "," << points << std::endl;
  showHighscore(true);
}

void Model::killedKeeper(const string& title, const string& keeper, const string& land,
    vector<const Creature*> kills, int points) {
  string text= "You have freed this land from the bloody reign of " + keeper + 
      ". You killed " + convertToString(kills.size()) +
      " enemies and scored " + convertToString(points) +
      " points. Thank you for playing KeeperRL alpha.\n \n";
  for (string stat : Statistics::getText())
    text += stat + "\n";
  view->presentText("Victory", text);
  ofstream("highscore.txt", std::ofstream::out | std::ofstream::app)
    << title << "," << "freed the land of " + land + "," << points << std::endl;
  showHighscore(true);
  throw GameOverException();
}

void Model::gameOver(const Creature* creature, int numKills, const string& enemiesString, int points) {
  string text = "And so dies ";
  string title;
  if (auto firstName = creature->getFirstName())
    title = *firstName + " ";
  title += "the " + creature->getName();
  text += title;
  string killer;
  if (const Creature* c = creature->getLastAttacker()) {
    killer = c->getName();
    text += ", killed by a " + killer;
  }
  text += ". He killed " + convertToString(numKills) 
      + " " + enemiesString + " and scored " + convertToString(points) + " points.\n \n";
  for (string stat : Statistics::getText())
    text += stat + "\n";
  view->presentText("Game over", text);
  ofstream("highscore.txt", std::ofstream::out | std::ofstream::app)
    << title << "," << "killed by a " + killer << "," << points << std::endl;
  showHighscore(true);
/*  if (view->yesOrNoPrompt("Would you like to see the last messages?"))
    messageBuffer.showHistory();*/
  throw GameOverException();
}

void Model::showCredits() {
  ifstream in("credits.txt");
  vector<View::ListElem> lines;
  while (1) {
    char buf[100];
    in.getline(buf, 100);
    if (!in)
      break;
    string s(buf);
    if (s.back() == ':')
      lines.emplace_back(s, View::TITLE);
    else
      lines.emplace_back(s, View::NORMAL);
  }
  view->presentList("Credits", lines);
}

void Model::showHighscore(bool highlightLast) {
  struct Elem {
    string name;
    string killer;
    int points;
    bool highlight = false;
  };
  vector<Elem> v;
  ifstream in("highscore.txt");
  while (1) {
    char buf[100];
    in.getline(buf, 100);
    if (!in)
      break;
    vector<string> p = split(string(buf), {','});
    CHECK(p.size() == 3) << "Input error " << p;
    Elem e;
    e.name = p[0];
    e.killer = p[1];
    e.points = convertFromString<int>(p[2]);
    v.push_back(e);
  }
  if (v.empty())
    return;
  if (highlightLast)
    v.back().highlight = true;
  sort(v.begin(), v.end(), [] (const Elem& a, const Elem& b) -> bool {
      return a.points > b.points;
    });
  vector<View::ListElem> scores;
  for (Elem& elem : v) {
    scores.push_back(View::ListElem(elem.name + ", " + elem.killer + "       " +
        convertToString(elem.points) + " points",
        highlightLast && !elem.highlight ? View::INACTIVE : View::NORMAL));
  }
  view->presentList("High scores", scores);
}
