#ifndef _REPLAY_VIEW
#define _REPLAY_VIEW

class PushBackStream {
  public:
  PushBackStream(ifstream& in) : input(in) {}

  template <class T>
  PushBackStream& operator >> (T& elem) {
    CHECK(pushed.empty());
    input >> elem;
    return *this;
  }

  PushBackStream& operator >> (string& elem) {
    if (!pushed.empty()) {
      elem = pushed;
      pushed = "";
    } else
      input >> elem;
    return *this;
  }

  void pushBack(const string& a) {
    CHECK(pushed.empty());
    pushed = a;
  }

  void close() {
    input.close();
  }

  private:
  string pushed;
  ifstream& input;
};

template <class T>
class ReplayView : public T {
  public:
    ReplayView(ifstream& iff) : input(iff) {
    }

    virtual void close() override {
      input.close();
      T::close();
    }

    virtual void drawLevelMap(const Level*, const CreatureView*) {
      return;
    }

    bool equal(double a, double b) {
      return fabs(a - b) < 0.001;
    }

    virtual CollectiveAction getClick(double time) override {
      T::getClick(time);
      if (stackedAction) {
        if (equal(time, stackedTime)) {
          CollectiveAction ret = *stackedAction;
          stackedAction = Nothing();
          return ret;
        }
        CHECK(time < stackedTime);
        return CollectiveAction::IDLE;
      }
      string method;
      double actionTime;
      CollectiveAction action;
      input >> method;
      if (method != "getClick") {
        input.pushBack(method);
        return CollectiveAction::IDLE;
      }
      input >> action >> actionTime;
      Debug() << "get Click " << time << " " <<actionTime;
      CHECKEQ(method, "getClick");
      if (equal(time, actionTime))
        return action;
      else {
        CHECK(actionTime > time);
        CHECK(!stackedAction);
        stackedAction = action;
        stackedTime = actionTime;
        return CollectiveAction::IDLE;
      }
    }

    virtual Action getAction() override {
 //     T::getAction();
 //     usleep(300000);
      string method;
      Action action;
      input >> method >> action;
      if (method == "exit")
        exit(0);
      CHECKEQ(method, "getAction");
      return action;
    }

    virtual Optional<int> chooseFromList(const string& title, const vector<View::ListElem>& options, int index,
        Optional<ActionId> a) override {
      string method;
      string action;
      input >> method >> action;
      CHECKEQ(method, "chooseFromList");
      if (action == "nothing")
        return Nothing();
      else
        return convertFromString<int>(action);
    }

    virtual Optional<Vec2> chooseDirection(const string& message) override {
      string method;
      string action;
      input >> method >> action;
      CHECKEQ(method, "chooseDirection");
      if (action == "nothing")
        return Nothing();
      else {
        vector<string> s = split(action, ',');
        CHECKEQ((int)s.size(), 2);
        return Vec2(convertFromString<int>(s[0]), convertFromString<int>(s[1]));
      }
    }

    virtual bool yesOrNoPrompt(const string& message) override {
      string method;
      bool action;
      input >> method >> action;
      CHECKEQ(method, "yesOrNoPrompt");
      return action;
    }

    virtual Optional<int> getNumber(const string& title, int max, int increments) {
      string method;
      string action;
      input >> method >> action;
      CHECKEQ(method, "getNumber");
      if (action == "nothing")
        return Nothing();
      else
        return convertFromString<int>(action);
    }
  private:
    PushBackStream input;
    Optional<CollectiveAction> stackedAction;
    double stackedTime = -1000;
};

#endif
