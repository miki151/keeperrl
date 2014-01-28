#include "stdafx.h"

#include "view.h"
#include "model.h"
#include "quest.h"
#include "tribe.h"
#include "message_buffer.h"

using namespace std;

int main(int argc, char* argv[]) {
  View* view;
  ifstream input;
  ofstream output;
  string lognamePref = "log";
  Debug::init();
  int seed = time(0);
  bool dwarf = false;
  if (argc == 2 && argv[1][0] == 'd')
    dwarf = true;
  else
  if (argc == 2 && argv[1][0] != 'l') {
    seed = convertFromString<int>(argv[1]);
    argc = 1;
  }
  if (argc == 1 || dwarf) {
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
  Tribe::init();
  Item::identifyEverything();
  NameGenerator::init("first_names.txt", "aztec_names.txt", "creatures.txt",
      "artifacts.txt", "world.txt", "town_names.txt", "dwarfs.txt", "gods.txt", "demons.txt", "dogs.txt");
  ItemFactory::init();
  bool modelReady = false;
  messageBuffer.initialize(view);
  string heroName = NameGenerator::firstNames.getNext();
  view->initialize();
  while (1) {
    auto choice = view->chooseFromList("Welcome to KeeperRL.", { "Keeper mode", "Adventure mode", "Highscores"});
    if (!choice)
      exit(0);
    if (choice == 2) {
      Model* m = new Model(view);
      m->showHighscore();
      continue;
    }
    dwarf = (choice == 1);
    Model* model;
    thread t = !dwarf ? (thread([&] { model = Model::collectiveModel(view); modelReady = true; })) :
      (thread([&] { model = Model::heroModel(view, heroName); modelReady = true; }));
    view->displaySplash(modelReady);
    int var = 0;
    view->setTimeMilli(0);
    while (1) {
      if (model->isTurnBased())
        model->update(var++);
      else
        model->update(double(view->getTimeMilli()) / 300);
    }
  }
  return 0;
}

