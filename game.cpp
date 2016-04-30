#include "stdafx.h"
#include "game.h"
#include "view.h"
#include "clock.h"
#include "tribe.h"
#include "music.h"
#include "player_control.h"
#include "village_control.h"
#include "model.h"
#include "creature.h"
#include "spectator.h"
#include "statistics.h"
#include "collective.h"
#include "options.h"
#include "territory.h"
#include "level.h"
#include "highscores.h"
#include "player.h"
#include "item_factory.h"
#include "item.h"
#include "map_memory.h"
#include "creature_attributes.h"
#include "name_generator.h"
#include "campaign.h"
#include "game_events.h"
#include "save_file_info.h"

template <class Archive> 
void Game::serialize(Archive& ar, const unsigned int version) { 
  serializeAll(ar, villainsByType, collectives, lastTick, playerControl, playerCollective, won, currentTime);
  serializeAll(ar, worldName, musicType, portals, statistics, spectator, tribes, gameIdentifier, player);
  serializeAll(ar, gameDisplayName, finishCurrentMusic, models, visited, baseModel, campaign, localTime);
  if (Archive::is_loading::value)
    sunlightInfo.update(currentTime);
}

SERIALIZABLE(Game);
SERIALIZATION_CONSTRUCTOR_IMPL(Game);

static string getNewIdSuffix() {
  vector<char> chars;
  for (char c : Range(128))
    if (isalnum(c))
      chars.push_back(c);
  string ret;
  for (int i : Range(4))
    ret += Random.choose(chars);
  return ret;
}

static string getGameId(SaveFileInfo info) {
  return info.filename.substr(0, info.filename.size() - 4);
}

Game::Game(const string& world, const string& player, Table<PModel>&& m, Vec2 basePos, optional<Campaign> c)
    : worldName(world), models(std::move(m)), visited(models.getBounds(), false), baseModel(basePos),
      tribes(Tribe::generateTribes()), musicType(MusicType::PEACEFUL), campaign(c) {
  sunlightInfo.update(currentTime);
  gameIdentifier = player + "_" + worldName + getNewIdSuffix();
  gameDisplayName = player + " of " + worldName;
  for (Vec2 v : models.getBounds())
    if (Model* m = models[v].get()) {
      for (Collective* c : m->getCollectives()) {
        collectives.push_back(c);
        if (auto type = c->getVillainType()) {
          villainsByType[*type].push_back(c);
          if (type == VillainType::PLAYER) {
            playerControl = NOTNULL(dynamic_cast<PlayerControl*>(c->getControl()));
            playerCollective = c;
          }
        }
      }
      m->setGame(this);
      m->updateSunlightMovement();
    }
}

Game::~Game() {}

PGame Game::campaignGame(Table<PModel>&& models, Vec2 basePos, const string& playerName, const Campaign& campaign) {
  PGame game(new Game(campaign.getWorldName(), playerName, std::move(models), basePos, campaign));
  return game;
}

PGame Game::singleMapGame(const string& worldName, const string& playerName, PModel&& model) {
  Table<PModel> t(1, 1);
  t[0][0] = std::move(model);
  return PGame(new Game(worldName, playerName, std::move(t), Vec2(0, 0)));
}

PGame Game::splashScreen(PModel&& model) {
  Table<PModel> t(1, 1);
  t[0][0] = std::move(model);
  PGame game(new Game("", "", std::move(t), Vec2(0, 0)));
  game->spectator.reset(new Spectator(game->models[0][0]->getTopLevel()));
  return game;
}

bool Game::isTurnBased() {
  return !spectator && (!playerControl || playerControl->isTurnBased());
}

bool Game::isSingleModel() const {
  return models.getBounds().getSize() == Vec2(1, 1);
}

double Game::getGlobalTime() const {
  return currentTime;
}

const vector<Collective*>& Game::getVillains(VillainType type) const {
  static vector<Collective*> empty;
  if (villainsByType.count(type))
    return villainsByType.at(type);
  else
    return empty;
}

Model* Game::getCurrentModel() const {
  if (Creature* c = getPlayer())
    return c->getPosition().getModel();
  else
    return models[baseModel].get();
}

PModel& Game::getMainModel() {
  return models[baseModel];
}

void Game::prepareRetirement() {
  Model* mainModel = models[baseModel].get();
  for (Vec2 v : models.getBounds())
    if (models[v]) {
      if (v != baseModel)
        models[v]->lockSerialization();
      else {
        models[v]->setGame(nullptr);
        models[v]->clearDeadCreatures();
      }
    }
  playerCollective->setVillainType(VillainType::MAIN);
  playerCollective->limitKnownTilesToModel();
  vector<Position> locationPos = playerCollective->getAllSquares({SquareId::LIBRARY});
  if (locationPos.empty())
    locationPos = playerCollective->getTerritory().getAll();
  if (!locationPos.empty())
    playerCollective->getLevel()->addMarkedLocation(Rectangle::boundingBox(transform2<Vec2>(locationPos, 
      [](const Position& p) { return p.getCoord();})));
  playerControl->getKeeper()->modViewObject().setId(ViewId::RETIRED_KEEPER);
  playerControl = nullptr;
  playerCollective->setControl(PCollectiveControl(
        new VillageControl(playerCollective, CONSTRUCT(VillageControl::Villain,
          c.minPopulation = 6;
          c.minTeamSize = 5;
          c.triggers = LIST({AttackTriggerId::ROOM_BUILT, SquareId::THRONE}, {AttackTriggerId::SELF_VICTIMS},
            AttackTriggerId::STOLEN_ITEMS, {AttackTriggerId::ROOM_BUILT, SquareId::IMPALED_HEAD});
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_LEADER);
          c.ransom = make_pair(0.8, Random.get(500, 700));))));
  for (Collective* col : models[baseModel]->getCollectives())
    for (Creature* c : col->getCreatures())
      if (c->getPosition().getModel() != mainModel)
        transferCreature(c, mainModel);
  for (Vec2 v : models.getBounds())
    if (models[v] && v != baseModel)
      for (Collective* col : models[v]->getCollectives())
        for (Creature* c : col->getCreatures())
          if (c->getPosition().getModel() == mainModel)
            transferCreature(c, models[v].get());
  // So we don't have references to creatures in another model.
  for (Creature* c : mainModel->getAllCreatures())
    c->clearLastAttacker();
  TribeId::switchForSerialization(TribeId::getKeeper(), TribeId::getRetiredKeeper());
}

void Game::doneRetirement() {
  TribeId::clearSwitch();
}

optional<Game::ExitInfo> Game::update(double timeDiff) {
  currentTime += timeDiff;
  Model* currentModel = getCurrentModel();
  // Give every model a couple of turns so that things like shopkeepers can initialize.
  for (Vec2 v : models.getBounds())
    if (models[v] && !localTime.count(models[v].get())) {
      localTime[models[v].get()] = models[v]->getTime() + 2;
      updateModel(models[v].get(), localTime[models[v].get()]);
    }
  localTime[currentModel] += timeDiff;
  while (currentTime > lastTick + 1) {
    lastTick += 1;
    tick(lastTick);
  }
  return updateModel(currentModel, localTime[currentModel]);
}

optional<Game::ExitInfo> Game::updateModel(Model* model, double totalTime) {
  int absoluteTime = view->getTimeMilliAbsolute();
  if (absoluteTime - lastUpdate > 20) {
    if (playerControl)
      playerControl->render(view);
    if (spectator)
      view->updateView(spectator.get(), false);
    lastUpdate = absoluteTime;
  } 
  do {
    if (spectator)
      while (1) {
        UserInput input = view->getAction();
        if (input.getId() == UserInputId::EXIT)
          return ExitInfo{ExitId::QUIT};
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
    if (model->getTime() >= totalTime)
      return none;
    model->update(totalTime);
    if (exitInfo)
      return exitInfo;
    if (wasTransfered) {
      wasTransfered = false;
      return none;
    }
  } while (1);
}

bool Game::isVillainActive(const Collective* col) {
  const Model* m = col->getLevel()->getModel();
  return m == getMainModel().get() || campaign->isInInfluence(getModelCoords(m));
}

void Game::tick(double time) {
  auto previous = sunlightInfo.getState();
  sunlightInfo.update(currentTime);
  if (previous != sunlightInfo.getState())
    for (Vec2 v : models.getBounds())
      if (Model* m = models[v].get())
        m->updateSunlightMovement();
  Debug() << "Global time " << time;
  if (playerControl) {
    bool conquered = true;
    for (Collective* col : getCollectives()) {
      conquered &= col->isConquered() || col->getVillainType() != VillainType::MAIN;
      if (col->isConquered() && campaign && col->getVillainType()) {
        Vec2 coords = getModelCoords(col->getLevel()->getModel());
        if (!campaign->isDefeated(coords))
          if (auto retired = campaign->getSites()[coords].getRetired())
            gameEvents->addRetiredConquered(getGameId(retired->fileInfo), getPlayerName());
        campaign->setDefeated(coords);
      }
    }
    if (!getVillains(VillainType::MAIN).empty() && conquered && !won) {
      playerControl->onConqueredLand();
      won = true;
    }
  }
  for (Collective* col : collectives) {
    if (isVillainActive(col))
      col->update(col->getLevel()->getModel() == getCurrentModel());
  }
  if (musicType == MusicType::PEACEFUL && sunlightInfo.getState() == SunlightState::NIGHT)
    setCurrentMusic(MusicType::NIGHT, true);
  else if (musicType == MusicType::NIGHT && sunlightInfo.getState() == SunlightState::DAY)
    setCurrentMusic(MusicType::PEACEFUL, true);
}

void Game::exitAction() {
  enum Action { SAVE, RETIRE, OPTIONS, ABANDON};
#ifdef RELEASE
  bool canRetire = playerControl && won && !getPlayer();
#else
  bool canRetire = playerControl && !getPlayer();
#endif
  vector<ListElem> elems { "Save the game",
    {"Retire", canRetire ? ListElem::NORMAL : ListElem::INACTIVE} , "Change options", "Abandon the game" };
  auto ind = view->chooseFromList("Would you like to:", elems);
  if (!ind)
    return;
  switch (Action(*ind)) {
    case RETIRE:
      if (view->yesOrNoPrompt("Retire your dungeon and share it online?")) {
        if (isSingleModel())
          exitInfo = ExitInfo(ExitId::SAVE, GameSaveType::RETIRED_SINGLE);
        else
          exitInfo = ExitInfo(ExitId::SAVE, GameSaveType::RETIRED_SITE);
        return;
      }
      break;
    case SAVE:
      if (!playerControl) {
        exitInfo = ExitInfo(ExitId::SAVE, GameSaveType::ADVENTURER);
        return;
      } else {
        exitInfo = ExitInfo(ExitId::SAVE, GameSaveType::KEEPER);
        return;
      }
    case ABANDON:
      if (view->yesOrNoPrompt("Are you sure you want to abandon your game?")) {
        exitInfo = ExitInfo(ExitId::QUIT);
        return;
      }
      break;
    case OPTIONS: options->handle(view, OptionSet::GENERAL); break;
    default: break;
  }
}

Position Game::getTransferPos(Model* from, Model* to) const {
  return to->getTopLevel()->getLandingSquare(StairKey::transferLanding(),
      getModelCoords(from) - getModelCoords(to));
}

void Game::transferCreature(Creature* c, Model* to) {
  Model* from = c->getLevel()->getModel();
  if (from != to)
    to->transferCreature(from->extractCreature(c), getModelCoords(from) - getModelCoords(to));
}

bool Game::canTransferCreature(Creature* c, Model* to) {
  return to->canTransferCreature(c, getModelCoords(c->getLevel()->getModel()) - getModelCoords(to));
}

int Game::getModelDistance(const Collective* c1, const Collective* c2) const {
  return getModelCoords(c1->getLevel()->getModel()).dist8(getModelCoords(c2->getLevel()->getModel()));
}
 
Vec2 Game::getModelCoords(const Model* m) const {
  for (Vec2 v : models.getBounds())
    if (models[v].get() == m)
      return v;
  FAIL << "Model not found";
  return Vec2();
}

void Game::presentWorldmap() {
  view->presentWorldmap(*campaign);
}

void Game::transferAction(vector<Creature*> creatures) {
  if (!campaign)
    return;
  if (auto dest = view->chooseSite("Choose destination site:", *campaign,
        getModelCoords(creatures[0]->getLevel()->getModel()))) {
    Model* to = NOTNULL(models[*dest].get());
    vector<CreatureInfo> cant;
    for (Creature* c : copyOf(creatures))
      if (!canTransferCreature(c, to)) {
        cant.push_back(c);
        removeElement(creatures, c);
      }
    if (!cant.empty() && !view->creaturePrompt("These minions will be left behind due to sunlight.", cant))
      return;
    if (!creatures.empty()) {
      for (Creature* c : creatures)
        transferCreature(c, models[*dest].get());
      if (!visited[*dest]) {
        visited[*dest] = true;
        if (auto retired = campaign->getSites()[*dest].getRetired())
          gameEvents->addRetiredLoaded(getGameId(retired->fileInfo), getPlayerName());

      }
      wasTransfered = true;
    }
  }
}

Statistics& Game::getStatistics() {
  return *statistics;
}

const Statistics& Game::getStatistics() const {
  return *statistics;
}

Tribe* Game::getTribe(TribeId id) const {
  return tribes.at(id).get();
}

Collective* Game::getPlayerCollective() const {
  return playerCollective;
}

MusicType Game::getCurrentMusic() const {
  return musicType;
}

void Game::setCurrentMusic(MusicType type, bool now) {
  musicType = type;
  finishCurrentMusic = now;
}

bool Game::changeMusicNow() const {
  return finishCurrentMusic;
}

const SunlightInfo& Game::getSunlightInfo() const {
  return sunlightInfo;
}


void Game::onTechBookRead(Technology* tech) {
  if (playerControl)
    playerControl->onTechBookRead(tech);
}

void Game::onAlarm(Position pos) {
  Model* model = pos.getModel();
  for (auto& col : model->getCollectives())
    if (col->getTerritory().contains(pos))
      col->onAlarm(pos);
  if (const Creature* c = getPlayer()) {
    if (pos == c->getPosition())
      c->playerMessage("An alarm sounds near you.");
    else if (pos.isSameLevel(c->getPosition()))
      c->playerMessage("An alarm sounds in the " + 
          getCardinalName(c->getPosition().getDir(pos).getBearing().getCardinalDir()));
  }
}

void Game::landHeroPlayer() {
  auto handicap = view->getNumber("Choose handicap (your adventurer's strength and dexterity increase)", 0, 20, 5);
  PCreature player = CreatureFactory::getAdventurer(handicap.get_value_or(0));
  string advName = options->getStringValue(OptionId::ADVENTURER_NAME);
  if (!advName.empty())
    player->getName().setFirst(advName);
  Level* target = models[0][0]->getTopLevel();
  CHECK(target->landCreature(target->getAllPositions(), std::move(player))) << "No place to spawn player";
}

string Game::getGameDisplayName() const {
  return gameDisplayName;
}

string Game::getGameIdentifier() const {
  return gameIdentifier;
}

void Game::onKilledLeader(const Collective* victim, const Creature* leader) {
  if (isSingleModel() && victim->getVillainType() == VillainType::MAIN) {
    if (Creature* c = getPlayer())
      killedKeeper(*c->getName().first(), c->getName().title(), worldName, c->getKills().getSize(), c->getPoints());
  }
}

void Game::onTorture(const Creature* who, const Creature* torturer) {
  for (Collective* col : getCollectives())
    if (contains(col->getCreatures(), torturer))
      col->onTorture(who, torturer);
}

void Game::onSurrender(Creature* who, const Creature* to) {
  for (auto& col : collectives)
    if (contains(col->getCreatures(), to))
      col->onSurrender(who);
}

void Game::onTrapTrigger(Position pos) {
  for (auto& col : collectives)
    if (col->getTerritory().contains(pos))
      col->onTrapTrigger(pos);
}

void Game::onTrapDisarm(Position pos, const Creature* who) {
  for (auto& col : collectives)
    if (col->getTerritory().contains(pos))
      col->onTrapDisarm(who, pos);
}

void Game::onSquareDestroyed(Position pos) {
  for (auto& col : collectives)
    if (col->getTerritory().contains(pos))
      col->onSquareDestroyed(pos);
}

void Game::onEquip(const Creature* c, const Item* it) {
  for (auto& col : collectives)
    if (contains(col->getCreatures(), c))
      col->onEquip(c, it);
}

View* Game::getView() const {
  return view;
}

void Game::conquered(const string& title, int numKills, int points) {
  string text= "You have conquered this land. You killed " + toString(numKills) +
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
        c.turns = getGlobalTime();
        c.gameType = Highscores::Score::KEEPER;
  );
  highscores->add(score);
  highscores->present(view, score);
}

void Game::killedKeeper(const string& title, const string& keeper, const string& land, int numKills, int points) {
  string text= "You have freed this land from the bloody reign of " + keeper + 
      ". You killed " + toString(numKills) + " enemies and scored " + toString(points) +
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
        c.turns = getGlobalTime();
        c.gameType = Highscores::Score::ADVENTURER;
  );
  highscores->add(score);
  highscores->present(view, score);
}

bool Game::isGameOver() const {
  return !!exitInfo;
}

void Game::gameOver(const Creature* creature, int numKills, const string& enemiesString, int points) {
  string text = "And so dies " + creature->getName().title();
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
        c.playerName = *creature->getName().first();
        c.gameResult = creature->getDeathReason().get_value_or("");
        c.gameWon = false;
        c.turns = getGlobalTime();
        c.gameType = playerControl ? Highscores::Score::KEEPER : Highscores::Score::ADVENTURER;
  );
  highscores->add(score);
  highscores->present(view, score);
  exitInfo = ExitInfo(ExitId::QUIT);
}

Options* Game::getOptions() {
  return options;
}

void Game::initialize(Options* o, Highscores* h, View* v, GameEvents* e) {
  options = o;
  highscores = h;
  view = v;
  gameEvents = e;
}

const string& Game::getWorldName() const {
  return worldName;
}

optional<Position> Game::getOtherPortal(Position position) const {
  if (auto index = findElement(portals, position)) {
    if (*index % 2 == 1)
      return portals[*index - 1];
    if (*index < portals.size() - 1)
      return portals[*index + 1];
  }
  return none;
}

void Game::registerPortal(Position pos) {
  if (!contains(portals, pos))
    portals.push_back(pos);
}

const vector<Collective*>& Game::getCollectives() const {
  return collectives;
}

void Game::setPlayer(Creature* c) {
  player = c;
}

Creature* Game::getPlayer() const {
  if (player && !player->isDead())
    return player;
  else
    return nullptr;
}

void Game::cancelPlayer(Creature* c) {
  CHECK(c == player);
  player = nullptr;
}

static SavedGameInfo::MinionInfo getMinionInfo(const Creature* c) {
  SavedGameInfo::MinionInfo ret;
  ret.level = c->getAttributes().getExpLevel();
  ret.viewId = c->getViewObject().id();
  return ret;
}

string Game::getPlayerName() const {
  return *getPlayerCollective()->getLeader()->getName().first();
}

SavedGameInfo Game::getSavedGameInfo() const {
  Collective* col = getPlayerCollective();
  vector<Creature*> creatures = col->getCreatures();
  CHECK(!creatures.empty());
  Creature* leader = col->getLeader();
  CHECK(!leader->isDead());
  sort(creatures.begin(), creatures.end(), [leader] (const Creature* c1, const Creature* c2) {
      return c1 == leader
        || (c2 != leader && c1->getAttributes().getExpLevel() > c2->getAttributes().getExpLevel());});
  CHECK(creatures[0] == leader);
  creatures.resize(min<int>(creatures.size(), 4));
  vector<SavedGameInfo::MinionInfo> minions;
  for (Creature* c : creatures)
    minions.push_back(getMinionInfo(c));
  return SavedGameInfo(minions, col->getDangerLevel(), getPlayerName());
}

