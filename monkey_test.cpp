#include "stdafx.h"

using namespace std;

class MonkeyView : public View {
  public:
  MonkeyView() : View(100, 80) {}
  virtual void initialize() {
  }
  virtual void close() override {}
  virtual void addMessage(const string& message) override {}
  virtual void addImportantMessage(const string& message) override {}
  virtual Action getAction() override {
    static int cnt = 0;
    Debug() << ++cnt << " moves.";
    return (Action)Random.getRandom(24);
  }
  virtual Optional<int> chooseFromList(const string& title, const vector<string>& options) override {
    if (options.size() == 0)
      return Nothing();
    int ind;
    do {
      ind = Random.getRandom(options.size());
    } while (View::hasTitlePrefix(options[ind]));
    int titles = 0;
    for (int i = 0; i < ind; ++i)
      if (View::hasTitlePrefix(options[i]))
        ++titles;
    return ind - titles;
  }
  virtual Optional<int> getNumber(const string&, int max) override {
    return Random.getRandom(1, max + 1);
  }
  virtual void presentList(const string& title, const vector<string>& options, bool) override {
  }
  virtual Optional<Vec2> chooseDirection(const string&) override {
    return Vec2::neighbors8()[Random.getRandom(8)];
  }
  virtual bool yesOrNoPrompt(const string&) override {
    return Random.roll(2);
  }
  virtual void onRefreshView(const vector<pair<Vec2, ViewObject> >& objects) override {}
  virtual void onPlaceObject(Vec2 pos, const ViewObject& object) override {}
  virtual void onMoveObject(Vec2 from, Vec2 to, const ViewObject& object) override {}
  virtual void onEraseObject(Vec2 pos, const ViewObject& object) override {}
};

int main() {
  Quest::initialize();
  ItemFactory::init();
  Tribe::init();
  View* view = new MonkeyView();
  view->initialize();
  messageBuffer.initialize(view);
  Model model(view);
  while (1) {
    model.update();
  }
  return 0;
}
