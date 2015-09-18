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
#include "clock.h"
#include "model_builder.h"
#include "parse_game.h"
#include "name_generator.h"
#include "view.h"
#include "village_control.h"

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

static const int saveVersion = 100;

static bool isCompatible(int loadedVersion) {
  return loadedVersion > 2 && loadedVersion <= saveVersion && loadedVersion / 100 == saveVersion / 100;
}

static string getSaveSuffix(Model::GameType t) {
  switch (t) {
    case Model::GameType::KEEPER: return ".kep";
    case Model::GameType::ADVENTURER: return ".adv";
    case Model::GameType::RETIRED_KEEPER: return ".ret";
    case Model::GameType::AUTOSAVE: return ".aut";
  }
}

template <typename InputType>
static unique_ptr<Model> loadGameUsing(const string& filename, bool eraseFile) {
  unique_ptr<Model> model;
  try {
    InputType input(filename.c_str());
    string discard;
    int version;
    input.getArchive() >>BOOST_SERIALIZATION_NVP(version) >> BOOST_SERIALIZATION_NVP(discard);
    Serialization::registerTypes(input.getArchive(), version);
    input.getArchive() >> BOOST_SERIALIZATION_NVP(model);
  } catch (boost::archive::archive_exception& ex) {
      return nullptr;
  }
  if (eraseFile)
    remove(filename.c_str());
  return model;
}

static unique_ptr<Model> loadGame(const string& filename, bool eraseFile) {
  if (auto model = loadGameUsing<CompressedInput>(filename, eraseFile))
    return model;
  return loadGameUsing<CompressedInput2>(filename, eraseFile); // Try alternative format that doesn't crash on OSX.
}

bool isNonAscii(char c) {
  return !(c>=0 && c <128);
}

string stripNonAscii(string s) {
  s.erase(remove_if(s.begin(),s.end(), isNonAscii), s.end());
  return s;
}

static void saveGame(PModel& model, const string& path) {
  try {
    CompressedOutput out(path);
    Serialization::registerTypes(out.getArchive(), saveVersion);
    string game = model->getGameDisplayName();
    out.getArchive() << BOOST_SERIALIZATION_NVP(saveVersion) << BOOST_SERIALIZATION_NVP(game)
        << BOOST_SERIALIZATION_NVP(model);
  } catch (boost::archive::archive_exception& ex) {
    FAIL << ex.what();
  }
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

string MainLoop::getSavePath(Model* model, Model::GameType gameType) {
  return userPath + "/" + stripNonAscii(model->getGameIdentifier()) + getSaveSuffix(gameType);
}

void MainLoop::saveUI(PModel& model, Model::GameType type, SplashType splashType) {
  string path = getSavePath(model.get(), type);
  doWithSplash(splashType, 62500,
      [&] (ProgressMeter& meter) {
        Square::progressMeter = &meter;
        MEASURE(saveGame(model, path), "saving time")});
  Square::progressMeter = nullptr;
  if (type == Model::GameType::RETIRED_KEEPER && options->getBoolValue(OptionId::ONLINE))
    uploadFile(path);
}

void MainLoop::eraseAutosave(Model* model) {
  remove(getSavePath(model, Model::GameType::AUTOSAVE).c_str());
}

static string getGameDesc(const FileSharing::GameInfo& game) {
  if (game.totalGames > 0)
    return getPlural("daredevil", game.totalGames) + " " + toString(game.totalGames - game.wonGames) + " killed";
  else
    return "";
}

void MainLoop::getSaveOptions(const vector<FileSharing::GameInfo>& onlineGames,
    const vector<pair<Model::GameType, string>>& games, vector<ListElem>& options, vector<SaveFileInfo>& allFiles) {
  for (auto elem : games) {
    vector<SaveFileInfo> files = getSaveFiles(userPath, getSaveSuffix(elem.first));
    files = ::filter(files, [this] (const SaveFileInfo& info) { return isCompatible(getSaveVersion(info));});
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

void MainLoop::playModel(PModel model, bool withMusic, bool noAutoSave) {
  view->reset();
  const int stepTimeMilli = 3;
  Intervalometer meter(stepTimeMilli);
  double totTime = model->getTime();
  double lastMusicUpdate = -1000;
  double lastAutoSave = model->getTime();
  while (1) {
    double gameTimeStep = view->getGameSpeed() / stepTimeMilli;
    if (auto exitInfo = model->update(totTime)) {
      if (!exitInfo->isAbandoned())
        saveUI(model, exitInfo->getGameType(), SplashType::SAVING);
      eraseAutosave(model.get());
      return;
    }
    if (lastMusicUpdate < totTime - 1 && withMusic) {
      jukebox->setType(model->getCurrentMusic(), model->changeMusicNow());
      lastMusicUpdate = totTime;
    }
    if (model->isTurnBased())
      ++totTime;
    else
      totTime += min(1.0, double(meter.getCount(view->getTimeMilli())) * gameTimeStep);
    if (lastAutoSave < model->getTime() - getAutosaveFreq() && !noAutoSave) {
      if (options->getBoolValue(OptionId::AUTOSAVE))
        saveUI(model, Model::GameType::AUTOSAVE, SplashType::AUTOSAVING);
      lastAutoSave = model->getTime();
    }
    if (useSingleThread)
      view->refreshView();
  }
}

int MainLoop::getAutosaveFreq() {
  return 1500;
}

void MainLoop::playGameChoice() {
  while (1) {
    GameTypeChoice choice;
    if (forceGame)
      choice = *forceGame;
    else
      choice = view->chooseGameType();
    PModel model;
    RandomGen random;
    switch (choice) {
      case GameTypeChoice::KEEPER:
        options->setDefaultString(OptionId::KEEPER_NAME, NameGenerator::get(NameGeneratorId::FIRST)->getNext());
        options->setDefaultString(OptionId::KEEPER_SEED, NameGenerator::get(NameGeneratorId::SCROLL)->getNext());
        if (forceGame || options->handleOrExit(view, OptionSet::KEEPER, -1)) {
          string seed = options->getStringValue(OptionId::KEEPER_SEED);
          random.init(hash<string>()(seed));
          ofstream(userPath + "/seeds.txt", std::fstream::out | std::fstream::app) << seed << std::endl;
          model = keeperGame(random);
        }
        break;
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
      case GameTypeChoice::LOAD:
        model = loadPrevious(eraseSave(options));
        break;
      case GameTypeChoice::BACK:
        return;
    }
    if (forceGame != GameTypeChoice::QUICK_LEVEL)
      forceGame.reset();
    if (model) {
      Random = std::move(random);
      model->setOptions(options);
      model->setHighscores(highscores);
      playModel(std::move(model));
    }
    view->reset();
  }
}

void MainLoop::splashScreen() {
  ProgressMeter meter(1);
  jukebox->setType(MusicType::INTRO, true);
  playModel(ModelBuilder::splashModel(meter, view, dataFreePath + "/splash.txt"), false, true);
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
        model = ModelBuilder::quickModel(meter, random, options, view);
      });
  return model;
}

PModel MainLoop::keeperGame(RandomGen& random) {
  PModel model;
  NameGenerator::init(dataFreePath + "/names");
  doWithSplash(SplashType::CREATING, 166000,
      [&model, this, &random] (ProgressMeter& meter) {
        model = ModelBuilder::collectiveModel(meter, random, options, view,
            NameGenerator::get(NameGeneratorId::WORLD)->getNext());
      });
  return model;
}

PModel MainLoop::loadModel(string file, bool erase) {
  PModel model;
  doWithSplash(SplashType::LOADING, 62500,
      [&] (ProgressMeter& meter) {
        Square::progressMeter = &meter;
        Debug() << "Loading from " << file;
        model = loadGame(userPath + "/" + file, erase);});
  if (model) {
    model->setView(view);
  } else
    view->presentText("Sorry", "This save file is corrupted :(");
  Square::progressMeter = nullptr;
  return model;
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

PModel MainLoop::adventurerGame() {
  vector<ListElem> elems;
  vector<SaveFileInfo> files;
  vector<FileSharing::GameInfo> games = fileSharing->listGames();
  sort(games.begin(), games.end(), [] (const FileSharing::GameInfo& a, const FileSharing::GameInfo& b) {
      return a.totalGames > b.totalGames || (a.totalGames == b.totalGames && a.time > b.time); });
  getSaveOptions(games, {
      {Model::GameType::RETIRED_KEEPER, "Retired local games:"}}, elems, files);
  if (options->getBoolValue(OptionId::ONLINE))
    getDownloadOptions(games, elems, files, "Retired online games:");
  optional<SaveFileInfo> savedGame = chooseSaveFile(elems, files, "No retired games found.", view);
  if (savedGame) {
    if (savedGame->download)
      if (!downloadGame(savedGame->filename))
        return nullptr;
    return loadModel(savedGame->filename, false);
  }
  else
    return nullptr;
}

PModel MainLoop::loadPrevious(bool erase) {
  vector<ListElem> options;
  vector<SaveFileInfo> files;
  getSaveOptions({}, {
      {Model::GameType::AUTOSAVE, "Recovered games:"},
      {Model::GameType::KEEPER, "Keeper games:"},
      {Model::GameType::ADVENTURER, "Adventurer games:"}}, options, files);
  optional<SaveFileInfo> savedGame = chooseSaveFile(options, files, "No save files found.", view);
  if (savedGame)
    return loadModel(savedGame->filename, erase);
  else
    return nullptr;
}

bool MainLoop::eraseSave(Options* options) {
#ifdef RELEASE
  return !options->getBoolValue(OptionId::KEEP_SAVEFILES);
#endif
  return false;
}

