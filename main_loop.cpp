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

MainLoop::MainLoop(View* v, Highscores* h, FileSharing* fSharing, const string& freePath,
    const string& uPath, Options* o, Jukebox* j, std::atomic<bool>& fin, bool singleThread,
    optional<GameTypeChoice> force)
      : view(v), dataFreePath(freePath), userPath(uPath), options(o), jukebox(j),
        highscores(h), fileSharing(fSharing), finished(fin), useSingleThread(singleThread), forceGame(force) {
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

static const int saveVersion = 1100;

static bool isCompatible(int loadedVersion) {
  return loadedVersion > 2 && loadedVersion <= saveVersion && loadedVersion / 100 == saveVersion / 100;
}

static string getSaveSuffix(GameSaveType t) {
  switch (t) {
    case GameSaveType::KEEPER: return ".kep";
    case GameSaveType::ADVENTURER: return ".adv";
    case GameSaveType::RETIRED_SINGLE: return ".ret";
    case GameSaveType::RETIRED_SITE: return ".sit";
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
        if (type == GameSaveType::RETIRED_SINGLE)
          error = fileSharing->uploadRetired(path, meter);
        else if (type == GameSaveType::RETIRED_SITE)
          error = fileSharing->uploadSite(path, meter);
      },
      [&] {
        cancelled = true;
        fileSharing->cancel();
      });
  if (error && !cancelled)
    view->presentText("Error uploading file", *error);
}

string MainLoop::getSavePath(PGame& game, GameSaveType gameType) {
  return userPath + "/" + stripFilename(game->getGameIdentifier()) + getSaveSuffix(gameType);
}

const int singleModelGameSaveTime = 100000;

void MainLoop::saveUI(PGame& game, GameSaveType type, SplashType splashType) {
  string path = getSavePath(game, type);
  int saveTime = 0;
  if (game->isSingleModel() || type == GameSaveType::RETIRED_SITE)
    saveTime = singleModelGameSaveTime;
  else {
    auto savedGameInfo = game->getSavedGameInfo();
    saveTime = savedGameInfo.getNumSites();
  }
  if (type == GameSaveType::RETIRED_SITE)
    doWithSplash(splashType, "Retiring site...", saveTime,
        [&] (ProgressMeter& meter) {
        Square::progressMeter = &meter;
        MEASURE(saveMainModel(game, path), "saving time")});
  else
    doWithSplash(splashType, "Saving game...", saveTime,
        [&] (ProgressMeter& meter) {
        if (game->isSingleModel())
          Square::progressMeter = &meter;
        else
          Model::progressMeter = &meter;
        MEASURE(saveGame(game, path), "saving time")});
  Square::progressMeter = nullptr;
  Model::progressMeter = nullptr;
  if (contains({GameSaveType::RETIRED_SINGLE, GameSaveType::RETIRED_SITE}, type))
    uploadFile(path, type);
}

void MainLoop::eraseSaveFile(PGame& game, GameSaveType type) {
  remove(getSavePath(game, type).c_str());
}

static string getGameDesc(const FileSharing::GameInfo& game) {
  if (game.totalGames > 0)
    return getPlural("daredevil", game.totalGames) + " " + toString(game.totalGames - game.wonGames) + " killed";
  else
    return "";
}

void MainLoop::getSaveOptions(const vector<FileSharing::GameInfo>& onlineGames,
    const vector<pair<GameSaveType, string>>& games, vector<ListElem>& options, vector<SaveFileInfo>& allFiles) {
  for (auto elem : games) {
    vector<SaveFileInfo> files = getSaveFiles(userPath, getSaveSuffix(elem.first));
    files = ::filter(files, [this] (const SaveFileInfo& info) { return isCompatible(getSaveVersion(info));});
    if (!onlineGames.empty())
      sort(files.begin(), files.end(), [&](const SaveFileInfo& f1, const SaveFileInfo& f2) {
            int played1 = 0, played2 = 0;
            for (auto& elem : onlineGames)
              if (elem.filename == f1.filename)
                played1 = elem.totalGames;
              else if (elem.filename == f2.filename)
                played2 = elem.totalGames;
            return played1 > played2;
          });
    append(allFiles, files);
    if (!files.empty()) {
      options.emplace_back(elem.second, ListElem::TITLE);
      append(options, transform2<ListElem>(files,
            [this, &onlineGames] (const SaveFileInfo& info) {
              auto nameAndVersion = getNameAndVersion(userPath + "/" + info.filename);
              for (auto& elem : onlineGames)
                if (elem.filename == info.filename)
                  return ListElem(nameAndVersion->first, getGameDesc(elem));
              return ListElem(nameAndVersion->first, getDateString(info.date));}));
    }
  }
}

void MainLoop::getDownloadOptions(const vector<FileSharing::GameInfo>& games,
    vector<ListElem>& options, vector<SaveFileInfo>& allFiles, const string& title) {
  options.emplace_back(title, ListElem::TITLE);
  for (FileSharing::GameInfo info : games)
    if (isCompatible(info.version)) {
      bool dup = false;
      for (auto& elem : allFiles)
        if (elem.filename == info.filename) {
          dup = true;
          break;
        }
      if (dup)
        continue;
      options.emplace_back(info.displayName, getGameDesc(info));
      allFiles.push_back({info.filename, info.time, true});
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
  const double stepTimeMilli = 3;
  Intervalometer meter(stepTimeMilli);
  double lastMusicUpdate = -1000;
  double lastAutoSave = game->getGlobalTime();
  while (1) {
    double step = 1;
    if (!game->isTurnBased()) {
      double gameTimeStep = view->getGameSpeed() / stepTimeMilli;
      long long timeMilli = view->getTimeMilli();
      double count = meter.getCount(timeMilli);
      //Debug() << "Intervalometer " << timeMilli << " " << count;
      step = min(1.0, double(count) * gameTimeStep);
    }
    Debug() << "Time step " << step;
    if (auto exitInfo = game->update(step)) {
      if (exitInfo->getId() == Game::ExitId::QUIT && eraseSave()) {
        eraseSaveFile(game, GameSaveType::KEEPER);
        eraseSaveFile(game, GameSaveType::ADVENTURER);
        eraseSaveFile(game, GameSaveType::AUTOSAVE);
      }
      if (exitInfo->getId() == Game::ExitId::SAVE) {
        bool retired = false;
        if (exitInfo->get<GameSaveType>() == GameSaveType::RETIRED_SITE) {
          game->prepareSiteRetirement();
          retired = true;
        }
        if (exitInfo->get<GameSaveType>() == GameSaveType::RETIRED_SINGLE) {
          game->prepareSingleMapRetirement();
          retired = true;
        }
        saveUI(game, exitInfo->get<GameSaveType>(), SplashType::BIG);
        if (retired)
          game->doneRetirement();
      }
      return;
    }
    double gameTime = game->getGlobalTime();
    if (lastMusicUpdate < gameTime - 1 && withMusic) {
      jukebox->setType(game->getCurrentMusic(), game->changeMusicNow());
      lastMusicUpdate = gameTime;
    }
    if (lastAutoSave < gameTime - getAutosaveFreq() && !noAutoSave) {
      if (options->getBoolValue(OptionId::AUTOSAVE))
        saveUI(game, GameSaveType::AUTOSAVE, SplashType::AUTOSAVING);
      lastAutoSave = gameTime;
    }
    if (useSingleThread)
      view->refreshView();
  }
}

RetiredGames MainLoop::getRetiredGames() {
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

PGame MainLoop::prepareCampaign(RandomGen& random) {
  if (auto choice = view->chooseGameType()) {
    random.init(Random.get(1234567));
    switch (*choice) {
      case GameTypeChoice::KEEPER:
        if (auto campaign = Campaign::prepareCampaign(view, options, getRetiredGames(), random, Campaign::KEEPER))
          return Game::campaignGame(prepareCampaignModels(*campaign, random), *campaign->getPlayerPos(),
            options->getStringValue(OptionId::KEEPER_NAME), *campaign);
        break;
      case GameTypeChoice::ADVENTURER:
        if (auto campaign = Campaign::prepareCampaign(view, options, getRetiredGames(), random,
              Campaign::ADVENTURER)) {
          PGame ret = Game::campaignGame(prepareCampaignModels(*campaign, random), *campaign->getPlayerPos(),
              options->getStringValue(OptionId::ADVENTURER_NAME), *campaign);
          ret->getMainModel()->landHeroPlayer(options->getStringValue(OptionId::ADVENTURER_NAME), 0);
          return ret;
        }
        break;
      default: FAIL << "Bad campaign mode";
    }
  }
  return nullptr;
}

PGame MainLoop::prepareSingleMap(RandomGen& random) {
  optional<GameTypeChoice> choice;
  if (forceGame)
    choice = *forceGame;
  else
    choice = view->chooseGameType();
  if (!choice)
    return nullptr;
  switch (*choice) {
    case GameTypeChoice::KEEPER:
      options->setDefaultString(OptionId::KEEPER_NAME, NameGenerator::get(NameGeneratorId::FIRST)->getNext());
      options->setDefaultString(OptionId::KEEPER_SEED, NameGenerator::get(NameGeneratorId::SCROLL)->getNext());
      if (forceGame || options->handleOrExit(view, OptionSet::KEEPER, -1)) {
        string seed = options->getStringValue(OptionId::KEEPER_SEED);
        random.init(hash<string>()(seed));
        ofstream(userPath + "/seeds.txt", std::fstream::out | std::fstream::app) << seed << std::endl;
        return Game::singleMapGame(NameGenerator::get(NameGeneratorId::WORLD)->getNext(),
            options->getStringValue(OptionId::KEEPER_NAME) ,keeperSingleMap(random));
      }
      break;
    case GameTypeChoice::QUICK_LEVEL:
      random = Random;
      return Game::singleMapGame("", "", quickGame(random));
    case GameTypeChoice::ADVENTURER:
      options->setDefaultString(OptionId::ADVENTURER_NAME,
          NameGenerator::get(NameGeneratorId::FIRST)->getNext());
      if (options->handleOrExit(view, OptionSet::ADVENTURER, -1))
        return adventurerGame();
  }
  return nullptr;
}

void MainLoop::playGameChoice() {
  while (1) {
    playMenuMusic();
    PGame game;
    RandomGen random;
    optional<int> choice = view->chooseFromList("", {
        "Campaign", "Single map", "Load game", "Go back"}, 0, MenuType::MAIN);
    if (!choice)
      return;
    switch (*choice) {
      case 0: game = prepareCampaign(random); break;
      case 1: game = prepareSingleMap(random); break;
      case 2: game = loadPrevious(); break;
      case 3: return;
    }
    if (forceGame != GameTypeChoice::QUICK_LEVEL)
      forceGame.reset();
    if (game) {
      Random = std::move(random);
      playGame(std::move(game), true, false);
    }
    view->reset();
  }
}

void MainLoop::splashScreen() {
  ProgressMeter meter(1);
  jukebox->setType(MusicType::INTRO, true);
  playGame(Game::splashScreen(ModelBuilder(&meter, Random, options)
        .splashModel(dataFreePath + "/splash.txt")), false, true);
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

void MainLoop::start(bool tilesPresent) {
  if (options->getBoolValue(OptionId::MUSIC))
    jukebox->toggle(true);
  NameGenerator::init(dataFreePath + "/names");
  if (!forceGame)
    splashScreen();
  view->reset();
  if (!tilesPresent)
    view->presentText("", "You are playing a version of KeeperRL without graphical tiles. "
        "Besides lack of graphics and music, this "
        "is the same exact game as the full version. If you'd like to buy the full version, "
        "please visit keeperrl.com.\n \nYou can also get it by donating to any wildlife charity. "
        "More information on the website.");
  if (options->getIntValue(OptionId::GAME_EVENTS) == 1) {
    if (view->yesOrNoPrompt("The imps would like to gather statistics while you're playing the game and send them anonymously to the developer. This would be very helpful in designing the game. Do you agree?"))
      options->setValue(OptionId::GAME_EVENTS, 2);
    else
      options->setValue(OptionId::GAME_EVENTS, 0);
  }
  int lastIndex = 0;
  while (1) {
    playMenuMusic();
    optional<int> choice;
    if (forceGame)
      choice = 0;
    else
      choice = view->chooseFromList("", {
        "Play game", "Change settings", "View high scores", "View credits", "Quit"}, lastIndex, MenuType::MAIN);
    if (!choice)
      continue;
    lastIndex = *choice;
    switch (*choice) {
      case 0: playGameChoice(); break;
      case 1: options->handle(view, OptionSet::GENERAL); break;
      case 2: highscores->present(view); break;
      case 3: showCredits(dataFreePath + "/credits.txt", view); break;
      case 4: finished = true; break;
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
    thread t = makeThread([fun, &meter, this] { fun(meter); view->clearSplash(); });
    view->refreshView();
    t.join();
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
        model = ModelBuilder(&meter, random, options).quickModel();
      });
  return model;
}

void MainLoop::modelGenTest(int numTries, RandomGen& random, Options* options) {
  NameGenerator::init(dataFreePath + "/names");
  ProgressMeter meter(1);
  ModelBuilder(&meter, random, options).measureSiteGen(numTries);
}

Table<PModel> MainLoop::prepareCampaignModels(Campaign& campaign, RandomGen& random) {
  Table<PModel> models(campaign.getSites().getBounds());
  auto& sites = campaign.getSites();
  for (Vec2 v : sites.getBounds())
    if (auto retired = sites[v].getRetired()) {
      if (retired->fileInfo.download)
        downloadGame(retired->fileInfo.filename);
    }
  optional<string> failedToLoad;
  NameGenerator::init(dataFreePath + "/names");
  int numSites = campaign.getNumNonEmpty();
  doWithSplash(SplashType::BIG, "Generating map...", numSites,
      [&] (ProgressMeter& meter) {
        ModelBuilder modelBuilder(nullptr, random, options);
        for (Vec2 v : sites.getBounds()) {
          if (!sites[v].isEmpty())
            meter.addProgress();
          if (sites[v].getKeeper()) {
            models[v] = modelBuilder.campaignBaseModel("Campaign base site");
            modelBuilder.spawnKeeper(models[v].get());
          } else if (auto villain = sites[v].getVillain())
            models[v] = modelBuilder.campaignSiteModel("Campaign enemy site", villain->enemyId, villain->type);
          else if (auto retired = sites[v].getRetired()) {
            if (PModel m = loadModelFromFile(userPath + "/" + retired->fileInfo.filename))
              models[v] = std::move(m);
            else {
              failedToLoad = retired->fileInfo.filename;
              campaign.clearSite(v);
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
        ModelBuilder modelBuilder(&meter, random, options);
        model = modelBuilder.singleMapModel(NameGenerator::get(NameGeneratorId::WORLD)->getNext());
        modelBuilder.spawnKeeper(model.get());
      });
  return model;
}

PGame MainLoop::loadGame(string file) {
  SavedGameInfo info = *getSavedGameInfo(file);
  int loadTime = 0;
  if (info.getNumSites() == 1)
    loadTime = 100000;
  else
    loadTime = info.getNumSites();
  PGame game;
  doWithSplash(SplashType::BIG, "Loading " + file + "...", loadTime,
      [&] (ProgressMeter& meter) {
        if (info.getNumSites() == 1)
          Square::progressMeter = &meter;
        else
          Model::progressMeter = &meter;
        Debug() << "Loading from " << file;
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

PGame MainLoop::adventurerGame() {
  vector<ListElem> elems;
  vector<SaveFileInfo> files;
  vector<FileSharing::GameInfo> games;
  optional<vector<FileSharing::GameInfo>> onlineGames;
  doWithSplash(SplashType::SMALL, "Fetching list of retired dungeons from the server...",
      [&] { onlineGames = fileSharing->listGames(); }, [&] { fileSharing->cancel(); });
  if (onlineGames)
    games = *onlineGames;
  else
    view->presentText("", "Failed to fetch list of retired dungeons from the server.");
  sort(games.begin(), games.end(), [] (const FileSharing::GameInfo& a, const FileSharing::GameInfo& b) {
      return a.totalGames > b.totalGames || (a.totalGames == b.totalGames && a.time > b.time); });
  getSaveOptions(games, {
      {GameSaveType::RETIRED_SINGLE, "Retired local games:"}}, elems, files);
  getDownloadOptions(games, elems, files, "Retired online games:");
  optional<SaveFileInfo> savedGame = chooseSaveFile(elems, files, "No retired games found.", view);
  if (savedGame) {
    if (savedGame->download)
      if (!downloadGame(savedGame->filename))
        return nullptr;
    if (PGame game = loadGame(savedGame->filename)) {
      game->initialize(options, highscores, view, fileSharing);
      CHECK(game->isSingleModel());
      auto handicap = view->getNumber("Choose handicap (your adventurer's strength and dexterity increase)",
          0, 20, 5);
      game->getMainModel()->landHeroPlayer(options->getStringValue(OptionId::ADVENTURER_NAME),
          handicap.get_value_or(0));
      return game;
    }
  }
  return nullptr;
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
  getSaveOptions({}, {
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

