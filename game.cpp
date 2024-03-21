#include "owner_pointer.h"
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
#include "collective_config.h"
#include "attack_behaviour.h"
#include "village_behaviour.h"
#include "collective_builder.h"
#include "game_event.h"
#include "version.h"
#include "content_factory.h"
#include "collective_name.h"
#include "avatar_info.h"
#include "scripted_ui.h"
#include "scripted_ui_data.h"
#include "body.h"
#include "enemy_aggression_level.h"
#include "unlocks.h"
#include "steam_achievements.h"

template <class Archive>
void Game::serialize(Archive& ar, const unsigned int version) {
  ar & SUBCLASS(OwnedObject<Game>);
  ar(villainsByType, collectives, lastTick, playerControl, playerCollective, currentTime, avatarId, numLesserVillainsDefeated);
  ar(musicType, statistics, tribes, gameIdentifier, players, contentFactory, sunlightTimeOffset, allianceAttackPossible);
  ar(gameDisplayName, models, visited, baseModel, campaign, localTime, turnEvents, effectFlags, zLevelGroups);
  if (version == 1)
    ar(enemyAggressionLevel);
  else if (Archive::is_loading::value)
    enemyAggressionLevel = EnemyAggressionLevel::NONE;
  if (Archive::is_loading::value)
    sunlightInfo.update(getGlobalTime() + sunlightTimeOffset);
}

SERIALIZABLE(Game);
SERIALIZATION_CONSTRUCTOR_IMPL(Game);

Game::Game(Table<PModel>&& m, Vec2 basePos, const CampaignSetup& c, ContentFactory f)
    : models(std::move(m)), visited(models.getBounds(), false), baseModel(basePos),
      tribes(Tribe::generateTribes()), musicType(MusicType::PEACEFUL), campaign(c.campaign),
      contentFactory(std::move(f)) {
  for (auto pos : models.getBounds())
    if (models[pos])
      models[pos]->position = pos;
  gameIdentifier = c.gameIdentifier;
  gameDisplayName = c.gameDisplayName;
  enemyAggressionLevel = c.enemyAggressionLevel;
  for (Vec2 v : models.getBounds())
    if (Model* m = models[v].get()) {
      for (Collective* col : m->getCollectives()) {
        auto control = dynamic_cast<VillageControl*>(col->getControl());
        control->updateAggression(c.enemyAggressionLevel);
        addCollective(col);
      }
      m->updateSunlightMovement();
      for (auto c : m->getAllCreatures())
        c->setGlobalTime(getGlobalTime());
    }
  turnEvents = {0, 10, 50, 100, 300, 500};
  for (int i : Range(200))
    turnEvents.insert(1000 * (i + 1));
}

EnemyAggressionLevel Game::getEnemyAggressionLevel() const {
  return enemyAggressionLevel;
}

void Game::addCollective(Collective* col) {
  if (!collectives.contains(col)) {
    collectives.push_back(col);
    auto type = col->getVillainType();
    villainsByType[type].push_back(col);
  }
}

void Game::spawnKeeper(AvatarInfo avatarInfo, vector<string> introText) {
  auto model = getMainModel().get();
  Level* level = model->getGroundLevel();
  Creature* keeperRef = avatarInfo.playerCreature.get();
  CHECK(level->landCreature(StairKey::keeperSpawn(), keeperRef)) << "Couldn't place keeper on level.";
  model->addCreature(std::move(avatarInfo.playerCreature));
  auto& keeperInfo = avatarInfo.creatureInfo;
  auto builder = CollectiveBuilder(CollectiveConfig::keeper(
          TimeInterval(keeperInfo.immigrantInterval), keeperInfo.maxPopulation, keeperInfo.populationString,
          keeperInfo.prisoners, ConquerCondition::KILL_LEADER, keeperInfo.requireQuartersForExp),
      keeperRef->getTribeId())
      .setModel(model)
      .addCreature(keeperRef, keeperInfo.minionTraits);
  if (avatarInfo.chosenBaseName)
    builder.setLocationName(*avatarInfo.chosenBaseName);
  if (avatarInfo.creatureInfo.startingBase) {
    builder.setLevel(level);
    builder.addArea(level->getLandingSquares(StairKey::keeperSpawn())
        .transform([](auto& pos){ return pos.getCoord(); }));
  }
  model->addCollective(builder.build(contentFactory.get()));
  playerCollective = model->getCollectives().back();
  CHECK(!!playerCollective->getName()->shortened);
  auto playerControlOwned = PlayerControl::create(playerCollective, introText, keeperInfo.tribeAlignment);
  playerControl = playerControlOwned.get();
  playerCollective->setControl(std::move(playerControlOwned));
  playerCollective->setVillainType(VillainType::PLAYER);
  addCollective(playerCollective);
  playerControl->loadImmigrationAndWorkshops(contentFactory.get(), keeperInfo);
  for (auto tech : keeperInfo.initialTech)
    playerCollective->acquireTech(tech, false);
  for (auto resource : keeperInfo.credit)
    playerCollective->returnResource(resource);
  for (auto& f : keeperInfo.flags)
    effectFlags.insert(f);
  zLevelGroups = keeperInfo.zLevelGroups;
  allianceAttackPossible =
#ifdef RELEASE
    Random.roll(3);
#else
    true;
#endif
}

Game::~Game() {}

PGame Game::campaignGame(Table<PModel>&& models, CampaignSetup setup, AvatarInfo avatar,
    ContentFactory contentFactory, map<string, string> analytics) {
  auto ret = makeOwner<Game>(std::move(models), setup.campaign.getPlayerPos(), setup, std::move(contentFactory));
  ret->avatarId = avatar.avatarId;
  ret->analytics = analytics;
  for (auto model : ret->getAllModels())
    model->setGame(ret.get());
  auto avatarCreature = avatar.playerCreature.get();
  if (avatarCreature->getAttributes().isAffectedPermanently(LastingEffect::SUNLIGHT_VULNERABLE) ||
      avatarCreature->getBody().isIntrinsicallyAffected(LastingEffect::SUNLIGHT_VULNERABLE, ret->getContentFactory()))
    ret->sunlightTimeOffset = 1501_visible;
  // Remove sunlight vulnerability temporarily otherwise placing the creature anywhere without cover will fail.
  avatarCreature->getAttributes().removePermanentEffect(LastingEffect::SUNLIGHT_VULNERABLE, 1);
  ret->sunlightInfo.update(ret->getGlobalTime() + ret->sunlightTimeOffset);
  ret->spawnKeeper(std::move(avatar), setup.introMessages);
  // Restore vulnerability. If the effect wasn't present in the first place then it will zero-out.
  avatarCreature->getAttributes().addPermanentEffect(LastingEffect::SUNLIGHT_VULNERABLE, 1);
  return ret;
}

PGame Game::warlordGame(Table<PModel> models, CampaignSetup setup, vector<PCreature> creatures,
    ContentFactory contentFactory, string avatarId) {
  auto ret = makeOwner<Game>(std::move(models), setup.campaign.getPlayerPos(), setup, std::move(contentFactory));
  ret->avatarId = std::move(avatarId);
  for (auto model : ret->getAllModels())
    model->setGame(ret.get());
  for (auto& c : creatures)
    if (c->getAttributes().isAffectedPermanently(LastingEffect::SUNLIGHT_VULNERABLE))
      ret->sunlightTimeOffset = 1501_visible;
  // Remove sunlight vulnerability temporarily otherwise placing the creature anywhere without cover will fail.
  for (auto& c : creatures)
    c->getAttributes().removePermanentEffect(LastingEffect::SUNLIGHT_VULNERABLE, 1);
  ret->sunlightInfo.update(ret->getGlobalTime() + ret->sunlightTimeOffset);
  auto ref = getWeakPointers(creatures);
  ret->getMainModel()->landWarlord(std::move(creatures));
  // Restore vulnerability. If the effect wasn't present in the first place then it will zero-out.
  for (auto& c : ref)
    c->getAttributes().addPermanentEffect(LastingEffect::SUNLIGHT_VULNERABLE, 1);
  return ret;
}

PGame Game::splashScreen(PModel&& model, const CampaignSetup& s, ContentFactory f, View* view) {
  auto modelRef = model.get();
  Table<PModel> t(1, 1);
  t[0][0] = std::move(model);
  auto game = makeOwner<Game>(std::move(t), Vec2(0, 0), s, std::move(f));
  for (auto model : game->getAllModels())
    model->setGame(game.get());
  auto spectator = makeOwner<Spectator>(game->models[0][0]->getGroundLevel(), view);
  spectator->subscribeTo(modelRef);
  game->spectator = std::move(spectator);
  game->turnEvents.clear();
  return game;
}

bool Game::isTurnBased() {
  return !getPlayerCreatures().empty();
}

GlobalTime Game::getGlobalTime() const {
  PROFILE;
  return GlobalTime((int) currentTime);
}

const vector<Collective*>& Game::getVillains(VillainType type) const {
  static vector<Collective*> empty;
  if (villainsByType.count(type))
    return villainsByType.at(type);
  else
    return empty;
}

Model* Game::getCurrentModel() const {
  if (!players.empty())
    return players[0]->getPosition().getModel();
  else
    return models[baseModel].get();
}

int Game::getModelDifficulty(const Model* model) const {
  return campaign->getBaseLevelIncrease(model->position);
}

bool Game::passesMaxAggressorCutOff(const Model* model) {
  return campaign->passesMaxAggressorCutOff(model->position);
}

int Game::getNumLesserVillainsDefeated() const {
  return numLesserVillainsDefeated;
}

PModel& Game::getMainModel() {
  return models[baseModel];
}

vector<Model*> Game::getAllModels() const {
  vector<Model*> ret;
  for (Vec2 v : models.getBounds())
    if (models[v])
      ret.push_back(models[v].get());
  return ret;
}

bool Game::isSingleModel() const {
  return getAllModels().size() == 1;
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
  for (Collective* col : models[baseModel]->getCollectives())
    if (col != playerCollective)
      col->setVillainType(VillainType::NONE);
  if (playerCollective->getVillainType() == VillainType::PLAYER) {
    // if it's not PLAYER then it's a conquered collective and villainType and VillageControl is already set up
    playerCollective->setVillainType(VillainType::RETIRED);
    playerCollective->setControl(VillageControl::create(
        playerCollective, CONSTRUCT(VillageBehaviour,
            c.minPopulation = 24;
            c.minTeamSize = 5;
            c.triggers = makeVec<AttackTrigger>(
                RoomTrigger{FurnitureType("THRONE"), 0.0003},
                SelfVictims{},
                StolenItems{}
            );
            c.attackBehaviour = KillLeader{};
            c.ransom = make_pair(0.8, Random.get(500, 700));)));
  }
  playerCollective->retire();
  vector<Position> locationPos;
  for (auto f : contentFactory->furniture.getTrainingFurniture(AttrType("SPELL_DAMAGE")))
    for (auto pos : playerCollective->getConstructions().getBuiltPositions(f))
      locationPos.push_back(pos);
  if (locationPos.empty())
    locationPos = playerCollective->getTerritory().getAll();
  if (!locationPos.empty())
    playerCollective->getTerritory().setCentralPoint(
        Position(Rectangle::boundingBox(locationPos.transform([](Position p){ return p.getCoord();})).middle(),
            playerCollective->getModel()->getGroundLevel()));
  for (auto c : copyOf(playerCollective->getCreatures()))
    c->retire();
  playerControl = nullptr;
  Model* mainModel = models[baseModel].get();
  mainModel->setGame(nullptr);
  for (Collective* col : models[baseModel]->getCollectives())
    for (Creature* c : copyOf(col->getCreatures()))
      if (c->getPosition().getModel() != mainModel)
        transferCreature(c, mainModel);
  for (Vec2 v : models.getBounds())
    if (models[v] && v != baseModel)
      for (Collective* col : models[v]->getCollectives())
        for (Creature* c : copyOf(col->getCreatures()))
          if (c->getPosition().getModel() == mainModel)
            transferCreature(c, models[v].get());
  mainModel->prepareForRetirement();
  UniqueEntity<Item>::offsetForSerialization(Random.getLL());
  UniqueEntity<Creature>::offsetForSerialization(Random.getLL());
}

void Game::doneRetirement() {
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
  if (playerControl && !isTurnBased()) {
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
  for (auto col : getCollectives())
    col->update(col->getModel() == getCurrentModel());
  // Give every model a couple of turns so that things like shopkeepers can initialize.
  for (Vec2 v : models.getBounds())
    if (auto model = models[v].get()) {
      for (auto c : model->getAllCreatures()) {
        //c->tick(); Ticking crashes if it's a player and it dies. It was most likely only an optimization
        auto level = c->getPosition().getLevel();
        level->getSectors(c->getMovementType());
        level->getSectors(c->getMovementType().setForced());
        level->getSectors(MovementType(MovementTrait::WALK).setForced());
        level->getSectors(MovementType(MovementTrait::FLY));
      }
      // Use top level's id as unique id of the model.
      auto id = model->getGroundLevel()->getUniqueId();
      if (!localTime.count(id))
        localTime[id] = (model->getLocalTime() + initialModelUpdate).getDouble();
      if (getCurrentModel() != model)
        updateModel(model, localTime[id], none);
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

optional<ExitInfo> Game::update(double timeDiff, milliseconds endTime) {
  //CHECK(timeDiff >= 0); this will probably fail - check
  PROFILE_BLOCK("Game::update");
  if (auto exitInfo = updateInput())
    return exitInfo;
  considerRealTimeRender();
  Model* currentModel = getCurrentModel();
  auto currentId = currentModel->getGroundLevel()->getUniqueId();
  while (!lastTick || currentTime >= *lastTick + 1) {
    if (!lastTick)
      lastTick = (int)currentTime;
    else
      *lastTick += 1;
    tick(GlobalTime(*lastTick));
  }
  considerRetiredLoadedEvent(currentModel->position);
  if (!updateModel(currentModel, localTime[currentId] + timeDiff, endTime)) {
    localTime[currentId] += timeDiff;
    increaseTime(timeDiff);
  } // Consider setting back the model's local time as now it's desynced from the localTime table.
  return exitInfo;
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

void Game::setWasTransfered() {
  wasTransfered = true;
}

// Return true when the player has just left turn-based mode so we don't increase time in that case.
bool Game::updateModel(Model* model, double totalTime, optional<milliseconds> endTime) {
  do {
    bool wasPlayer = !getPlayerCreatures().empty();
    if (!model->update(totalTime))
      return false;
    if (wasPlayer && getPlayerCreatures().empty())
      return true;
    if (wasTransfered) {
      wasTransfered = false;
      return false;
    }
    if (exitInfo)
      return true;
    if (endTime && Clock::getRealMillis() > *endTime)
      return true;
  } while (1);
}

bool Game::isVillainActive(const Collective* col) {
  const Model* m = col->getModel();
  return m == getMainModel().get() || campaign->isInInfluence(m->position);
}

void Game::updateSunlightMovement() {
  auto previous = sunlightInfo.getState();
  sunlightInfo.update(getGlobalTime() + sunlightTimeOffset);
  if (previous != sunlightInfo.getState())
    for (Vec2 v : models.getBounds())
      if (Model* m = models[v].get()) {
        m->updateSunlightMovement();
        if (playerControl)
          playerControl->onSunlightVisibilityChanged();
      }
}

void Game::tick(GlobalTime time) {
  PROFILE_BLOCK("Game::tick");
  if (!turnEvents.empty() && time.getVisibleInt() > *turnEvents.begin()) {
    auto turn = *turnEvents.begin();
    if (turn == 0) {
      auto values = campaign->getParameters();
      values["current_mod"] = getOptions()->getStringValue(OptionId::CURRENT_MOD2);
      values["version"] = string(BUILD_DATE) + " " + string(BUILD_VERSION);
      values["avatar_id"] = avatarId;
      for (auto& elem : analytics)
        values.insert(elem);
      uploadEvent("campaignStarted", values);
    } else
      uploadEvent("turn", {{"turn", toString(turn)}});
    turnEvents.erase(turn);
  }
  updateSunlightMovement();
  INFO << "Global time " << time;
  for (Collective* col : collectives) {
    if (isVillainActive(col))
      col->update(col->getModel() == getCurrentModel());
  }
  considerAllianceAttack();
}

void Game::setExitInfo(ExitInfo info) {
  exitInfo = std::move(info);
}

void Game::exitAction() {
  ScriptedUIState state;
  auto data = ScriptedUIDataElems::Record{};
  data.elems["save"] = ScriptedUIDataElems::Callback{[this]{
    if (getView()->yesOrNoPrompt("Do you want save and exit the game?")) {
      setExitInfo(GameSaveType::KEEPER);
      return true;
    }
    return false;
  }};
  data.elems["abandon"] = ScriptedUIDataElems::Callback{[this] {
    if (getView()->yesOrNoPrompt("Do you want to abandon your game? This is permanent and the save file will be removed!")) {
      addAnalytics("gameAbandoned", "");
      setExitInfo(ExitAndQuit());
      return true;
    }
    return false;
  }};
  data.elems["options"] = ScriptedUIDataElems::Callback{[this]{
    getOptions()->handle(getView(), &*contentFactory, OptionSet::GENERAL);
    return false;
  }};
#ifdef RELEASE
  bool canRetire = playerControl && !playerControl->getTutorial() && getPlayerCreatures().empty() && gameWon();
#else
  bool canRetire = playerControl && !playerControl->getTutorial() && getPlayerCreatures().empty();
#endif
  if (canRetire)
    data.elems["retire"] = ScriptedUIDataElems::Callback{[this]{
      getView()->stopClock();
      playerControl->takeScreenshot();
      return true;
    }};
  else
    data.elems["retire_inactive"] = ScriptedUIDataElems::Record{};
  getView()->scriptedUI("exit_menu", data, state);
}

Position Game::getTransferPos(Model* from, Model* to) const {
  return to->getGroundLevel()->getLandingSquare(StairKey::transferLanding(),
      from->position - to->position);
}

void Game::transferCreature(Creature* c, Model* to, const vector<Position>& destinations) {
  Model* from = c->getLevel()->getModel();
  if (from != to && !c->getRider()) {
    if (destinations.empty())
      to->transferCreature(from->extractCreature(c), from->position - to->position);
    else
      to->transferCreature(from->extractCreature(c), destinations);
    for (auto& summon : c->getCompanions())
      if (c->getSteed() != summon)
        transferCreature(summon, to, destinations);
    if (auto steed = c->getSteed())
      for (auto& summon : steed->getCompanions())
        transferCreature(summon, to, destinations);
  }
}

bool Game::canTransferCreature(Creature* c, Model* to) {
  return to->canTransferCreature(c, c->getLevel()->getModel()->position - to->position);
}

int Game::getModelDistance(const Collective* c1, const Collective* c2) const {
  return c1->getModel()->position.dist8(c2->getModel()->position);
}

void Game::presentWorldmap() {
  view->presentWorldmap(*campaign, baseModel);
}

Model* Game::chooseSite(const string& message, Model* current) const {
  if (auto dest = view->chooseSite("Choose destination site:", *campaign, current->position))
    return NOTNULL(models[*dest].get());
  return nullptr;
}

void Game::considerRetiredLoadedEvent(Vec2 coord) {
  if (!visited[coord]) {
    visited[coord] = true;
    if (auto retired = campaign->getSites()[coord].getRetired())
        uploadEvent("retiredLoaded", {{"retiredId", retired->fileInfo.getGameId()}});
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

PlayerControl* Game::getPlayerControl() const {
  return playerControl;
}

MusicType Game::getCurrentMusic() const {
  return musicType;
}

void Game::setDefaultMusic() {
  if (sunlightInfo.getState() == SunlightState::NIGHT)
    musicType = MusicType::NIGHT;
  else
    musicType = getCurrentModel()->getDefaultMusic().value_or(MusicType::PEACEFUL);
}

void Game::setCurrentMusic(MusicType type) {
  musicType = type;
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

string Game::getGameOrRetiredIdentifier(Position pos) const {
  Vec2 coords = pos.getModel()->position;
  if (auto retired = campaign->getSites()[coords].getRetired())
    return retired->fileInfo.getGameId();
  return gameIdentifier;
}

View* Game::getView() const {
  return view;
}

ContentFactory* Game::getContentFactory() {
  return &*contentFactory;
}

WarlordInfoWithReference Game::getWarlordInfo() {
  auto creatures = playerCollective->getLeaders();
  for (auto c : playerCollective->getCreatures(MinionTrait::FIGHTER))
    if (!creatures.contains(c))
      creatures.push_back(c);
  return WarlordInfoWithReference {
    creatures.transform([&](auto c) {
      CHECK(c->getPosition().getModel() == this->getMainModel().get());
      c->removeGameReferences();
      return c->getThis().giveMeSharedPointer();
    }),
    getContentFactory(),
    getGameIdentifier()
  };
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
  );
  highscores->add(score);
  highscores->present(view, score);
}

void Game::retired(const string& title, int numKills, int points) {
  int turns = getGlobalTime().getVisibleInt();
  int dungeonTurns =
      (getPlayerCollective()->getLocalTime() - initialModelUpdate).getVisibleInt();
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
        c.turns = turns;
        c.campaignType = campaign->getType();
  );
  highscores->add(score);
  highscores->present(view, score);
}

bool Game::isGameOver() const {
  return !!exitInfo;
}

void Game::gameOver(const Creature* creature, int numKills, const string& enemiesString, int points) {
  int turns = getGlobalTime().getVisibleInt();
  int dungeonTurns = (getPlayerCollective()->getLocalTime() - initialModelUpdate).getVisibleInt();
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
  view->presentTextBelow("Game over", text);
  Highscores::Score score = CONSTRUCT(Highscores::Score,
        c.worldName = getWorldName();
        c.points = points;
        c.gameId = getGameIdentifier();
        c.playerName = creature->getName().firstOrBare();
        c.gameResult = creature->getDeathReason().value_or("");
        c.gameWon = false;
        c.turns = turns;
        c.campaignType = campaign->getType();
  );
  highscores->add(score);
  highscores->present(view, score);
  exitInfo = ExitInfo(ExitAndQuit());
}

Options* Game::getOptions() {
  return options;
}

Encyclopedia* Game::getEncyclopedia() {
  return encyclopedia;
}

Unlocks* Game::getUnlocks() const {
  return unlocks;
}

void Game::initialize(Options* o, Highscores* h, View* v, FileSharing* f, Encyclopedia* e, Unlocks* u,
    SteamAchievements* achievements) {
  options = o;
  highscores = h;
  view = v;
  fileSharing = f;
  encyclopedia = e;
  unlocks = u;
  steamAchievements = achievements;
}

const string& Game::getWorldName() const {
  return campaign->getWorldName();
}

const vector<Collective*>& Game::getCollectives() const {
  return collectives;
}

void Game::addPlayer(Creature* c) {
  if (!players.contains(c))
    players.push_back(c);
}

void Game::removePlayer(Creature* c) {
  players.removeElement(c);
}

const vector<Creature*>& Game::getPlayerCreatures() const {
  return players;
}

SavedGameInfo Game::getSavedGameInfo(vector<string> spriteMods) const {
  auto factory = contentFactory.get();
  auto sortMinions = [&](vector<Creature*>& minions, Creature* leader) {
    sort(minions.begin(), minions.end(), [&] (const Creature* c1, const Creature* c2) {
        return c1 == leader ||
            (c2 != leader && c1->getBestAttack(factory).value > c2->getBestAttack(factory).value);});
    CHECK(minions[0] == leader);
  };
  if (Collective* col = getPlayerCollective()) {
    vector<Creature*> creatures = col->getCreatures();
    CHECK(!creatures.empty());
    Creature* leader = col->getLeaders()[0];
    CHECK(leader);
    sortMinions(creatures, leader);
    creatures.resize(min<int>(creatures.size(), 4));
    vector<SavedGameInfo::MinionInfo> minions;
    for (Creature* c : creatures)
      minions.push_back(SavedGameInfo::MinionInfo::get(factory, c));
    optional<SavedGameInfo::RetiredEnemyInfo> retiredInfo;
    if (auto id = col->getEnemyId()) {
      retiredInfo = SavedGameInfo::RetiredEnemyInfo{*id, col->getVillainType()};
      CHECK(retiredInfo->villainType == VillainType::LESSER || retiredInfo->villainType == VillainType::MAIN)
          << EnumInfo<VillainType>::getString(retiredInfo->villainType);
    }
    auto name = col->getName()->shortened.value_or("???"_s);
    return SavedGameInfo{minions, retiredInfo, std::move(name), getSaveProgressCount(), std::move(spriteMods)};
  } else {
    vector<Creature*> allCreatures;
    for (auto player : players)
      for (auto c : dynamic_cast<Player*>(player->getController())->getTeam())
        if (!allCreatures.contains(c))
          allCreatures.push_back(c);
    sortMinions(allCreatures, players[0]);
    return SavedGameInfo{
        allCreatures.transform([&](auto c) { return SavedGameInfo::MinionInfo::get(factory, c); }),
        none,
        players[0]->getName().bare(),
        getSaveProgressCount(),
        std::move(spriteMods)};
  }
}

void Game::uploadEvent(const string& name, const map<string, string>& m) {
  auto values = m;
  values["eventType"] = name;
  values["gameId"] = getGameIdentifier();
  fileSharing->uploadGameEvent(values);
}

void Game::addAnalytics(const string& name, const string& value) {
  uploadEvent("customEvent", {
    {"name", name},
    {"value", value}
  });
}

void Game::achieve(AchievementId id) const {
  if (steamAchievements)
    steamAchievements->achieve(id);
  if (!unlocks->isAchieved(id)) {
    unlocks->achieve(id);
    if (!steamAchievements) {
      auto& info = contentFactory->achievements.at(id);
      view->windowedMessage(info.viewId, "Achievement unlocked: " + info.name);
    }
  }
}

void Game::handleMessageBoard(Position pos, Creature* c) {
  auto gameId = getGameOrRetiredIdentifier(pos);
  auto boardId = int(combineHash(pos.getCoord(), pos.getLevel()->getUniqueId(), gameId));
  FileSharing::CancelFlag cancel;
  view->displaySplash(nullptr, "Fetching board contents...", [&] {
    cancel.cancel();
  });
  vector<FileSharing::BoardMessage> messages;
  optional<string> error;
  thread t([&] {
    if (auto m = fileSharing->getBoardMessages(cancel, boardId))
      messages = *m;
    else
      error = m.error();
    view->clearSplash();
  });
  view->refreshView();
  t.join();
  if (error) {
    view->presentText("", *error);
    return;
  }
  auto data = ScriptedUIDataElems::Record{};
  auto list = ScriptedUIDataElems::List{};
  ScriptedUIState uiState{};
  for (auto message : messages) {
    list.push_back(ScriptedUIDataElems::Record{
      {
        {"author", message.author},
        {"text", "\"" + message.text + "\""}
      }
    });
  }
  data.elems["messages"] = std::move(list);
  bool wrote = false;
  data.elems["write_something"] = ScriptedUIDataElems::Callback{
      [this, c, boardId, gameId] {
        if (auto text = view->getText("Enter message", "", 80)) {
          if (text->size() >= 2) {
            if (!fileSharing->uploadBoardMessage(gameId, boardId, c->getName().title(), *text))
              view->presentText("", "Please enable online features in the settings.");
          } else
            view->presentText("", "The message was too short.");
        }
        return true;
      }
  };
  view->scriptedUI("message_board", data, uiState);
}

void Game::considerAllianceAttack() {
  if (!allianceAttackPossible || !getPlayerControl() || !Random.roll(1000))
    return;
  int numTriggered = 0;
  vector<Collective*> candidates;
  for (auto col : getVillains(VillainType::MAIN)) {
    if (!isVillainActive(col) || !col->getControl()->canPerformAttack())
      return;
    auto triggers = col->getTriggers(getPlayerCollective());
    if (triggers.empty())
      continue;
    candidates.push_back(col);
    if (!triggers.filter([](auto& t) { return t.value > 0; }).empty())
      ++numTriggered;
  }
  if (numTriggered >= candidates.size() - 1 && candidates.size() >= 2) {
    for (auto col : candidates)
      col->getControl()->launchAllianceAttack(candidates.filter([&](auto other) { return other != col; }));
    getPlayerControl()->addAllianceAttack(candidates);
    allianceAttackPossible = false;
    addAnalytics("alliance", "");
  }
}

bool Game::gameWon() const {
  for (Collective* col : getCollectives())
    if (!col->isConquered() && col->getVillainType() == VillainType::MAIN)
      return false;
  return true;
}

void Game::considerAchievement(const GameEvent& event) {
  using namespace EventInfo;
  event.visit<void>(
      [&](const ConqueredEnemy& info) {
        if (info.byPlayer && info.collective != playerCollective)
          switch (info.collective->getVillainType()) {
            case VillainType::LESSER:
              achieve(AchievementId("lesser_villain"));
              break;
            case VillainType::MAIN:
              achieve(AchievementId("main_villain"));
              break;
            default:
              break;
          }
      },
      [&](const CreatureKilled& info) {
        if (auto& a = info.victim->getAttributes().killedAchievement)
          achieve(*a);
      },
      [&](const CreatureStunned& info) {
        if (auto& a = info.victim->getAttributes().killedAchievement)
          achieve(*a);
      },
      [&](const RetiredGame& info) {
        achieve(AchievementId("retired"));
      },
      [](auto&) {}
  );
}

void Game::addEvent(const GameEvent& event) {
  if (event.contains<EventInfo::CreatureMoved>() && !!playerControl) {
    playerControl->onEvent(event); // shortcut to optimize because only PlayerControl cares about this event
  } else
    for (Vec2 v : models.getBounds())
      if (models[v])
        models[v]->addEvent(event);
  using namespace EventInfo;
  event.visit<void>(
      [&](const ConqueredEnemy& info) {
        Collective* col = info.collective;
        if (col->getVillainType() != VillainType::NONE) {
          if (auto id = col->getEnemyId())
            uploadEvent("customEvent", {
              {"name", "villainConquered"},
              {"value", id->data()}
            });
          Vec2 coords = col->getModel()->position;
          if (!campaign->isDefeated(coords)) {
            if (auto retired = campaign->getSites()[coords].getRetired())
              uploadEvent("retiredConquered", {{"retiredId", retired->fileInfo.getGameId()}});
            if (coords != campaign->getPlayerPos())
              campaign->setDefeated(contentFactory.get(), coords);
          }
        }
        if (col->getVillainType() == VillainType::LESSER || col->getVillainType() == VillainType::MAIN)
          ++numLesserVillainsDefeated;
        if (col->getVillainType() == VillainType::MAIN && gameWon()) {
          addEvent(WonGame{});
        }
      },
      [&](const auto&) {}
  );
  considerAchievement(event);
}

