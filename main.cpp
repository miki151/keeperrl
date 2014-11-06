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
#include "script_context.h"
#include "tile.h"
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
  struct dirent *ent;
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

static string getSaveSuffix(GameType t) {
  switch (t) {
    case GameType::KEEPER: return ".kep";
    case GameType::ADVENTURER: return ".adv";
    case GameType::RETIRED_KEEPER: return ".ret";
  }
  FAIL << "wef";
  return "";
}

static string getDateString(time_t t) {
  char buf[100];
  strftime(buf, sizeof(buf), "%c", std::localtime(&t));
  return buf;
}

static Optional<string> chooseSaveFile(vector<pair<GameType, string>> games, string noSaveMsg, View* view) {
  vector<View::ListElem> options;
  bool noGames = true;
  vector<SaveFileInfo> allFiles;
  for (auto elem : games) {
    vector<SaveFileInfo> files = getSaveFiles(getSaveSuffix(elem.first));
    append(allFiles, files);
    if (!files.empty()) {
      noGames = false;
      auto removeSuf = [&] (const SaveFileInfo& s) { return s.path.substr(0, s.path.size() - 
          getSaveSuffix(elem.first).size()) + "  (" + getDateString(s.date) + ")"; };
      options.emplace_back(elem.second, View::TITLE);
      append(options, View::getListElem(transform2<string>(files, removeSuf)));
    }
  }
  if (noGames) {
    view->presentText("", noSaveMsg);
    return Nothing();
  }
  auto ind = view->chooseFromList("Choose game", options, 0, View::MAIN_MENU);
  if (ind)
    return allFiles[*ind].path;
  else
    return Nothing();
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

typedef StreamCombiner<ofstream, binary_oarchive> CompressedOutput;
typedef StreamCombiner<ifstream, binary_iarchive> CompressedInput;

static unique_ptr<Model> loadGame(const string& filename, bool eraseFile) {
  unique_ptr<Model> model;
  {
    CompressedInput input(filename.c_str());
    Serialization::registerTypes(input.getArchive());
    input.getArchive() >> BOOST_SERIALIZATION_NVP(model);
  }
#ifdef RELEASE
  if (eraseFile && !Options::getValue(OptionId::KEEP_SAVEFILES))
    CHECK(!remove(filename.c_str()));
#endif
  return model;
}

static void saveGame(unique_ptr<Model> model, const string& filename) {
  CompressedOutput out(filename.c_str());
  Serialization::registerTypes(out.getArchive());
  out.getArchive() << BOOST_SERIALIZATION_NVP(model);
}

/*static Table<bool> readSplashTable(const string& path) {
  ifstream in(path);
  int x, y;
  in >> x >> y;
  Table<bool> ret(x, y);
  for (int i : Range(y))
    for (int j : Range(x))
      in >> ret[j][i];
  return ret;
}*/

static void saveExceptionLine(const string& path, const string& line) {
  ofstream of(path, std::fstream::out | std::fstream::app);
  of << line << std::endl;
}

void clearAndInitialize() {
  Item::identifyEverything();
  Quest::clearAll();
  Tribe::clearAll();
  Technology::clearAll();
  Skill::clearAll();
  Vision::clearAll();
  Epithet::clearAll();
  NameGenerator::clearAll();
  Tribe::init();
  Skill::init();
  Technology::init();
  Statistics::init();
  Vision::init();
  NameGenerator::init();
  ItemFactory::init();
  Epithet::init();
}

int main(int argc, char* argv[]) {
  options_description options("Flags");
  options.add_options()
    ("help", "Print help")
    ("run_tests", "Run all unit tests and exit")
    ("gen_world_exit", "Exit after creating a world")
    ("force_keeper", "Skip main menu and force keeper mode")
    ("seed", value<int>(), "Use given seed")
    ("replay", value<string>(), "Replay game from file");
  variables_map vars;
  store(parse_command_line(argc, argv, options), vars);
  if (vars.count("help")) {
    std::cout << options << endl;
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
  Options::init("options.txt");
  int seed = vars.count("seed") ? vars["seed"].as<int>() : time(0);
  int forceMode = vars.count("force_keeper") ? 0 : -1;
  bool genExit = vars.count("gen_world_exit");
  if (vars.count("replay")) {
    string fname = vars["replay"].as<string>();
    Debug() << "Reading from " << fname;
    seed = convertFromString<int>(fname.substr(lognamePref.size()));
    Random.init(seed);
    input.reset(new CompressedInput(fname));
    view.reset(View::createReplayView(input->getArchive()));
  } else {
#ifndef RELEASE
    Random.init(seed);
    string fname(lognamePref);
    fname += convertToString(seed);
    output.reset(new CompressedOutput(fname));
    Debug() << "Writing to " << fname;
  Debug() << int(sizeof(SquareType));
    view.reset(View::createLoggingView(output->getArchive()));
#else
    view.reset(View::createDefaultView());
    ofstream("seeds.txt", std::ios_base::app) << seed << endl;
#endif
  } 
  int lastIndex = 0;
  std::atomic<bool> viewInitialized(false);
  ScriptContext::init();
  Tile::initialize();
  AsyncLoop renderingThread([&] {
      view->initialize();
      viewInitialized = true;
    },
    [&] {
      view->refreshView();
      sf::sleep(sf::milliseconds(1));
    }
  );
  while (!viewInitialized);
  Jukebox jukebox("music/peaceful1.ogg");
  jukebox.addTrack(Jukebox::PEACEFUL, "music/peaceful2.ogg");
  jukebox.addTrack(Jukebox::PEACEFUL, "music/peaceful3.ogg");
  jukebox.addTrack(Jukebox::PEACEFUL, "music/peaceful4.ogg");
  jukebox.addTrack(Jukebox::PEACEFUL, "music/peaceful5.ogg");
  jukebox.addTrack(Jukebox::BATTLE, "music/battle1.ogg");
  jukebox.addTrack(Jukebox::BATTLE, "music/battle2.ogg");
  jukebox.addTrack(Jukebox::BATTLE, "music/battle3.ogg");
  jukebox.addTrack(Jukebox::BATTLE, "music/battle4.ogg");
  jukebox.addTrack(Jukebox::BATTLE, "music/battle5.ogg");
  view->setJukebox(&jukebox);
  if (!WindowView::areTilesOk())
    view->presentText("", "You are playing a version of KeeperRL without graphical tiles. "
        "Besides lack of graphics and music, this "
        "is the same exact game as the full version. If you'd like to buy the full version, "
        "please visit keeperrl.com.\n \nYou can also get it by donating to any wildlife charity. More information "
        "on the website.");
  while (1) {
    view->reset();
    clearAndInitialize();
    auto choice = forceMode > -1 ? Optional<int>(forceMode) : view->chooseFromList("", {
        View::ListElem("Choose your role:", View::TITLE),
          "Keeper", "Adventurer",
        View::ListElem("Or simply:", View::TITLE),
          "Load a game", "Change settings", "View high scores", "View credits", "Quit"},
        lastIndex, View::MAIN_MENU);
    if (!choice)
      continue;
    lastIndex = *choice;
    Optional<string> savedGame;
    if (choice == 1) {
      savedGame = chooseSaveFile({
          {GameType::RETIRED_KEEPER, "Retired keeper games:"}},
          "No retired games found.", view.get());
      if (!savedGame)
        continue;
    }
    if (choice == 2) {
      savedGame = chooseSaveFile({
          {GameType::KEEPER, "Keeper games:"},
          {GameType::ADVENTURER, "Adventurer games:"}},
          "No save files found.", view.get());
      if (!savedGame)
        continue;
    }
    if (choice == 3) {
      Options::handle(view.get(), OptionSet::GENERAL);
      continue;
    }
    if (choice == 0 && forceMode == -1) {
      if (!Options::handleOrExit(view.get(), OptionSet::KEEPER, -1))
        continue;
    } 
    if (choice == 4) {
      unique_ptr<Model> m(new Model(view.get()));
      m->showHighscore();
      continue;
    }
    if (choice == 5) {
      unique_ptr<Model> m(new Model(view.get()));
      m->showCredits();
      continue;
    }
    if (choice == 6) {
      return 0;
    }
    unique_ptr<Model> model;
    string ex;
    atomic<bool> ready(false);
    view->displaySplash(savedGame ? View::LOADING : View::CREATING, ready);
    for (int i : Range(5)) {
      try {
        if (savedGame) {
          model = loadGame(*savedGame, choice == 2);
        }
        else {
          CHECK(choice == 0);
          model.reset(Model::collectiveModel(view.get()));
        }
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
    ready = true;
    model->setView(view.get());
    if (genExit) {
      model.reset();
      clearAndInitialize();
      return 0;
    }
    try {
      const double gameTimeStep = 0.01;
      const int stepTimeMilli = 3;
      Intervalometer meter(stepTimeMilli);
      double totTime = model->getTime();
      while (1) {
        model->update(totTime);
        if (model->isTurnBased())
          ++totTime;
        else
          totTime += min(1.0, double(meter.getCount()) * gameTimeStep);
      }
    } 
#ifdef RELEASE
    catch (string ex) {
      view->presentText("Sorry!", "The game has crashed with the following error:\n \n" + ex +
          "\n \nIf you would be so kind, please send the file \'crash.log\'"
          " and a description of the circumstances to rusolis@poczta.fm Thanks!");
      saveExceptionLine("crash.log", ex);
    }
#endif
    catch (GameOverException ex) {
    }
    catch (SaveGameException ex) {
      atomic<bool> ready(false);
      view->displaySplash(View::SAVING, ready);
      string id = model->getGameIdentifier() + getSaveSuffix(ex.type);
      saveGame(std::move(model), id);
      ready = true;
    }
  }
  return 0;
}

