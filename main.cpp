#include "stdafx.h"

#include <ctime>
#include <locale>
#include <sys/types.h>
#include <sys/stat.h>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include "gzstream.h"

#include "dirent.h"

#include "view.h"
#include "model.h"
#include "quest.h"
#include "tribe.h"
#include "message_buffer.h"
#include "statistics.h"
#include "options.h"
#include "technology.h"
#include "gui_elem.h"
#include "music.h"

using namespace boost::iostreams;

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
    if (name.size() > suffix.size() && name.substr(name.size() - suffix.size()) == suffix) {
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
  auto ind = view->chooseFromList("Choose game", options);
  if (ind)
    return allFiles[*ind].path;
  else
    return Nothing();
}

static unique_ptr<Model> loadGame(const string& filename, bool eraseFile) {
  unique_ptr<Model> model;
  {
    igzstream ifs(filename.c_str());
    CHECK(ifs.good()) << "File not found: " << filename;
    filtering_streambuf<input> in;
    in.push(ifs);
    boost::archive::binary_iarchive ia(in);
    Serialization::registerTypes(ia);
    ia >> BOOST_SERIALIZATION_NVP(model);
  }
#ifdef RELEASE
  if (eraseFile)
    CHECK(!remove(filename.c_str()));
#endif
  return model;
}

static void saveGame(unique_ptr<Model> model, const string& filename) {
  ogzstream ofs(filename.c_str());
  boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
  out.push(ofs);
  boost::archive::binary_oarchive oa(ofs);
  Serialization::registerTypes(oa);
  oa << BOOST_SERIALIZATION_NVP(model);
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

int main(int argc, char* argv[]) {
  View* view;
  ifstream input;
  ofstream output;
  string lognamePref = "log";
  Debug::init();
  Options::init("options.txt");
  int seed = time(0);
  int forceMode = -1;
  bool genExit = false;
  if (argc == 3) {
    genExit = true;
    if (argv[2][0] == 'a')
      forceMode = 1;
    else
      forceMode = 0;
  }
  if (argc == 2 && argv[1][0] == 'd')
    forceMode = 1;
  else
  if (argc == 2 && argv[1][0] != 'l') {
    seed = convertFromString<int>(argv[1]);
    argc = 1;
  }
  if (argc == 1 || forceMode > -1) {
    Random.init(seed);
    string fname(lognamePref);
    fname += convertToString(seed);
    output.open(fname);
    CHECK(output.is_open());
    Debug() << "Writing to " << fname;
    view = View::createLoggingView(output);
  } else {
    string fname = argv[1];
    Debug() << "Reading from " << fname;
    seed = convertFromString<int>(fname.substr(lognamePref.size()));
    Random.init(seed);
    input.open(fname);
    CHECK(input.is_open());
    view = View::createReplayView(input);
  }
  //Table<bool> splash = readSplashTable("splash.map");
  int lastIndex = 0;
  view->initialize();
  jukebox.initialize("intro.ogg", "peaceful.ogg", "battle.ogg");
  GuiElem::initialize("frame.png");
  while (1) {
    Item::identifyEverything();
    Quests::clearAll();
    Creature::initialize();
    Tribes::clearAll();
    Technology::clearAll();
    Skill::clearAll();
    EventListener::initialize();
    Tribe::init();
    Technology::init();
    Skill::init();
    Statistics::init();
    NameGenerator::init("first_names.txt", "aztec_names.txt", "creatures.txt",
        "artifacts.txt", "world.txt", "town_names.txt", "dwarfs.txt", "gods.txt", "demons.txt", "dogs.txt",
        "insults.txt");
    ItemFactory::init();
    bool modelReady = false;
    messageBuffer.initialize(view);
    view->reset();
    auto choice = forceMode > -1 ? Optional<int>(forceMode) : view->chooseFromList("", {
        View::ListElem("Choose your role:", View::TITLE), "Keeper", "Adventurer", "Adventurer vs. Keeper",
        View::ListElem("Or simply:", View::TITLE), "Load a game", "Change settings", "View high scores", "Quit"},
        lastIndex, View::MAIN_MENU);
    if (!choice)
      continue;
    lastIndex = *choice;
    Optional<string> savedGame;
    if (choice == 2) {
      savedGame = chooseSaveFile({
          {GameType::RETIRED_KEEPER, "Retired keeper games:"}},
          "No retired games found.", view);
      if (!savedGame)
        continue;
    }
    if (choice == 3) {
      savedGame = chooseSaveFile({
          {GameType::KEEPER, "Keeper games:"},
          {GameType::ADVENTURER, "Adventurer games:"}},
          "No save files found.", view);
      if (!savedGame)
        continue;
    }
    if (choice == 4) {
      Options::handle(view, OptionSet::GENERAL);
      continue;
    }
    if (choice == 0) {
      if (!Options::handleOrExit(view, OptionSet::KEEPER, -1))
        continue;
    } 
    if (choice == 1) {
      if (!Options::handleOrExit(view, OptionSet::ADVENTURER, -1))
        continue;
    } 
    if (choice == 5) {
      unique_ptr<Model> m(new Model(view));
      m->showHighscore();
      continue;
    }
    if (choice == 6)
      exit(0);
    unique_ptr<Model> model;
    string ex;
    thread t([&] {
      for (int i : Range(5)) {
        try {
          if (savedGame) {
            model = loadGame(*savedGame, choice == 3);
          }
          else if (choice == 1)
            model.reset(Model::heroModel(view));
          else {
            CHECK(choice == 0);
            model.reset(Model::collectiveModel(view));
          }
          break;
        } catch (string s) {
          ex = s;
        }
      }
      modelReady = true;
    });
    view->displaySplash(savedGame ? View::LOADING : View::CREATING, modelReady);
    t.join();
    model->setView(view);
    if (genExit)
      break;
    if (!model) {
      view->presentText("Sorry!", "World generation permanently failed with the following error:\n \n" + ex +
          "\n \nIf you would be so kind, please send the file \'crash.log\'"
          " to rusolis@poczta.fm Thanks!");
      saveExceptionLine("crash.log", ex);
    }
    int var = 0;
    try {
      while (1) {
        if (model->isTurnBased())
          model->update(var++);
        else
          model->update(double(view->getTimeMilli()) / 300);
      }
    } catch (GameOverException ex) {
    } catch (SaveGameException ex) {
      bool ready = false;
      thread t([&] {
        saveGame(std::move(model), model->getGameIdentifier() + getSaveSuffix(ex.type));
        ready = true; });
      view->displaySplash(View::SAVING, ready);
      t.join();
    }
#ifdef RELEASE
    catch (string ex) {
      view->presentText("Sorry!", "The game has crashed with the following error:\n \n" + ex +
          "\n \nIf you would be so kind, please send the file \'crash.log\'"
          " and a description of the circumstances to rusolis@poczta.fm Thanks!");
      saveExceptionLine("crash.log", ex);
    }
#endif
  }
  return 0;
}

