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

#ifndef _REPLAY_VIEW
#define _REPLAY_VIEW

#include "logging_view.h"

template <class T>
class ReplayView : public T {
  public:
    template <typename... Args>
    ReplayView(InputArchive& iff, Args... args) : T(args...),  input(iff) {
    }

    virtual void close() override {
      T::close();
    }

    virtual void drawLevelMap(const CreatureView*) override {
      return;
    }

    bool equal(double a, double b) {
      return fabs(a - b) < 0.001;
    }

    void checkMethod(LoggingToken token) {
      LoggingToken t;
      input >> t;
      CHECKEQ(int(t), int(token));
    }

    virtual int getTimeMilli() override {
      checkMethod(LoggingToken::GET_TIME);
      int time;
      input >> time;
      return time;
    }

    virtual void presentText(const string& title, const string& text) override {
      return;
    }

    virtual bool isClockStopped() override {
      T::isClockStopped();
      checkMethod(LoggingToken::IS_CLOCK_STOPPED);
      bool ret;
      input >> ret;
      return ret;
    }

    virtual UserInput getAction() override {
      T::getAction();
      checkMethod(LoggingToken::GET_ACTION);
      UserInput action;
      input >> action;
      return action;
    }

    virtual Optional<int> chooseFromList(const string& title, const vector<View::ListElem>& options, int index,
        View::MenuType, int* scrollPos, Optional<UserInputId> a) override {
      checkMethod(LoggingToken::CHOOSE_FROM_LIST);
      Optional<int> action;
      input >> action;
      return action;
    }

    virtual Optional<Vec2> chooseDirection(const string& message) override {
      checkMethod(LoggingToken::CHOOSE_DIRECTION);
      Optional<Vec2> action;
      input >> action;
      return action;
    }

    virtual bool yesOrNoPrompt(const string& message) override {
      checkMethod(LoggingToken::YES_OR_NO_PROMPT);
      bool action;
      input >> action;
      return action;
    }

    virtual Optional<int> getNumber(const string& title, int min, int max, int increments) override {
      checkMethod(LoggingToken::GET_NUMBER);
      Optional<int> action;
      input >> action;
      return action;
    }
  private:
    InputArchive& input;
};

#endif
