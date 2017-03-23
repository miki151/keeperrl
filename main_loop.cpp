#include "stdafx.h"
#include "main_loop.h"
#include "view.h"
#include "highscores.h"
#include "music.h"
#include "options.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "dirent.h"
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

MainLoop::MainLoop(View* v, Highscores* h, FileSharing* fSharing, const string& freePath,
    const string& uPath, Options* o, Jukebox* j, SokobanInput* soko, std::atomic<bool>& fin, bool singleThread,
    optional<ForceGameInfo> force)
      : view(v), dataFreePath(freePath), userPath(uPath), options(o), jukebox(j),
        highscores(h), fileSharing(fSharing), finished(fin), useSingleThread(singleThread), forceGame(force),
        sokobanInput(soko) {
}

vector<SaveFileInfo> MainLoop::getSaveFiles(const string& path, const string& suffix) {
  vector<SaveFileInfo> ret;
  DIR* dir = opendir(path.c_str());
  CHECK(dir) << "Couldn't open " + path;
  while (dirent* ent = readdir(dir)) {
    string name(path + "/" + ent->d_name);
    if (endsWith(name, suffix)) {
      struct stat buf;
      stat(name.c_str(), &buf);
      ret.push_back({ent->d_name, buf.st_mtime, false});
    }
  }
  closedir(dir);
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

static const int saveVersion = 1400;

static bool isCompatible(int loadedVersion) {
  return loadedVersion > 2 && loadedVersion <= saveVersion && loadedVersion / 100 == saveVersion / 100;
}

static string getSaveSuffix(GameSaveType t) {
  switch (t) {
    case GameSaveType::KEEPER: return ".kep";
    case GameSaveType::ADVENTURER: return ".adv";
    case GameSaveType::RETIRED_SITE: return ".sit";
    case GameSaveType::RETIRED_CAMPAIGN: return ".cam";
    case GameSaveType::AUTOSAVE: return ".aut";
  }
}

template <typename InputType, typename T>
static T loadGameUsing(const string& filename) {
  T obj;
  try {
    InputType input(filename.c_str());
    string discard;
    SavedGameInfo discard2;
    int version;
    input.getArchive() >>BOOST_SERIALIZATION_NVP(version) >> BOOST_SERIALIZATION_NVP(discard)
      >> BOOST_SERIALIZATION_NVP(discard2);
    Serialization::registerTypes(input.getArchive(), version);
    input.getArchive() >> BOOST_SERIALIZATION_NVP(obj);
  } catch (boost::archive::archive_exception& ex) {
      return T();
  }
  return obj;
}

static PGame loadGameFromFile(const string& filename) {
  if (auto game = loadGameUsing<CompressedInput, PGame>(filename))
    return game;
  // Try alternative format that doesn't crash on OSX.
  return loadGameUsing<CompressedInput2, PGame>(filename); 
}

static PModel loadModelFromFile(const string& filename) {
  if (auto model = loadGameUsing<CompressedInput, PModel>(filename))
    return model;
  // Try alternative format that doesn't crash on OSX.
  return loadGameUsing<CompressedInput2, PModel>(filename); 
}

bool isNotFilename(char c) {
  return !(tolower(c) >= 'a' && tolower(c) <= 'z') && !isdigit(c) && c != '_';
}

static string stripFilename(string s) {
  s.erase(remove_if(s.begin(),s.end(), isNotFilename), s.end());
  return s;
}

static void saveGame(PGame& game, const string& path) {
  CompressedOutput out(path.c_str());
  string name = game->getGameDisplayName();
  SavedGameInfo savedInfo = game->getSavedGameInfo();
  out.getArchive() << BOOST_SERIALIZATION_NVP(saveVersion) << BOOST_SERIALIZATION_NVP(name)
      << BOOST_SERIALIZATION_NVP(savedInfo);
  Serialization::registerTypes(out.getArchive(), saveVersion);
  out.getArchive() << BOOST_SERIALIZATION_NVP(game);
}

static void saveMainModel(PGame& game, const string& path) {
  CompressedOutput out(path.c_str());
  string name = game->getGameDisplayName();
  SavedGameInfo savedInfo = game->getSavedGameInfo();
  out.getArchive() << BOOST_SERIALIZATION_NVP(saveVersion) << BOOST_SERIALIZATION_NVP(name)
      << BOOST_SERIALIZATION_NVP(savedInfo);
  Serialization::registerTypes(out.getArchive(), saveVersion);
  out.getArchive() << BOOST_SERIALIZATION_NVP(game->getMainModel());
}

int MainLoop::getSaveVersion(const SaveFileInfo& save) {
  if (auto info = getNameAndVersion(userPath + "/" + save.filename))
    return info->second;
  else
    return -1;
}

void MainLoop::uploadFile(const string& path, GameSaveType type) {
  atomic<bool> cancelled(false);
  optional<string> error;
  doWithSplash(SplashType::BIG, "Uploading " + path + "...", 1,
      [&] (ProgressMeter& meter) {
        error = fileSharing->uploadSite(path, meter);
      },
      [&] {
        cancelled = true;
        fileSharing->cancel();
      });
  if (error && !cancelled)
    view->presentText("Error uploading file", *error);
}

string MainLoop::getSavePath(const PGame& game, GameSaveType gameType) {
  return userPath + "/" + stripFilename(game->getGameIdentifier()) + getSaveSuffix(gameType);
}

const int singleModelGameSaveTime = 100000;

void MainLoop::saveUI(PGame& game, GameSaveType type, SplashType splashType) {
  string path = getSavePath(game, type);
  if (type == GameSaveType::RETIRED_SITE) {
    int saveTime = game->getMainModel()->getSaveProgressCount();
    doWithSplash(splashType, "Retiring site...", saveTime,
        [&] (ProgressMeter& meter) {
        Square::progressMeter = &meter;
        MEASURE(saveMainModel(game, path), "saving time")});
  } else {
    int saveTime = game->getSaveProgressCount();
    doWithSplash(splashType, "Saving game...", saveTime,
        [&] (ProgressMeter& meter) {
        Square::progressMeter = &meter;
        MEASURE(saveGame(game, path), "saving time")});
  }
  Square::progressMeter = nullptr;
  if (GameSaveType::RETIRED_SITE == type)
    uploadFile(path, type);
}

void MainLoop::eraseSaveFile(const PGame& game, GameSaveType type) {
  remove(getSavePath(game, type).c_str());
}

void MainLoop::getSaveOptions(const vector<pair<GameSaveType, string>>& games, vector<ListElem>& options,
    vector<SaveFileInfo>& allFiles) {
  for (auto elem : games) {
    vector<SaveFileInfo> files = getSaveFiles(userPath, getSaveSuffix(elem.first));
    files = ::filter(files, [this] (const SaveFileInfo& info) { return isCompatible(getSaveVersion(info));});
    append(allFiles, files);
    if (!files.empty()) {
      options.emplace_back(elem.second, ListElem::TITLE);
      append(options, transform2(files,
          [this] (const SaveFileInfo& info) {
              auto nameAndVersion = getNameAndVersion(userPath + "/" + info.filename);
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

int MainLoop::getAutosaveFreq() {
  return 1500;
}

void MainLoop::playGame(PGame&& game, bool withMusic, bool noAutoSave) {
  view->reset();
  game->initialize(options, highscores, view, fileSharing);
  const milliseconds stepTimeMilli {3};
  Intervalometer meter(stepTimeMilli);
  double lastMusicUpdate = -1000;
  double lastAutoSave = game->getGlobalTime();
  while (1) {
    double step = 1;
    if (!game->isTurnBased()) {
      double gameTimeStep = view->getGameSpeed() / stepTimeMilli.count();
      auto timeMilli = view->getTimeMilli();
      double count = meter.getCount(timeMilli);
      //INFO << "Intervalometer " << timeMilli << " " << count;
      step = min(1.0, double(count) * gameTimeStep);
    }
    INFO << "Time step " << step;
    if (auto exitInfo = game->update(step)) {
      apply_visitor(*exitInfo, makeVisitor<void>(
          [&](GameSaveType type) {
            if (type == GameSaveType::RETIRED_SITE) {
              game->prepareSiteRetirement();
              saveUI(game, type, SplashType::BIG);
              game->doneRetirement();
            } else
              saveUI(game, type, SplashType::BIG);
            eraseAllSavesExcept(game, type);
          },
          [&](ExitAndQuit) {
            eraseAllSavesExcept(game, none);
          }
      ));
      return;
    }
    double gameTime = game->getGlobalTime();
    if (lastMusicUpdate < gameTime - 1 && withMusic) {
      jukebox->setType(game->getCurrentMusic(), game->changeMusicNow());
      lastMusicUpdate = gameTime;
    }
    if (lastAutoSave < gameTime - getAutosaveFreq() && !noAutoSave) {
      if (options->getBoolValue(OptionId::AUTOSAVE)) {
        saveUI(game, GameSaveType::AUTOSAVE, SplashType::AUTOSAVING);
        eraseAllSavesExcept(game, GameSaveType::AUTOSAVE);
      }
      lastAutoSave = gameTime;
    }
    if (useSingleThread)
      view->refreshView();
  }
}

void MainLoop::eraseAllSavesExcept(const PGame& game, optional<GameSaveType> except) {
  for (auto erasedType : ENUM_ALL(GameSaveType))
    if (erasedType != except)
      eraseSaveFile(game, erasedType);
}

optional<RetiredGames> MainLoop::getRetiredGames(CampaignType type) {
  switch (type) {
    case CampaignType::FREE_PLAY: {
      RetiredGames ret;
      for (auto& info : getSaveFiles(userPath, getSaveSuffix(GameSaveType::RETIRED_SITE)))
        if (isCompatible(getSaveVersion(info)))
          if (auto saved = getSavedGameInfo(userPath + "/" + info.filename))
            ret.addLocal(*saved, info);
      optional<vector<FileSharing::SiteInfo>> onlineSites;
      doWithSplash(SplashType::SMALL, "Fetching list of retired dungeons from the server...",
          [&] { onlineSites = fileSharing->listSites(); }, [&] { fileSharing->cancel(); });
      if (onlineSites) {
        for (auto& elem : *onlineSites)
          if (isCompatible(elem.version))
            ret.addOnline(elem.gameInfo, elem.fileInfo, elem.totalGames, elem.wonGames);
      } else
        view->presentText("", "Failed to fetch list of retired dungeons from the server.");
      ret.sort();
      return ret;
    }
    case CampaignType::CAMPAIGN: {
      RetiredGames ret;
      for (auto& info : getSaveFiles(userPath, getSaveSuffix(GameSaveType::RETIRED_CAMPAIGN)))
        if (isCompatible(getSaveVersion(info)))
          if (auto saved = getSavedGameInfo(userPath + "/" + info.filename))
            ret.addLocal(*saved, info);
      for (int i : All(ret.getAllGames()))
        ret.setActive(i, true);
      return ret;
    }
    default:
      return none;
  }
}

PGame MainLoop::prepareCampaign(RandomGen& random, const optional<ForceGameInfo>& forceGameInfo) {
  if (forceGameInfo) {
    CampaignBuilder builder(view, random, options, forceGameInfo->role);
    auto result = builder.prepareCampaign(bindMethod(&MainLoop::getRetiredGames, this), forceGameInfo->type);
    return Game::campaignGame(prepareCampaignModels(*result, random), *result);
  }
  auto choice = PlayerRoleChoice(PlayerRole::KEEPER);
  while (1) {
    choice = view->getPlayerRoleChoice(choice);
    if (auto ret = apply_visitor(choice, makeVisitor<optional<PGame>>(
        [&](PlayerRole role) -> optional<PGame> {
          CampaignBuilder builder(view, random, options, role);
          if (auto result = builder.prepareCampaign(bindMethod(&MainLoop::getRetiredGames, this), CampaignType::CAMPAIGN)) {
            return Game::campaignGame(prepareCampaignModels(*result, random), *result);
          } else
            return none;
        },
        [&](LoadGameChoice&) -> optional<PGame> {
          if (auto game = loadPrevious())
            return std::move(game);
          else
            return none;
        },
        [&](GoBackChoice&) -> optional<PGame> {
          return PGame(nullptr);
        }
      )))
      return std::move(*ret);
  }
}

void MainLoop::splashScreen() {
  ProgressMeter meter(1);
  jukebox->setType(MusicType::INTRO, true);
  playGame(Game::splashScreen(ModelBuilder(&meter, Random, options, sokobanInput)
        .splashModel(dataFreePath + "/splash.txt"), CampaignBuilder::getEmptyCampaign()), false, true);
}

void MainLoop::showCredits(const string& path, View* view) {
  ifstream in(path);
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

void MainLoop::start(bool tilesPresent) {
  if (options->getBoolValue(OptionId::MUSIC))
    jukebox->toggle(true);
  NameGenerator::init(dataFreePath + "/names");
  if (!forceGame)
    splashScreen();
  view->reset();
  considerFreeVersionText(tilesPresent);
  considerGameEventsPrompt();
  int lastIndex = 0;
  while (1) {
    playMenuMusic();
    optional<int> choice;
    if (forceGame)
      choice = 1;
    else
      choice = view->chooseFromList("", {
        "Tutorial", "Play", "Settings", "High scores", "Credits", "Quit"}, lastIndex, MenuType::MAIN);
    if (!choice)
      continue;
    lastIndex = *choice;
    switch (*choice) {
      case 0:
        if (PGame game = prepareCampaign(Random, ForceGameInfo { PlayerRole::KEEPER, CampaignType::KEEPER_TUTORIAL }))
          playGame(std::move(game), true, false);
        view->reset();
        break;
      case 1:
        if (PGame game = prepareCampaign(Random, forceGame))
          playGame(std::move(game), true, false);
        view->reset();
        forceGame = none;
        break;
      case 2: options->handle(view, OptionSet::GENERAL); break;
      case 3: highscores->present(view); break;
      case 4: showCredits(dataFreePath + "/credits.txt", view); break;
      case 5: finished = true; break;
    }
    if (finished)
      break;
  }
}

#ifdef OSX // see thread comment in stdafx.h
static thread::attributes getAttributes() {
  thread::attributes attr;
  attr.set_stack_size(4096 * 4000);
  return attr;
}

static thread makeThread(function<void()> fun) {
  return thread(getAttributes(), fun);
}

#else

static thread makeThread(function<void()> fun) {
  return thread(fun);
}

#endif

void MainLoop::doWithSplash(SplashType type, const string& text, int totalProgress,
    function<void(ProgressMeter&)> fun, function<void()> cancelFun) {
  ProgressMeter meter(1.0 / totalProgress);
  view->displaySplash(&meter, text, type, cancelFun);
  if (useSingleThread) {
    // A bit confusing, but the flag refers to using a single thread for rendering and gameplay.
    // This forces us to build the world on an extra thread to be able to display a progress bar.
    thread t = makeThread([fun, &meter, this] {
      try {
        fun(meter);
        view->clearSplash();
      } catch (Progress::InterruptedException) {}
    });
    try {
      view->refreshView();
      t.join();
    } catch (GameExitException e) {
      Progress::interrupt();
      t.join();
      throw e;
    }
  } else {
    fun(meter);
    view->clearSplash();
  }
}

void MainLoop::doWithSplash(SplashType type, const string& text, function<void()> fun, function<void()> cancelFun) {
  view->displaySplash(nullptr, text, type, cancelFun);
  if (useSingleThread) {
    // A bit confusing, but the flag refers to using a single thread for rendering and gameplay.
    // This forces us to build the world on an extra thread to be able to display a progress bar.
    thread t = makeThread([fun, this] { fun(); view->clearSplash(); });
    view->refreshView();
    t.join();
  } else {
    fun();
    view->clearSplash();
  }
}

PModel MainLoop::quickGame(RandomGen& random) {
  PModel model;
  NameGenerator::init(dataFreePath + "/names");
  doWithSplash(SplashType::BIG, "Generating map...", 166000,
      [&] (ProgressMeter& meter) {
        model = ModelBuilder(&meter, random, options, sokobanInput).quickModel();
      });
  return model;
}

void MainLoop::modelGenTest(int numTries, RandomGen& random, Options* options) {
  NameGenerator::init(dataFreePath + "/names");
  ProgressMeter meter(1);
  ModelBuilder(&meter, random, options, sokobanInput).measureSiteGen(numTries);
}

PModel MainLoop::getBaseModel(ModelBuilder& modelBuilder, CampaignSetup& setup) {
  switch (setup.campaign.getType()) {
    case CampaignType::SINGLE_KEEPER:
      return modelBuilder.singleMapModel(setup.campaign.getWorldName(), std::move(setup.player));
    default: {
      auto ret = modelBuilder.campaignBaseModel("Campaign base site",
          setup.campaign.getType() == CampaignType::ENDLESS);
      STutorial tutorial;
      if (setup.campaign.getType() == CampaignType::KEEPER_TUTORIAL)
        tutorial = make_shared<Tutorial>();
      modelBuilder.spawnKeeper(ret.get(), std::move(setup.player), tutorial);
      return ret;
    }
  }
}

Table<PModel> MainLoop::prepareCampaignModels(CampaignSetup& setup, RandomGen& random) {
  Table<PModel> models(setup.campaign.getSites().getBounds());
  auto& sites = setup.campaign.getSites();
  for (Vec2 v : sites.getBounds())
    if (auto retired = sites[v].getRetired()) {
      if (retired->fileInfo.download)
        downloadGame(retired->fileInfo.filename);
    }
  optional<string> failedToLoad;
  NameGenerator::init(dataFreePath + "/names");
  int numSites = setup.campaign.getNumNonEmpty();
  doWithSplash(SplashType::BIG, "Generating map...", numSites,
      [&] (ProgressMeter& meter) {
        ModelBuilder modelBuilder(nullptr, random, options, sokobanInput);
        for (Vec2 v : sites.getBounds()) {
          if (!sites[v].isEmpty())
            meter.addProgress();
          if (sites[v].getKeeper()) {
            models[v] = getBaseModel(modelBuilder, setup);
          } else if (auto villain = sites[v].getVillain())
            models[v] = modelBuilder.campaignSiteModel("Campaign enemy site", villain->enemyId, villain->type);
          else if (auto retired = sites[v].getRetired()) {
            if (PModel m = loadModelFromFile(userPath + "/" + retired->fileInfo.filename))
              models[v] = std::move(m);
            else {
              failedToLoad = retired->fileInfo.filename;
              setup.campaign.clearSite(v);
            }
          }
        }
      });
  if (failedToLoad)
    view->presentText("Sorry", "Error reading " + *failedToLoad + ". Leaving blank site.");
  return models;
}

PModel MainLoop::keeperSingleMap(RandomGen& random) {
  PModel model;
  NameGenerator::init(dataFreePath + "/names");
  doWithSplash(SplashType::BIG, "Generating map...", 300000,
      [&] (ProgressMeter& meter) {
        ModelBuilder modelBuilder(&meter, random, options, sokobanInput);
        PCreature player = CreatureFactory::fromId(CreatureId::KEEPER, TribeId::getKeeper());
        model = modelBuilder.singleMapModel(NameGenerator::get(NameGeneratorId::WORLD)->getNext(), std::move(player));
      });
  return model;
}

PGame MainLoop::loadGame(string file) {
  SavedGameInfo info = *getSavedGameInfo(userPath + "/" + file);
  PGame game;
  doWithSplash(SplashType::BIG, "Loading " + file + "...", info.getProgressCount(),
      [&] (ProgressMeter& meter) {
        Square::progressMeter = &meter;
        INFO << "Loading from " << file;
        game = loadGameFromFile(userPath + "/" + file);});
  if (!game)
    view->presentText("Sorry", "This save file is corrupted :(");
  Square::progressMeter = nullptr;
  return game;
}

bool MainLoop::downloadGame(const string& filename) {
  atomic<bool> cancelled(false);
  optional<string> error;
  doWithSplash(SplashType::BIG, "Downloading " + filename + "...", 1,
      [&] (ProgressMeter& meter) {
        error = fileSharing->download(filename, userPath, meter);
      },
      [&] {
        cancelled = true;
        fileSharing->cancel();
      });
  if (error && !cancelled)
    view->presentText("Error downloading file", *error);
  return !error;
}

static void changeSaveType(const string& file, GameSaveType newType) {
  string newFile;
  for (GameSaveType oldType : ENUM_ALL(GameSaveType)) {
    string suf = getSaveSuffix(oldType);
    if (file.substr(file.size() - suf.size()) == suf) {
      if (oldType == newType)
        return;
      newFile = file.substr(0, file.size() - suf.size()) + getSaveSuffix(newType);
      break;
    }
  }
  CHECK(!newFile.empty());
  remove(newFile.c_str());
  rename(file.c_str(), newFile.c_str());
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
    PGame ret = loadGame(savedGame->filename);
    if (eraseSave())
      changeSaveType(savedGame->filename, GameSaveType::AUTOSAVE);
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

