#include "stdafx.h"

#include "view.h"
#include "model.h"
#include "quest.h"
#include "tribe.h"
#include "message_buffer.h"
#include "statistics.h"

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
  while (1) {
    Tribe::init();
    Item::identifyEverything();
    EventListener::initialize();
    Statistics::init();
    NameGenerator::init("first_names.txt", "aztec_names.txt", "creatures.txt",
        "artifacts.txt", "world.txt", "town_names.txt", "dwarfs.txt", "gods.txt", "demons.txt", "dogs.txt");
    ItemFactory::init();
    bool modelReady = false;
    messageBuffer.initialize(view);
    string heroName = NameGenerator::firstNames.getNext();
    view->initialize();
    auto choice = view->chooseFromList("", {
        View::ListElem("Choose your profession:", View::TITLE), "Keeper", "Adventurer",
        View::ListElem("Or simply:", View::TITLE), "View high scores", "Quit"});
    if (!choice)
      continue;
    if (choice == 3)
      exit(0);
    if (choice == 2) {
      Model* m = new Model(view);
      m->showHighscore();
      continue;
    }
    dwarf = (choice == 1);
    unique_ptr<Model> model;
    thread t = !dwarf ? (thread([&] { model.reset(Model::collectiveModel(view)); modelReady = true; })) :
      (thread([&] { model.reset(Model::heroModel(view, heroName)); modelReady = true; }));
    view->displaySplash(modelReady);
    t.join();
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
  }
  return 0;
}

