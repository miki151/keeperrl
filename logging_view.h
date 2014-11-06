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


template <class T>
class LoggingView : public T {
  public:
    LoggingView(binary_oarchive& of) : output(of) {
    }

    virtual void close() override {
 //     output.close();
      T::close();
    }

    virtual int getTimeMilli() override {
      int res = T::getTimeMilli();
      output << "getTime" << res;
      return res;
    }

    virtual bool isClockStopped() override {
      bool res = T::isClockStopped();
      output << "isClockStopped" << res;
      return res;
    }
    
    virtual UserInput getAction() override {
      UserInput res = T::getAction();
      output << "getAction" << res;
      return res;
    }

    virtual Optional<int> chooseFromList(const string& title, const vector<View::ListElem>& options, int index,
        View::MenuType type, int* scrollPos, Optional<UserInputId> action) override {
      auto res = T::chooseFromList(title, options, index, type, scrollPos, action);
      output << "chooseFromList" << res;
      return res;
    }

    virtual Optional<Vec2> chooseDirection(const string& message) override {
      auto res = T::chooseDirection(message);
      output << "chooseDirection" << res;
      return res;
    }

    virtual bool yesOrNoPrompt(const string& message) override {
      auto res = T::yesOrNoPrompt(message);
      output << "yesOrNoPrompt" << res;
      return res;
    }

    virtual Optional<int> getNumber(const string& title, int min, int max, int increments) override {
      auto res = T::getNumber(title, min, max, increments);
      output << "getNumber" << res;
      return res;
    }
  private:
    binary_oarchive& output;
};

#endif
