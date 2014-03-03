#include "stdafx.h"

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
//#include <boost/iostreams/filter/bzip2.hpp>

#include "dirent.h"

#include "view.h"
#include "model.h"
#include "quest.h"
#include "tribe.h"
#include "message_buffer.h"
#include "statistics.h"
#include "options.h"


using namespace boost::iostreams;

static vector<string> getSaveFiles(const string& suffix) {
  vector<string> ret;
  struct dirent *ent;
  DIR* dir = opendir(".");
  CHECK(dir) << "Couldn't open current directory";
  while (dirent* ent = readdir(dir)) {
    string name(ent->d_name);
    if (name.size() > suffix.size() && name.substr(name.size() - suffix.size()) == suffix)
      ret.push_back(name);
  }
  closedir(dir);
  return ret;
}

static string getSaveSuffix(Model::GameType t) {
  switch (t) {
    case Model::KEEPER: return ".kep";
    case Model::ADVENTURER: return ".adv";
  }
  FAIL << "wef";
  return "";
}

static Optional<string> chooseSaveFile(View* view) {
  vector<string> filesKeeper = getSaveFiles(getSaveSuffix(Model::KEEPER));
  vector<string> filesAdventurer = getSaveFiles(getSaveSuffix(Model::ADVENTURER));
  if (filesKeeper.empty() && filesAdventurer.empty()) {
    view->presentText("", "No save files found.");
    return Nothing();
  }
  vector<View::ListElem> options;
  if (!filesKeeper.empty()) {
    auto removeSuf = [&] (const string& s) { return s.substr(0, s.size() - getSaveSuffix(Model::KEEPER).size()); };
    options.emplace_back("Keeper games:", View::TITLE);
    append(options, View::getListElem(transform2<string>(filesKeeper, removeSuf)));
  }
  if (!filesAdventurer.empty()) {
    auto removeSuf = [&] (const string& s) { return s.substr(0, s.size() - getSaveSuffix(Model::ADVENTURER).size());};
    options.emplace_back("Adventurer games:", View::TITLE);
    append(options, View::getListElem(transform2<string>(filesAdventurer, removeSuf)));
  }
  auto ind = view->chooseFromList("Choose game", options);
  if (ind)
    return concat(filesKeeper, filesAdventurer)[*ind];
  else
    return Nothing();
}

static unique_ptr<Model> loadGame(const string& filename) {
  unique_ptr<Model> model;
  ifstream ifs(filename);
  CHECK(ifs.good()) << "File not found: " << filename;
  filtering_streambuf<input> in;
 // in.push(bzip2_compressor());
  in.push(ifs);
  boost::archive::binary_iarchive ia(in);
  Serialization::registerTypes(ia);
  ia >> BOOST_SERIALIZATION_NVP(model);
  return model;
}

static void saveGame(unique_ptr<Model> model, const string& filename) {
  ofstream ofs(filename);
  boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
 // out.push(zlib_compressor());
  out.push(ofs);
  boost::archive::binary_oarchive oa(ofs);
  Serialization::registerTypes(oa);
  oa << BOOST_SERIALIZATION_NVP(model);
}

int main(int argc, char* argv[]) {
  View* view;
  ifstream input;
  ofstream output;
  string lognamePref = "log";
  Debug::init();
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
  int lastIndex = 0;
  view->initialize();
  while (1) {
    Tribe::init();
    Item::identifyEverything();
    EventListener::initialize();
    Statistics::init();
    Options::init("options.txt");
    NameGenerator::init("first_names.txt", "aztec_names.txt", "creatures.txt",
        "artifacts.txt", "world.txt", "town_names.txt", "dwarfs.txt", "gods.txt", "demons.txt", "dogs.txt");
    ItemFactory::init();
    bool modelReady = false;
    messageBuffer.initialize(view);
    view->reset();
    auto choice = forceMode > -1 ? Optional<int>(forceMode) : view->chooseFromList("", {
        View::ListElem("Choose your profession:", View::TITLE), "Keeper", "Adventurer",
        View::ListElem("Or simply:", View::TITLE), "Load a game", "Change options", "View high scores", "Quit"}, lastIndex);
    if (!choice)
      continue;
    lastIndex = *choice;
    Optional<string> savedGame;
    if (choice == 2) {
      savedGame = chooseSaveFile(view);
      if (!savedGame)
        continue;
    }
    if (choice == 3) {
      Options::handle(view, false);
      continue;
    }
    if (choice == 4) {
      Model* m = new Model(view);
      m->showHighscore();
      continue;
    }
    if (choice == 5)
      exit(0);
    unique_ptr<Model> model;
    string ex;
    thread t([&] {
      for (int i : Range(5)) {
        try {
          if (savedGame) {
            model = loadGame(*savedGame);
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
    if (!model)
      view->presentText("Sorry!", "World generation permanently failed with the following error:\n \n" + ex +
          "\n \nIf you would be so kind, please send this line to rusolis@poczta.fm Thanks!");

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
        saveGame(std::move(model), model->getGameIdentifier() + getSaveSuffix(model->getGameType()));
        ready = true; });
      view->displaySplash(View::SAVING, ready);
      t.join();
    }
#ifdef RELEASE
    catch (string ex) {
      view->presentText("Sorry!", "The game has crashed with the following error:\n \n" + ex +
          "\n \nIf you would be so kind, please send this line and a description of the circumstances to "
          " rusolis@poczta.fm Thanks!");
    }
#endif
  }
  return 0;
}

