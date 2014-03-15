#ifndef _LOGGING_VIEW
#define _LOGGING_VIEW


template <class T>
class LoggingView : public T {
  public:
    LoggingView(ofstream& of) : output(of) {
    }

    virtual void close() override {
      output.close();
      T::close();
    }

    virtual CollectiveAction getClick(double time) override {
      CollectiveAction res = T::getClick(time);
      if (res.getType() != CollectiveAction::IDLE) {
        output << "getClick " << res << " " << time << endl;
        output.flush();
      }
      return res;
    }

    virtual Action getAction() override {
      Action res = T::getAction();
      output << "getAction " << res << endl;
      output.flush();
      return res;
    }

    virtual Optional<int> chooseFromList(const string& title, const vector<View::ListElem>& options, int index,
        Optional<ActionId> action) override {
      auto res = T::chooseFromList(title, options, index, action);
      output << "chooseFromList ";
      if (res)
        output << *res << endl;
      else
        output << "nothing" << endl;
      output.flush();
      return res;
    }

    virtual Optional<Vec2> chooseDirection(const string& message) override {
      auto res = T::chooseDirection(message);
      output << "chooseDirection ";
      if (res)
        output << res->x << "," << res->y << endl;
      else
        output << "nothing" << endl;
      output.flush();
      return res;
    }

    virtual bool yesOrNoPrompt(const string& message) override {
      auto res = T::yesOrNoPrompt(message);
      output << "yesOrNoPrompt " << res << endl;
      output.flush();
      return res;
    }

    virtual Optional<int> getNumber(const string& title, int max, int increments) override {
      auto res = T::getNumber(title, max, increments);
      output << "getNumber ";
      if (res)
        output << *res << endl;
      else
        output << "nothing" << endl;
      output.flush();
      return res;
    }
  private:
    ofstream& output;
};

#endif
