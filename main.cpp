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

#include <ctime>
#include <locale>
#include <sys/types.h>
#include <sys/stat.h>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "gzstream.h"

#include "dirent.h"

#include "view.h"
#include "model.h"
#include "tribe.h"
#include "statistics.h"
#include "options.h"
#include "technology.h"
#include "gui_elem.h"
#include "music.h"
#include "item.h"
#include "name_generator.h"
#include "item_factory.h"
#include "creature.h"
#include "test.h"
#include "pantheon.h"
#include "tile.h"
#include "clock.h"
#include "progress_meter.h"
#include "square.h"
#include "spell.h"
#include "window_view.h"
#include "clock.h"
#include "model_builder.h"
#include "file_sharing.h"

#ifndef DATA_DIR
#define DATA_DIR "."
#endif

#ifndef USER_DIR
#define USER_DIR "."
#endif

using namespace boost::iostreams;
using namespace boost::program_options;
using namespace boost::archive;

struct SaveFileInfo {
  string path;
  time_t date;
};

static vector<SaveFileInfo> getSaveFiles(const string& path, const string& suffix) {
  vector<SaveFileInfo> ret;
  DIR* dir = opendir(path.c_str());
  CHECK(dir) << "Couldn't open " + path;
  while (dirent* ent = readdir(dir)) {
    string name(path + "/" + ent->d_name);
    if (endsWith(name, suffix)) {
      struct stat buf;
      stat(name.c_str(), &buf);
      ret.push_back({name, buf.st_mtime});
    }
  }
  closedir(dir);
  sort(ret.begin(), ret.end(), [](const SaveFileInfo& a, const SaveFileInfo& b) {
        return a.date > b.date;
      });
  return ret;
}

static string getSaveSuffix(Model::GameType t) {
  switch (t) {
    case Model::GameType::KEEPER: return ".kep";
    case Model::GameType::ADVENTURER: return ".adv";
    case Model::GameType::RETIRED_KEEPER: return ".ret";
  }
  FAIL << "wef";
  return "";
}

static string getDateString(time_t t) {
  char buf[100];
  strftime(buf, sizeof(buf), "%c", std::localtime(&t));
  return buf;
}

template <class T, class U>
class StreamCombiner {
  public:
  StreamCombiner(const string& filename) : stream(filename.c_str()), archive(stream) {
    CHECK(stream.good()) << "File not found: " << filename;
  }

  U& getArchive() {
    return archive;
  }

  private:
  T stream;
  U archive;
};

typedef StreamCombiner<ogzstream, OutputArchive> CompressedOutput;
typedef StreamCombiner<igzstream, InputArchive> CompressedInput;

string getGameName(const SaveFileInfo& save) {
  CompressedInput input(save.path.c_str());
  Serialization::registerTypes(input.getArchive());
  string name;
  input.getArchive() >> name;
  return name + " (" + getDateString(save.date) + ")";
}

static unique_ptr<Model> loadGame(const string& filename, bool eraseFile) {
  unique_ptr<Model> model;
  {
    CompressedInput input(filename.c_str());
    Serialization::registerTypes(input.getArchive());
    string discard;
    input.getArchive() >> BOOST_SERIALIZATION_NVP(discard) >> BOOST_SERIALIZATION_NVP(model);
  }
  if (eraseFile)
    CHECK(!remove(filename.c_str()));
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
  string game = model->getGameIdentifier();
  out.getArchive() << BOOST_SERIALIZATION_NVP(game) << BOOST_SERIALIZATION_NVP(model);
}

static void saveExceptionLine(const string& path, const string& line) {
  ofstream of(path, std::fstream::out | std::fstream::app);
  of << line << std::endl;
}

void renderLoop(View* view, Options* options, atomic<bool>& finished, atomic<bool>& initialized) {
  view->initialize();
  initialized = true;
  while (!finished) {
    view->refreshView();
  }
}

static bool tilesPresent;

#ifdef OSX // see thread comment in stdafx.h
static thread::attributes getAttributes() {
  thread::attributes attr;
  attr.set_stack_size(4096 * 2000);
  return attr;
}
#endif

void initializeRendererTiles(Renderer& r, const string& path) {
  r.loadTilesFromFile(path + "/tiles_int.png", Vec2(36, 36));
  r.loadTilesFromFile(path + "/tiles2_int.png", Vec2(36, 36));
  r.loadTilesFromFile(path + "/tiles3_int.png", Vec2(36, 36));
  r.loadTilesFromFile(path + "/tiles4_int.png", Vec2(24, 24));
  r.loadTilesFromFile(path + "/tiles5_int.png", Vec2(36, 36));
  r.loadTilesFromFile(path + "/tiles6_int.png", Vec2(36, 36));
  r.loadTilesFromFile(path + "/tiles7_int.png", Vec2(36, 36));
  r.loadTilesFromDir(path + "/shroom24", Vec2(24, 24));
  r.loadTilesFromDir(path + "/shroom36", Vec2(36, 36));
  r.loadTilesFromDir(path + "/shroom46", Vec2(46, 46));
}

vector<pair<MusicType, string>> getMusicTracks(const string& path) {
  if (!tilesPresent)
    return {};
  else
    return {
      {MusicType::INTRO, path + "/intro.ogg"},
      {MusicType::MAIN, path + "/main.ogg"},
      {MusicType::PEACEFUL, path + "/peaceful1.ogg"},
      {MusicType::PEACEFUL, path + "/peaceful2.ogg"},
      {MusicType::PEACEFUL, path + "/peaceful3.ogg"},
      {MusicType::PEACEFUL, path + "/peaceful4.ogg"},
      {MusicType::PEACEFUL, path + "/peaceful5.ogg"},
      {MusicType::BATTLE, path + "/battle1.ogg"},
      {MusicType::BATTLE, path + "/battle2.ogg"},
      {MusicType::BATTLE, path + "/battle3.ogg"},
      {MusicType::BATTLE, path + "/battle4.ogg"},
      {MusicType::BATTLE, path + "/battle5.ogg"},
      {MusicType::NIGHT, path + "/night1.ogg"},
      {MusicType::NIGHT, path + "/night2.ogg"},
      {MusicType::NIGHT, path + "/night3.ogg"},
    };
}

class MainLoop {
  public:
  MainLoop(View* v, const string& _dataFreePath, const string& _userPath, const string& _uploadUrl, Options* o, Jukebox* j,
      std::atomic<bool>& fin)
      : view(v), dataFreePath(_dataFreePath), userPath(_userPath), uploadUrl(_uploadUrl), options(o), jukebox(j), finished(fin) {}

  void uploadFile(const string& path) {
    ProgressMeter meter(1);
    FileSharing f(uploadUrl);
    view->displaySplash(meter, View::UPLOADING, f.getCancelFun());
    optional<string> error = f.upload(path, meter);
    view->clearSplash();
    if (error)
      view->presentText("Error uploading file", *error);
  }

  void saveUI(PModel model, Model::GameType type) {
    ProgressMeter meter(1.0 / 62500);
    Square::progressMeter = &meter;
    view->displaySplash(meter, View::SAVING);
    string path = userPath + "/" + stripNonAscii(model->getShortGameIdentifier()) + getSaveSuffix(type);
    saveGame(std::move(model), path);
    view->clearSplash();
    Square::progressMeter = nullptr;
    if (type == Model::GameType::RETIRED_KEEPER)
      uploadFile(path);
  }

  optional<string> chooseSaveFile(vector<pair<Model::GameType, string>> games, string noSaveMsg, View* view) {
    vector<View::ListElem> options;
    bool noGames = true;
    vector<SaveFileInfo> allFiles;
    for (auto elem : games) {
      vector<SaveFileInfo> files = getSaveFiles(userPath, getSaveSuffix(elem.first));
      append(allFiles, files);
      if (!files.empty()) {
        noGames = false;
        options.emplace_back(elem.second, View::TITLE);
        append(options, View::getListElem(transform2<string>(files, getGameName)));
      }
    }
    if (noGames) {
      view->presentText("", noSaveMsg);
      return none;
    }
    auto ind = view->chooseFromList("Choose game", options, 0);
    if (ind)
      return allFiles[*ind].path;
    else
      return none;
  }

  void playModel(PModel model, bool withMusic = true) {
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

  void playGameChoice() {
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
        jukebox->setType(MusicType::PEACEFUL);
        playModel(std::move(model));
      }
      view->reset();
    }
  }

  void splashScreen() {
    ProgressMeter meter(1);
    jukebox->setType(MusicType::INTRO);
    playModel(PModel(ModelBuilder::splashModel(meter, view, dataFreePath + "/splash.txt")), false);
  }

  void showCredits(const string& path, View* view) {
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

  void start() {
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
        case 2: Model::showHighscore(view); break;
        case 3: showCredits(dataFreePath + "/credits.txt", view); break;
        case 4: finished = true; break;
      }
      if (finished)
        break;
    }
  }

  PModel keeperGame() {
    PModel model;
    string ex;
    ProgressMeter meter(1.0 / 166000);
    view->displaySplash(meter, View::CREATING);
    for (int i : Range(5)) {
      try {
        model.reset(ModelBuilder::collectiveModel(meter, options, view,
              NameGenerator::get(NameGeneratorId::WORLD)->getNext()));
        break;
      } catch (string s) {
        ex = s;
      }
    }
    if (!model) {
      view->presentText("Sorry!", "World generation permanently failed with the following error:\n \n" + ex +
          "\n \nIf you would be so kind, please send the file \'crash.log\'"
          " to rusolis@poczta.fm Thanks!");
      saveExceptionLine("crash.log", ex);
    }
    view->clearSplash();
    return model;
  }

  PModel loadModel(string file, bool erase) {
    ProgressMeter meter(1.0 / 62500);
    Square::progressMeter = &meter;
    view->displaySplash(meter, View::LOADING);
    Debug() << "Loading from " << file;
    PModel ret = loadGame(file, erase);
    ret->setView(view);
    view->clearSplash();
    return ret;
  }

  PModel adventurerGame() {
    optional<string> savedGame = chooseSaveFile({
        {Model::GameType::RETIRED_KEEPER, "Retired keeper games:"}},
        "No retired games found.", view);
    if (savedGame)
      return loadModel(*savedGame, false);
    else
      return nullptr;
  }

  PModel loadPrevious(bool erase) {
    optional<string> savedGame = chooseSaveFile({
        {Model::GameType::KEEPER, "Keeper games:"},
        {Model::GameType::ADVENTURER, "Adventurer games:"}},
        "No save files found.", view);
    if (savedGame)
      return loadModel(*savedGame, erase);
    else
      return nullptr;
  }

  static bool eraseSave(Options* options) {
#ifdef RELEASE
    return !options->getBoolValue(OptionId::KEEP_SAVEFILES);
#endif
    return false;
  }

  private:
  View* view;
  string dataFreePath;
  string userPath;
  string uploadUrl;
  Options* options;
  Jukebox* jukebox;
  std::atomic<bool>& finished;
};

void makeDir(const string& path) {
  boost::filesystem::create_directories(path.c_str());
}

int main(int argc, char* argv[]) {
  options_description flags("Flags");
  flags.add_options()
    ("help", "Print help")
    ("user_dir", value<string>(), "Directory for options and save files")
    ("data_dir", value<string>(), "Directory containing the game data")
    ("upload_url", value<string>(), "URL for uploading maps")
    ("run_tests", "Run all unit tests and exit")
    ("gen_world_exit", "Exit after creating a world")
    ("force_keeper", "Skip main menu and force keeper mode")
    ("seed", value<int>(), "Use given seed")
    ("replay", value<string>(), "Replay game from file");
  variables_map vars;
  store(parse_command_line(argc, argv, flags), vars);
  if (vars.count("help")) {
    std::cout << flags << endl;
    return 0;
  }
  if (vars.count("run_tests")) {
    testAll();
    return 0;
  }
  unique_ptr<View> view;
  unique_ptr<CompressedInput> input;
  unique_ptr<CompressedOutput> output;
  string lognamePref = "log";
  Debug::init();
  Skill::init();
  Technology::init();
  Spell::init();
  Epithet::init();
  Vision::init();
  string dataPath;
  if (vars.count("data_dir"))
    dataPath = vars["data_dir"].as<string>();
  else
    dataPath = DATA_DIR;
  string freeDataPath = dataPath + "/data_free";
  string paidDataPath = dataPath + "/data";
  string contribDataPath = dataPath + "/data_contrib";
  tilesPresent = !!opendir(paidDataPath.c_str());
  string userPath;
  if (vars.count("user_dir"))
    userPath = vars["user_dir"].as<string>();
  else
  if (const char* localPath = std::getenv("XDG_DATA_HOME"))
    userPath = localPath + string("/KeeperRL");
  else
    userPath = USER_DIR;
  Debug() << "Data path: " << dataPath;
  Debug() << "User path: " << userPath;
  string uploadUrl;
  if (vars.count("upload_url"))
    uploadUrl = vars["upload_url"].as<string>();
  else
    uploadUrl = "http://keeperrl.com/retired";
  makeDir(userPath);
  Options options(userPath + "/options.txt");
  Renderer renderer("KeeperRL", Vec2(36, 36), contribDataPath);
  GuiFactory guiFactory;
  guiFactory.loadFreeImages(freeDataPath + "/images");
  if (tilesPresent)
    guiFactory.loadNonFreeImages(paidDataPath + "/images");
  Clock clock;
  if (tilesPresent)
    initializeRendererTiles(renderer, paidDataPath + "/images");
  int seed = vars.count("seed") ? vars["seed"].as<int>() : int(time(0));
 // int forceMode = vars.count("force_keeper") ? 0 : -1;
  bool genExit = vars.count("gen_world_exit");
  if (vars.count("replay")) {
    string fname = vars["replay"].as<string>();
    Debug() << "Reading from " << fname;
    seed = fromString<int>(fname.substr(lognamePref.size()));
    Random.init(seed);
    input.reset(new CompressedInput(fname));
    view.reset(WindowView::createReplayView(input->getArchive(),
          {renderer, guiFactory, tilesPresent, &options, &clock}));
  } else {
    Random.init(seed);
#ifndef RELEASE
    string fname(lognamePref);
    fname += toString(seed);
    output.reset(new CompressedOutput(fname));
    Debug() << "Writing to " << fname;
    view.reset(WindowView::createLoggingView(output->getArchive(),
          {renderer, guiFactory, tilesPresent, &options, &clock}));
#else
    view.reset(WindowView::createDefaultView(
          {renderer, guiFactory, tilesPresent, &options, &clock}));
#endif
  } 
  std::atomic<bool> gameFinished(false);
  std::atomic<bool> viewInitialized(false);
  Tile::initialize(renderer, tilesPresent);
  NameGenerator::init(freeDataPath + "/names");
  Jukebox jukebox(&options, getMusicTracks(paidDataPath + "/music"));
  MainLoop loop(view.get(), freeDataPath, userPath, uploadUrl, &options, &jukebox, gameFinished);
  auto game = [&] { while (!viewInitialized) {} loop.start(); };
  auto render = [&] { renderLoop(view.get(), &options, gameFinished, viewInitialized); };
#ifdef OSX // see thread comment in stdafx.h
  thread t(getAttributes(), game);
  render();
#else
  thread t(render);
  game();
#endif
  t.join();
  return 0;
}

