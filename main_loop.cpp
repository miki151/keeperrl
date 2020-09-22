#include "avatar_info.h"
#include "file_path.h"
#include "stdafx.h"
#include "main_loop.h"
#include "view.h"
#include "highscores.h"
#include "music.h"
#include "options.h"
#include "progress_meter.h"
#include "file_sharing.h"
#include "square.h"
#include "model_builder.h"
#include "parse_game.h"
#include "name_generator.h"
#include "view.h"
#include "village_control.h"
#include "campaign.h"
#include "game.h"
#include "model.h"
#include "clock.h"
#include "view_id.h"
#include "saved_game_info.h"
#include "retired_games.h"
#include "save_file_info.h"
#include "creature.h"
#include "campaign_builder.h"
#include "player_role.h"
#include "campaign_type.h"
#include "game_save_type.h"
#include "exit_info.h"
#include "tutorial.h"
#include "model.h"
#include "item_type.h"
#include "pretty_printing.h"
#include "content_factory.h"
#include "enemy_factory.h"
#include "external_enemies.h"
#include "game_config.h"
#include "avatar_menu_option.h"
#include "creature_name.h"
#include "tileset.h"
#include "content_factory.h"
#include "scroll_position.h"
#include "miniunz.h"
#include "external_enemies_type.h"
#include "mod_info.h"
#include "container_range.h"
#include "extern/iomanip.h"
#include "enemy_info.h"
#include "level.h"
#include "simple_game.h"
#include "monster_ai.h"
#include "mem_usage_counter.h"
#include "gui_elem.h"
#include "encyclopedia.h"
#include "game_info.h"
#include "tribe_alignment.h"

#ifdef USE_STEAMWORKS
#include "steam_ugc.h"
#include "steam_client.h"
#endif

MainLoop::MainLoop(View* v, Highscores* h, FileSharing* fSharing, const DirectoryPath& freePath,
    const DirectoryPath& uPath, const DirectoryPath& modsDir, Options* o, Jukebox* j, SokobanInput* soko,
    TileSet* tileSet, int sv, string modVersion)
      : view(v), dataFreePath(freePath), userPath(uPath), modsDir(modsDir), options(o), jukebox(j), highscores(h), fileSharing(fSharing),
        sokobanInput(soko), tileSet(tileSet), saveVersion(sv), modVersion(modVersion) {
}

vector<SaveFileInfo> MainLoop::getSaveFiles(const DirectoryPath& path, const string& suffix) {
  vector<SaveFileInfo> ret;
  for (auto file : path.getFiles()) {
    if (file.hasSuffix(suffix))
      ret.push_back({file.getFileName(), file.getModificationTime(), false});
  }
  sort(ret.begin(), ret.end(), [](const SaveFileInfo& a, const SaveFileInfo& b) {
        return a.date > b.date;
      });
  return ret;
}

static string getDateString(time_t t) {
  char buf[100];
  strftime(buf, sizeof(buf), "%c", std::localtime(&t));
  return buf;
}

bool MainLoop::isCompatible(int loadedVersion) {
  return loadedVersion > 2 && loadedVersion <= saveVersion && loadedVersion / 100 == saveVersion / 100;
}

static string getSaveSuffix(GameSaveType t) {
  switch (t) {
    case GameSaveType::KEEPER: return ".kep";
    case GameSaveType::ADVENTURER: return ".adv";
    case GameSaveType::RETIRED_SITE: return ".sit";
    case GameSaveType::RETIRED_CAMPAIGN: return ".cam";
    case GameSaveType::WARLORD: return ".war";
    case GameSaveType::AUTOSAVE: return ".aut";
  }
}

bool MainLoop::useSingleThread() {
  return options->getBoolValue(OptionId::SINGLE_THREAD);
}

template <typename T>
optional<T> MainLoop::loadFromFile(const FilePath& filename) {
  auto f = [&] {
    T obj;
    CompressedInput input(filename.getPath());
    string discard;
    SavedGameInfo discard2;
    int version;
    input.getArchive() >> version >> discard >> discard2;
    input.getArchive() >> obj;
    return std::move(obj);
  };
  if (useSingleThread())
    return f();
  else
    try { return f(); }
  catch (...) {
    return none;
  }
}

void MainLoop::saveGame(PGame& game, const FilePath& path) {
  CompressedOutput out(path.getPath());
  string name = game->getGameDisplayName();
  SavedGameInfo savedInfo = game->getSavedGameInfo(tileSet->getSpriteMods());
  out.getArchive() << saveVersion << name << savedInfo;
  out.getArchive() << game;
}

struct RetiredModelInfo {
  PModel SERIAL(model);
  ContentFactory SERIAL(factory);
  SERIALIZE_ALL_NO_VERSION(model, factory)
};

struct RetiredModelInfoWithReference {
  shared_ptr<Model> SERIAL(model);
  ContentFactory* SERIAL(factory);
  SERIALIZE_ALL_NO_VERSION(model, serializeAsValue(factory))
};

optional<RetiredModelInfo> MainLoop::loadRetiredModelFromFile(const FilePath& path) {
  for (auto alignment : ENUM_ALL(TribeAlignment))
    TribeId::switchForSerialization(getPlayerTribeId(alignment), TribeId::getRetiredKeeper());
  auto _ = OnExit([]{TribeId::clearSwitch();});
  return loadFromFile<RetiredModelInfo>(path);
}

void MainLoop::saveMainModel(PGame& game, const FilePath& modelPath, const FilePath& warlordPath) {
  CompressedOutput modelOut(modelPath.getPath());
  string name = game->getGameDisplayName();
  SavedGameInfo savedInfo = game->getSavedGameInfo(tileSet->getSpriteMods());
  modelOut.getArchive() << saveVersion << name << savedInfo;
  RetiredModelInfoWithReference info {
    game->getMainModel().giveMeSharedPointer(),
    game->getContentFactory()
  };
  modelOut.getArchive() << info;
  CompressedOutput warlordOut(warlordPath.getPath());
  auto warlordInfo = game->getWarlordInfo();
  game->getMainModel()->discardForRetirement();
  warlordOut.getArchive() << saveVersion << name << savedInfo << warlordInfo;
}

int MainLoop::getSaveVersion(const SaveFileInfo& save) {
  if (auto info = getNameAndVersion(userPath.file(save.filename)))
    return info->second;
  else
    return -1;
}

void MainLoop::uploadFile(const FilePath& path, const string& title, const SavedGameInfo& info) {
  atomic<bool> cancelled(false);
  optional<string> error;
  optional<string> url;
  doWithSplash("Uploading "_s + path.getPath() + "...", 1,
      [&] (ProgressMeter& meter) {
        error = fileSharing->uploadSite(path, title, getOldInfo(info), meter, url);
      },
      [&] {
        cancelled = true;
        fileSharing->cancel();
      });
  if (url)
    if (view->yesOrNoPrompt("Your retired dungeon has been uploaded to Steam Workshop. "
        "Would you like to open its page in your browser now?"))
      openUrl("https://steamcommunity.com/sharedfiles/filedetails/?id=" + *url);
  if (error && !cancelled)
    view->presentText("Error uploading file", *error);
}

FilePath MainLoop::getSavePath(const PGame& game, GameSaveType gameType) {
  auto id = game->getGameIdentifier();
  CHECK(stripFilename(id) == id);
  return userPath.file(id + getSaveSuffix(gameType));
}

void MainLoop::saveUI(PGame& game, GameSaveType type) {
  auto path = getSavePath(game, type);
  function<void()> uploadFun = nullptr;
  if (type == GameSaveType::RETIRED_SITE) {
    int saveTime = game->getMainModel()->getSaveProgressCount();
    doWithSplash("Retiring site...", saveTime,
        [&] (ProgressMeter& meter) {
          Square::progressMeter = &meter;
          if (!game->getSavedGameInfo(tileSet->getSpriteMods()).retiredEnemyInfo)
            // only upload if it's not a retired enemy
            uploadFun = [this, path, name = game->getGameDisplayName(),
                savedInfo = game->getSavedGameInfo(tileSet->getSpriteMods())] {
              uploadFile(path, name, savedInfo);
            };
          auto warlordPath = getSavePath(game, GameSaveType::WARLORD);
          saveMainModel(game, path, warlordPath);
        });
  } else {
    int saveTime = game->getSaveProgressCount();
    doWithSplash(type == GameSaveType::AUTOSAVE ? "Autosaving" : "Saving game...", saveTime,
        [&] (ProgressMeter& meter) {
        Square::progressMeter = &meter;
        MEASURE(saveGame(game, path), "saving time")});
  }
  Square::progressMeter = nullptr;
  if (uploadFun)
    uploadFun();
}

void MainLoop::eraseSaveFile(const PGame& game, GameSaveType type) {
  remove(getSavePath(game, type).getPath());
}

void MainLoop::getSaveOptions(const vector<pair<GameSaveType, string>>& games, vector<ListElem>& options,
    vector<SaveFileInfo>& allFiles) {
  for (auto elem : games) {
    vector<SaveFileInfo> files = getSaveFiles(userPath, getSaveSuffix(elem.first));
    files = files.filter([this] (const SaveFileInfo& info) { return isCompatible(getSaveVersion(info));});
    append(allFiles, files);
    if (!files.empty()) {
      options.emplace_back(elem.second, ListElem::TITLE);
      append(options, files.transform(
          [this] (const SaveFileInfo& info) {
              auto nameAndVersion = getNameAndVersion(userPath.file(info.filename));
              return ListElem(nameAndVersion->first, getDateString(info.date));}));
    }
  }
}

optional<SaveFileInfo> MainLoop::chooseSaveFile(const vector<ListElem>& options,
    const vector<SaveFileInfo>& allFiles, string noSaveMsg, View* view) {
  if (options.empty()) {
    view->presentText("", noSaveMsg);
    return none;
  }
  auto ind = view->chooseFromList("Choose game", options, 0);
  if (ind)
    return allFiles[*ind];
  else
    return none;
}

enum class MainLoop::ExitCondition {
  ALLIES_WON,
  ENEMIES_WON,
  TIMEOUT,
  UNKNOWN
};

void MainLoop::bugReportSave(PGame& game, FilePath path) {
  int saveTime = game->getSaveProgressCount();
  doWithSplash("Saving game...", saveTime,
      [&] (ProgressMeter& meter) {
      Square::progressMeter = &meter;
      MEASURE(saveGame(game, path), "saving time")});
  Square::progressMeter = nullptr;
}

template <class T>
static void dumpMemUsage(const T& elem) {
#ifdef MEM_USAGE_TEST
  dumpGuiLineNumbers(std::cout);
  MemUsageArchive ar;
  ar << elem;
  ar.dumpUsage(std::cout);
#endif
}

MainLoop::ExitCondition MainLoop::playGame(PGame game, bool withMusic, bool noAutoSave, bool splashScreen,
    function<optional<ExitCondition>(WGame)> exitCondition, milliseconds stepTimeMilli, optional<int> maxTurns) {
  if (!splashScreen)
    registerModPlaytime(true);
  OnExit on_exit([&]() {
    if (!splashScreen)
      registerModPlaytime(false);
  });
  if (tileSet)
    tileSet->setTilePathsAndReload(game->getContentFactory()->tilePaths);
  view->reset();
  if (!noAutoSave)
    view->setBugReportSaveCallback([&] (FilePath path) { bugReportSave(game, path); });
  DestructorFunction removeCallback([&] { view->setBugReportSaveCallback(nullptr); });
  Encyclopedia encyclopedia(game->getContentFactory());
  game->initialize(options, highscores, view, fileSharing, &encyclopedia);
  if (splashScreen)
    game->initializeModels();
  else
    doWithSplash("Initializing game...", 0,
        [&] (ProgressMeter& meter) {
          game->initializeModels();
        });
  Intervalometer meter(stepTimeMilli);
  Intervalometer pausingMeter(stepTimeMilli);
  auto lastMusicUpdate = GlobalTime(-1000);
  auto lastAutoSave = game->getGlobalTime();
  optional<GlobalTime> exitTime;
  if (maxTurns)
    exitTime = game->getGlobalTime() + TimeInterval(*maxTurns);
  while (1) {
    if (exitTime && game->getGlobalTime() >= *exitTime)
      throw GameExitException();
    double step = 1;
    if (!game->isTurnBased()) {
      double gameTimeStep = view->getGameSpeed() / stepTimeMilli.count();
      auto timeMilli = view->getTimeMilli();
      double count = meter.getCount(timeMilli);
      //INFO << "Intervalometer " << timeMilli << " " << count;
      step = min(1.0, double(count) * gameTimeStep);
      if (maxTurns)
        step = 1;
      if (view->isClockStopped()) {
        // Advance the clock a little more until the local time reaches 0.99,
        // so creature animations are paused at their actual positions.
        double localTime = game->getMainModel()->getLocalTimeDouble();
        if (localTime - trunc(localTime) < pauseAnimationRemainder) {
          step = min(1.0, double(pausingMeter.getCount(view->getTimeMilliAbsolute())) * gameTimeStep);
          step = min(step, pauseAnimationRemainder - (localTime - trunc(localTime)));
        }
      } else
        pausingMeter.clear();
    }
    INFO << "Time step " << step;
    if (auto exitInfo = game->update(step)) {
      exitInfo->visit(
          [&](ExitAndQuit) {
            eraseAllSavesExcept(game, none);
            if (!splashScreen) {
              dumpMemUsage(game);
            }
          },
          [&](GameSaveType type) {
            if (type == GameSaveType::RETIRED_SITE) {
              game->prepareSiteRetirement();
              saveUI(game, type);
              game->doneRetirement();
            } else
              saveUI(game, type);
            eraseAllSavesExcept(game, type);
          }
      );
      return ExitCondition::UNKNOWN;
    }
    if (exitCondition)
      if (auto c = exitCondition(game.get()))
        return *c;
    auto gameTime = game->getGlobalTime();
    if (lastMusicUpdate < gameTime && withMusic) {
      jukebox->setType(game->getCurrentMusic(), true);
      lastMusicUpdate = gameTime;
    }
    if (lastAutoSave < gameTime - TimeInterval(options->getIntValue(OptionId::AUTOSAVE2)) && !noAutoSave) {
      if (options->getBoolValue(OptionId::AUTOSAVE2)) {
        saveUI(game, GameSaveType::AUTOSAVE);
        eraseAllSavesExcept(game, GameSaveType::AUTOSAVE);
      }
      lastAutoSave = gameTime;
    }
    view->refreshView();
  }
}

void MainLoop::eraseAllSavesExcept(const PGame& game, optional<GameSaveType> except) {
  for (auto erasedType : ENUM_ALL(GameSaveType))
    if (erasedType != GameSaveType::WARLORD && erasedType != except)
      eraseSaveFile(game, erasedType);
}

optional<RetiredGames> MainLoop::getRetiredGames(CampaignType type) {
  switch (type) {
    case CampaignType::FREE_PLAY: {
      RetiredGames ret;
      for (auto& info : getSaveFiles(userPath, getSaveSuffix(GameSaveType::RETIRED_CAMPAIGN)))
        if (isCompatible(getSaveVersion(info)))
          if (auto saved = loadSavedGameInfo(userPath.file(info.filename)))
            ret.addLocal(*saved, info, true);
      for (auto& info : getSaveFiles(userPath, getSaveSuffix(GameSaveType::RETIRED_SITE)))
        if (isCompatible(getSaveVersion(info)))
          if (auto saved = loadSavedGameInfo(userPath.file(info.filename)))
            if (!saved->retiredEnemyInfo)
              ret.addLocal(*saved, info, false);
      vector<FileSharing::SiteInfo> onlineSites;
      optional<string> error;
      doWithSplash("Fetching list of retired dungeons from the server...",
          [&] {
            if (auto sites = fileSharing->listSites())
              onlineSites = *sites;
            else
              error = sites.error();
          },
          [&] { fileSharing->cancel(); });
      if (error)
        view->presentText("", *error);
      for (auto& elem : onlineSites)
        if (isCompatible(elem.version))
          ret.addOnline(fromOldInfo(elem.gameInfo), elem.fileInfo, elem.totalGames, elem.wonGames, elem.subscribed);
      ret.sort();
      return ret;
    }
    default:
      return none;
  }
}

PGame MainLoop::prepareTutorial(const ContentFactory* contentFactory) {
  PGame game = loadGame(dataFreePath.file("tutorial.kep"));
  if (game) {
    USER_CHECK(contentFactory->immigrantsData.count("tutorial"));
    Tutorial::createTutorial(*game, contentFactory);
  } else
    view->presentText("Sorry", "Failed to load the tutorial :(");
  return game;
}

struct ModelTable {
  Table<PModel> models;
  vector<ContentFactory> factories;
  int numRetiredVillains;
};

TilePaths MainLoop::getTilePathsForAllMods() const {
  auto readTiles = [&] (const GameConfig* config, vector<string> modNames) {
    vector<TileInfo> tileDefs;
    if (auto res = config->readObject(tileDefs, GameConfigId::TILES, nullptr))
      return optional<TilePaths>();
    return optional<TilePaths>(TilePaths(std::move(tileDefs), std::move(modNames)));
  };
  auto currentMod = options->getVectorStringValue(OptionId::CURRENT_MOD2);
  GameConfig currentConfig = getGameConfig(currentMod);
  auto ret = readTiles(&currentConfig, currentMod);
  for (auto modName : modsDir.getSubDirs()) {
    GameConfig config({modsDir.subdirectory(modName)});
    if (auto paths = readTiles(&config, {modName})) {
      if (ret)
        ret->merge(*paths);
      else
        ret = paths;
    }
  }
  USER_CHECK(ret) << "No available tile paths found";
  return *ret;
}

vector<WarlordInfo> MainLoop::readWarlordInfos() {
  vector<ListElem> optionsUnused;
  vector<SaveFileInfo> files;
  getSaveOptions({{GameSaveType::WARLORD, ""}}, optionsUnused, files);
  vector<WarlordInfo> ret;
  for (auto& f : files)
    if (auto res = loadFromFile<WarlordInfo>(userPath.file(f.filename)))
      ret.push_back(std::move(*res));
  return ret;
};

PGame MainLoop::prepareCampaign(RandomGen& random) {
  while (1) {
    ContentFactory contentFactory;
    tileSet->clear();
    doWithSplash("Loading gameplay data", [&] {
      contentFactory = createContentFactory(false);
      if (tileSet)
        tileSet->setTilePaths(contentFactory.tilePaths);
    });
    tileSet->loadTextures();
    if (options->getIntValue(OptionId::SUGGEST_TUTORIAL) == 1) {
      auto tutorialIndex = view->chooseFromList("", {ListElem("Would you like to start with the tutorial?", ListElem::TITLE),
          ListElem("Yes"), ListElem("No"), ListElem("No, and don't ask me again")}, 0, MenuType::YES_NO);
      if (tutorialIndex == 0) {
        auto contentFactory2 = createContentFactory(true);
        if (auto ret = prepareTutorial(&contentFactory2))
          return ret;
      } else
      if (tutorialIndex == 2) {
        options->setValue(OptionId::SUGGEST_TUTORIAL, 0);
      }
    }
    auto avatarChoice = getAvatarInfo(view, contentFactory.keeperCreatures, contentFactory.adventurerCreatures,
        readWarlordInfos(), &contentFactory);
    if (auto avatar = avatarChoice.getReferenceMaybe<AvatarInfo>()) {
      CampaignBuilder builder(view, random, options, contentFactory.villains, contentFactory.gameIntros, *avatar);
      tileSet->setTilePathsAndReload(getTilePathsForAllMods());
      if (auto setup = builder.prepareCampaign(&contentFactory, bindMethod(&MainLoop::getRetiredGames, this),
          CampaignType::FREE_PLAY,
          contentFactory.getCreatures().getNameGenerator()->getNext(NameGeneratorId("WORLD")))) {
        auto models = prepareCampaignModels(*setup, *avatar, random, &contentFactory);
        for (auto& f : models.factories)
          contentFactory.merge(std::move(f));
        map<string, string> analytics {
          {"retired_villains", toString(models.numRetiredVillains)},
          {"biome", setup->startingBiome.data()}
        };
        return Game::campaignGame(std::move(models.models), *setup, std::move(*avatar), std::move(contentFactory),
            std::move(analytics));
      } else
        continue;
    } else
    if (auto warlordInfo = avatarChoice.getReferenceMaybe<WarlordInfo>()) {
      auto retiredGames = *getRetiredGames(CampaignType::FREE_PLAY);
      for (int i : All(retiredGames.getAllGames()))
        if (retiredGames.getAllGames()[i].fileInfo.getGameId() == warlordInfo->gameIdentifier) {
          retiredGames.erase(i);
          break;
        }
      auto playerInfos = warlordInfo->creatures.transform(
          [&](auto& c) { return PlayerInfo(c.get(), &warlordInfo->contentFactory); });
      sort(++playerInfos.begin(), playerInfos.end(),
           [](auto c1, auto c2) { return c1.bestAttack.value > c2.bestAttack.value; });
      auto chosen = view->prepareWarlordGame(retiredGames, playerInfos, 12, 10);
      if (!chosen.empty()) {
        auto setup = CampaignBuilder::getWarlordCampaign(retiredGames.getActiveGames(),
            warlordInfo->creatures[0]->getName().firstOrBare());
        EnemyFactory enemyFactory(Random, contentFactory.getCreatures().getNameGenerator(), contentFactory.enemies,
            contentFactory.buildingInfo, {});
        ModelBuilder modelBuilder(nullptr, random, options, sokobanInput, &contentFactory, std::move(enemyFactory));
        auto models = prepareCampaignModels(setup, TribeAlignment::LAWFUL, std::move(modelBuilder));
        for (auto& f : models.factories)
          warlordInfo->contentFactory.merge(std::move(f));
        vector<PCreature> creatures;
        for (int index : chosen)
          creatures.push_back(
              [&]{
                for (auto& c : warlordInfo->creatures)
                  if (c && c->getUniqueId() == playerInfos[index].creatureId)
                    return std::move(c);
                fail();
              }()
          );
        return Game::warlordGame(std::move(models.models), setup, std::move(creatures),
            std::move(warlordInfo->contentFactory), warlordInfo->gameIdentifier);
      }
    } else {
      auto option = *avatarChoice.getValueMaybe<AvatarMenuOption>();
      switch (option) {
        case AvatarMenuOption::GO_BACK:
          return nullptr;
        case AvatarMenuOption::CHANGE_MOD:
          showMods();
          continue;
        case AvatarMenuOption::TUTORIAL: {
          auto contentFactory = createContentFactory(true);
          if (auto ret = prepareTutorial(&contentFactory))
            return ret;
          else
            continue;
        }
        case AvatarMenuOption::LOAD_GAME:
          if (auto ret = loadPrevious())
            return ret;
          else
            continue;
      }
    }
  }
}

void MainLoop::splashScreen() {
  ProgressMeter meter(1);
  jukebox->setType(MusicType::INTRO, true);
  auto gameConfig = getVanillaConfig();
  auto contentFactory = createContentFactory(true);
  if (tileSet)
    tileSet->setTilePathsAndReload(contentFactory.tilePaths);
  EnemyFactory enemyFactory(Random, contentFactory.getCreatures().getNameGenerator(), contentFactory.enemies,
      contentFactory.buildingInfo, {});
  auto model = ModelBuilder(&meter, Random, options, sokobanInput, &contentFactory, std::move(enemyFactory))
      .splashModel(dataFreePath.file("splash.txt"));
  playGame(Game::splashScreen(std::move(model), CampaignBuilder::getEmptyCampaign(), std::move(contentFactory)),
      false, true, true);
}

void MainLoop::showCredits(const FilePath& path) {
  ifstream in(path.getPath());
  CHECK(!!in);
  vector<ListElem> lines;
  while (1) {
    char buf[100];
    in.getline(buf, 100);
    if (!in)
      break;
    string s(buf);
    if (s.back() == ':')
      lines.emplace_back(s, ListElem::TITLE);
    else
      lines.emplace_back(s, ListElem::NORMAL);
  }
  view->presentList("Credits", lines, false);
}

const auto modVersionFilename = "version_info";

optional<ModVersionInfo> MainLoop::getLocalModVersionInfo(const string& mod) {
  ifstream in(modsDir.subdirectory(mod).file(modVersionFilename).getPath());
  ModVersionInfo info {};
  in >> info.steamId >> info.version >> info.compatibilityTag;
  if (info.compatibilityTag == modVersion) // this also handles the check if the file existed and had sane contents
    return info;
  else
    return none;
}

void MainLoop::updateLocalModVersion(const string& mod, const ModVersionInfo& info) {
  ofstream out(modsDir.subdirectory(mod).file(modVersionFilename).getPath());
  if (!!out) {
    out << info.steamId << "\n" << info.version << "\n" << info.compatibilityTag << "\n";
  }
}

const auto modDetailsFilename = "details.txt";

optional<ModDetails> MainLoop::getLocalModDetails(const string& mod) {
  ifstream in(modsDir.subdirectory(mod).file(modDetailsFilename).getPath());
  ModDetails ret;
  in >> std::quoted(ret.author) >> std::quoted(ret.description);
  return ret;
}

void MainLoop::updateLocalModDetails(const string& mod, const ModDetails& info) {
  ofstream out(modsDir.subdirectory(mod).file(modDetailsFilename).getPath());
  if (!!out) {
    out << std::quoted(info.author) << "\n" << std::quoted(info.description) << "\n";
  }
}

void MainLoop::removeMod(const string& name) {
  // TODO: how to make it safer?
  auto modDir = modsDir.subdirectory(name);
  modDir.removeRecursively();
}

// When mod changes name, we have to remove old directory
void MainLoop::removeOldSteamMod(SteamId steamId, const string& newName) {
  auto modDir = modsDir;
  auto modList = modDir.getSubDirs();
  for (auto& modName : modList)
    if (modName != newName)
      if (auto ver = getLocalModVersionInfo(modName))
        if (ver->steamId == steamId)
          removeMod(modName);
}

vector<ModInfo> MainLoop::getAllMods(const vector<ModInfo>& onlineMods) {
  vector<string> modList = modsDir.getSubDirs();
  // check if the currentMod exists and has current version
  vector<ModInfo> allMods;
  set<SteamId> alreadyDownloaded;
  for (auto& mod : modList)
    if (auto version = getLocalModVersionInfo(mod)) {
      ModInfo modInfo;
      modInfo.versionInfo = *version;
      modInfo.name = mod;
      modInfo.canUpload = true;
      if (auto details = getLocalModDetails(mod))
        modInfo.details = *details;
      for (auto& onlineMod : onlineMods)
        if (onlineMod.versionInfo.steamId == version->steamId) {
          modInfo = onlineMod;
          if (!modInfo.canUpload && modInfo.versionInfo.version > version->version)
            modInfo.actions.push_back("Update");
          alreadyDownloaded.insert(onlineMod.versionInfo.steamId);
          break;
        }
      if (options->hasVectorStringValue(OptionId::CURRENT_MOD2, mod)) {
        modInfo.isActive = true;
        modInfo.actions.push_back("Deactivate");
      } else
        modInfo.actions.push_back("Activate");
      if (modInfo.canUpload)
        modInfo.actions.push_back("Upload");
      modInfo.isLocal = true;
      allMods.push_back(std::move(modInfo));
    }
  for (auto& mod : onlineMods)
    if (!alreadyDownloaded.count(mod.versionInfo.steamId)) {
      allMods.push_back(mod);
      allMods.back().actions.push_back("Download");
    }
  return allMods;
}

void MainLoop::downloadMod(ModInfo& mod) {
  atomic<bool> cancelled(false);
  optional<string> error;
  doWithSplash("Downloading mod \"" + mod.name + "\"...", 1,
      [&] (ProgressMeter& meter) {
        modsDir.createIfDoesntExist();
        error = fileSharing->downloadMod(mod.name, mod.versionInfo.steamId, modsDir, meter);
        if (!error) {
          updateLocalModVersion(mod.name, mod.versionInfo);
          updateLocalModDetails(mod.name, mod.details);
          removeOldSteamMod(mod.versionInfo.steamId, mod.name);
        }
      },
      [&] {
        cancelled = true;
        fileSharing->cancel();
      });
  if (error && !cancelled)
    view->presentText("Error downloading file", *error);
}

void MainLoop::uploadMod(ModInfo& mod) {
  auto config = getGameConfig({mod.name});
  ContentFactory f;
  if (auto err = f.readData(&config, {mod.name})) {
    view->presentText("Mod \"" + mod.name + "\" has errors: ", *err);
    return;
  }
  atomic<bool> cancelled(false);
  optional<string> error;
  doWithSplash("Uploading mod \"" + mod.name + "\"...", 1,
      [&] (ProgressMeter& meter) {
        error = fileSharing->uploadMod(mod, modsDir, meter);
        updateLocalModVersion(mod.name, mod.versionInfo);
      },
      [&] {
        cancelled = true;
        fileSharing->cancel();
      });
  if (error && !cancelled)
    view->presentText("Error uploading mod:", *error);
}

void MainLoop::createNewMod() {
  if (auto name = view->getText("Enter name of new mod", "", 15)) {
    if (modsDir.getSubDirs().contains(*name)) {
      view->presentText("Error", "Mod \"" + *name + "\" is alread installed");
      return;
    }
    auto targetPath = modsDir.subdirectory(*name);
    targetPath.createIfDoesntExist();
    view->presentText("", "Your mod is located in folder \""_s + targetPath.absolute().getPath() + "\"");
    updateLocalModVersion(*name, ModVersionInfo{0, 0, modVersion});
    updateLocalModDetails(*name, ModDetails{"", ""});
  }
}

vector<ModInfo> MainLoop::getOnlineMods() {
  vector<ModInfo> ret;
  optional<string> error;
  doWithSplash( "Downloading list of online mods...", 1,
      [&] (ProgressMeter& meter) {
        if (auto mods = fileSharing->getOnlineMods())
          ret = *mods;
        else
          error = mods.error();
        sort(ret.begin(), ret.end(), [](const ModInfo& m1, const ModInfo& m2) { return m1.upvotes > m2.upvotes; });
      }, [&]{fileSharing->cancel();  });
  if (error)
    view->presentText("", *error);
  return ret;
}

void MainLoop::showMods() {
  int highlighted = 0;
  vector<ModInfo> onlineMods = getOnlineMods();
  while (1) {
    vector<ModInfo> allMods = getAllMods(onlineMods);
    auto choice = view->getModAction(highlighted, allMods);
    if (!choice)
      break;
    highlighted = choice->index;
    if (highlighted == -1) {
      createNewMod();
      return showMods();
    }
    auto& chosenMod = allMods[highlighted];
    auto action = chosenMod.actions[choice->actionId];
    if (action == "Activate")
      options->addVectorStringValue(OptionId::CURRENT_MOD2, allMods[highlighted].name);
    else if (action == "Deactivate")
      options->removeVectorStringValue(OptionId::CURRENT_MOD2, allMods[highlighted].name);
    else if (action == "Download" || action == "Update")
      downloadMod(chosenMod);
    else if (action == "Upload") {
      uploadMod(chosenMod);
      onlineMods = getOnlineMods();
    }
  }
}

void MainLoop::playMenuMusic() {
  jukebox->setType(MusicType::MAIN, true);
}

void MainLoop::considerGameEventsPrompt() {
  if (options->getIntValue(OptionId::GAME_EVENTS) == 1) {
    if (view->yesOrNoPrompt("The imps would like to gather statistics while you're playing the game and send them anonymously to the developer. This would be very helpful in designing the game. Do you agree?"))
      options->setValue(OptionId::GAME_EVENTS, 2);
    else
      options->setValue(OptionId::GAME_EVENTS, 0);
  }
}

void MainLoop::considerFreeVersionText(bool tilesPresent) {
  if (!tilesPresent)
    view->presentText("", "You are playing a version of KeeperRL without graphical tiles. "
        "Besides lack of graphics and music, this "
        "is the same exact game as the full version. If you'd like to buy the full version, "
        "please visit keeperrl.com.\n \nYou can also get it by donating to any wildlife charity. "
        "More information on the website.");
}

DirectoryPath MainLoop::getVanillaDir() const {
  return dataFreePath.subdirectory("game_config");
}

GameConfig MainLoop::getVanillaConfig() const {
  return GameConfig({getVanillaDir()});
}

void MainLoop::playSimpleGame() {
  auto factory = createContentFactory(false);
  SimpleGame game(&factory, this);
  while (1) {
    game.update();
  }
}

GameConfig MainLoop::getGameConfig(const vector<string>& modNames) const {
  return GameConfig(concat({getVanillaDir()}, modNames.transform([&](const string& name) { return modsDir.subdirectory(name); })));
}

ContentFactory MainLoop::createContentFactory(bool vanillaOnly) const {
  ContentFactory ret;
  auto tryConfig = [&](const vector<string>& modNames) {
    auto config = getGameConfig(modNames);
    return ret.readData(&config, modNames);
  };
  if (vanillaOnly) {
#ifdef RELEASE
    if (auto err = tryConfig({}))
      USER_FATAL << "Error loading vanilla game data: " << *err;
#else
    while (true) {
      if (auto err = tryConfig({}))
        USER_INFO << "Error loading vanilla game data: " << *err;
      else
        break;
    }
#endif
  } else {
    auto chosenMod = options->getVectorStringValue(OptionId::CURRENT_MOD2);
    if (auto err = tryConfig(chosenMod)) {
      USER_INFO << "Error loading mod \"" << chosenMod << "\": " << *err << "\n\nUsing vanilla game data";
      if (auto err = tryConfig({}))
        USER_FATAL << "Error loading vanilla game data: " << *err;
    }
  }
  return ret;
}

void MainLoop::launchQuickGame(optional<int> maxTurns) {
  vector<ListElem> optionsUnused;
  vector<SaveFileInfo> files;
  getSaveOptions({
      {GameSaveType::AUTOSAVE, "Recovered games:"},
      {GameSaveType::KEEPER, "Keeper games:"}, {GameSaveType::ADVENTURER, "Adventurer games:"}}, optionsUnused, files);
  auto toLoad = std::min_element(files.begin(), files.end(),
      [](const auto& f1, const auto& f2) { return f1.date > f2.date; });
  PGame game;
  if (toLoad != files.end())
    game = loadGame(userPath.file((*toLoad).filename));
  auto contentFactory = createContentFactory(true);
  if (!game) {
    AvatarInfo avatar = getQuickGameAvatar(view, contentFactory.keeperCreatures, &contentFactory.getCreatures());
    CampaignBuilder builder(view, Random, options, contentFactory.villains, contentFactory.gameIntros, avatar);
    auto result = builder.prepareCampaign(&contentFactory, bindMethod(&MainLoop::getRetiredGames, this),
        CampaignType::QUICK_MAP, "[world]");
    auto models = prepareCampaignModels(*result, std::move(avatar), Random, &contentFactory);
    game = Game::campaignGame(std::move(models.models), *result, std::move(avatar), std::move(contentFactory), {});
    dumpMemUsage(game);
  }
  playGame(std::move(game), true, false, false, nullptr, milliseconds{3}, maxTurns);
}

void MainLoop::start(bool tilesPresent) {
  splashScreen();
  view->reset();
  considerFreeVersionText(tilesPresent);
  considerGameEventsPrompt();
  int lastIndex = 0;
  while (1) {
    playMenuMusic();
    optional<int> choice;
    choice = view->chooseFromList("", {
        "Play", "Settings", "High scores", "Credits", "Quit"}, lastIndex, MenuType::MAIN);
    if (!choice)
      continue;
    lastIndex = *choice;
    switch (*choice) {
      case 0: {
        if (PGame game = prepareCampaign(Random))
          playGame(std::move(game), true, false, false);
        view->reset();
        break;
      }
      case 1: options->handle(view, OptionSet::GENERAL); break;
      case 2: highscores->present(view); break;
      case 3: showCredits(dataFreePath.file("credits.txt")); break;
      case 4: return;
    }
  }
}

void MainLoop::doWithSplash(const string& text, int totalProgress,
    function<void(ProgressMeter&)> fun, function<void()> cancelFun) {
  if (useSingleThread()) {
    ProgressMeter meter(1.0 / totalProgress);
    fun(meter);
  } else
    view->doWithSplash(text, totalProgress, std::move(fun), std::move(cancelFun));
}

void MainLoop::doWithSplash(const string& text, function<void()> fun, function<void()> cancelFun) {
  if (useSingleThread())
    fun();
  else {
    view->displaySplash(nullptr, text, cancelFun);
    auto t = makeScopedThread([fun, this] { fun(); view->clearSplash(); });
    view->refreshView();
  }
}

void MainLoop::modelGenTest(int numTries, const vector<string>& types, RandomGen& random, Options* options) {
  ProgressMeter meter(1);
  auto contentFactory = createContentFactory(false);
  vector<BiomeId> biomes;
  for (auto& elem : contentFactory.biomeInfo)
    if (elem.second.keeperBiome)
      biomes.push_back(elem.first);
  EnemyFactory enemyFactory(Random, contentFactory.getCreatures().getNameGenerator(), contentFactory.enemies,
      contentFactory.buildingInfo, {});
  ModelBuilder(&meter, random, options, sokobanInput, &contentFactory, std::move(enemyFactory))
      .measureSiteGen(numTries, types, std::move(biomes));
}

static CreatureList readAlly(ifstream& input) {
  string ally;
  input >> ally;
  CreatureList ret(100, CreatureId(ally.data()));
  string levelIncreaseText;
  input >> levelIncreaseText;
  EnumMap<ExperienceType, int> levelIncrease;
  vector<ItemType> equipment;
  for (auto increase : split(levelIncreaseText, {','})) {
    int amount = fromString<int>(increase.substr(1));
    switch (increase[0]) {
      case 'M':
        levelIncrease[ExperienceType::MELEE] = amount;
        break;
      case 'S':
        levelIncrease[ExperienceType::SPELL] = amount;
        break;
      case 'A':
        levelIncrease[ExperienceType::ARCHERY] = amount;
        break;
      default:
        FATAL << "Can't parse level increase " << increase;
    }
  }
  string equipmentText;
  input >> equipmentText;
  for (auto id : split(equipmentText, {','})) {
    ItemType type;
    if (auto error = PrettyPrinting::parseObject(type, id))
      FATAL << "Can't parse item type: " << id << ": " << *error;
    else
      equipment.push_back(type);
  }
  ret.addInventory(equipment);
  ret.increaseBaseLevel(levelIncrease);
  return ret;
}

void MainLoop::battleTest(int numTries, const FilePath& levelPath, const FilePath& battleInfoPath, string enemy) {
  ifstream input(battleInfoPath.getPath());
  CreatureList enemies;
  for (auto& elem : split(enemy, {','})) {
    auto enemySplit = split(elem, {':'});
    auto enemyId = enemySplit[0];
    int count = enemySplit.size() > 1 ? fromString<int>(enemySplit[1]) : 1;
    for (int i : Range(count))
      enemies.addUnique(CreatureId(enemyId.data()));
  }
  int cnt = 0;
  input >> cnt;
  auto contentFactory = createContentFactory(false);
  for (int i : Range(cnt)) {
    auto allies = readAlly(input);
    std::cout << allies.getSummary(&contentFactory.getCreatures()) << ": ";
    battleTest(numTries, levelPath, {allies}, {enemies});
  }
}

static vector<CreatureList> readAllies(const FilePath& battleInfoPath) {
  ifstream input(battleInfoPath.getPath());
  int cnt = 0;
  input >> cnt;
  vector<CreatureList> allies;
  for (int i : Range(cnt))
    allies.push_back(readAlly(input));
  return allies;
}

void MainLoop::campaignBattleText(int numTries, const FilePath& levelPath, EnemyId keeperId, VillainGroup group) {
  auto contentFactory = createContentFactory(false);
  for (auto villainType : {VillainType::NONE, VillainType::LESSER, VillainType::MAIN})
    for (auto& villain : contentFactory.villains.at(group))
      if (villain.type == villainType) {
        std::cerr << "Running " << villain.enemyId.data() << std::endl;
        auto res = campaignBattleText(numTries, levelPath, keeperId, villain.enemyId);
        std::cerr << villain.enemyId.data() << " RES " << res << std::endl;
      }
}

int MainLoop::campaignBattleText(int numTries, const FilePath& levelPath, EnemyId keeperId, EnemyId enemyId) {
  auto contentFactory = createContentFactory(false);
  auto minionsTmp = contentFactory.enemies.at(keeperId).settlement.inhabitants;
  auto enemy = contentFactory.enemies.at(enemyId);
  for (int increase : Range(0, 100)) {
    vector<CreatureList> minions = {minionsTmp.leader, minionsTmp.fighters};
    for (auto& elem : minions) {
      //elem.clearExpLevel();
      elem.clearBaseLevel();
      elem.increaseBaseLevel(EnumMap<ExperienceType, int>([increase](ExperienceType t) { return increase; }));
    }
    std::cerr << "Increase " << increase << std::endl;
    int res = battleTest(numTries, levelPath, minions,
        {enemy.settlement.inhabitants.fighters, enemy.settlement.inhabitants.leader});
    if (res >= numTries * 9 / 10)
      return increase;
  }
  return -1;
}

void MainLoop::endlessTest(int numTries, const FilePath& levelPath, const FilePath& battleInfoPath, optional<int> numEnemy) {
  auto allies = readAllies(battleInfoPath);
  auto contentFactory = createContentFactory(false);
  //RandomGen random;
  ExternalEnemies enemies(Random, &contentFactory.getCreatures(), EnemyFactory(Random, contentFactory.getCreatures().getNameGenerator(),
      contentFactory.enemies, contentFactory.buildingInfo, contentFactory.externalEnemies.at("basic"))
      .getExternalEnemies(), ExternalEnemiesType::FROM_START);
  for (int turn : Range(100000))
    if (auto wave = enemies.popNextWave(LocalTime(turn))) {
      std::cerr << "Turn " << turn << ": " << wave->enemy.name << "\n";
      int totalWins = 0;
      for (auto& allyInfo : allies) {
        //std::cerr << allyInfo.getSummary(&contentFactory.getCreatures()) << ": ";
        int numWins = battleTest(numTries, levelPath, {allyInfo}, {wave->enemy.creatures});
        totalWins += numWins;
      }
      std::cerr << totalWins << " wins\n";
      std::cout << "Turn " << turn << ": " << wave->enemy.name << ": " << totalWins << "\n";
    }
}

optional<string> MainLoop::verifyMod(const string& path) {
  /*auto modsPath = userPath.subdirectory("mods_tmp");
  OnExit ex123([modsPath] { modsPath.removeRecursively(); });
  modsPath.createIfDoesntExist();
  if (auto err = unzip(path, modsPath.getPath()))
    return err;
  for (auto mod : modsPath.getSubDirs()) {
    GameConfig config({modsPath.subdirectory(mod)});
    ContentFactory f;
    if (auto err = f.readData(&config, mod))
      return err;
    else {
      std::cout << mod << std::endl;
      return none;
    }
  }*/
  return "Failed to load any mod"_s;
}

int MainLoop::battleTest(int numTries, const FilePath& levelPath, vector<CreatureList> ally, vector<CreatureList> enemies) {
  vector<PCreature> allies;
  auto contentFactory = createContentFactory(false);
  for (auto& elem : ally)
    allies.append(elem.generate(Random, &contentFactory.getCreatures(), TribeId::getDarkKeeper(), MonsterAIFactory::monster()));
  return battleTest(numTries, levelPath, getWeakPointers(allies), enemies);
}

int MainLoop::battleTest(int numTries, const FilePath& levelPath, vector<Creature*> ally, vector<CreatureList> enemies) {
  ProgressMeter meter(1);
  int numAllies = 0;
  int numEnemies = 0;
  int numUnknown = 0;
  auto allyTribe = TribeId::getDarkKeeper();
  for (int i : Range(numTries)) {
    auto contentFactory = createContentFactory(false);
    EnemyFactory enemyFactory(Random, contentFactory.getCreatures().getNameGenerator(),
        contentFactory.enemies, contentFactory.buildingInfo, {});
    auto allyCopy = ally.transform([&](Creature* c) { return contentFactory.getCreatures().makeCopy(c); });
    auto model = ModelBuilder(&meter, Random, options, sokobanInput,
        &contentFactory, std::move(enemyFactory)).battleModel(levelPath, std::move(allyCopy), enemies);
    auto game = Game::splashScreen(std::move(model), CampaignBuilder::getEmptyCampaign(), std::move(contentFactory));
    auto exitCondition = [&](WGame game) -> optional<ExitCondition> {
      unordered_set<TribeId, CustomHash<TribeId>> tribes;
      for (auto& m : game->getAllModels())
        for (auto c : m->getAllCreatures())
          tribes.insert(c->getTribeId());
      if (tribes.size() == 1) {
        if (*tribes.begin() == allyTribe)
          return ExitCondition::ALLIES_WON;
        else
          return ExitCondition::ENEMIES_WON;
      }
      if (game->getGlobalTime().getVisibleInt() > 200)
        return ExitCondition::TIMEOUT;
      if (tribes.empty())
        return ExitCondition::UNKNOWN;
      else
        return none;
    };
    auto result = playGame(std::move(game), false, true, false, exitCondition, milliseconds{3});
    switch (result) {
      case ExitCondition::ALLIES_WON:
        ++numAllies;
        std::cerr << "a";
        break;
      case ExitCondition::ENEMIES_WON:
        ++numEnemies;
        std::cerr << "e";
        break;
      case ExitCondition::TIMEOUT:
        ++numUnknown;
        std::cerr << "t";
        break;
      case ExitCondition::UNKNOWN:
        ++numUnknown;
        std::cerr << "u";
        break;
    }
    std::cerr.flush();
  }
  std::cerr << " " << numAllies << ":" << numEnemies;
  if (numUnknown > 0)
    std::cerr << " (" << numUnknown << ") unknown";
  std::cerr << "\n";
  return numAllies;
}

PModel MainLoop::getBaseModel(ModelBuilder& modelBuilder, CampaignSetup& setup, TribeId tribe,
    TribeAlignment alignment) {
  auto ret = [&] {
    switch (setup.campaign.getType()) {
      case CampaignType::SINGLE_KEEPER:
        return modelBuilder.singleMapModel(tribe, alignment);
      case CampaignType::QUICK_MAP:
        return modelBuilder.tutorialModel();
      default:
        return modelBuilder.campaignBaseModel(tribe, alignment, setup.startingBiome, setup.externalEnemies);
    }
  }();
  return ret;
}

vector<ExternalEnemy> getExternalEnemiesFor(const AvatarInfo& info, const ContentFactory* contentFactory) {
  return info.creatureInfo.visit(
      [&] (const KeeperCreatureInfo& i) {
        vector<ExternalEnemy> ret;
        for (auto& g : i.endlessEnemyGroups)
          ret.append(contentFactory->externalEnemies.at(g));
        return ret;
      },
      [&] (const AdventurerCreatureInfo&) {
        return vector<ExternalEnemy>();
      });
}

ModelTable MainLoop::prepareCampaignModels(CampaignSetup& setup, const AvatarInfo& avatarInfo, RandomGen& random,
    ContentFactory* contentFactory) {
  EnemyFactory enemyFactory(Random, contentFactory->getCreatures().getNameGenerator(), contentFactory->enemies,
      contentFactory->buildingInfo, getExternalEnemiesFor(avatarInfo, contentFactory));
  ModelBuilder modelBuilder(nullptr, random, options, sokobanInput, contentFactory, std::move(enemyFactory));
  return prepareCampaignModels(setup, avatarInfo.tribeAlignment, std::move(modelBuilder));
}

ModelTable MainLoop::prepareCampaignModels(CampaignSetup& setup, TribeAlignment tribeAlignment,
    ModelBuilder modelBuilder) {
  Table<PModel> models(setup.campaign.getSites().getBounds());
  auto& sites = setup.campaign.getSites();
  for (Vec2 v : sites.getBounds())
    if (auto retired = sites[v].getRetired()) {
      if (retired->fileInfo.download)
        downloadGame(retired->fileInfo);
    }
  optional<string> failedToLoad;
  int numSites = setup.campaign.getNumNonEmpty();
  vector<ContentFactory> factories;
  int numRetiredVillains = 0;
  doWithSplash("Generating map...", numSites,
      [&] (ProgressMeter& meter) {
        for (Vec2 v : sites.getBounds()) {
          if (!sites[v].isEmpty())
            meter.addProgress();
          if (auto info = sites[v].getKeeper()) {
            models[v] = getBaseModel(modelBuilder, setup, info->tribe, tribeAlignment);
          } else if (auto villain = sites[v].getVillain()) {
            for (auto& info : getSaveFiles(userPath, getSaveSuffix(GameSaveType::RETIRED_SITE)))
              if (isCompatible(getSaveVersion(info)))
                if (auto saved = loadSavedGameInfo(userPath.file(info.filename)))
                  if (auto& retiredInfo = saved->retiredEnemyInfo)
                    if (retiredInfo->enemyId == villain->enemyId)
                      if (auto model = loadRetiredModelFromFile(userPath.file(info.filename))) {
                        models[v] = std::move(model->model);
                        ++numRetiredVillains;
                        remove(userPath.file(info.filename).getPath());
                        break;
                      }
            if (!models[v])
              models[v] = modelBuilder.campaignSiteModel(villain->enemyId, villain->type, tribeAlignment);
          } else if (auto retired = sites[v].getRetired()) {
            if (auto info = loadRetiredModelFromFile(userPath.file(retired->fileInfo.filename))) {
              models[v] = std::move(info->model);
              factories.push_back(std::move(info->factory));
            } else {
              failedToLoad = retired->fileInfo.filename;
              setup.campaign.clearSite(v);
            }
          }
        }
      });
  if (failedToLoad)
    view->presentText("Sorry", "Error reading " + *failedToLoad + ". Leaving blank site.");
  return ModelTable{std::move(models), std::move(factories), numRetiredVillains};
}

PGame MainLoop::loadGame(const FilePath& file) {
  optional<PGame> game;
  if (auto info = loadSavedGameInfo(file))
    doWithSplash("Loading "_s + file.getPath() + "...", info->progressCount,
        [&] (ProgressMeter& meter) {
          Square::progressMeter = &meter;
          INFO << "Loading from " << file;
          MEASURE(game = loadFromFile<PGame>(file), "Loading game");
    });
  Square::progressMeter = nullptr;
  return game ? std::move(*game) : nullptr;
}

bool MainLoop::downloadGame(const SaveFileInfo& file) {
  atomic<bool> cancelled(false);
  optional<string> error;
  doWithSplash("Downloading " + file.filename + "...", 1,
      [&] (ProgressMeter& meter) {
        error = fileSharing->downloadSite(file, userPath, meter);
      },
      [&] {
        cancelled = true;
        fileSharing->cancel();
      });
  if (error && !cancelled)
    view->presentText("Error downloading file", *error);
  return !error;
}

static void changeSaveType(const FilePath& file, GameSaveType newType) {
  optional<FilePath> newFile;
  for (GameSaveType oldType : ENUM_ALL(GameSaveType)) {
    string suf = getSaveSuffix(oldType);
    if (file.hasSuffix(suf)) {
      if (oldType == newType)
        return;
      newFile = file.changeSuffix(suf, getSaveSuffix(newType));
      break;
    }
  }
  CHECK(!!newFile);
  remove(newFile->getPath());
  rename(file.getPath(), newFile->getPath());
}

PGame MainLoop::loadPrevious() {
  vector<ListElem> options;
  vector<SaveFileInfo> files;
  getSaveOptions({
      {GameSaveType::AUTOSAVE, "Recovered games:"},
      {GameSaveType::KEEPER, "Keeper games:"},
      {GameSaveType::ADVENTURER, "Adventurer games:"}}, options, files);
  optional<SaveFileInfo> savedGame = chooseSaveFile(options, files, "No saved games found.", view);
  if (savedGame) {
    PGame ret = loadGame(userPath.file(savedGame->filename));
    if (ret) {
      if (eraseSave())
        changeSaveType(userPath.file(savedGame->filename), GameSaveType::AUTOSAVE);
    } else
      view->presentText("Sorry", "Failed to load the save file :(");
    return ret;
  } else
    return nullptr;
}

bool MainLoop::eraseSave() {
#ifdef RELEASE
  return !options->getBoolValue(OptionId::KEEP_SAVEFILES);
#endif
  return false;
}

void MainLoop::registerModPlaytime(bool started) {
#ifdef USE_STEAMWORKS
  if (!steam::Client::isAvailable())
    return;

  string currentMod = options->getStringValue(OptionId::CURRENT_MOD2);
  if (auto localVer = getLocalModVersionInfo(currentMod)) {
    steam::ItemId itemId(localVer->steamId);
    auto& ugc = steam::UGC::instance();
    if (started)
      ugc.startPlaytimeTracking({itemId});
    else
      ugc.stopPlaytimeTracking({itemId});
  }
#endif
}
