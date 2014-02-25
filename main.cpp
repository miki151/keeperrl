#include "stdafx.h"

#include "view.h"
#include "model.h"
#include "quest.h"
#include "tribe.h"
#include "message_buffer.h"
#include "statistics.h"
#include "options.h"



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
    view->initialize();
    auto choice = forceMode > -1 ? Optional<int>(forceMode) : view->chooseFromList("", {
        View::ListElem("Choose your profession:", View::TITLE), "Keeper", "Adventurer",
        View::ListElem("Or simply:", View::TITLE), "Change options", "View high scores", "Quit"}, lastIndex);
    if (!choice)
      continue;
    lastIndex = *choice;
    if (choice == 2) {
      Options::handle(view, false);
      continue;
    }
    if (choice == 4)
      exit(0);
    if (choice == 3) {
      Model* m = new Model(view);
      m->showHighscore();
      continue;
    }
    unique_ptr<Model> model;
    string ex;
    thread t = (thread([&] {
      for (int i : Range(5)) {
        try {
          model.reset(choice == 1 ? Model::heroModel(view) : Model::collectiveModel(view));
          break;
        } catch (string s) {
          ex = s;
        }
      }
      modelReady = true;
    }));
    view->displaySplash(modelReady);
    t.join();
    if (genExit)
      break;
    if (!model)
      view->presentText("Sorry!", "World generation permanently failed with the following error:\n \n" + ex +
          "\n \nIf you would be so kind, please send this line to rusolis@poczta.fm Thanks!");

    int var = 0;
    view->setTimeMilli(0);
    try {
      while (1) {
        if (model->isTurnBased())
          model->update(var++);
        else
          model->update(double(view->getTimeMilli()) / 300);
      }
    } catch (GameOverException ex) {
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

