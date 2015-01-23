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
#include "collective_config.h"
#include "music.h"
#include "spectator.h"

template <class Archive> 
void Model::serialize(Archive& ar, const unsigned int version) { 
  ar& SVAR(levels)
    & SVAR(collectives)
    & SVAR(mainVillains)
    & SVAR(timeQueue)
    & SVAR(deadCreatures)
    & SVAR(lastTick)
    & SVAR(levelLinks)
    & SVAR(playerControl)
    & SVAR(playerCollective)
    & SVAR(won)
    & SVAR(addHero)
    & SVAR(adventurer)
    & SVAR(currentTime)
    & SVAR(worldName)
    & SVAR(musicType)
    & SVAR(spectator);
  CHECK_SERIAL;
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
  return !spectator && (!playerControl || playerControl->isTurnBased());
}

const double dayLength = 1500;
const double nightLength = 1500;

const double duskLength  = 180;

MusicType Model::getCurrentMusic() const {
  return musicType;
}

void Model::setCurrentMusic(MusicType type) {
  musicType = type;
}

const Model::SunlightInfo& Model::getSunlightInfo() const {
  return sunlightInfo;
}

void Model::updateSunlightInfo() {
  double d = 0;
/*  if (options->getBoolValue(OptionId::START_WITH_NIGHT))
    d = -dayLength + 10;*/
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

const Creature* Model::getPlayer() const {
  for (const PLevel& l : levels)
    if (l->getPlayer())
      return l->getPlayer();
  return nullptr;
}

optional<Model::ExitInfo> Model::update(double totalTime) {
  if (addHero) {
    CHECK(playerControl && playerControl->isRetired());
    landHeroPlayer();
    addHero = false;
  }
  int absoluteTime = view->getTimeMilliAbsolute();
  if (playerControl && absoluteTime - lastUpdate > 20) {
    playerControl->render(view);
    lastUpdate = absoluteTime;
  } else
  if (spectator && absoluteTime - lastUpdate > 20) {
    view->updateView(spectator.get(), false);
    lastUpdate = absoluteTime;
  } 
  do {
    Creature* creature = timeQueue.getNextCreature();
    CHECK(creature) << "No more creatures";
    Debug() << creature->getName().the() << " moving now " << creature->getTime();
    currentTime = creature->getTime();
    if (spectator)
      while (1) {
        UserInput input = view->getAction();
        if (input.getId() == UserInputId::EXIT)
          return ExitInfo::abandonGame();
        if (input.getId() == UserInputId::IDLE)
          break;
      }
    if (playerControl && !playerControl->isTurnBased()) {
      while (1) {
        UserInput input = view->getAction();
        if (input.getId() == UserInputId::IDLE)
          break;
        else
          lastUpdate = -10;
        playerControl->processInput(view, input);
        if (exitInfo)
          return exitInfo;
      }
    }
    if (currentTime > totalTime)
      return none;
    if (currentTime >= lastTick + 1) {
      MEASURE({ tick(currentTime); }, "ticking time");
    }
    if (!creature->isDead()) {
#ifndef RELEASE
      CreatureAction::checkUsage(true);
#endif
      creature->makeMove();
#ifndef RELEASE
/*      } catch (GameOverException ex) {
        CreatureAction::checkUsage(false);
        throw ex;
      } catch (SaveGameException ex) {
        CreatureAction::checkUsage(false);
        throw ex;
      }*/
      CreatureAction::checkUsage(false);
#endif
      if (exitInfo)
        return exitInfo;
    }
    for (PCollective& c : collectives)
      c->update(creature);
    if (!creature->isDead()) {
      Level* level = creature->getLevel();
      CHECK(level->getSafeSquare(creature->getPosition())->getCreature() == creature);
    }
  } while (1);
}

const vector<Collective*> Model::getMainVillains() const {
  return mainVillains;
}

void Model::tick(double time) {
  auto previous = sunlightInfo.state;
  updateSunlightInfo();
  if (previous != sunlightInfo.state)
    GlobalEvents.addSunlightChangeEvent();
  Debug() << "Turn " << time;
  for (Creature* c : timeQueue.getAllCreatures()) {
    c->tick(time);
  }
  for (PLevel& l : levels)
    l->tick(time);
  lastTick = time;
  if (playerControl) {
    if (!playerControl->isRetired()) {
      for (PCollective& col : collectives)
        col->tick(time);
      bool conquered = true;
      for (Collective* col : mainVillains)
        conquered &= col->isConquered();
      if (conquered && !won) {
        playerControl->onConqueredLand(worldName);
        won = true;
      }
    } else // temp fix to the player gets the location message
      playerControl->tick(time);
  }
}

void Model::addCreature(PCreature c) {
  c->setTime(timeQueue.getCurrentTime() + 1 + Random.getDouble());
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

Model::Model(View* v, const string& world) : view(v), worldName(world), musicType(MusicType::PEACEFUL) {
  updateSunlightInfo();
}

Model::~Model() {
}

Level* Model::prepareTopLevel(ProgressMeter& meter, vector<SettlementInfo> settlements) {
  Level* top = buildLevel(
      Level::Builder(meter, 250, 250, "Wilderness", false),
      LevelMaker::topLevel(CreatureFactory::forrest(Tribe::get(TribeId::WILDLIFE)), settlements));
  return top;
}

static Location* getVillageLocation(bool markSurprise = false) {
  return new Location(NameGenerator::get(NameGeneratorId::TOWN)->getNext(), "", markSurprise);
}

PCreature Model::makePlayer(int handicap) {
  map<UniqueEntity<Level>::Id, MapMemory>* levelMemory = new map<UniqueEntity<Level>::Id, MapMemory>();
  PCreature player = CreatureFactory::addInventory(
      PCreature(new Creature(Tribe::get(TribeId::PLAYER),
      CATTR(
          c.viewId = ViewId::PLAYER;
          c.attr[AttrType::SPEED] = 100;
          c.weight = 90;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 13 + handicap;
          c.attr[AttrType::DEXTERITY] = 15 + handicap;
          c.barehandedDamage = 5;
          c.humanoid = true;
          c.name = "Adventurer";
          c.firstName = NameGenerator::get(NameGeneratorId::FIRST)->getNext();
          c.skills.insert(SkillId::AMBUSH);), Player::getFactory(this, levelMemory))), {
      ItemId::FIRST_AID_KIT,
      ItemId::SWORD,
      ItemId::KNIFE,
      ItemId::LEATHER_GLOVES,
      ItemId::LEATHER_ARMOR,
      ItemId::LEATHER_HELM});
  for (int i : Range(Random.get(70, 131)))
    player->take(ItemFactory::fromId(ItemId::GOLD_PIECE));
  return player;
}

double Model::getTime() const {
  return currentTime;
}

Model::ExitInfo Model::ExitInfo::saveGame(GameType type) {
  ExitInfo ret;
  ret.type = type;
  ret.abandon = false;
  return ret;
}

Model::ExitInfo Model::ExitInfo::abandonGame() {
  ExitInfo ret;
  ret.abandon = true;
  return ret;
}

bool Model::ExitInfo::isAbandoned() const {
  return abandon;
}

Model::GameType Model::ExitInfo::getGameType() const {
  CHECK(!abandon);
  return type;
}

void Model::exitAction() {
  enum Action { SAVE, RETIRE, OPTIONS, ABANDON};
  vector<View::ListElem> elems { "Save the game",
    {"Retire", playerControl->isRetired() ? View::INACTIVE : View::NORMAL} , "Change options", "Abandon the game" };
  auto ind = view->chooseFromList("Would you like to:", elems);
  if (!ind)
    return;
  switch (Action(*ind)) {
    case RETIRE:
      if (view->yesOrNoPrompt("Are you sure you want to retire your dungeon?")) {
        retireCollective();
        exitInfo = ExitInfo::saveGame(GameType::RETIRED_KEEPER);
        return;
      }
      break;
    case SAVE:
      if (!playerControl || playerControl->isRetired()) {
        exitInfo = ExitInfo::saveGame(GameType::ADVENTURER);
        return;
      } else {
        exitInfo = ExitInfo::saveGame(GameType::KEEPER);
        return;
      }
    case ABANDON:
      if (view->yesOrNoPrompt("Are you sure you want to abandon your game?")) {
        exitInfo = ExitInfo::abandonGame();
        return;
      }
      break;
    case OPTIONS: options->handle(view, OptionSet::GENERAL); break;
    default: break;
  }
}

void Model::retireCollective() {
  CHECK(playerControl);
  Statistics::init();
  playerControl->retire();
  won = false;
  addHero = true;
}

void Model::landHeroPlayer() {
  auto handicap = view->getNumber("Choose handicap (your adventurer's strength and dexterity increase)", 0, 20, 5);
  PCreature player = makePlayer(handicap.get_value_or(0));
  string advName = options->getStringValue(OptionId::ADVENTURER_NAME);
  if (!advName.empty())
    player->setFirstName(advName);
  levels[0]->landCreature(StairDirection::UP, StairKey::HERO_SPAWN, std::move(player));
  adventurer = true;
}

string Model::getGameIdentifier() const {
  string playerName = !adventurer
      ? *NOTNULL(playerControl)->getKeeper()->getFirstName()
      : *NOTNULL(getPlayer())->getFirstName();
  return playerName + " of " + worldName;
}

string Model::getShortGameIdentifier() const {
  string playerName = !adventurer
      ? *NOTNULL(playerControl)->getKeeper()->getFirstName()
      : *NOTNULL(getPlayer())->getFirstName();
  return playerName + "_" + worldName;
}

void Model::onKilledLeaderEvent(const Collective* victim, const Creature* leader) {
  if (playerControl && playerControl->isRetired() && playerCollective == victim) {
    const Creature* c = getPlayer();
    killedKeeper(c->getNameAndTitle(), leader->getNameAndTitle(), worldName, c->getKills(), c->getPoints());
  }
}

typedef VillageControl::Villain VillainInfo;

struct EnemyInfo {
  SettlementInfo settlement;
  CollectiveConfig config;
  vector<VillainInfo> villains;
};

static EnemyInfo getVault(SettlementType type, CreatureFactory factory, Tribe* tribe, int num,
    optional<ItemFactory> itemFactory = none, vector<VillainInfo> villains = {}) {
  return {CONSTRUCT(SettlementInfo,
      c.type = type;
      c.creatures = factory;
      c.numCreatures = num;
      c.location = new Location(true);
      c.tribe = tribe;
      c.buildingId = BuildingId::DUNGEON;
      c.shopFactory = itemFactory;), CollectiveConfig::noImmigrants(),
    villains};
}

static EnemyInfo getVault(SettlementType type, CreatureId id, Tribe* tribe, int num,
    optional<ItemFactory> itemFactory = none, vector<VillainInfo> villains = {}) {
  return getVault(type, CreatureFactory::singleType(tribe, id), tribe, num, itemFactory, villains);
}

struct FriendlyVault {
  CreatureId id;
  int min;
  int max;
};

static vector<FriendlyVault> friendlyVaults {
  {CreatureId::SPECIAL_HUMANOID, 1, 2},
  {CreatureId::ORC, 3, 8},
  {CreatureId::OGRE, 2, 5},
  {CreatureId::VAMPIRE, 2, 5},
};

static vector<EnemyInfo> getVaults() {
  vector<EnemyInfo> ret {
    getVault(SettlementType::CAVE, CreatureId::GREEN_DRAGON,
        Tribe::get(TribeId::DRAGON), 1, ItemFactory::dragonCave(),
        { CONSTRUCT(VillainInfo,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.leaderAttacks = true;
            c.triggers = LIST({AttackTriggerId::ENEMY_POPULATION, 22});
            c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_MEMBERS, 7);
            c.attackMessage = VillageControl::CREATURE_TITLE;)}),
    getVault(SettlementType::CAVE, CreatureId::RED_DRAGON,
        Tribe::get(TribeId::DRAGON), 1, ItemFactory::dragonCave(),
        { CONSTRUCT(VillainInfo,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.leaderAttacks = true;
            c.triggers = LIST({AttackTriggerId::ENEMY_POPULATION, 30});
            c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_MEMBERS, 12);
            c.attackMessage = VillageControl::CREATURE_TITLE;)}),
    getVault(SettlementType::CAVE, CreatureId::CYCLOPS,
        Tribe::get(TribeId::DRAGON), 1, ItemFactory::mushrooms(true),
        { CONSTRUCT(VillainInfo,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.leaderAttacks = true;
            c.triggers = LIST({AttackTriggerId::ENEMY_POPULATION, 15});
            c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_MEMBERS, 4);
            c.attackMessage = VillageControl::CREATURE_TITLE;)}),
    getVault(SettlementType::VAULT, CreatureFactory::insects(Tribe::get(TribeId::MONSTER)),
        Tribe::get(TribeId::MONSTER), Random.get(6, 12)),
    getVault(SettlementType::VAULT, CreatureId::ORC, Tribe::get(TribeId::KEEPER), Random.get(3, 8)),
    getVault(SettlementType::VAULT, CreatureId::CYCLOPS, Tribe::get(TribeId::MONSTER), 1,
        ItemFactory::mushrooms(true)),
    getVault(SettlementType::VAULT, CreatureId::RAT, Tribe::get(TribeId::PEST), Random.get(3, 8),
        ItemFactory::armory()),
  };
  for (int i : Range(Random.get(1, 3))) {
    FriendlyVault v = chooseRandom(friendlyVaults);
    ret.push_back(getVault(SettlementType::VAULT, v.id, Tribe::get(TribeId::KEEPER), Random.get(v.min, v.max)));
  }
  return ret;
}

vector<EnemyInfo> getEnemyInfo() {
  vector<EnemyInfo> ret;
  for (int i : Range(6, 12)) {
    ret.push_back({CONSTRUCT(SettlementInfo,
        c.type = SettlementType::COTTAGE;
        c.creatures = CreatureFactory::humanVillage(Tribe::get(TribeId::HUMAN));
        c.numCreatures = Random.get(3, 7);
        c.location = new Location();
        c.tribe = Tribe::get(TribeId::HUMAN);
        c.buildingId = BuildingId::WOOD;), CollectiveConfig::noImmigrants(), {}});
  }
  for (int i : Range(2, 5)) {
    ret.push_back({CONSTRUCT(SettlementInfo,
        c.type = SettlementType::SMALL_MINETOWN;
        c.creatures = CreatureFactory::gnomeVillage(Tribe::get(TribeId::DWARVEN));
        c.numCreatures = Random.get(3, 7);
        c.location = new Location(true);
        c.tribe = Tribe::get(TribeId::DWARVEN);
        c.buildingId = BuildingId::DUNGEON;
        c.stockpiles = LIST({StockpileInfo::MINERALS, 300});), CollectiveConfig::noImmigrants(), {}});
  }
  ret.push_back({CONSTRUCT(SettlementInfo,
        c.type = SettlementType::ISLAND_VAULT;
        c.location = new Location(true);
        c.buildingId = BuildingId::DUNGEON;
        c.stockpiles = LIST({StockpileInfo::GOLD, 400});), CollectiveConfig::noImmigrants(), {}});
  append(ret, getVaults());
  append(ret, {
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::CASTLE2;
          c.creatures = CreatureFactory::vikingTown(Tribe::get(TribeId::HUMAN));
          c.numCreatures = Random.get(12, 16);
          c.location = getVillageLocation();
          c.tribe = Tribe::get(TribeId::HUMAN);
          c.buildingId = BuildingId::WOOD_CASTLE;
          c.stockpiles = LIST({StockpileInfo::GOLD, 400});
          c.guardId = CreatureId::WARRIOR;
          c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::BEAST_MUT);),
       CollectiveConfig::withImmigrants(0.003, 16, {
           CONSTRUCT(ImmigrantInfo,
               c.id = CreatureId::WARRIOR;
               c.frequency = 1;
               c.traits = LIST(MinionTrait::FIGHTER);),          
           }),
       {CONSTRUCT(VillainInfo,
          c.minPopulation = 6;
          c.minTeamSize = 5;
          c.triggers = LIST({AttackTriggerId::POWER}, {AttackTriggerId::SELF_VICTIMS});
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_LEADER);
          c.attackMessage = VillageControl::TRIBE_AND_NAME;)}},
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::VILLAGE;
          c.creatures = CreatureFactory::lizardTown(Tribe::get(TribeId::LIZARD));
          c.numCreatures = Random.get(8, 14);
          c.location = getVillageLocation();
          c.tribe = Tribe::get(TribeId::LIZARD);
          c.buildingId = BuildingId::MUD;
          c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::HUMANOID_MUT);
          c.shopFactory = ItemFactory::mushrooms();),
       CollectiveConfig::withImmigrants(0.007, 15, {
           CONSTRUCT(ImmigrantInfo,
               c.id = CreatureId::LIZARDMAN;
               c.frequency = 1;
               c.traits = LIST(MinionTrait::FIGHTER);),          
           }),
       {CONSTRUCT(VillainInfo,
          c.minPopulation = 4;
          c.minTeamSize = 4;
          c.triggers = LIST({AttackTriggerId::POWER}, {AttackTriggerId::SELF_VICTIMS});
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_LEADER);
          c.attackMessage = VillageControl::TRIBE_AND_NAME;)}},
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::VILLAGE2;
          c.creatures = CreatureFactory::elvenVillage(Tribe::get(TribeId::ELVEN));
          c.numCreatures = Random.get(11, 18);
          c.location = getVillageLocation();
          c.tribe = Tribe::get(TribeId::ELVEN);
          c.stockpiles = LIST({StockpileInfo::GOLD, 400});
          c.buildingId = BuildingId::WOOD;
          c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::SPELLS_MAS);),
       CollectiveConfig::withImmigrants(0.002, 18, {
           CONSTRUCT(ImmigrantInfo,
               c.id = CreatureId::ELF_ARCHER;
               c.frequency = 1;
               c.traits = LIST(MinionTrait::FIGHTER);),          
           }), {}},
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::MINETOWN;
          c.creatures = CreatureFactory::dwarfTown(Tribe::get(TribeId::DWARVEN));
          c.numCreatures = Random.get(9, 14);
          c.location = getVillageLocation(true);
          c.tribe = Tribe::get(TribeId::DWARVEN);
          c.buildingId = BuildingId::DUNGEON;
          c.stockpiles = LIST({StockpileInfo::GOLD, 400}, {StockpileInfo::MINERALS, 600});
          c.shopFactory = ItemFactory::dwarfShop();),
       CollectiveConfig::withImmigrants(0.002, 15, {
           CONSTRUCT(ImmigrantInfo,
               c.id = CreatureId::DWARF;
               c.frequency = 1;
               c.traits = LIST(MinionTrait::FIGHTER);),          
           }),
       {CONSTRUCT(VillainInfo,
          c.minPopulation = 3;
          c.minTeamSize = 4;
          c.triggers = LIST({AttackTriggerId::POWER}, {AttackTriggerId::SELF_VICTIMS});
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_MEMBERS, 3);
          c.attackMessage = VillageControl::TRIBE_AND_NAME;)}},
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::CASTLE;
          c.creatures = CreatureFactory::humanCastle(Tribe::get(TribeId::HUMAN));
          c.numCreatures = Random.get(20, 26);
          c.location = getVillageLocation();
          c.tribe = Tribe::get(TribeId::HUMAN);
          c.stockpiles = LIST({StockpileInfo::GOLD, 400});
          c.buildingId = BuildingId::BRICK;
          c.guardId = CreatureId::CASTLE_GUARD;
          c.shopFactory = ItemFactory::villageShop();),
       CollectiveConfig::withImmigrants(0.003, 26, {
           CONSTRUCT(ImmigrantInfo,
               c.id = CreatureId::KNIGHT;
               c.frequency = 1;
               c.traits = LIST(MinionTrait::FIGHTER);),
           CONSTRUCT(ImmigrantInfo,
               c.id = CreatureId::ARCHER;
               c.frequency = 1;
               c.traits = LIST(MinionTrait::FIGHTER);),          
           }),
       {CONSTRUCT(VillainInfo,
          c.minPopulation = 12;
          c.minTeamSize = 10;
          c.triggers = LIST({AttackTriggerId::POWER}, {AttackTriggerId::SELF_VICTIMS});
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_LEADER);
          c.attackMessage = VillageControl::TRIBE_AND_NAME;)}},
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::WITCH_HOUSE;
          c.creatures = CreatureFactory::singleType(Tribe::get(TribeId::MONSTER), CreatureId::WITCH);
          c.numCreatures = 1;
          c.location = new Location();
          c.tribe = Tribe::get(TribeId::MONSTER);
          c.buildingId = BuildingId::WOOD;
          c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::ALCHEMY_ADV);), CollectiveConfig::noImmigrants(), {}},
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::CAVE;
          c.creatures = CreatureFactory::singleType(Tribe::get(TribeId::BANDIT), CreatureId::BANDIT);
          c.numCreatures = Random.get(4, 9);
          c.location = new Location();
          c.tribe = Tribe::get(TribeId::BANDIT);
          c.buildingId = BuildingId::DUNGEON;),
       CollectiveConfig::withImmigrants(0.001, 10, {
           CONSTRUCT(ImmigrantInfo,
               c.id = CreatureId::BANDIT;
               c.frequency = 1;
               c.traits = LIST(MinionTrait::FIGHTER);),
           }),
       {CONSTRUCT(VillainInfo,
          c.minPopulation = 0;
          c.minTeamSize = 3;
          c.triggers = LIST({AttackTriggerId::GOLD, 300});
          c.behaviour = VillageBehaviour(VillageBehaviourId::STEAL_GOLD);
          c.attackMessage = VillageControl::TRIBE_AND_NAME;)}},
  });
  return ret;
}

static CollectiveConfig getKeeperConfig(bool fastImmigration) {
  return CollectiveConfig::keeper(
      fastImmigration ? 0.1 : 0.011,
      500,
      3,
      {
      CONSTRUCT(ImmigrantInfo,
        c.id = CreatureId::GOBLIN;
        c.frequency = 1;
        c.attractions = LIST(
          {{AttractionId::SQUARE, SquareId::WORKSHOP}, 1.0, 12.0},
          {{AttractionId::SQUARE, SquareId::JEWELER}, 1.0, 9.0},
          {{AttractionId::SQUARE, SquareId::FORGE}, 1.0, 9.0},
          );
        c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_EQUIPMENT);
        c.salary = 10;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::ORC;
          c.frequency = 0.7;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 1.0, 12.0},
            );
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 20;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::ORC_SHAMAN;
          c.frequency = 0.15;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::LIBRARY}, 1.0, 16.0},
            {{AttractionId::SQUARE, SquareId::LABORATORY}, 1.0, 9.0},
            );
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 20;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::OGRE;
          c.frequency = 0.3;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 3.0, 16.0}
            );
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 40;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::HARPY;
          c.frequency = 0.3;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 3.0, 16.0},
            {{AttractionId::ITEM_INDEX, ItemIndex::RANGED_WEAPON}, 1.0, 3.0, true}
            );
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 40;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::SPECIAL_HUMANOID;
          c.frequency = 0.2;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 3.0, 16.0},
            );
          c.traits = {MinionTrait::FIGHTER};
          c.spawnAtDorm = true;
          c.techId = TechId::HUMANOID_MUT;
          c.salary = 40;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::ZOMBIE;
          c.frequency = 0.5;
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 10;
          c.spawnAtDorm = true;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::VAMPIRE;
          c.frequency = 0.3;
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 40;
          c.spawnAtDorm = true;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 2.0, 12.0}
            );),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::LOST_SOUL;
          c.frequency = 0.3;
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 0;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::RITUAL_ROOM}, 1.0, 9.0}
            );
          c.spawnAtDorm = true;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::SUCCUBUS;
          c.frequency = 0.3;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_EQUIPMENT);
          c.salary = 0;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::RITUAL_ROOM}, 2.0, 12.0}
            );
          c.spawnAtDorm = true;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::DOPPLEGANGER;
          c.frequency = 0.2;
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 0;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::RITUAL_ROOM}, 4.0, 12.0}
            );
          c.spawnAtDorm = true;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::RAVEN;
          c.frequency = 1.0;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.limit = Model::SunlightInfo::DAY;
          c.salary = 0;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::BAT;
          c.frequency = 1.0;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.limit = Model::SunlightInfo::NIGHT;
          c.salary = 0;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::WOLF;
          c.frequency = 0.15;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.groupSize = Range(3, 9);
          c.autoTeam = true;
          c.salary = 0;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::CAVE_BEAR;
          c.frequency = 0.1;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.salary = 0;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::WEREWOLF;
          c.frequency = 0.1;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 4.0, 12.0}
            );
          c.salary = 0;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::SPECIAL_MONSTER_KEEPER;
          c.frequency = 0.2;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.spawnAtDorm = true;
          c.techId = TechId::BEAST_MUT;
          c.salary = 0;)});
}

static EnumMap<CollectiveResourceId, int> getKeeperCredit(bool resourceBonus) {
  return {{CollectiveResourceId::MANA, 200}};
  if (resourceBonus) {
    EnumMap<CollectiveResourceId, int> credit;
    for (auto elem : ENUM_ALL(CollectiveResourceId))
      credit[elem] = 10000;
    return credit;
  }
 
}

Model* Model::collectiveModel(ProgressMeter& meter, Options* options, View* view, const string& worldName) {
  Model* m = new Model(view, worldName);
  m->setOptions(options);
  vector<EnemyInfo> enemyInfo = getEnemyInfo();
  vector<SettlementInfo> settlements;
  for (auto& elem : enemyInfo) {
    elem.settlement.collective =
      new CollectiveBuilder(elem.config, elem.settlement.tribe);
    settlements.push_back(elem.settlement);
  }
  Level* top = m->prepareTopLevel(meter, settlements);
  m->collectives.push_back(CollectiveBuilder(
        getKeeperConfig(options->getBoolValue(OptionId::FAST_IMMIGRATION)), Tribe::get(TribeId::KEEPER))
      .setLevel(top)
      .setCredit(getKeeperCredit(options->getBoolValue(OptionId::STARTING_RESOURCE)))
      .build("Keeper"));
 
  m->playerCollective = m->collectives.back().get();
  m->playerControl = new PlayerControl(m->playerCollective, m, top);
  m->playerCollective->setControl(PCollectiveControl(m->playerControl));
  PCreature c = CreatureFactory::fromId(CreatureId::KEEPER, Tribe::get(TribeId::KEEPER),
      MonsterAIFactory::collective(m->playerCollective));
  string keeperName = options->getStringValue(OptionId::KEEPER_NAME);
  if (!keeperName.empty())
    c->setFirstName(keeperName);
  Creature* ref = c.get();
  top->landCreature(StairDirection::UP, StairKey::PLAYER_SPAWN, c.get());
  m->addCreature(std::move(c));
  m->playerControl->addKeeper(ref);
  for (int i : Range(4)) {
    PCreature c = CreatureFactory::fromId(CreatureId::IMP, Tribe::get(TribeId::KEEPER),
        MonsterAIFactory::collective(m->playerCollective));
    top->landCreature(StairDirection::UP, StairKey::PLAYER_SPAWN, c.get());
    m->playerControl->addImp(c.get());
    m->addCreature(std::move(c));
  }
  for (int i : All(enemyInfo)) {
    if (!enemyInfo[i].settlement.collective->hasCreatures())
      continue;
    PVillageControl control;
    Location* location = enemyInfo[i].settlement.location;
    PCollective collective = enemyInfo[i].settlement.collective->build(
        location->hasName() ? location->getName() : "");
    if (!enemyInfo[i].villains.empty())
      getOnlyElement(enemyInfo[i].villains).collective = m->playerCollective;
    control.reset(new VillageControl(collective.get(), location, enemyInfo[i].villains));
    if (location->hasName())
      m->mainVillains.push_back(collective.get());
    collective->setControl(std::move(control));
    m->collectives.push_back(std::move(collective));
  }
  return m;
}

View* Model::getView() {
  return view;
}

void Model::setView(View* v) {
  view = v;
}

Options* Model::getOptions() {
  return options;
}

void Model::setOptions(Options* o) {
  options = o;
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
  string text= "You have conquered this land. You killed " + toString(kills.size()) +
      " innocent beings and scored " + toString(points) +
      " points. Thank you for playing KeeperRL alpha.\n \n";
  for (string stat : Statistics::getText())
    text += stat + "\n";
  view->presentText("Victory", text);
  ofstream("highscore.txt", std::ofstream::out | std::ofstream::app)
    << title << "," << "conquered the land of " + land + "," << points << std::endl;
  showHighscore(view, true);
}

void Model::killedKeeper(const string& title, const string& keeper, const string& land,
    vector<const Creature*> kills, int points) {
  string text= "You have freed this land from the bloody reign of " + keeper + 
      ". You killed " + toString(kills.size()) +
      " enemies and scored " + toString(points) +
      " points. Thank you for playing KeeperRL alpha.\n \n";
  for (string stat : Statistics::getText())
    text += stat + "\n";
  view->presentText("Victory", text);
  ofstream("highscore.txt", std::ofstream::out | std::ofstream::app)
    << title << "," << "freed the land of " + land + "," << points << std::endl;
  showHighscore(view, true);
}

void Model::gameOver(const Creature* creature, int numKills, const string& enemiesString, int points) {
  string text = "And so dies " + creature->getNameAndTitle();
  string killer;
  if (const Creature* c = creature->getLastAttacker()) {
    killer = c->getName().a();
    text += ", killed by " + killer;
  }
  text += ". He killed " + toString(numKills) 
      + " " + enemiesString + " and scored " + toString(points) + " points.\n \n";
  for (string stat : Statistics::getText())
    text += stat + "\n";
  view->presentText("Game over", text);
  ofstream("highscore.txt", std::ofstream::out | std::ofstream::app)
    << creature->getNameAndTitle() << (killer.empty() ? "" : ", killed by " + killer) << "," << points << std::endl;
  showHighscore(view, true);
  exitInfo = ExitInfo::abandonGame();
}

const string& Model::getWorldName() const {
  return worldName;
}

namespace {

struct HighscoreElem {

  static HighscoreElem parse(const string& s) {
    vector<string> p = split(s, {','});
    if (p.size() != 2 && p.size() != 3)
      return CONSTRUCT(HighscoreElem, c.name = "ERROR: " + s;);
    HighscoreElem e;
    e.name = p[0];
    if (p.size() == 3) {
      e.killer = p[1];
      e.points = fromString<int>(p[2]);
    } else
      e.points = fromString<int>(p[1]);
    return e; 
  }

  View::ElemMod getHighlight(bool highlightLast) {
    if (highlight || !highlightLast)
      return View::NORMAL;
    else
      return View::INACTIVE;
  }

  View::ListElem toListElem(bool highlightLast) {
    if (killer)
      return View::ListElem(name + ", " + *killer, toString(points) + " points", getHighlight(highlightLast));
    else
      return View::ListElem(name, toString(points) + " points", getHighlight(highlightLast));
  }

  bool highlight = false;
  string name;
  optional<string> killer;
  int points;
};

}

void Model::showHighscore(View* view, bool highlightLast) {
  vector<HighscoreElem> v;
  ifstream in("highscore.txt");
  while (1) {
    char buf[100];
    in.getline(buf, 100);
    if (!in)
      break;
    v.push_back(HighscoreElem::parse(buf));
  }
  if (v.empty())
    return;
  if (highlightLast)
    v.back().highlight = true;
  sort(v.begin(), v.end(), [] (const HighscoreElem& a, const HighscoreElem& b) -> bool {
      return a.points > b.points;
    });
  vector<View::ListElem> scores;
  for (HighscoreElem& elem : v) {
    scores.push_back(elem.toListElem(highlightLast));
  }
  view->presentList("High scores", scores, false);
}

Model* Model::splashModel(ProgressMeter& meter, View* view) {
  Model* m = new Model(view, "");
  Level* l = m->buildLevel(
      Level::Builder(meter, Level::getSplashBounds().getW(), Level::getSplashBounds().getH(), "Wilderness", false),
      LevelMaker::splashLevel(
          CreatureFactory::singleType(Tribe::get(TribeId::HUMAN), CreatureId::AVATAR),
          CreatureFactory::splashHeroes(Tribe::get(TribeId::HUMAN)),
          CreatureFactory::splashMonsters(Tribe::get(TribeId::KEEPER)),
          CreatureFactory::singleType(Tribe::get(TribeId::KEEPER), CreatureId::IMP) 
          ));
  m->spectator.reset(new Spectator(l));
  return m;
}

