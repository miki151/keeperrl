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
  GET_GAME_SPEED,
  CHOOSE_GAME_TYPE,
  GET_MINION_ACTION,
  GET_TIME_ABSOLUTE,
  CHOOSE_ITEM,
  CHOOSE_RECRUIT,
  TRAVEL_INTERRUPT,
  GET_TEXT
};

#include "view.h"

class LoggingView : public View {
  public:
    LoggingView(OutputArchive& of, View* d) : output(of), delegate(d) {
    }

    template<class T>
    T logAndGet(T val, LoggingToken token) {
      output << BOOST_SERIALIZATION_NVP(token) << BOOST_SERIALIZATION_NVP(val);
      return val;
    }

    virtual int getTimeMilli() override {
      return logAndGet(delegate->getTimeMilli(), LoggingToken::GET_TIME);
    }

    virtual int getTimeMilliAbsolute() override {
      return logAndGet(delegate->getTimeMilliAbsolute(), LoggingToken::GET_TIME_ABSOLUTE);
    }

    virtual double getGameSpeed() override {
      return logAndGet(delegate->getGameSpeed(), LoggingToken::GET_GAME_SPEED);
    }

    virtual bool isClockStopped() override {
      return logAndGet(delegate->isClockStopped(), LoggingToken::IS_CLOCK_STOPPED);
    }
    
    virtual UserInput getAction() override {
      return logAndGet(delegate->getAction(), LoggingToken::GET_ACTION);
    }

    virtual optional<int> chooseFromList(const string& title, const vector<ListElem>& options, int index,
        MenuType type, double* scrollPos, optional<UserInputId> action) override {
      return logAndGet(delegate->chooseFromList(title, options, index, type, scrollPos, action),
          LoggingToken::CHOOSE_FROM_LIST);
    }

    virtual GameTypeChoice chooseGameType() override {
      return logAndGet(delegate->chooseGameType(), LoggingToken::CHOOSE_GAME_TYPE);
    }

    virtual optional<Vec2> chooseDirection(const string& message) override {
      return logAndGet(delegate->chooseDirection(message), LoggingToken::CHOOSE_DIRECTION);
    }

    virtual bool yesOrNoPrompt(const string& message, bool defNo) override {
      return logAndGet(delegate->yesOrNoPrompt(message, defNo), LoggingToken::YES_OR_NO_PROMPT);
    }

    virtual optional<int> getNumber(const string& title, int min, int max, int increments) override {
      return logAndGet(delegate->getNumber(title, min, max, increments), LoggingToken::GET_NUMBER);
    }

    virtual optional<MinionAction> getMinionAction(const vector<PlayerInfo>& info,
        UniqueEntity<Creature>::Id& current) override {
      return logAndGet(delegate->getMinionAction(info, current), LoggingToken::GET_MINION_ACTION);
    }

    virtual optional<int> chooseItem(const vector<PlayerInfo>& info, UniqueEntity<Creature>::Id& current,
        const vector<ItemInfo>& items, double* scrollpos) override {
      return logAndGet(delegate->chooseItem(info, current, items, scrollpos), LoggingToken::CHOOSE_ITEM);
    }

    virtual optional<UniqueEntity<Creature>::Id> chooseRecruit(const string& title, pair<ViewId, int> budget,
        const vector<CreatureInfo>& c, double* scrollPos) override {
      return logAndGet(delegate->chooseRecruit(title, budget, c, scrollPos), LoggingToken::CHOOSE_RECRUIT);
    }

    virtual bool travelInterrupt() override {
      return logAndGet(delegate->travelInterrupt(), LoggingToken::TRAVEL_INTERRUPT);
    }

    virtual optional<string> getText(const string& title, const string& value, int maxLength,
        const string& hint = "") override {
      return logAndGet(delegate->getText(title, value, maxLength, hint), LoggingToken::GET_TEXT);
    }

    virtual void presentHighscores(const vector<HighscoreList>&) override {
    }

    virtual void initialize() override {
      delegate->initialize();
    }

    virtual void reset() override {
      delegate->reset();
    }

    virtual void displaySplash(const ProgressMeter& m, SplashType type, function<void()> cancelFun) override {
      delegate->displaySplash(m, type, cancelFun);
    }

    virtual void clearSplash() override {
      delegate->clearSplash();
    }

    virtual void close() override {
      delegate->close();
    }

    virtual void refreshView() override {
      delegate->refreshView();
    }

    virtual void presentText(const string& title, const string& text) override {
      delegate->presentText(title, text);
    };

    virtual void presentList(const string& title, const vector<ListElem>& options, bool scrollDown,
      MenuType menu, optional<UserInputId> exitAction) override {
      delegate->presentList(title, options, scrollDown, menu, exitAction);
    }

    virtual void animateObject(vector<Vec2> trajectory, ViewObject object) override {
      delegate->animateObject(trajectory, object);
    }

    virtual void animation(Vec2 pos, AnimationId id) override {
      delegate->animation(pos, id);
    }

    virtual void stopClock() override {
      delegate->stopClock();
    }

    virtual void continueClock() override {
      delegate->continueClock();
    }

    virtual void updateView(const CreatureView* creatureView, bool noRefresh) override {
      delegate->updateView(creatureView, noRefresh);
    }

    virtual void drawLevelMap(const CreatureView* view) override {
      delegate->drawLevelMap(view);
    }

    virtual void resetCenter() override {
      delegate->resetCenter();
    }

  private:
    OutputArchive& output;
    View* delegate;
};

#endif
