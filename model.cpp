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
    & SVAR(currentTime);
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
    d = -dayLength + 10;
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
        if (input.getId() == UserInputId::IDLE)
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
    if (!creature->isDead()) {
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
    }
    for (PCollective& c : collectives)
      c->update(creature);
    if (!creature->isDead()) {
      Level* level = creature->getLevel();
      CHECK(level->getSquare(creature->getPosition())->getCreature() == creature);
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

Level* Model::prepareTopLevel(vector<SettlementInfo> settlements) {
  Level* top = buildLevel(
      Level::Builder(250, 250, "Wilderness", false),
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

void Model::exitAction() {
  enum Action { SAVE, RETIRE, OPTIONS, ABANDON};
  vector<View::ListElem> options { "Save the game",
    {"Retire", playerControl->isRetired() ? View::INACTIVE : View::NORMAL} , "Change options", "Abandon the game" };
  auto ind = view->chooseFromList("Would you like to:", options);
  if (!ind)
    return;
  switch (Action(*ind)) {
    case RETIRE:
      if (view->yesOrNoPrompt("Are you sure you want to retire your dungeon?")) {
        retireCollective();
        throw SaveGameException(GameType::RETIRED_KEEPER);
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
  playerControl->retire();
  won = false;
  addHero = true;
}

void Model::landHeroPlayer() {
  auto handicap = view->getNumber("Choose handicap (strength and dexterity increase)", 0, 20, 5);
  PCreature player = makePlayer(handicap.getOr(0));
  levels[0]->landCreature(StairDirection::UP, StairKey::HERO_SPAWN, std::move(player));
  adventurer = true;
}

string Model::getGameIdentifier() const {
  if (!adventurer)
    return *NOTNULL(playerControl)->getKeeper()->getFirstName();
  else
    return *NOTNULL(getPlayer())->getFirstName();
}

void Model::onKilledLeaderEvent(const Collective* victim, const Creature* leader) {
  if (playerControl && playerControl->isRetired() && playerCollective == victim) {
    const Creature* c = getPlayer();
    killedKeeper(c->getNameAndTitle(), leader->getNameAndTitle(), NameGenerator::get(NameGeneratorId::WORLD)->getNext(), c->getKills(), c->getPoints());
  }
}

typedef VillageControl::Villain VillainInfo;

struct EnemyInfo {
  SettlementInfo settlement;
  vector<VillainInfo> villains;
};

static EnemyInfo getVault(SettlementType type, CreatureFactory factory, Tribe* tribe, int num,
    Optional<ItemFactory> itemFactory = Nothing(), vector<VillainInfo> villains = {}) {
  return {CONSTRUCT(SettlementInfo,
      c.type = type;
      c.creatures = factory;
      c.numCreatures = num;
      c.location = new Location(true);
      c.tribe = tribe;
      c.buildingId = BuildingId::DUNGEON;
      c.shopFactory = itemFactory;),
    villains};
}

static EnemyInfo getVault(SettlementType type, CreatureId id, Tribe* tribe, int num,
    Optional<ItemFactory> itemFactory = Nothing(), vector<VillainInfo> villains = {}) {
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
    getVault(SettlementType::CAVE, chooseRandom({CreatureId::RED_DRAGON, CreatureId::GREEN_DRAGON}),
        Tribe::get(TribeId::DRAGON), 1, ItemFactory::dragonCave(),
        { CONSTRUCT(VillainInfo,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.triggers = LIST({AttackTriggerId::ENEMY_POPULATION, 30});
            c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_MEMBERS, 5);
            c.attackMessage = VillageControl::CREATURE_TITLE;)}),
 /*   getVault(SettlementType::CAVE, CreatureId::GREEN_DRAGON, Tribe::get(TribeId::DRAGON), 1,
        ItemFactory::dragonCave(), {VillageControlInfo::DRAGON}),*/
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
        c.buildingId = BuildingId::WOOD;), {}});
  }
  for (int i : Range(2, 5)) {
    ret.push_back({CONSTRUCT(SettlementInfo,
        c.type = SettlementType::SMALL_MINETOWN;
        c.creatures = CreatureFactory::gnomeVillage(Tribe::get(TribeId::DWARVEN));
        c.numCreatures = Random.get(3, 7);
        c.location = new Location(true);
        c.tribe = Tribe::get(TribeId::DWARVEN);
        c.buildingId = BuildingId::DUNGEON;
        c.stockpiles = LIST({StockpileInfo::MINERALS, 300});), {}});
  }
  ret.push_back({CONSTRUCT(SettlementInfo,
        c.type = SettlementType::ISLAND_VAULT;
        c.location = new Location(true);
        c.buildingId = BuildingId::DUNGEON;
        c.stockpiles = LIST({StockpileInfo::GOLD, 400});), {}});
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
          c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::SPELLS_MAS);), {}},
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::MINETOWN;
          c.creatures = CreatureFactory::dwarfTown(Tribe::get(TribeId::DWARVEN));
          c.numCreatures = Random.get(9, 14);
          c.location = getVillageLocation(true);
          c.tribe = Tribe::get(TribeId::DWARVEN);
          c.buildingId = BuildingId::DUNGEON;
          c.stockpiles = LIST({StockpileInfo::GOLD, 400}, {StockpileInfo::MINERALS, 600});
          c.shopFactory = ItemFactory::dwarfShop();),
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
       {CONSTRUCT(VillainInfo,
          c.minPopulation = 2;
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
          c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::ALCHEMY_ADV);), {}},
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::CAVE;
          c.creatures = CreatureFactory::singleType(Tribe::get(TribeId::BANDIT), CreatureId::BANDIT);
          c.numCreatures = Random.get(4, 9);
          c.location = new Location();
          c.tribe = Tribe::get(TribeId::BANDIT);
          c.buildingId = BuildingId::DUNGEON;),
       {CONSTRUCT(VillainInfo,
          c.minPopulation = 0;
          c.minTeamSize = 3;
          c.triggers = LIST({AttackTriggerId::GOLD, 300});
          c.behaviour = VillageBehaviour(VillageBehaviourId::STEAL_GOLD);
          c.attackMessage = VillageControl::TRIBE_AND_NAME;)}},
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
  Level* top = m->prepareTopLevel(settlements);
  m->collectives.push_back(CollectiveBuilder(
        CollectiveConfigId::KEEPER, Tribe::get(TribeId::KEEPER)).setLevel(top).build("Keeper"));
  m->playerCollective = m->collectives.back().get();
  m->playerControl = new PlayerControl(m->playerCollective, m, top);
  m->playerCollective->setControl(PCollectiveControl(m->playerControl));
  PCreature c = CreatureFactory::fromId(CreatureId::KEEPER, Tribe::get(TribeId::KEEPER),
      MonsterAIFactory::collective(m->playerCollective));
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
  string text= "You have conquered this land. You killed " + toString(kills.size()) +
      " innocent beings and scored " + toString(points) +
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
      ". You killed " + toString(kills.size()) +
      " enemies and scored " + toString(points) +
      " points. Thank you for playing KeeperRL alpha.\n \n";
  for (string stat : Statistics::getText())
    text += stat + "\n";
  view->presentText("Victory", text);
  ofstream("highscore.txt", std::ofstream::out | std::ofstream::app)
    << title << "," << "freed the land of " + land + "," << points << std::endl;
  showHighscore(true);
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
  text += ". He killed " + toString(numKills) 
      + " " + enemiesString + " and scored " + toString(points) + " points.\n \n";
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
  view->presentList("Credits", lines, false);
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
    e.points = fromString<int>(p[2]);
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
        toString(elem.points) + " points",
        highlightLast && !elem.highlight ? View::INACTIVE : View::NORMAL));
  }
  view->presentList("High scores", scores, false);
}
