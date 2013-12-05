#ifndef _REPLAY_VIEW
#define _REPLAY_VIEW


template <class T>
class ReplayView : public T {
  public:
    ReplayView(ifstream& iff) : input(iff) {
    }

    virtual void close() override {
      input.close();
      T::close();
    }

    virtual void addMessage(const string&) override {
    }

    virtual void presentList(const string& title, const vector<string>& options, bool scrollDown) override {
    }

    virtual void addImportantMessage(const string&) override {
    }

    virtual Action getAction() override {
      T::getAction();
 //     usleep(300000);
      string method;
      Action action;
      input >> method >> action;
      if (method == "exit")
        exit(0);
      CHECKEQ(method, "getAction");
      return action;
    }

    virtual Optional<int> chooseFromList(const string& title, const vector<string>& options, int index) override {
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

    virtual Optional<int> getNumber(const string& title, int max) {
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
    ifstream& input;
};

#endif
