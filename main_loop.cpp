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

MainLoop::MainLoop(View* v, Highscores* h, FileSharing* fSharing, const string& freePath,
    const string& uPath, Options* o, Jukebox* j, std::atomic<bool>& fin)
      : view(v), dataFreePath(freePath), userPath(uPath), options(o), jukebox(j),
        highscores(h), fileSharing(fSharing), finished(fin) {
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

static const int saveVersion = 1;

static string getSaveSuffix(Model::GameType t) {
  switch (t) {
    case Model::GameType::KEEPER: return ".kep";
    case Model::GameType::ADVENTURER: return ".adv";
    case Model::GameType::RETIRED_KEEPER: return ".ret";
  }
}

static unique_ptr<Model> loadGame(const string& filename, bool eraseFile) {
  unique_ptr<Model> model;
#ifdef RELEASE
  try {
#endif
    CompressedInput input(filename.c_str());
    Serialization::registerTypes(input.getArchive());
    string discard;
    int version;
    input.getArchive() >>BOOST_SERIALIZATION_NVP(version) >> BOOST_SERIALIZATION_NVP(discard)
      >> BOOST_SERIALIZATION_NVP(model);
#ifdef RELEASE
  } catch (boost::archive::archive_exception& ex) {
    return nullptr;
  }
#endif
  if (eraseFile)
    remove(filename.c_str());
  return model;
}

bool isNonAscii(char c) {
  return !(c>=0 && c <128);
}

string stripNonAscii(string s) {
  s.erase(remove_if(s.begin(),s.end(), isNonAscii), s.end());
  return s;
}

static void saveGame(unique_ptr<Model> model, const string& path) {
  CompressedOutput out(path);
  Serialization::registerTypes(out.getArchive());
  string game = model->getGameDisplayName();
  out.getArchive() << BOOST_SERIALIZATION_NVP(saveVersion) << BOOST_SERIALIZATION_NVP(game)
    << BOOST_SERIALIZATION_NVP(model);
}

View::ListElem MainLoop::getGameName(const SaveFileInfo& save) {
  CompressedInput input(userPath + "/" + save.filename.c_str());
  string name;
  int version;
  input.getArchive() >> version >> name;
  return View::ListElem(name, getDateString(save.date));
}

int MainLoop::getSaveVersion(const SaveFileInfo& save) {
  try {
    CompressedInput input(userPath + "/" + save.filename.c_str());
    string name;
    int version;
    input.getArchive() >> version;
    return version;
  } catch (boost::archive::archive_exception& ex) {
    return -1;
  }
}


void MainLoop::uploadFile(const string& path) {
  ProgressMeter meter(1);
  atomic<bool> cancelled(false);
  view->displaySplash(meter, View::UPLOADING, [&] { cancelled = true; fileSharing->getCancelFun()();});
  optional<string> error = fileSharing->uploadRetired(path, meter);
  view->clearSplash();
  if (error && !cancelled)
    view->presentText("Error uploading file", *error);
}

void MainLoop::saveUI(PModel model, Model::GameType type) {
  ProgressMeter meter(1.0 / 62500);
  Square::progressMeter = &meter;
  view->displaySplash(meter, View::SAVING);
  string path = userPath + "/" + stripNonAscii(model->getGameIdentifier()) + getSaveSuffix(type);
  saveGame(std::move(model), path);
  view->clearSplash();
  Square::progressMeter = nullptr;
  if (type == Model::GameType::RETIRED_KEEPER)
    uploadFile(path);
}

void MainLoop::getSaveOptions(const vector<pair<Model::GameType, string>>& games,
    vector<View::ListElem>& options, vector<SaveFileInfo>& allFiles) {
  for (auto elem : games) {
    vector<SaveFileInfo> files = getSaveFiles(userPath, getSaveSuffix(elem.first));
    files = ::filter(files, [this] (const SaveFileInfo& info) { return getSaveVersion(info) == saveVersion;});
    append(allFiles, files);
    if (!files.empty()) {
      options.emplace_back(elem.second, View::TITLE);
      append(options, transform2<View::ListElem>(files,
            [this] (const SaveFileInfo& info) { return getGameName(info);}));
    }
  }
}

void MainLoop::getDownloadOptions(vector<View::ListElem>& options, vector<SaveFileInfo>& allFiles,
    const string& title) {
  vector<FileSharing::GameInfo> games = fileSharing->listGames(saveVersion);
  options.emplace_back(title, View::TITLE);
  for (FileSharing::GameInfo info : games) {
    bool dup = false;
    for (auto& elem : allFiles)
      if (elem.filename == info.filename) {
        dup = true;
        break;
      }
    if (dup)
      continue;
    options.emplace_back(info.displayName, getDateString(info.time));
    allFiles.push_back({info.filename, info.time, true});
  }
}

optional<MainLoop::SaveFileInfo> MainLoop::chooseSaveFile(const vector<View::ListElem>& options,
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

void MainLoop::playModel(PModel model, bool withMusic) {
  view->reset();
  const int stepTimeMilli = 3;
  Intervalometer meter(stepTimeMilli);
  double totTime = model->getTime();
  double lastMusicUpdate = -1000;
  while (1) {
    double gameTimeStep = view->getGameSpeed() / stepTimeMilli;
    if (auto exitInfo = model->update(totTime)) {
      if (!exitInfo->isAbandoned())
        saveUI(std::move(model), exitInfo->getGameType());
      return;
    }
    if (lastMusicUpdate < totTime - 1 && withMusic) {
      jukebox->setType(model->getCurrentMusic());
      lastMusicUpdate = totTime;
    } if (model->isTurnBased())
    ++totTime;
    else
      totTime += min(1.0, double(meter.getCount(view->getTimeMilli())) * gameTimeStep);
  }
}

void MainLoop::playGameChoice() {
  while (1) {
    auto choice = view->chooseGameType();
    PModel model;
    switch (choice) {
      case View::KEEPER_CHOICE:
        options->setDefaultString(OptionId::KEEPER_NAME, NameGenerator::get(NameGeneratorId::FIRST)->getNext());
        if (options->handleOrExit(view, OptionSet::KEEPER, -1)) {
          model = keeperGame();
        }
        break;
      case View::ADVENTURER_CHOICE:
        options->setDefaultString(OptionId::ADVENTURER_NAME,
            NameGenerator::get(NameGeneratorId::FIRST)->getNext());
        if (options->handleOrExit(view, OptionSet::ADVENTURER, -1)) {
          model = adventurerGame();
        }
        break;
      case View::LOAD_CHOICE:
        model = loadPrevious(eraseSave(options));
        break;
      case View::BACK_CHOICE:
        return;
    }
    if (model) {
      model->setOptions(options);
      model->setHighscores(highscores);
      jukebox->setType(MusicType::PEACEFUL);
      playModel(std::move(model));
    }
    view->reset();
  }
}

void MainLoop::splashScreen() {
  ProgressMeter meter(1);
  jukebox->setType(MusicType::INTRO);
  NameGenerator::init(dataFreePath + "/names");
  playModel(ModelBuilder::splashModel(meter, view, dataFreePath + "/splash.txt"), false);
}

void MainLoop::showCredits(const string& path, View* view) {
  ifstream in(path);
  CHECK(!!in);
  vector<View::ListElem> lines;
  while (1) {
    char buf[100];
    in.getline(buf, 100);
    if (!in)
      break;
    string s(buf);
    if (s.back() == ':')
      lines.emplace_back(s, View::TITLE);
    else
      lines.emplace_back(s, View::NORMAL);
  }
  view->presentList("Credits", lines, false);
}

void MainLoop::start(bool tilesPresent) {
  if (options->getBoolValue(OptionId::MUSIC))
    jukebox->toggle(true);
  splashScreen();
  view->reset();
  if (!tilesPresent)
    view->presentText("", "You are playing a version of KeeperRL without graphical tiles. "
        "Besides lack of graphics and music, this "
        "is the same exact game as the full version. If you'd like to buy the full version, "
        "please visit keeperrl.com.\n \nYou can also get it by donating to any wildlife charity."
        "More information on the website.");
  int lastIndex = 0;
  while (1) {
    jukebox->setType(MusicType::MAIN);
    auto choice = view->chooseFromList("", {
        "Play game", "Change settings", "View high scores", "View credits", "Quit"}, lastIndex, View::MAIN_MENU);
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

PModel MainLoop::keeperGame() {
  PModel model;
  string ex;
  ProgressMeter meter(1.0 / 166000);
  view->displaySplash(meter, View::CREATING);
  NameGenerator::init(dataFreePath + "/names");
  model = ModelBuilder::collectiveModel(meter, options, view,
      NameGenerator::get(NameGeneratorId::WORLD)->getNext());
  view->clearSplash();
  return model;
}

PModel MainLoop::loadModel(string file, bool erase) {
  ProgressMeter meter(1.0 / 62500);
  Square::progressMeter = &meter;
  view->displaySplash(meter, View::LOADING);
  Debug() << "Loading from " << file;
  PModel ret = loadGame(userPath + "/" + file, erase);
  view->clearSplash();
  if (ret) {
    ret->setView(view);
  } else
    view->presentText("Sorry", "This save file is corrupted :(");
  return ret;
}

bool MainLoop::downloadGame(const string& filename) {
  ProgressMeter meter(1);
  atomic<bool> cancelled(false);
  view->displaySplash(meter, View::DOWNLOADING, [&] { cancelled = true; fileSharing->getCancelFun()();});
  optional<string> error = fileSharing->download(filename, userPath, meter);
  view->clearSplash();
  if (error && !cancelled)
    view->presentText("Error downloading file", *error);
  return !error;
}

PModel MainLoop::adventurerGame() {
  vector<View::ListElem> options;
  vector<SaveFileInfo> files;
  getSaveOptions({
      {Model::GameType::RETIRED_KEEPER, "Retired local games:"}}, options, files);
  getDownloadOptions(options, files, "Retired online games:");
  optional<SaveFileInfo> savedGame = chooseSaveFile(options, files, "No retired games found.", view);
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
  vector<View::ListElem> options;
  vector<SaveFileInfo> files;
  getSaveOptions({{Model::GameType::KEEPER, "Keeper games:"},
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

