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
    & SVAR(adventurer);
  CHECK_SERIAL;
  Skill::serializeAll(ar);
  Deity::serializeAll(ar);
  Quest::serializeAll(ar);
  Tribe::serializeAll(ar);
  Creature::serializeAll(ar);
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
      sunlightInfo = {0, d - currentTime, SunlightInfo::NIGHT};
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
  if (playerControl) {
    playerControl->render(view);
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
    if (playerControl)
      playerControl->update(creature);
    if (!creature->isDead()) {
      Level* level = creature->getLevel();
      CHECK(level->getSquare(creature->getPosition())->getCreature() == creature);
    }
    if (unpossessed)
      break;
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
    }
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
      LevelMaker::topLevel2(CreatureFactory::forrest(), settlements));
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
      LevelMaker::topLevel(CreatureFactory::forrest(), settlements));
  Level* c1 = buildLevel(
      Level::Builder(30, 20, "Crypt"),
      LevelMaker::cryptLevel(CreatureFactory::crypt(),{StairKey::CRYPT}, {}));
 /* Level* p1 = buildLevel(
      Level::Builder(13, 13, "Pyramid Level 2"),
      LevelMaker::pyramidLevel(CreatureFactory::pyramid(1), {StairKey::PYRAMID},  {StairKey::PYRAMID}));
  Level* p2 = buildLevel(
      Level::Builder(11, 11, "Pyramid Level 3"),
      LevelMaker::pyramidLevel(CreatureFactory::pyramid(2), {}, {StairKey::PYRAMID}));*/
  Level* cellar = buildLevel(
      Level::Builder(30, 20, "Cellar"),
      LevelMaker::cellarLevel(castleNem1.first,
          SquareType::LOW_ROCK_WALL, StairLook::CELLAR, {StairKey::CASTLE_CELLAR}, {}));
  Level* dragon = buildLevel(
      Level::Builder(40, 30, capitalFirst(castleNem2.second) + "'s Cave"),
      LevelMaker::cavernLevel(CreatureFactory::singleType(Tribe::get(TribeId::DRAGON), castleNem2.first),
          SquareType::MUD_WALL, SquareType::MUD, StairLook::NORMAL, {StairKey::DRAGON}, {}));
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

Collective* Model::getNewCollective() {
  collectives.push_back(PCollective(new Collective()));
  return collectives.back().get();
}

Model* Model::heroModel(View* view) {
  Model* m = new Model(view);
  m->adventurer = true;
  Location* banditLocation = new Location("bandit hideout", "The bandits have robbed many travelers and townsfolk.");
  Level* top = m->prepareTopLevel({
      {SettlementType::CASTLE, CreatureFactory::humanCastle(), Random.getRandom(10, 20),
        getVillageLocation(), Tribe::get(TribeId::HUMAN), BuildingId::BRICK, {StairKey::CASTLE_CELLAR}, {},
          CreatureId::CASTLE_GUARD, Nothing(), ItemFactory::villageShop(), m->getNewCollective()},
      {SettlementType::VILLAGE, CreatureFactory::lizardTown(), Random.getRandom(5, 10),
          getVillageLocation(), Tribe::get(TribeId::LIZARD), BuildingId::MUD, {}, {}, Nothing(), Nothing(),
          ItemFactory::mushrooms(), m->getNewCollective()},
      {SettlementType::VILLAGE2, CreatureFactory::elvenVillage(), Random.getRandom(10, 20),
          getVillageLocation(), Tribe::get(TribeId::ELVEN), BuildingId::WOOD, {}, {}, Nothing(), Nothing(), Nothing(),
          m->getNewCollective()},
      {SettlementType::WITCH_HOUSE, CreatureFactory::singleType(Tribe::get(TribeId::MONSTER), CreatureId::WITCH),
      1, new Location(), nullptr, BuildingId::WOOD, {}, {}, Nothing(), Nothing(), Nothing(), m->getNewCollective()},
      {SettlementType::COTTAGE, CreatureFactory::singleType(Tribe::get(TribeId::BANDIT), CreatureId::BANDIT),
      Random.getRandom(4, 7), banditLocation, Tribe::get(TribeId::BANDIT), BuildingId::WOOD, {}, {}, Nothing(),
      Nothing(), Nothing(), m->getNewCollective()}
      });
  Quest::get(QuestId::BANDITS)->setLocation(banditLocation);
  Level* d1 = m->buildLevel(
      Level::Builder(60, 35, "Dwarven Halls"),
      LevelMaker::mineTownLevel({SettlementType::MINETOWN, CreatureFactory::dwarfTown(),
          Random.getRandom(10, 20), getVillageLocation(), Tribe::get(TribeId::DWARVEN),
          BuildingId::BRICK, {StairKey::DWARF}, {StairKey::DWARF}, Nothing(), Nothing(), ItemFactory::dwarfShop(),
          m->getNewCollective()}));
  Level* g1 = m->buildLevel(
      Level::Builder(60, 35, "Goblin Den"),
      LevelMaker::mineTownLevel({SettlementType::MINETOWN, CreatureFactory::goblinTown(1),
          Random.getRandom(10, 20), getVillageLocation(), Tribe::get(TribeId::GOBLIN),
          BuildingId::BRICK, {}, {StairKey::DWARF}, Nothing(), Nothing(), ItemFactory::goblinShop(),
          m->getNewCollective()}));
  vector<Level*> gnomish;
  int numGnomLevels = 8;
 // int towerLinkIndex = Random.getRandom(1, numGnomLevels - 1);
  for (int i = 0; i < numGnomLevels; ++i) {
    vector<StairKey> upKeys {StairKey::DWARF};
 /*   if (i == towerLinkIndex)
      upKeys.push_back(StairKey::TOWER);*/
    gnomish.push_back(m->buildLevel(
          Level::Builder(60, 35, "Gnomish Mines Level " + convertToString(i + 1)),
          LevelMaker::roomLevel(CreatureFactory::level(i + 1), upKeys, {StairKey::DWARF})));
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
  map<Level*, MapMemory>* levelMemory = new map<Level*, MapMemory>();
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
      ItemId::LEATHER_ARMOR, ItemId::LEATHER_HELM});
  for (int i : Range(Random.getRandom(70, 131)))
    player->take(ItemFactory::fromId(ItemId::GOLD_PIECE));
  return player;
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
      if (canRetire) {
        retireCollective();
        throw SaveGameException(GameType::RETIRED_KEEPER);
      }
      }
    case SAVE:
      if (!playerControl || playerControl->isRetired())
        throw SaveGameException(GameType::ADVENTURER);
      else
        throw SaveGameException(GameType::KEEPER);
    case ABANDON: throw GameOverException();
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
  return {{type, factory, num, new Location(true), tribe,
          BuildingId::DUNGEON, {}, {}, Nothing(), Nothing(), itemFactory, new Collective()}, controlInfo};
}

static EnemyInfo getVault(SettlementType type, CreatureId id, Tribe* tribe, int num,
    Optional<ItemFactory> itemFactory, VillageControlInfo controlInfo) {
  return getVault(type, CreatureFactory::singleType(tribe, id), tribe, num, itemFactory, controlInfo);
}

static vector<EnemyInfo> getVaults() {
  return {
    getVault(SettlementType::CAVE, chooseRandom({CreatureId::RED_DRAGON, CreatureId::GREEN_DRAGON}),
        Tribe::get(TribeId::DRAGON), 1, ItemFactory::dragonCave(), {VillageControlInfo::DRAGON}),
 /*   getVault(SettlementType::CAVE, CreatureId::GREEN_DRAGON, Tribe::get(TribeId::DRAGON), 1,
        ItemFactory::dragonCave(), {VillageControlInfo::DRAGON}),*/
    getVault(SettlementType::VAULT, CreatureFactory::insects(Tribe::get(TribeId::DRAGON)),
        Tribe::get(TribeId::DRAGON), Random.getRandom(6, 12), Nothing(), {VillageControlInfo::PEACEFUL}),
    getVault(SettlementType::VAULT, CreatureId::SPECIAL_HUMANOID, Tribe::get(TribeId::KEEPER), 1, Nothing(),
        {VillageControlInfo::PEACEFUL}),
    getVault(SettlementType::VAULT, CreatureId::GOBLIN, Tribe::get(TribeId::KEEPER), Random.getRandom(3, 8),
        Nothing(), {VillageControlInfo::PEACEFUL}),
    getVault(SettlementType::VAULT, CreatureId::RAT, Tribe::get(TribeId::MONSTER), Random.getRandom(3, 8),
        ItemFactory::armory(), {VillageControlInfo::PEACEFUL})
  };
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
    ret.push_back({
        {SettlementType::COTTAGE, CreatureFactory::humanVillage(), Random.getRandom(3, 7), new Location(),
            Tribe::get(TribeId::HUMAN), BuildingId::WOOD, {}, {}, Nothing(), Nothing(), Nothing(), new Collective()},
        {VillageControlInfo::PEACEFUL}});
  }
  append(ret, getVaults());
  append(ret, {
      {{SettlementType::CASTLE2, CreatureFactory::vikingTown(),
         Random.getRandom(12, 16),getVillageLocation(), Tribe::get(TribeId::HUMAN),
         BuildingId::WOOD_CASTLE, {}, {}, CreatureId::WARRIOR, ItemId::BEAST_MUT_BOOK, Nothing(), new Collective()},
      {VillageControlInfo::POWER_BASED, getKilledCoeff(), getPowerCoeff()}},
      {{SettlementType::VILLAGE, CreatureFactory::lizardTown(),
         Random.getRandom(8, 14), getVillageLocation(), Tribe::get(TribeId::LIZARD),
         BuildingId::MUD, {}, {}, Nothing(), ItemId::HUMANOID_MUT_BOOK, ItemFactory::mushrooms(), new Collective()},
      {VillageControlInfo::POWER_BASED, getKilledCoeff(), getPowerCoeff()}},
      {{SettlementType::VILLAGE2, CreatureFactory::elvenVillage(), Random.getRandom(11, 18),
         getVillageLocation(), Tribe::get(TribeId::ELVEN), BuildingId::WOOD, {}, {}, Nothing(),
         ItemId::SPELLS_MAS_BOOK, Nothing(), new Collective()},
      {VillageControlInfo::PEACEFUL}},
      {{SettlementType::MINETOWN, CreatureFactory::dwarfTown(),
          Random.getRandom(9, 14),getVillageLocation(true), Tribe::get(TribeId::DWARVEN),
          BuildingId::DUNGEON, {}, {}, Nothing(), Nothing(), ItemFactory::dwarfShop(), new Collective()},
      {VillageControlInfo::POWER_BASED_DISCOVER, getKilledCoeff(), getPowerCoeff()}},
      {{SettlementType::CASTLE, CreatureFactory::humanCastle(), Random.getRandom(15, 26),
         getVillageLocation(), Tribe::get(TribeId::HUMAN), BuildingId::BRICK, {}, {}, CreatureId::CASTLE_GUARD,
         Nothing(), ItemFactory::villageShop(), new Collective()},
      {VillageControlInfo::FINAL_ATTACK}},
      {{SettlementType::WITCH_HOUSE, CreatureFactory::singleType(Tribe::get(TribeId::MONSTER), CreatureId::WITCH), 1,
      new Location(), nullptr, BuildingId::WOOD, {}, {}, Nothing(), ItemId::ALCHEMY_ADV_BOOK, Nothing(),
      new Collective()},
      {VillageControlInfo::FINAL_ATTACK}},
  });
  return ret;
}

Model* Model::collectiveModel(View* view) {
  Model* m = new Model(view);
  vector<EnemyInfo> enemyInfo = getEnemyInfo();
  vector<SettlementInfo> settlements;
  for (auto& elem : enemyInfo)
    settlements.push_back(elem.settlement);
  Level* top = m->prepareTopLevel2(settlements);
  m->collectives.push_back(PCollective(new Collective()));
  Collective* keeperCollective = m->collectives.back().get();
  m->playerControl = new PlayerControl(keeperCollective, m, top);
  keeperCollective->setControl(PCollectiveControl(m->playerControl));
  PCreature c = CreatureFactory::fromId(CreatureId::KEEPER, Tribe::get(TribeId::KEEPER),
      MonsterAIFactory::collective(keeperCollective));
  Creature* ref = c.get();
  top->landCreature(StairDirection::UP, StairKey::PLAYER_SPAWN, c.get());
  m->addCreature(std::move(c));
  m->playerControl->addCreature(ref);
  for (int i : Range(4)) {
    PCreature c = CreatureFactory::fromId(CreatureId::IMP, Tribe::get(TribeId::KEEPER),
        MonsterAIFactory::collective(keeperCollective));
    top->landCreature(StairDirection::UP, StairKey::PLAYER_SPAWN, c.get());
    m->playerControl->addCreature(c.get());
    m->addCreature(std::move(c));
  }
  for (int i : All(enemyInfo)) {
    Collective* collective = enemyInfo[i].settlement.collective;
    PVillageControl control;
    if (enemyInfo[i].controlInfo.id != VillageControlInfo::FINAL_ATTACK)
      control = VillageControl::get(enemyInfo[i].controlInfo, collective, m->playerControl,
          enemyInfo[i].settlement.location);
    else
      control = VillageControl::getFinalAttack(collective, m->playerControl, enemyInfo[i].settlement.location,
          m->villageControls);
    m->villageControls.push_back(control.get());
    enemyInfo[i].settlement.collective->setControl(std::move(control));
    m->collectives.push_back(PCollective(collective));
  }
  setHandicap(Tribe::get(TribeId::KEEPER), Options::getValue(OptionId::EASY_KEEPER));
  return m;
}

Model* Model::splashModel(View* view, const Table<bool>& bitmap) {
  Model* m = new Model(view);
  Level* top = m->buildLevel(
      Level::Builder(bitmap.getWidth(), bitmap.getHeight(), "Wilderness", false), LevelMaker::grassAndTrees());
  CreatureFactory factory = CreatureFactory::splash();
  Collective* collective = new Collective();
  m->playerControl = new PlayerControl(collective, m, top);
  for (Vec2 v : bitmap.getBounds())
    if (bitmap[v]) {
      PCreature c = factory.random(MonsterAIFactory::guardSquare(v));
      Creature* ref = c.get();
      top->landCreature({bitmap.getBounds().randomVec2()}, std::move(c));
      m->playerControl->addCreature(ref);
    }
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
