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

#ifndef _LOGGING_VIEW
#define _LOGGING_VIEW

enum class LoggingToken {
  GET_TIME,
  IS_CLOCK_STOPPED,
  GET_ACTION,
  CHOOSE_FROM_LIST,
  CHOOSE_DIRECTION,
  YES_OR_NO_PROMPT,
  GET_NUMBER,
};

template <class T>
class LoggingView : public T {
  public:
    template <typename... Args>
    LoggingView(OutputArchive& of, Args... args) : T(args...), output(of) {
    }

    virtual void close() override {
 //     output.close();
      T::close();
    }

    virtual int getTimeMilli() override {
      int res = T::getTimeMilli();
      auto token = LoggingToken::GET_TIME;
      output << token << res;
      return res;
    }

    virtual bool isClockStopped() override {
      bool res = T::isClockStopped();
      auto token = LoggingToken::IS_CLOCK_STOPPED;
      output << token << res;
      return res;
    }
    
    virtual UserInput getAction() override {
      UserInput res = T::getAction();
      auto token = LoggingToken::GET_ACTION;
      output << token << res;
      return res;
    }

    virtual Optional<int> chooseFromList(const string& title, const vector<View::ListElem>& options, int index,
        View::MenuType type, int* scrollPos, Optional<UserInputId> action) override {
      auto res = T::chooseFromList(title, options, index, type, scrollPos, action);
      auto token = LoggingToken::CHOOSE_FROM_LIST;
      output << token << res;
      return res;
    }

    virtual Optional<Vec2> chooseDirection(const string& message) override {
      auto res = T::chooseDirection(message);
      auto token = LoggingToken::CHOOSE_DIRECTION;
      output << token << res;
      return res;
    }

    virtual bool yesOrNoPrompt(const string& message) override {
      auto res = T::yesOrNoPrompt(message);
      auto token = LoggingToken::YES_OR_NO_PROMPT;
      output << token << res;
      return res;
    }

    virtual Optional<int> getNumber(const string& title, int min, int max, int increments) override {
      auto res = T::getNumber(title, min, max, increments);
      auto token = LoggingToken::GET_NUMBER;
      output << token << res;
      return res;
    }
  private:
    OutputArchive& output;
};

#endif
