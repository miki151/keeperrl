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
#include "gzstream.h"

#include "dirent.h"

#include "view.h"
#include "model.h"
#include "quest.h"
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

using namespace boost::iostreams;
using namespace boost::program_options;
using namespace boost::archive;

struct SaveFileInfo {
  string path;
  time_t date;
};

static vector<SaveFileInfo> getSaveFiles(const string& suffix) {
  vector<SaveFileInfo> ret;
  DIR* dir = opendir(".");
  CHECK(dir) << "Couldn't open current directory";
  while (dirent* ent = readdir(dir)) {
    string name(ent->d_name);
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

static Optional<string> chooseSaveFile(vector<pair<Model::GameType, string>> games, string noSaveMsg, View* view) {
  vector<View::ListElem> options;
  bool noGames = true;
  vector<SaveFileInfo> allFiles;
  for (auto elem : games) {
    vector<SaveFileInfo> files = getSaveFiles(getSaveSuffix(elem.first));
    append(allFiles, files);
    if (!files.empty()) {
      noGames = false;
      options.emplace_back(elem.second, View::TITLE);
      append(options, View::getListElem(transform2<string>(files, getGameName)));
    }
  }
  if (noGames) {
    view->presentText("", noSaveMsg);
    return Nothing();
  }
  auto ind = view->chooseFromList("Choose game", options, 0);
  if (ind)
    return allFiles[*ind].path;
  else
    return Nothing();
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

static void saveGame(unique_ptr<Model> model, const string& saveSuffix) {
  string filename = model->getShortGameIdentifier() + saveSuffix;
  filename = stripNonAscii(filename);
  CompressedOutput out(filename.c_str());
  Serialization::registerTypes(out.getArchive());
  string game = model->getGameIdentifier();
  out.getArchive() << BOOST_SERIALIZATION_NVP(game) << BOOST_SERIALIZATION_NVP(model);
}

static void saveExceptionLine(const string& path, const string& line) {
  ofstream of(path, std::fstream::out | std::fstream::app);
  of << line << std::endl;
}

void clearSingletons() {
  Quest::clearAll();
  Tribe::clearAll();
  Technology::clearAll();
  Skill::clearAll();
  Vision::clearAll();
  Epithet::clearAll();
  NameGenerator::clearAll();
  Spell::clearAll();
}

void initializeSingletons() {
  Item::identifyEverything();
  Tribe::init();
  Skill::init();
  Technology::init();
  Statistics::init();
  Vision::init();
  NameGenerator::init();
  ItemFactory::init();
  Epithet::init();
  Spell::init();
}

void renderLoop(View* view, Options* options, atomic<bool>& finished, atomic<bool>& initialized) {
  view->initialize();
  initialized = true;
  while (!finished) {
    view->refreshView();
    sf::sleep(sf::milliseconds(1));
  }
}
bool tilesPresent() {
  return ifstream("tiles_int.png");
}

#ifdef OSX // see thread comment in stdafx.h
static thread::attributes getAttributes() {
  thread::attributes attr;
  attr.set_stack_size(4096 * 2000);
  return attr;
}
#endif

void initializeRendererTiles(Renderer& r) {
  r.loadTilesFromFile("tiles_int.png", Vec2(36, 36));
  r.loadTilesFromFile("tiles2_int.png", Vec2(36, 36));
  r.loadTilesFromFile("tiles3_int.png", Vec2(36, 36));
  r.loadTilesFromFile("tiles4_int.png", Vec2(24, 24));
  r.loadTilesFromFile("tiles5_int.png", Vec2(36, 36));
  r.loadTilesFromFile("tiles6_int.png", Vec2(36, 36));
  r.loadTilesFromFile("tiles7_int.png", Vec2(36, 36));
  r.loadTilesFromDir("shroom24", Vec2(24, 24));
  r.loadTilesFromDir("shroom36", Vec2(36, 36));
  r.loadTilesFromDir("shroom46", Vec2(46, 46));
}

void initializeJukebox(Jukebox& jukebox) {
  jukebox.addTrack(MusicType::MAIN, "music/main.ogg");
  jukebox.addTrack(MusicType::PEACEFUL, "music/peaceful1.ogg");
  jukebox.addTrack(MusicType::PEACEFUL, "music/peaceful2.ogg");
  jukebox.addTrack(MusicType::PEACEFUL, "music/peaceful3.ogg");
  jukebox.addTrack(MusicType::PEACEFUL, "music/peaceful4.ogg");
  jukebox.addTrack(MusicType::PEACEFUL, "music/peaceful5.ogg");
  jukebox.addTrack(MusicType::BATTLE, "music/battle1.ogg");
  jukebox.addTrack(MusicType::BATTLE, "music/battle2.ogg");
  jukebox.addTrack(MusicType::BATTLE, "music/battle3.ogg");
  jukebox.addTrack(MusicType::BATTLE, "music/battle4.ogg");
  jukebox.addTrack(MusicType::BATTLE, "music/battle5.ogg");
  jukebox.addTrack(MusicType::NIGHT, "music/night1.ogg");
  jukebox.addTrack(MusicType::NIGHT, "music/night2.ogg");
  jukebox.addTrack(MusicType::NIGHT, "music/night3.ogg");
}

class MainLoop {
  public:
  MainLoop(View* v, Options* o, Jukebox* j, std::atomic<bool>& fin)
      : view(v), options(o), jukebox(j), finished(fin) {}

  void saveUI(PModel model, Model::GameType type) {
    ProgressMeter meter(1.0 / 62500);
    Square::progressMeter = &meter;
    view->displaySplash(meter, View::SAVING);
    saveGame(std::move(model), getSaveSuffix(type));
    view->clearSplash();
    Square::progressMeter = nullptr;
  }

  void playModel(PModel model) {
    view->reset();
    jukebox->update(MusicType::PEACEFUL);
    const int stepTimeMilli = 3;
    Intervalometer meter(stepTimeMilli);
    double totTime = model->getTime();
    while (1) {
      double gameTimeStep = view->getGameSpeed() / stepTimeMilli;
      if (auto exitInfo = model->update(totTime)) {
        if (!exitInfo->isAbandoned())
          saveUI(std::move(model), exitInfo->getGameType());
        return;
      }
      jukebox->update(model->getCurrentMusic());
      if (model->isTurnBased())
        ++totTime;
      else
        totTime += min(1.0, double(meter.getCount()) * gameTimeStep);
    }
  }

  void playGameChoice() {
    while (1) {
      auto choice = view->chooseFromList("", {
          View::ListElem("Choose your role:", View::TITLE),
          "Keeper",
          "Adventurer", 
          View::ListElem("Or simply:", View::TITLE),
          "Load a game",
          "Go back"},
          0, View::MAIN_MENU);
      if (!choice)
        return;
      initializeSingletons();
      PModel model;
      switch (*choice) {
        case 0:
          options->setDefaultString(OptionId::KEEPER_NAME, NameGenerator::get(NameGeneratorId::FIRST)->getNext());
          if (options->handleOrExit(view, OptionSet::KEEPER, -1)) {
            model = keeperGame();
          }
          break;
        case 1:
          options->setDefaultString(OptionId::ADVENTURER_NAME,
              NameGenerator::get(NameGeneratorId::FIRST)->getNext());
          if (options->handleOrExit(view, OptionSet::ADVENTURER, -1)) {
            model = adventurerGame();
          }
          break;
        case 2:
          model = loadPrevious(eraseSave(options));
          break;
        case 3: 
          clearSingletons();
          return;
      }
      if (model) {
        model->setOptions(options);
        playModel(std::move(model));
      }
      clearSingletons();
      view->reset();
    }
  }


  void start() {
    if (!tilesPresent())
      view->presentText("", "You are playing a version of KeeperRL without graphical tiles. "
          "Besides lack of graphics and music, this "
          "is the same exact game as the full version. If you'd like to buy the full version, "
          "please visit keeperrl.com.\n \nYou can also get it by donating to any wildlife charity."
          "More information on the website.");
    int lastIndex = 0;
    if (options->getBoolValue(OptionId::MUSIC))
      jukebox->toggle();
    while (1) {
 //     jukebox->update(MusicType::MAIN);
      auto choice = view->chooseFromList("", {
          "Play game", "Change settings", "View high scores", "View credits", "Quit"}, lastIndex, View::MAIN_MENU);
      if (!choice)
        continue;
      lastIndex = *choice;
      switch (*choice) {
        case 0: playGameChoice(); break;
        case 1: options->handle(view, OptionSet::GENERAL); break;
        case 2: Model::showHighscore(view); break;
        case 3: Model::showCredits(view); break;
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
        model.reset(Model::collectiveModel(meter, options, view,
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
    PModel ret = loadGame(file, erase);
    ret->setView(view);
    view->clearSplash();
    return ret;
  }

  PModel adventurerGame() {
    Optional<string> savedGame = chooseSaveFile({
        {Model::GameType::RETIRED_KEEPER, "Retired keeper games:"}},
        "No retired games found.", view);
    if (savedGame)
      return loadModel(*savedGame, false);
    else
      return nullptr;
  }

  PModel loadPrevious(bool erase) {
    Optional<string> savedGame = chooseSaveFile({
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
  Options* options;
  Jukebox* jukebox;
  std::atomic<bool>& finished;
};

int main(int argc, char* argv[]) {
  options_description flags("Flags");
  flags.add_options()
    ("help", "Print help")
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
  Options options("options.txt");
  Renderer renderer("KeeperRL", Vec2(36, 36));
  Clock::set(new Clock());
  if (tilesPresent())
    initializeRendererTiles(renderer);
  int seed = vars.count("seed") ? vars["seed"].as<int>() : int(time(0));
 // int forceMode = vars.count("force_keeper") ? 0 : -1;
  bool genExit = vars.count("gen_world_exit");
  if (vars.count("replay")) {
    string fname = vars["replay"].as<string>();
    Debug() << "Reading from " << fname;
    seed = fromString<int>(fname.substr(lognamePref.size()));
    Random.init(seed);
    input.reset(new CompressedInput(fname));
    view.reset(WindowView::createReplayView(input->getArchive(), {renderer, tilesPresent(), &options}));
  } else {
    Random.init(seed);
    string fname(lognamePref);
    fname += toString(seed);
    output.reset(new CompressedOutput(fname));
    Debug() << "Writing to " << fname;
    view.reset(WindowView::createLoggingView(output->getArchive(), {renderer, tilesPresent(), &options}));
  } 
  std::atomic<bool> gameFinished(false);
  std::atomic<bool> viewInitialized(false);
  Tile::initialize(renderer, tilesPresent());
  Jukebox jukebox(&options);
  if (tilesPresent())
    initializeJukebox(jukebox);
  MainLoop loop(view.get(), &options, &jukebox, gameFinished);
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

