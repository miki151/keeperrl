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
#include "music.h"
#include "trigger.h"
#include "highscores.h"
#include "level_maker.h"
#include "map_memory.h"
#include "level_builder.h"
#include "tribe.h"
#include "time_queue.h"
#include "visibility_map.h"
#include "entity_name.h"
#include "creature_attributes.h"
#include "view.h"
#include "view_index.h"
#include "stair_key.h"
#include "territory.h"

template <class Archive> 
void Model::serialize(Archive& ar, const unsigned int version) { 
  ar& SVAR(levels)
    & SVAR(collectives)
    & SVAR(villainsByType)
    & SVAR(allVillains)
    & SVAR(timeQueue)
    & SVAR(deadCreatures)
    & SVAR(lastTick)
    & SVAR(playerControl)
    & SVAR(playerCollective)
    & SVAR(won)
    & SVAR(addHero)
    & SVAR(currentTime)
    & SVAR(worldName)
    & SVAR(musicType)
    & SVAR(danglingPortal)
    & SVAR(woodCount)
    & SVAR(statistics)
    & SVAR(spectator)
    & SVAR(tribeSet)
    & SVAR(gameIdentifier)
    & SVAR(gameDisplayName)
    & SVAR(finishCurrentMusic)
    & SVAR(stairNavigation)
    & SVAR(cemetery);
  Deity::serializeAll(ar);
  if (Archive::is_loading::value)
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

optional<Position> Model::getDanglingPortal() {
  return danglingPortal;
}

void Model::setDanglingPortal(Position p) {
  danglingPortal = p;
}

void Model::resetDanglingPortal() {
  danglingPortal.reset();
}

void Model::addWoodCount(int cnt) {
  woodCount += cnt;
}

int Model::getWoodCount() const {
  return woodCount;
}

Statistics& Model::getStatistics() {
  return *statistics;
}

const Statistics& Model::getStatistics() const {
  return *statistics;
}

Tribe* Model::getPestTribe() {
  return tribeSet->pest.get();
}

Tribe* Model::getKillEveryoneTribe() {
  return tribeSet->killEveryone.get();
}

Tribe* Model::getPeacefulTribe() {
  return tribeSet->peaceful.get();
}

MusicType Model::getCurrentMusic() const {
  return musicType;
}

void Model::setCurrentMusic(MusicType type, bool now) {
  musicType = type;
  finishCurrentMusic = now;
}

bool Model::changeMusicNow() const {
  return finishCurrentMusic;
}

const Model::SunlightInfo& Model::getSunlightInfo() const {
  return sunlightInfo;
}

void Model::updateSunlightInfo() {
  double d = 0;
  while (1) {
    d += dayLength;
    if (d > currentTime) {
      sunlightInfo = {1, d - currentTime, SunlightState::DAY};
      break;
    }
    d += duskLength;
    if (d > currentTime) {
      sunlightInfo = {(d - currentTime) / duskLength, d + nightLength - duskLength - currentTime,
        SunlightState::NIGHT};
      break;
    }
    d += nightLength - 2 * duskLength;
    if (d > currentTime) {
      sunlightInfo = {0, d + duskLength - currentTime, SunlightState::NIGHT};
      break;
    }
    d += duskLength;
    if (d > currentTime) {
      sunlightInfo = {1 - (d - currentTime) / duskLength, d - currentTime, SunlightState::NIGHT};
      break;
    }
  }
}

void Model::onTechBookRead(Technology* tech) {
  if (playerControl)
    playerControl->onTechBookRead(tech);
}

void Model::onAlarm(Position pos) {
  for (auto& col : collectives)
    if (col->getTerritory().contains(pos))
      col->onAlarm(pos);
  for (const PLevel& l : levels)
    if (const Creature* c = l->getPlayer()) {
      if (pos == c->getPosition())
        c->playerMessage("An alarm sounds near you.");
      else if (pos.isSameLevel(c->getPosition()))
        c->playerMessage("An alarm sounds in the " + 
            getCardinalName(c->getPosition().getDir(pos).getBearing().getCardinalDir()));
    }
}

const char* Model::SunlightInfo::getText() {
  switch (state) {
    case SunlightState::NIGHT: return "night";
    case SunlightState::DAY: return "day";
  }
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
    Creature* creature = timeQueue->getNextCreature();
    CHECK(creature) << "No more creatures";
    //Debug() << creature->getName().the() << " moving now " << creature->getTime();
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
      CreatureAction::checkUsage(false);
#endif
      if (exitInfo)
        return exitInfo;
    }
    for (PCollective& c : collectives)
      c->update(creature);
    if (!creature->isDead())
      CHECK(creature->getPosition().getCreature() == creature);
  } while (1);
}

static vector<Collective*> empty;

const vector<Collective*>& Model::getVillains(VillainType t) const {
  if (villainsByType.count(t))
    return villainsByType.at(t);
  else
    return empty;
}

const vector<Collective*>& Model::getAllVillains() const {
  return allVillains;
}

void Model::tick(double time) {
  auto previous = sunlightInfo.state;
  updateSunlightInfo();
  if (previous != sunlightInfo.state) {
    for (PLevel& l : levels)
      l->updateSunlightMovement();
  }
  Debug() << "Turn " << time;
  for (Creature* c : timeQueue->getAllCreatures()) {
    c->tick(time);
  }
  for (PLevel& l : levels)
    l->tick(time);
  lastTick = time;
  for (PCollective& col : collectives)
    col->tick(time);
  if (playerControl) {
    if (!playerControl->isRetired()) {
      bool conquered = true;
      for (Collective* col : getVillains(VillainType::MAIN))
        conquered &= col->isConquered();
      if (!getVillains(VillainType::MAIN).empty() && conquered && !won) {
        playerControl->onConqueredLand();
        won = true;
      }
    }
  }
  if (musicType == MusicType::PEACEFUL && sunlightInfo.state == SunlightState::NIGHT)
    setCurrentMusic(MusicType::NIGHT, true);
  else if (musicType == MusicType::NIGHT && sunlightInfo.state == SunlightState::DAY)
    setCurrentMusic(MusicType::PEACEFUL, true);
}

void Model::addCreature(PCreature c, double delay) {
  c->setTime(timeQueue->getCurrentTime() + 1 + delay + Random.getDouble());
  c->setModel(this);
  timeQueue->addCreature(std::move(c));
}

void Model::killCreature(Creature* c, Creature* attacker) {
  if (attacker)
    attacker->onKilled(c);
  c->getTribe()->onMemberKilled(c, attacker);
  for (auto& col : collectives)
    col->onKilled(c, attacker);
  deadCreatures.push_back(timeQueue->removeCreature(c));
  cemetery->landCreature(cemetery->getAllPositions(), c);
}

Level* Model::buildLevel(LevelBuilder&& b, LevelMaker* maker) {
  LevelBuilder builder(std::move(b));
  levels.push_back(builder.build(this, maker, levels.size()));
  return levels.back().get();
}

Model::Model(View* v, const string& world, TribeSet&& tribes)
  : tribeSet(std::move(tribes)), view(v), worldName(world), musicType(MusicType::PEACEFUL) {
  updateSunlightInfo();
  cemetery = LevelBuilder(Random, 100, 100, "Dead creatures", false).build(this, LevelMaker::emptyLevel(Random), 0);
}

Model::~Model() {
}

PCreature Model::makePlayer(int handicap) {
  MapMemory* levelMemory = new MapMemory(getLevels());
  PCreature player = CreatureFactory::addInventory(
      PCreature(new Creature(tribeSet->adventurer.get(),
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
#ifdef RELEASE
  bool canRetire = !playerControl->isRetired() && won;
#else
  bool canRetire = !playerControl->isRetired();
#endif
  vector<ListElem> elems { "Save the game",
    {"Retire", canRetire ? ListElem::NORMAL : ListElem::INACTIVE} , "Change options", "Abandon the game" };
  auto ind = view->chooseFromList("Would you like to:", elems);
  if (!ind)
    return;
  switch (Action(*ind)) {
    case RETIRE:
      if (view->yesOrNoPrompt("Retire your dungeon and share it online?")) {
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
  statistics->clear();
  playerControl->retire();
  won = false;
  addHero = true;
  musicType = MusicType::ADV_PEACEFUL;
  finishCurrentMusic = true;
}

void Model::landHeroPlayer() {
  auto handicap = view->getNumber("Choose handicap (your adventurer's strength and dexterity increase)", 0, 20, 5);
  PCreature player = makePlayer(handicap.get_value_or(0));
  string advName = options->getStringValue(OptionId::ADVENTURER_NAME);
  if (!advName.empty())
    player->setFirstName(advName);
  CHECK(levels[0]->landCreature(StairKey::heroSpawn(), std::move(player))) << "No place to spawn player";
}

string Model::getGameDisplayName() const {
  return gameDisplayName;
}

string Model::getGameIdentifier() const {
  return gameIdentifier;
}

void Model::onKilledLeader(const Collective* victim, const Creature* leader) {
  if (playerControl && playerControl->isRetired() && playerCollective == victim) {
    const Creature* c = getPlayer();
    killedKeeper(*c->getFirstName(), leader->getNameAndTitle(), worldName, c->getKills(), c->getPoints());
  }
}

void Model::onTorture(const Creature* who, const Creature* torturer) {
  for (auto& col : collectives)
    if (contains(col->getCreatures(), torturer))
      col->onTorture(who, torturer);
}

void Model::onSurrender(Creature* who, const Creature* to) {
  for (auto& col : collectives)
    if (contains(col->getCreatures(), to))
      col->onSurrender(who);
}

void Model::onAttack(Creature* victim, Creature* attacker) {
  victim->getTribe()->onMemberAttacked(victim, attacker);
}

void Model::onTrapTrigger(Position pos) {
  for (auto& col : collectives)
    if (col->getTerritory().contains(pos))
      col->onTrapTrigger(pos);
}

void Model::onTrapDisarm(Position pos, const Creature* who) {
  for (auto& col : collectives)
    if (col->getTerritory().contains(pos))
      col->onTrapDisarm(who, pos);
}

void Model::onSquareDestroyed(Position pos) {
  for (auto& col : collectives)
    if (col->getTerritory().contains(pos))
      col->onSquareDestroyed(pos);
}

void Model::onEquip(const Creature* c, const Item* it) {
  for (auto& col : collectives)
    if (contains(col->getCreatures(), c))
      col->onEquip(c, it);
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

void Model::setHighscores(Highscores* h) {
  highscores = h;
}

bool Model::changeLevel(StairKey key, Creature* c) {
  Level* current = c->getLevel();
  for (Level* target : getLevels())
    if (target != current && target->hasStairKey(key))
      return target->landCreature(key, c);
  FAIL << "Failed to find next level for " << key.getInternalKey() << " " << current->getName();
  return false;
}

void Model::calculateStairNavigation() {
  // Floyd-Warshall algorithm
  for (const Level* l1 : getLevels())
    for (const Level* l2 : getLevels())
      if (l1 != l2)
        if (auto stairKey = getStairsBetween(l1, l2))
          stairNavigation[{l1, l2}] = *stairKey;
  for (const Level* li : getLevels())
    for (const Level* l1 : getLevels())
      if (li != l1)
        for (const Level* l2 : getLevels())
          if (l2 != l1 && l2 != li && !stairNavigation.count({l1, l2}) && stairNavigation.count({li, l2}) &&
              stairNavigation.count({l1, li}))
            stairNavigation[{l1, l2}] = stairNavigation.at({l1, li});
  for (const Level* l1 : getLevels())
    for (const Level* l2 : getLevels())
      if (l1 != l2)
        CHECK(stairNavigation.count({l1, l2})) <<
            "No stair path between levels " << l1->getName() << " " << l2->getName();
}

optional<StairKey> Model::getStairsBetween(const Level* from, const Level* to) {
  for (StairKey key : from->getAllStairKeys())
    if (to->hasStairKey(key))
      return key;
  return none;
}

Position Model::getStairs(const Level* from, const Level* to) {
  CHECK(contains(getLevels(), from));
  CHECK(contains(getLevels(), to));
  CHECK(from != to);
  CHECK(stairNavigation.count({from, to})) << "No link " << from->getName() << " " << to->getName();
  return Random.choose(from->getLandingSquares(stairNavigation.at({from, to})));
}

bool Model::changeLevel(Position position, Creature* c) {
  return position.landCreature(c);
}
  
void Model::conquered(const string& title, vector<const Creature*> kills, int points) {
  string text= "You have conquered this land. You killed " + toString(kills.size()) +
      " innocent beings and scored " + toString(points) +
      " points. Thank you for playing KeeperRL alpha.\n \n";
  for (string stat : statistics->getText())
    text += stat + "\n";
  view->presentText("Victory", text);
  Highscores::Score score = CONSTRUCT(Highscores::Score,
        c.worldName = getWorldName();
        c.points = points;
        c.gameId = getGameIdentifier();
        c.playerName = title;
        c.gameResult = "achieved world domination";
        c.gameWon = true;
        c.turns = getTime();
        c.gameType = Highscores::Score::KEEPER;
  );
  highscores->add(score);
  highscores->present(view, score);
}

void Model::killedKeeper(const string& title, const string& keeper, const string& land,
    vector<const Creature*> kills, int points) {
  string text= "You have freed this land from the bloody reign of " + keeper + 
      ". You killed " + toString(kills.size()) +
      " enemies and scored " + toString(points) +
      " points. Thank you for playing KeeperRL alpha.\n \n";
  for (string stat : statistics->getText())
    text += stat + "\n";
  view->presentText("Victory", text);
  Highscores::Score score = CONSTRUCT(Highscores::Score,
        c.worldName = getWorldName();
        c.points = points;
        c.gameId = getGameIdentifier();
        c.playerName = title;
        c.gameResult = "freed his land from " + keeper;
        c.gameWon = true;
        c.turns = getTime();
        c.gameType = Highscores::Score::ADVENTURER;
  );
  highscores->add(score);
  highscores->present(view, score);
}

bool Model::isGameOver() const {
  return !!exitInfo;
}

void Model::gameOver(const Creature* creature, int numKills, const string& enemiesString, int points) {
  string text = "And so dies " + creature->getNameAndTitle();
  if (auto reason = creature->getDeathReason()) {
    text += ", " + *reason;
  }
  text += ". He killed " + toString(numKills) 
      + " " + enemiesString + " and scored " + toString(points) + " points.\n \n";
  for (string stat : statistics->getText())
    text += stat + "\n";
  view->presentText("Game over", text);
  Highscores::Score score = CONSTRUCT(Highscores::Score,
        c.worldName = getWorldName();
        c.points = points;
        c.gameId = getGameIdentifier();
        c.playerName = *creature->getFirstName();
        c.gameResult = creature->getDeathReason().get_value_or("");
        c.gameWon = false;
        c.turns = getTime();
        c.gameType = (!playerControl || playerControl->isRetired()) ? 
            Highscores::Score::ADVENTURER : Highscores::Score::KEEPER;
  );
  highscores->add(score);
  highscores->present(view, score);
  exitInfo = ExitInfo::abandonGame();
}

const string& Model::getWorldName() const {
  return worldName;
}

vector<Level*> Model::getLevels() const {
  return extractRefs(levels);
}

