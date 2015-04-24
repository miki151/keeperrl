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
    & SVAR(musicType);
  if (version >= 3)
    ar & SVAR(danglingPortal);
  else {
    Trigger* tmp; // SERIAL(tmp)
    ar & SVAR(tmp);
  }
  ar & SVAR(woodCount)
    & SVAR(statistics)
    & SVAR(spectator)
    & SVAR(tribeSet);
  if (version >= 1) {
    ar & SVAR(gameIdentifier)
       & SVAR(gameDisplayName);
  } else {
    gameIdentifier = *playerControl->getKeeper()->getFirstName() + "_" + worldName;
    gameDisplayName = *playerControl->getKeeper()->getFirstName() + " of " + worldName;
  }
  if (version >= 2)
    ar & SVAR(finishCurrentMusic);
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

optional<Model::PortalInfo> Model::getDanglingPortal() {
  return danglingPortal;
}

void Model::setDanglingPortal(Model::PortalInfo p) {
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
  return statistics;
}

const Statistics& Model::getStatistics() const {
  return statistics;
}

Tribe* Model::getPestTribe() {
  return tribeSet.pest.get();
}

Tribe* Model::getKillEveryoneTribe() {
  return tribeSet.killEveryone.get();
}

Tribe* Model::getPeacefulTribe() {
  return tribeSet.peaceful.get();
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
/*  if (options->getBoolValue(OptionId::START_WITH_NIGHT))
    d = -dayLength + 10;*/
  auto previous = sunlightInfo.state;
  while (1) {
    d += dayLength;
    if (d > currentTime) {
      sunlightInfo = {1, d - currentTime, SunlightInfo::DAY};
      break;
    }
    d += duskLength;
    if (d > currentTime) {
      sunlightInfo = {(d - currentTime) / duskLength, d + nightLength - duskLength - currentTime,
        SunlightInfo::NIGHT};
      break;
    }
    d += nightLength - 2 * duskLength;
    if (d > currentTime) {
      sunlightInfo = {0, d + duskLength - currentTime, SunlightInfo::NIGHT};
      break;
    }
    d += duskLength;
    if (d > currentTime) {
      sunlightInfo = {1 - (d - currentTime) / duskLength, d - currentTime, SunlightInfo::NIGHT};
      break;
    }
  }
  if (previous != sunlightInfo.state) {
    for (PLevel& l : levels)
      l->updateSunlightMovement();
  }
}

void Model::onTechBookRead(Technology* tech) {
  if (playerControl)
    playerControl->onTechBookRead(tech);
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
  updateSunlightInfo();
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
        playerControl->onConqueredLand();
        won = true;
      }
    } else // temp fix to the player gets the location message
      playerControl->tick(time);
  }
  if (musicType == MusicType::PEACEFUL && sunlightInfo.state == SunlightInfo::NIGHT)
    setCurrentMusic(MusicType::NIGHT, true);
  else if (musicType == MusicType::NIGHT && sunlightInfo.state == SunlightInfo::DAY)
    setCurrentMusic(MusicType::PEACEFUL, true);
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

Model::Model(View* v, const string& world, Tribe::Set&& tribes)
  : tribeSet(std::move(tribes)), view(v), worldName(world), musicType(MusicType::PEACEFUL) {
  updateSunlightInfo();
}

Model::~Model() {
}

PCreature Model::makePlayer(int handicap) {
  map<UniqueEntity<Level>::Id, MapMemory>* levelMemory = new map<UniqueEntity<Level>::Id, MapMemory>();
  PCreature player = CreatureFactory::addInventory(
      PCreature(new Creature(tribeSet.adventurer.get(),
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
  for (int __attribute__((unused)) i : Range(Random.get(70, 131)))
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
  vector<View::ListElem> elems { "Save the game",
    {"Retire", canRetire ? View::NORMAL : View::INACTIVE} , "Change options", "Abandon the game" };
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
  statistics.clear();
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

string Model::getGameDisplayName() const {
  return gameDisplayName;
}

string Model::getGameIdentifier() const {
  return gameIdentifier;
}

void Model::onKilledLeaderEvent(const Collective* victim, const Creature* leader) {
  if (playerControl && playerControl->isRetired() && playerCollective == victim) {
    const Creature* c = getPlayer();
    killedKeeper(*c->getFirstName(), leader->getNameAndTitle(), worldName, c->getKills(), c->getPoints());
  }
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
  
void Model::conquered(const string& title, vector<const Creature*> kills, int points) {
  string text= "You have conquered this land. You killed " + toString(kills.size()) +
      " innocent beings and scored " + toString(points) +
      " points. Thank you for playing KeeperRL alpha.\n \n";
  for (string stat : statistics.getText())
    text += stat + "\n";
  view->presentText("Victory", text);
  Highscores::Score score = CONSTRUCT(Highscores::Score,
        c.worldName = getWorldName();
        c.points = points;
        c.gameId = getGameIdentifier();
        c.playerName = title;
        c.gameResult = "achieved world domination";
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
  for (string stat : statistics.getText())
    text += stat + "\n";
  view->presentText("Victory", text);
  Highscores::Score score = CONSTRUCT(Highscores::Score,
        c.worldName = getWorldName();
        c.points = points;
        c.gameId = getGameIdentifier();
        c.playerName = title;
        c.gameResult = "freed his land from " + keeper;
  );
  highscores->add(score);
  highscores->present(view, score);
}

bool Model::isGameOver() const {
  return !!exitInfo;
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
  for (string stat : statistics.getText())
    text += stat + "\n";
  view->presentText("Game over", text);
  Highscores::Score score = CONSTRUCT(Highscores::Score,
        c.worldName = getWorldName();
        c.points = points;
        c.gameId = getGameIdentifier();
        c.playerName = *creature->getFirstName();
        c.gameResult = (killer.empty() ? "" : "killed by " + killer);
  );
  highscores->add(score);
  highscores->present(view, score);
  exitInfo = ExitInfo::abandonGame();
}

const string& Model::getWorldName() const {
  return worldName;
}

Level* Model::prepareTopLevel(ProgressMeter& meter, vector<SettlementInfo> settlements) {
  Level* top = buildLevel(
      Level::Builder(meter, 250, 250, "Wilderness", false),
      LevelMaker::topLevel(CreatureFactory::forrest(tribeSet.wildlife.get()), settlements));
  return top;
}


