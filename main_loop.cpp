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

MainLoop::MainLoop(View* v, Highscores* h, FileSharing* fSharing, const string& freePath,
    const string& uPath, Options* o, Jukebox* j, std::atomic<bool>& fin, bool singleThread,
    optional<GameTypeChoice> force)
      : view(v), dataFreePath(freePath), userPath(uPath), options(o), jukebox(j),
        highscores(h), fileSharing(fSharing), finished(fin), useSingleThread(singleThread), forceGame(force) {
}

vector<MainLoop::SaveFileInfo> MainLoop::getSaveFiles(const string& path, const string& suffix) {
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

static const int saveVersion = 300;

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
static T loadGameUsing(const string& filename, bool eraseFile) {
  T obj;
  try {
    InputType input(filename.c_str());
    string discard;
    int version;
    input.getArchive() >>BOOST_SERIALIZATION_NVP(version) >> BOOST_SERIALIZATION_NVP(discard);
    Serialization::registerTypes(input.getArchive(), version);
    input.getArchive() >> BOOST_SERIALIZATION_NVP(obj);
  } catch (boost::archive::archive_exception& ex) {
      return T();
  }
  if (eraseFile)
    remove(filename.c_str());
  return obj;
}

static PGame loadGameFromFile(const string& filename, bool eraseFile) {
  if (auto game = loadGameUsing<CompressedInput, PGame>(filename, eraseFile))
    return game;
  // Try alternative format that doesn't crash on OSX.
  return loadGameUsing<CompressedInput2, PGame>(filename, eraseFile); 
}

static PModel loadModelFromFile(const string& filename, bool eraseFile) {
  if (auto model = loadGameUsing<CompressedInput, PModel>(filename, eraseFile))
    return model;
  // Try alternative format that doesn't crash on OSX.
  return loadGameUsing<CompressedInput2, PModel>(filename, eraseFile); 
}

bool isNonAscii(char c) {
  return !(c>=0 && c <128);
}

string stripNonAscii(string s) {
  s.erase(remove_if(s.begin(),s.end(), isNonAscii), s.end());
  return s;
}

static void saveGame(PGame& game, const string& path) {
  CompressedOutput out(path);
  Serialization::registerTypes(out.getArchive(), saveVersion);
  string name = game->getGameDisplayName();
  out.getArchive() << BOOST_SERIALIZATION_NVP(saveVersion) << BOOST_SERIALIZATION_NVP(name)
      << BOOST_SERIALIZATION_NVP(game);
}

static void saveMainModel(PGame& game, const string& path) {
  CompressedOutput out(path);
  Serialization::registerTypes(out.getArchive(), saveVersion);
  string name = game->getGameDisplayName();
  game->getMainModel()->setGame(nullptr);
  out.getArchive() << BOOST_SERIALIZATION_NVP(saveVersion) << BOOST_SERIALIZATION_NVP(name)
      << BOOST_SERIALIZATION_NVP(game->getMainModel());
  game->getMainModel()->setGame(game.get());
}

int MainLoop::getSaveVersion(const SaveFileInfo& save) {
  if (auto info = getNameAndVersion(userPath + "/" + save.filename))
    return info->second;
  else
    return -1;
}

void MainLoop::uploadFile(const string& path) {
  atomic<bool> cancelled(false);
  optional<string> error;
  doWithSplash(SplashType::UPLOADING, 1,
      [&] (ProgressMeter& meter) {
        error = fileSharing->uploadRetired(path, meter);
      },
      [&] {
        cancelled = true;
        fileSharing->cancel();
      });
  if (error && !cancelled)
    view->presentText("Error uploading file", *error);
}

string MainLoop::getSavePath(PGame& game, GameSaveType gameType) {
  return userPath + "/" + stripNonAscii(game->getGameIdentifier()) + getSaveSuffix(gameType);
}

void MainLoop::saveUI(PGame& game, GameSaveType type, SplashType splashType) {
  string path = getSavePath(game, type);
  if (type == GameSaveType::RETIRED_SITE)
    doWithSplash(splashType, 62500,
        [&] (ProgressMeter& meter) {
        Square::progressMeter = &meter;
        MEASURE(saveMainModel(game, path), "saving time")});
  else
    doWithSplash(splashType, 62500,
        [&] (ProgressMeter& meter) {
        Square::progressMeter = &meter;
        MEASURE(saveGame(game, path), "saving time")});
  Square::progressMeter = nullptr;
  if (contains({GameSaveType::RETIRED_SINGLE, GameSaveType::RETIRED_SITE}, type) &&
      options->getBoolValue(OptionId::ONLINE))
    uploadFile(path);
}

void MainLoop::eraseAutosave(PGame& game) {
  remove(getSavePath(game, GameSaveType::AUTOSAVE).c_str());
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

optional<MainLoop::SaveFileInfo> MainLoop::chooseSaveFile(const vector<ListElem>& options,
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
  game->setView(view);
  const int stepTimeMilli = 3;
  Intervalometer meter(stepTimeMilli);
  double lastMusicUpdate = -1000;
  double lastAutoSave = game->getGlobalTime();
  while (1) {
    double step = 1;
    if (!game->isTurnBased()) {
      double gameTimeStep = view->getGameSpeed() / stepTimeMilli;
      step = min(1.0, double(meter.getCount(view->getTimeMilli())) * gameTimeStep);
    }
    if (auto exitInfo = game->update(step)) {
      if (exitInfo->getId() == Game::ExitId::SAVE)
        saveUI(game, exitInfo->get<GameSaveType>(), SplashType::SAVING);
      eraseAutosave(game);
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

vector<MainLoop::RetiredSiteInfo> MainLoop::getRetiredSites() {
  vector<RetiredSiteInfo> ret;
  for (auto& info : getSaveFiles(userPath, getSaveSuffix(GameSaveType::RETIRED_SITE))) {
    if (isCompatible(getSaveVersion(info)))
      ret.push_back({info, ViewId::KEEPER, getNameAndVersion(userPath + "/" + info.filename)->first});
  }
  return ret;
}

PGame MainLoop::prepareCampaign(RandomGen& random) {
  random.init(Random.get(1234567));
  vector<RetiredSiteInfo> retired = getRetiredSites();
  optional<Campaign> campaign = Campaign::prepareCampaign(view, retired,
      []{ return NameGenerator::get(NameGeneratorId::WORLD)->getNext(); }, random);
  if (!campaign)
    return nullptr;
/*  optional<GameTypeChoice> choice;
  if (forceGame)
    choice = *forceGame;
  else
    choice = view->chooseGameType();
  if (!choice)
    return;
  switch (*choice) {
    case GameTypeChoice::KEEPER:*/
      options->setDefaultString(OptionId::KEEPER_NAME, NameGenerator::get(NameGeneratorId::FIRST)->getNext());
      if (options->handleOrExit(view, OptionSet::KEEPER, -1))
        return Game::campaignGame(keeperCampaign(*campaign, random), *campaign->getPlayerPos(),
            options->getStringValue(OptionId::KEEPER_NAME), *campaign);
      else
        return nullptr;
/*      break;
    case GameTypeChoice::QUICK_LEVEL:
      random = Random;
      model = quickGame(random);
      break;
    case GameTypeChoice::ADVENTURER:
      options->setDefaultString(OptionId::ADVENTURER_NAME,
          NameGenerator::get(NameGeneratorId::FIRST)->getNext());
      if (options->handleOrExit(view, OptionSet::ADVENTURER, -1)) {
        model = adventurerGame();
      }
      break;
  }*/
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
    PGame game;
    RandomGen random;
    optional<int> choice = view->chooseFromList("", {
        "Campaign", "Single map", "Load game", "Go back"}, 0, MenuType::MAIN);
    if (!choice)
      return;
    switch (*choice) {
      case 0: game = prepareCampaign(random); break;
      case 1: game = prepareSingleMap(random); break;
      case 2: game = loadPrevious(eraseSave(options)); break;
      case 3: return;
    }
    if (forceGame != GameTypeChoice::QUICK_LEVEL)
      forceGame.reset();
    if (game) {
      Random = std::move(random);
      game->setOptions(options);
      game->setHighscores(highscores);
      playGame(std::move(game), true, false);
    }
    view->reset();
  }
}

void MainLoop::splashScreen() {
  ProgressMeter meter(1);
  jukebox->setType(MusicType::INTRO, true);
  playGame(Game::splashScreen(ModelBuilder::splashModel(&meter, dataFreePath + "/splash.txt")), false, true);
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
  int lastIndex = 0;
  while (1) {
    jukebox->setType(MusicType::MAIN, true);
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

void MainLoop::doWithSplash(SplashType type, int totalProgress, function<void(ProgressMeter&)> fun,
    function<void()> cancelFun) {
  ProgressMeter meter(1.0 / totalProgress);
  view->displaySplash(meter, type, cancelFun);
  if (useSingleThread) {
    // A bit confusing, but the flag refers to using a single thread for rendering and gameplay.
    // This forces us to build the world on an extra thread to be able to display a progress bar.
    thread t([fun, &meter, this] { fun(meter); view->clearSplash(); });
    view->refreshView();
    t.join();
  } else {
    fun(meter);
    view->clearSplash();
  }
}

PModel MainLoop::quickGame(RandomGen& random) {
  PModel model;
  NameGenerator::init(dataFreePath + "/names");
  doWithSplash(SplashType::CREATING, 166000,
      [&model, this, &random] (ProgressMeter& meter) {
        model = ModelBuilder::quickModel(&meter, random, options);
      });
  return model;
}

void MainLoop::modelGenTest(int numTries, RandomGen& random, Options* options) {
  NameGenerator::init(dataFreePath + "/names");
  ModelBuilder::measureSiteGen(numTries, Random, options);
}

Table<PModel> MainLoop::keeperCampaign(Campaign& campaign, RandomGen& random) {
  Table<PModel> models(campaign.getSites().getBounds());
  NameGenerator::init(dataFreePath + "/names");
  doWithSplash(SplashType::CREATING, campaign.getSites().getHeight() * campaign.getSites().getWidth(),
      [&models, this, &random, &campaign] (ProgressMeter& meter) {
        auto& sites = campaign.getSites();
        for (Vec2 v : sites.getBounds()) {
          meter.addProgress();
          if (v == campaign.getPlayerPos()) {
            models[v] = ModelBuilder::campaignBaseModel(nullptr, random, options, "pok");
            //ret[v] = ModelBuilder::quickModel(nullptr, random, options);
            ModelBuilder::spawnKeeper(models[v].get(), options);
          } else if (auto villain = sites[v].getVillain())
            //ret[v] = ModelBuilder::quickModel(nullptr, random, options);
            models[v] = ModelBuilder::campaignSiteModel(nullptr, random, options, "pok", villain->enemyId);
          else if (auto retired = sites[v].getRetired()) {
            if (PModel m = loadModelFromFile(userPath + "/" + retired->save.filename, false))
              models[v] = std::move(m);
          }
        }
      });
  return models;
}

PModel MainLoop::keeperSingleMap(RandomGen& random) {
  PModel model;
  NameGenerator::init(dataFreePath + "/names");
  doWithSplash(SplashType::CREATING, 166000,
      [&model, this, &random] (ProgressMeter& meter) {
        model = ModelBuilder::singleMapModel(&meter, random, options,
            NameGenerator::get(NameGeneratorId::WORLD)->getNext());
        ModelBuilder::spawnKeeper(model.get(), options);
      });
  return model;
}

PGame MainLoop::loadGame(string file, bool erase) {
  PGame game;
  doWithSplash(SplashType::LOADING, 62500,
      [&] (ProgressMeter& meter) {
        Square::progressMeter = &meter;
        Debug() << "Loading from " << file;
        game = loadGameFromFile(userPath + "/" + file, erase);});
  if (game) {
    game->setView(view);
  } else
    view->presentText("Sorry", "This save file is corrupted :(");
  Square::progressMeter = nullptr;
  game->setOptions(options);
  game->setView(view);
  return game;
}

bool MainLoop::downloadGame(const string& filename) {
  atomic<bool> cancelled(false);
  optional<string> error;
  doWithSplash(SplashType::DOWNLOADING, 1,
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
  if (options->getBoolValue(OptionId::ONLINE))
    append(games, fileSharing->listGames());
  sort(games.begin(), games.end(), [] (const FileSharing::GameInfo& a, const FileSharing::GameInfo& b) {
      return a.totalGames > b.totalGames || (a.totalGames == b.totalGames && a.time > b.time); });
  getSaveOptions(games, {
      {GameSaveType::RETIRED_SINGLE, "Retired local games:"}}, elems, files);
  if (options->getBoolValue(OptionId::ONLINE))
    getDownloadOptions(games, elems, files, "Retired online games:");
  optional<SaveFileInfo> savedGame = chooseSaveFile(elems, files, "No retired games found.", view);
  if (savedGame) {
    if (savedGame->download)
      if (!downloadGame(savedGame->filename))
        return nullptr;
    PGame game = loadGame(savedGame->filename, false);
    game->landHeroPlayer();
    return game;
  }
  else
    return nullptr;
}

PGame MainLoop::loadPrevious(bool erase) {
  vector<ListElem> options;
  vector<SaveFileInfo> files;
  getSaveOptions({}, {
      {GameSaveType::AUTOSAVE, "Recovered games:"},
      {GameSaveType::KEEPER, "Keeper games:"},
      {GameSaveType::ADVENTURER, "Adventurer games:"}}, options, files);
  optional<SaveFileInfo> savedGame = chooseSaveFile(options, files, "No save files found.", view);
  if (savedGame)
    return loadGame(savedGame->filename, erase);
  else
    return nullptr;
}

bool MainLoop::eraseSave(Options* options) {
#ifdef RELEASE
  return !options->getBoolValue(OptionId::KEEP_SAVEFILES);
#endif
  return false;
}

