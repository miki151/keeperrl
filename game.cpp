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
#include "save_file_info.h"
#include "file_sharing.h"
#include "villain_type.h"
#include "attack_trigger.h"
#include "view_object.h"
#include "campaign.h"
#include "construction_map.h"
#include "campaign_builder.h"
#include "campaign_type.h"
#include "game_save_type.h"
#include "player_role.h"
#include "collective_config.h"
#include "attack_behaviour.h"
#include "village_behaviour.h"
#include "collective_builder.h"

template <class Archive> 
void Game::serialize(Archive& ar, const unsigned int version) {
  ar & SUBCLASS(OwnedObject<Game>);
  ar(villainsByType, collectives, lastTick, playerControl, playerCollective, currentTime);
  ar(musicType, statistics, spectator, tribes, gameIdentifier, players);
  ar(gameDisplayName, finishCurrentMusic, models, visited, baseModel, campaign, localTime, turnEvents);
  if (Archive::is_loading::value)
    sunlightInfo.update(getGlobalTime());
}

SERIALIZABLE(Game);
SERIALIZATION_CONSTRUCTOR_IMPL(Game);

static string getGameId(SaveFileInfo info) {
  return info.filename.substr(0, info.filename.size() - 4);
}

Game::Game(Table<PModel>&& m, Vec2 basePos, const CampaignSetup& c)
    : models(std::move(m)), visited(models.getBounds(), false), baseModel(basePos),
      tribes(Tribe::generateTribes()), musicType(MusicType::PEACEFUL), campaign(c.campaign) {
  sunlightInfo.update(getGlobalTime());
  gameIdentifier = c.gameIdentifier;
  gameDisplayName = c.gameDisplayName;
  for (Vec2 v : models.getBounds())
    if (WModel m = models[v].get()) {
      for (WCollective col : m->getCollectives())
        addCollective(col);
      m->updateSunlightMovement();
      for (auto c : m->getAllCreatures())
        c->setGlobalTime(getGlobalTime());
    }
  turnEvents = {0, 10, 50, 100, 300, 500};
  for (int i : Range(200))
    turnEvents.insert(1000 * (i + 1));
}

void Game::addCollective(WCollective col) {
  collectives.push_back(col);
  auto type = col->getVillainType();
  villainsByType[type].push_back(col);
}

static CollectiveConfig getKeeperConfig(bool fastImmigration, bool regenerateMana) {
  return CollectiveConfig::keeper(
      TimeInterval(fastImmigration ? 10 : 140),
      10,
      regenerateMana);
}

void Game::spawnKeeper(AvatarInfo avatarInfo, bool regenerateMana, vector<string> introText,
    GameConfig* gameConfig) {
  auto model = getMainModel().get();
  WLevel level = model->getTopLevel();
  WCreature keeperRef = avatarInfo.playerCreature.get();
  CHECK(level->landCreature(StairKey::keeperSpawn(), keeperRef)) << "Couldn't place keeper on level.";
  model->addCreature(std::move(avatarInfo.playerCreature));
  auto keeperInfo = avatarInfo.creatureInfo.getReferenceMaybe<KeeperCreatureInfo>();
  model->addCollective(CollectiveBuilder(getKeeperConfig(false, regenerateMana), keeperRef->getTribeId())
      .setLevel(level)
      .addCreature(keeperRef, {MinionTrait::LEADER})
      .build());
  playerCollective = model->getCollectives().back();
  auto playerControlOwned = PlayerControl::create(playerCollective, introText, *keeperInfo);
  playerControl = playerControlOwned.get();
  playerCollective->setControl(std::move(playerControlOwned));
  playerCollective->setVillainType(VillainType::PLAYER);
  addCollective(playerCollective);
  if (auto error = playerControl->reloadImmigrationAndWorkshops(gameConfig))
    USER_FATAL << *error;
  for (auto tech : keeperInfo->initialTech)
    playerCollective->acquireTech(tech, false);
}

Game::~Game() {}

PGame Game::campaignGame(Table<PModel>&& models, CampaignSetup& setup, AvatarInfo avatar, GameConfig* gameConfig) {
  auto ret = makeOwner<Game>(std::move(models), *setup.campaign.getPlayerPos(), setup);
  for (auto model : ret->getAllModels())
    model->setGame(ret.get());
  if (setup.campaign.getPlayerRole() == PlayerRole::ADVENTURER)
    ret->getMainModel()->landHeroPlayer(std::move(avatar.playerCreature));
  else
    ret->spawnKeeper(std::move(avatar), setup.regenerateMana, setup.introMessages, gameConfig);
  return ret;
}

PGame Game::splashScreen(PModel&& model, const CampaignSetup& s) {
  Table<PModel> t(1, 1);
  t[0][0] = std::move(model);
  auto game = makeOwner<Game>(std::move(t), Vec2(0, 0), s);
  for (auto model : game->getAllModels())
    model->setGame(game.get());
  game->spectator.reset(new Spectator(game->models[0][0]->getTopLevel()));
  game->turnEvents.clear();
  return game;
}

bool Game::isTurnBased() {
  return !spectator && (!playerControl || playerControl->isTurnBased());
}

GlobalTime Game::getGlobalTime() const {
  PROFILE;
  return GlobalTime((int) currentTime);
}

const vector<WCollective>& Game::getVillains(VillainType type) const {
  static vector<WCollective> empty;
  if (villainsByType.count(type))
    return villainsByType.at(type);
  else
    return empty;
}

WModel Game::getCurrentModel() const {
  if (!players.empty())
    return players[0]->getPosition().getModel();
  else
    return models[baseModel].get();
}

PModel& Game::getMainModel() {
  return models[baseModel];
}

vector<WModel> Game::getAllModels() const {
  vector<WModel> ret;
  for (Vec2 v : models.getBounds())
    if (models[v])
      ret.push_back(models[v].get());
  return ret;
}

bool Game::isSingleModel() const {
  return models.getBounds().getSize() == Vec2(1, 1);
}

int Game::getSaveProgressCount() const {
  int saveTime = 0;
  for (auto model : getAllModels())
    saveTime += model->getSaveProgressCount();
  return saveTime;
}

void Game::prepareSiteRetirement() {
  for (Vec2 v : models.getBounds())
    if (models[v] && v != baseModel)
      models[v]->discardForRetirement();
  for (WCollective col : models[baseModel]->getCollectives())
    col->setVillainType(VillainType::NONE);
  playerCollective->setVillainType(VillainType::MAIN);
  playerCollective->retire();
  vector<Position> locationPos;
  for (auto f : CollectiveConfig::getTrainingFurniture(ExperienceType::SPELL))
    for (auto pos : playerCollective->getConstructions().getBuiltPositions(f))
      locationPos.push_back(pos);
  if (locationPos.empty())
    locationPos = playerCollective->getTerritory().getAll();
  if (!locationPos.empty())
    playerCollective->getTerritory().setCentralPoint(
        Position(Rectangle::boundingBox(locationPos.transform([](Position p){ return p.getCoord();})).middle(),
            playerCollective->getLevel()));
  for (auto c : playerCollective->getCreatures())
    c->retire();
  playerControl = nullptr;
  playerCollective->setControl(VillageControl::create(
      playerCollective, CONSTRUCT(VillageBehaviour,
          c.minPopulation = 24;
          c.minTeamSize = 5;
          c.triggers = LIST(
              {AttackTriggerId::ROOM_BUILT, RoomTriggerInfo{FurnitureType::THRONE, 0.0003}},
              {AttackTriggerId::SELF_VICTIMS},
              AttackTriggerId::STOLEN_ITEMS,
          );
          c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);
          c.ransom = make_pair(0.8, Random.get(500, 700));)));
  WModel mainModel = models[baseModel].get();
  mainModel->setGame(nullptr);
  for (WCollective col : models[baseModel]->getCollectives())
    for (WCreature c : copyOf(col->getCreatures()))
      if (c->getPosition().getModel() != mainModel)
        transferCreature(c, mainModel);
  for (Vec2 v : models.getBounds())
    if (models[v] && v != baseModel)
      for (WCollective col : models[v]->getCollectives())
        for (WCreature c : copyOf(col->getCreatures()))
          if (c->getPosition().getModel() == mainModel)
            transferCreature(c, models[v].get());
  // So we don't have references to creatures in another model.
  for (WCreature c : mainModel->getAllCreatures())
    c->clearInfoForRetiring();
  mainModel->clearExternalEnemies();
  TribeId::switchForSerialization(playerCollective->getTribeId(), TribeId::getRetiredKeeper());
  UniqueEntity<Item>::offsetForSerialization(Random.getLL());
  UniqueEntity<Creature>::offsetForSerialization(Random.getLL());
}

void Game::doneRetirement() {
  TribeId::clearSwitch();
  UniqueEntity<Item>::clearOffset();
  UniqueEntity<Creature>::clearOffset();
}

optional<ExitInfo> Game::updateInput() {
  if (spectator)
    while (1) {
      UserInput input = view->getAction();
      if (input.getId() == UserInputId::EXIT)
        return ExitInfo(ExitAndQuit());
      if (input.getId() == UserInputId::IDLE)
        break;
    }
  if (playerControl && !playerControl->isTurnBased()) {
    while (1) {
      UserInput input = view->getAction();
      if (input.getId() == UserInputId::IDLE)
        break;
      else
        lastUpdate = none;
      playerControl->processInput(view, input);
      if (exitInfo)
        return exitInfo;
    }
  }
  return none;
}

static const TimeInterval initialModelUpdate = 2_visible;

void Game::initializeModels() {
  // Give every model a couple of turns so that things like shopkeepers can initialize.
  for (Vec2 v : models.getBounds())
    if (models[v]) {
      // Use top level's id as unique id of the model.
      auto id = models[v]->getTopLevel()->getUniqueId();
      if (!localTime.count(id)) {
        localTime[id] = (models[v]->getLocalTime() + initialModelUpdate).getDouble();
        updateModel(models[v].get(), localTime[id]);
      }
  }
}

void Game::increaseTime(double diff) {
  auto before = getGlobalTime();
  currentTime += diff;
  auto after = getGlobalTime();
  if (after > before)
    for (auto m : getAllModels())
      for (auto c : m->getAllCreatures())
        c->setGlobalTime(after);
}

optional<ExitInfo> Game::update(double timeDiff) {
  ScopeTimer timer("Game::update timer");
  if (auto exitInfo = updateInput())
    return exitInfo;
  considerRealTimeRender();
  initializeModels();
  increaseTime(timeDiff);
  WModel currentModel = getCurrentModel();
  auto currentId = currentModel->getTopLevel()->getUniqueId();
  while (!lastTick || currentTime >= *lastTick + 1) {
    if (!lastTick)
      lastTick = (int)currentTime;
    else
      *lastTick += 1;
    tick(GlobalTime(*lastTick));
  }
  considerRetiredLoadedEvent(getModelCoords(currentModel));
  localTime[currentId] += timeDiff;
  return updateModel(currentModel, localTime[currentId]);
}

void Game::considerRealTimeRender() {
  auto absoluteTime = view->getTimeMilliAbsolute();
  if (!lastUpdate || absoluteTime - *lastUpdate > milliseconds{10}) {
    if (playerControl)
      playerControl->render(view);
    if (spectator)
      view->updateView(spectator.get(), false);
    lastUpdate = absoluteTime;
  }
}

optional<ExitInfo> Game::updateModel(WModel model, double totalTime) {
  do {
    if (!model->update(totalTime))
      return none;
    if (exitInfo)
      return exitInfo;
    if (wasTransfered) {
      wasTransfered = false;
      return none;
    }
  } while (1);
}

bool Game::isVillainActive(WConstCollective col) {
  const WModel m = col->getModel();
  return m == getMainModel().get() || campaign->isInInfluence(getModelCoords(m));
}

void Game::tick(GlobalTime time) {
  if (!turnEvents.empty() && time.getVisibleInt() > *turnEvents.begin()) {
    auto turn = *turnEvents.begin();
    if (turn == 0)
      uploadEvent("campaignStarted", campaign->getParameters());
    else
      uploadEvent("turn", {{"turn", toString(turn)}});
    turnEvents.erase(turn);
  }
  auto previous = sunlightInfo.getState();
  sunlightInfo.update(GlobalTime((int) currentTime));
  if (previous != sunlightInfo.getState())
    for (Vec2 v : models.getBounds())
      if (WModel m = models[v].get()) {
        m->updateSunlightMovement();
        if (playerControl)
          playerControl->onSunlightVisibilityChanged();
      }
  INFO << "Global time " << time;
  for (WCollective col : collectives) {
    if (isVillainActive(col))
      col->update(col->getModel() == getCurrentModel());
  }
}

void Game::exitAction() {
  enum Action { SAVE, RETIRE, OPTIONS, ABANDON};
#ifdef RELEASE
  bool canRetire = playerControl && !playerControl->getTutorial() && gameWon() && players.empty() &&
      campaign->getType() != CampaignType::SINGLE_KEEPER;
#else
  bool canRetire = playerControl && !playerControl->getTutorial() && players.empty();
#endif
  vector<ListElem> elems { "Save and exit the game",
    {"Retire", canRetire ? ListElem::NORMAL : ListElem::INACTIVE} , "Change options", "Abandon the game" };
  auto ind = view->chooseFromList("Would you like to:", elems);
  if (!ind)
    return;
  switch (Action(*ind)) {
    case RETIRE:
      if (view->yesOrNoPrompt("Retire your dungeon and share it online?")) {
        addEvent(EventInfo::RetiredGame{});
        exitInfo = ExitInfo(GameSaveType::RETIRED_SITE);
        return;
      }
      break;
    case SAVE:
      if (!playerControl) {
        exitInfo = ExitInfo(GameSaveType::ADVENTURER);
        return;
      } else {
        exitInfo = ExitInfo(GameSaveType::KEEPER);
        return;
      }
    case ABANDON:
      if (view->yesOrNoPrompt("Do you want to abandon your game? This is permanent and the save file will be removed!")) {
        exitInfo = ExitInfo(ExitAndQuit());
        return;
      }
      break;
    case OPTIONS: options->handle(view, OptionSet::GENERAL); break;
  }
}

Position Game::getTransferPos(WModel from, WModel to) const {
  return to->getTopLevel()->getLandingSquare(StairKey::transferLanding(),
      getModelCoords(from) - getModelCoords(to));
}

void Game::transferCreature(WCreature c, WModel to) {
  WModel from = c->getLevel()->getModel();
  if (from != to)
    to->transferCreature(from->extractCreature(c), getModelCoords(from) - getModelCoords(to));
}

bool Game::canTransferCreature(WCreature c, WModel to) {
  return to->canTransferCreature(c, getModelCoords(c->getLevel()->getModel()) - getModelCoords(to));
}

int Game::getModelDistance(WConstCollective c1, WConstCollective c2) const {
  return getModelCoords(c1->getModel()).dist8(getModelCoords(c2->getModel()));
}
 
Vec2 Game::getModelCoords(const WModel m) const {
  for (Vec2 v : models.getBounds())
    if (models[v].get() == m)
      return v;
  FATAL << "Model not found";
  return Vec2();
}

void Game::presentWorldmap() {
  view->presentWorldmap(*campaign);
}

void Game::transferAction(vector<WCreature> creatures) {
  if (auto dest = view->chooseSite("Choose destination site:", *campaign,
        getModelCoords(creatures[0]->getLevel()->getModel()))) {
    WModel to = NOTNULL(models[*dest].get());
    vector<CreatureInfo> cant;
    for (WCreature c : copyOf(creatures))
      if (!canTransferCreature(c, to)) {
        cant.push_back(CreatureInfo(c));
        creatures.removeElement(c);
      }
    if (!cant.empty()) {
      view->creatureInfo("These minions will be left behind due to sunlight.", false, cant);
      return;
    }
    if (!creatures.empty()) {
      for (WCreature c : creatures)
        transferCreature(c, models[*dest].get());
      wasTransfered = true;
    }
  }
}

void Game::considerRetiredLoadedEvent(Vec2 coord) {
  if (!visited[coord]) {
    visited[coord] = true;
    if (auto retired = campaign->getSites()[coord].getRetired())
        uploadEvent("retiredLoaded", {
            {"retiredId", getGameId(retired->fileInfo)},
            {"playerName", getPlayerName()}});
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

WCollective Game::getPlayerCollective() const {
  return playerCollective;
}

WPlayerControl Game::getPlayerControl() const {
  return playerControl;
}

MusicType Game::getCurrentMusic() const {
  return musicType;
}

void Game::setCurrentMusic(MusicType type, bool now) {
  if (type == MusicType::PEACEFUL && sunlightInfo.getState() == SunlightState::NIGHT)
    musicType = MusicType::NIGHT;
  else
    musicType = type;
  finishCurrentMusic = now;
}

bool Game::changeMusicNow() const {
  return true;
}

const SunlightInfo& Game::getSunlightInfo() const {
  return sunlightInfo;
}

string Game::getGameDisplayName() const {
  return gameDisplayName;
}

string Game::getGameIdentifier() const {
  return gameIdentifier;
}

View* Game::getView() const {
  return view;
}

GameConfig* Game::getGameConfig() const {
  return gameConfig;
}

void Game::conquered(const string& title, int numKills, int points) {
  string text= "You have conquered this land. You killed " + toString(numKills) +
      " enemies and scored " + toString(points) +
      " points. Thank you for playing KeeperRL!\n \n";
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
        c.turns = getGlobalTime().getVisibleInt();
        c.campaignType = campaign->getType();
        c.playerRole = campaign->getPlayerRole();
  );
  highscores->add(score);
  highscores->present(view, score);
}

void Game::retired(const string& title, int numKills, int points) {
  int turns = getGlobalTime().getVisibleInt();
  int dungeonTurns = campaign->getPlayerRole() == PlayerRole::ADVENTURER ? 0 :
      (getPlayerCollective()->getLocalTime() - initialModelUpdate).getVisibleInt();
  int scoredTurns = campaign->getType() == CampaignType::ENDLESS ? dungeonTurns : turns;
  string text = "You have survived in this land for " + toString(turns) + " turns. You killed " +
      toString(numKills) + " enemies.\n";
  if (dungeonTurns > 0) {
    text += toString(dungeonTurns) + " turns defending the base.\n";
    text += toString(turns - dungeonTurns) + " turns spent adventuring and attacking.\n";
  }
  text += " Thank you for playing KeeperRL!\n \n";
  for (string stat : statistics->getText())
    text += stat + "\n";
  view->presentText("Survived", text);
  Highscores::Score score = CONSTRUCT(Highscores::Score,
        c.worldName = getWorldName();
        c.points = points;
        c.gameId = getGameIdentifier();
        c.playerName = title;
        c.gameResult = "retired";
        c.gameWon = false;
        c.turns = scoredTurns;
        c.campaignType = campaign->getType();
        c.playerRole = campaign->getPlayerRole();
  );
  highscores->add(score);
  highscores->present(view, score);
}

bool Game::isGameOver() const {
  return !!exitInfo;
}

void Game::gameOver(WConstCreature creature, int numKills, const string& enemiesString, int points) {
  int turns = getGlobalTime().getVisibleInt();
  int dungeonTurns = campaign->getPlayerRole() == PlayerRole::ADVENTURER ? 0 :
      (getPlayerCollective()->getLocalTime() - initialModelUpdate).getVisibleInt();
  int scoredTurns = campaign->getType() == CampaignType::ENDLESS ? dungeonTurns : turns;
  string text = "And so dies " + creature->getName().title();
  if (auto reason = creature->getDeathReason()) {
    text += ", " + *reason;
  }
  text += ". You killed " + toString(numKills) + " " + enemiesString + " and scored " + toString(points) + " points.\n";
  if (dungeonTurns > 0) {
    text += toString(dungeonTurns) + " turns defending the base.\n";
    text += toString(turns - dungeonTurns) + " turns spent adventuring and attacking.\n";
  }
  for (string stat : statistics->getText())
    text += stat + "\n";
  view->presentText("Game over", text);
  Highscores::Score score = CONSTRUCT(Highscores::Score,
        c.worldName = getWorldName();
        c.points = points;
        c.gameId = getGameIdentifier();
        c.playerName = creature->getName().firstOrBare();
        c.gameResult = creature->getDeathReason().value_or("");
        c.gameWon = false;
        c.turns = scoredTurns;
        c.campaignType = campaign->getType();
        c.playerRole = campaign->getPlayerRole();
  );
  highscores->add(score);
  highscores->present(view, score);
  exitInfo = ExitInfo(ExitAndQuit());
}

Options* Game::getOptions() {
  return options;
}

void Game::initialize(Options* o, Highscores* h, View* v, FileSharing* f, GameConfig* g) {
  options = o;
  highscores = h;
  view = v;
  fileSharing = f;
  gameConfig = g;
}

const string& Game::getWorldName() const {
  return campaign->getWorldName();
}

const vector<WCollective>& Game::getCollectives() const {
  return collectives;
}

void Game::addPlayer(WCreature c) {
  if (!players.contains(c))
    players.push_back(c);
}

void Game::removePlayer(WCreature c) {
  players.removeElement(c);
}

const vector<WCreature>& Game::getPlayerCreatures() const {
  return players;
}

static SavedGameInfo::MinionInfo getMinionInfo(WConstCreature c) {
  SavedGameInfo::MinionInfo ret;
  ret.level = (int)c->getBestAttack().value;
  ret.viewId = c->getViewObject().id();
  return ret;
}

string Game::getPlayerName() const {
  if (playerCollective) {
    auto leader = playerCollective->getLeader();
    CHECK(leader);
    return leader->getName().firstOrBare();
  } else // adventurer mode
    return players.getOnlyElement()->getName().firstOrBare();
}

SavedGameInfo Game::getSavedGameInfo() const {
  if (WCollective col = getPlayerCollective()) {
    vector<WCreature> creatures = col->getCreatures();
    CHECK(!creatures.empty());
    WCreature leader = col->getLeader();
    CHECK(leader);
    sort(creatures.begin(), creatures.end(), [leader] (WConstCreature c1, WConstCreature c2) {
        return c1 == leader || (c2 != leader && c1->getBestAttack().value > c2->getBestAttack().value);});
    CHECK(creatures[0] == leader);
    creatures.resize(min<int>(creatures.size(), 4));
    vector<SavedGameInfo::MinionInfo> minions;
    for (WCreature c : creatures)
      minions.push_back(getMinionInfo(c));
    return SavedGameInfo(minions, col->getDangerLevel(), getPlayerName(), getSaveProgressCount());
  } else {
    auto player = players.getOnlyElement(); // adventurer mode
    return SavedGameInfo({getMinionInfo(player)}, 0, player->getName().bare(), getSaveProgressCount());
  }
}

void Game::uploadEvent(const string& name, const map<string, string>& m) {
  auto values = m;
  values["eventType"] = name;
  values["gameId"] = getGameIdentifier();
  fileSharing->uploadGameEvent(values);
}

void Game::handleMessageBoard(Position pos, WCreature c) {
  int boardId = pos.getHash();
  vector<ListElem> options;
  atomic<bool> cancelled(false);
  view->displaySplash(nullptr, "Fetching board contents...", SplashType::SMALL, [&] {
      cancelled = true;
      fileSharing->cancel();
      });
  optional<vector<FileSharing::BoardMessage>> messages;
  thread t([&] { messages = fileSharing->getBoardMessages(boardId); view->clearSplash(); });
  view->refreshView();
  t.join();
  if (!messages || cancelled) {
    view->presentText("", "Couldn't download board contents. Please check your internet connection and "
        "enable online features in the settings.");
    return;
  }
  for (auto message : *messages) {
    options.emplace_back(message.author + " wrote:", ListElem::TITLE);
    options.emplace_back("\"" + message.text + "\"", ListElem::TEXT);
  }
  if (messages->empty())
    options.emplace_back("The board is empty.", ListElem::TITLE);
  options.emplace_back("", ListElem::TEXT);
  options.emplace_back("[Write something]");
  if (auto index = view->chooseFromList("", options))
    if (auto text = view->getText("Enter message", "", 80)) {
      if (text->size() >= 2) {
        if (!fileSharing->uploadBoardMessage(getGameIdentifier(), pos.getHash(), c->getName().title(), *text))
          view->presentText("", "Please enable online features in the settings.");
      } else
        view->presentText("", "The message was too short.");
    }
}

bool Game::gameWon() const {
  for (WCollective col : getCollectives())
    if (!col->isConquered() && col->getVillainType() == VillainType::MAIN)
      return false;
  return true;
}

void Game::addEvent(const GameEvent& event) {
  for (Vec2 v : models.getBounds())
    if (models[v])
      models[v]->addEvent(event);
  using namespace EventInfo;
  event.visit(
      [&](const ConqueredEnemy& info) {
        WCollective col = info.collective;
        if (col->getVillainType() != VillainType::NONE) {
          Vec2 coords = getModelCoords(col->getModel());
          if (!campaign->isDefeated(coords)) {
            if (auto retired = campaign->getSites()[coords].getRetired())
              uploadEvent("retiredConquered", {
                  {"retiredId", getGameId(retired->fileInfo)},
                  {"playerName", getPlayerName()}});
            if (coords != campaign->getPlayerPos())
              campaign->setDefeated(coords);
          }
        }
        if (col->getVillainType() == VillainType::MAIN && gameWon()) {
          addEvent(WonGame{});
        }
      },
      [&](const auto&) {}
  );
}

