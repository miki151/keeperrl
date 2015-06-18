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
#include "pantheon.h"

class ReplayView : public View {
  public:
    ReplayView(InputArchive& iff, View* d) : input(iff), delegate(d) {
    }

    template <class T>
    T readValue(LoggingToken token) {
      LoggingToken t;
      input >> BOOST_SERIALIZATION_NVP(t);
      CHECKEQ(int(t), int(token));
      T ret;
      input >> BOOST_SERIALIZATION_NVP(ret);
      return ret;
    }

    virtual int getTimeMilli() override {
      return readValue<int>(LoggingToken::GET_TIME);
    }

    virtual int getTimeMilliAbsolute() override {
      return readValue<int>(LoggingToken::GET_TIME_ABSOLUTE);
    }

    virtual double getGameSpeed() override {
      return readValue<double>(LoggingToken::GET_GAME_SPEED);
    }

    virtual bool isClockStopped() override {
      return readValue<bool>(LoggingToken::IS_CLOCK_STOPPED);
    }

    virtual UserInput getAction() override {
      return readValue<UserInput>(LoggingToken::GET_ACTION);
    }

    virtual optional<int> chooseFromList(const string& title, const vector<View::ListElem>& options, int index,
        View::MenuType, double* scrollPos, optional<UserInputId> a) override {
      return readValue<optional<int>>(LoggingToken::CHOOSE_FROM_LIST);
    }

    virtual View::GameTypeChoice chooseGameType() override {
      return readValue<View::GameTypeChoice>(LoggingToken::CHOOSE_GAME_TYPE);
    }

    virtual optional<Vec2> chooseDirection(const string& message) override {
      return readValue<optional<Vec2>>(LoggingToken::CHOOSE_DIRECTION);
    }

    virtual bool yesOrNoPrompt(const string& message, bool defNo) override {
      return readValue<bool>(LoggingToken::YES_OR_NO_PROMPT);
    }

    virtual optional<int> getNumber(const string& title, int min, int max, int increments) override {
      return readValue<optional<int>>(LoggingToken::GET_NUMBER);
    }

    virtual optional<View::MinionAction> getMinionAction(const vector<GameInfo::PlayerInfo>&,
        UniqueEntity<Creature>::Id& current) override {
      return readValue<optional<View::MinionAction>>(LoggingToken::GET_MINION_ACTION);
    }

    virtual optional<int> chooseItem(const vector<GameInfo::PlayerInfo>&, UniqueEntity<Creature>::Id& current,
        const vector<GameInfo::ItemInfo>& items, double* scrollpos) override {
      return readValue<optional<int>>(LoggingToken::CHOOSE_ITEM);
    }

    virtual bool travelInterrupt() override {
      return readValue<bool>(LoggingToken::TRAVEL_INTERRUPT);
    }

    virtual optional<string> getText(const string& title, const string& value, int maxLength,
        const string& hint = "") override {
      return readValue<optional<string>>(LoggingToken::GET_TEXT);
    }

    virtual void initialize() override {
      if (delegate)
        delegate->initialize();
    }

    virtual void reset() override {
      if (delegate)
        delegate->reset();
    }

    virtual void displaySplash(const ProgressMeter& m, SplashType type, function<void()> cancelFun) override {
 //     if (delegate)
 //       delegate->displaySplash(m, type, cancelFun);
    }

    virtual void clearSplash() override {
 //     if (delegate)
 //       delegate->clearSplash();
    }

    virtual void close() override {
      if (delegate)
        delegate->close();
    }

    virtual void refreshView() override {
      if (delegate)
        delegate->refreshView();
    }

    virtual void presentText(const string& title, const string& text) override {
 //     if (delegate)
 //       delegate->presentText(title, text);
    };

    virtual void presentList(const string& title, const vector<ListElem>& options, bool scrollDown,
        MenuType menu, optional<UserInputId> exitAction) override {
//      if (delegate)
//        delegate->presentList(title, options, scrollDown, menu, exitAction);
    }

    virtual void animateObject(vector<Vec2> trajectory, ViewObject object) override {
      if (delegate)
        delegate->animateObject(trajectory, object);
    }

    virtual void animation(Vec2 pos, AnimationId id) override {
      if (delegate)
        delegate->animation(pos, id);
    }

    virtual void stopClock() override {
      if (delegate)
        delegate->stopClock();
    }

    virtual void continueClock() override {
      if (delegate)
        delegate->continueClock();
    }

    virtual void updateView(const CreatureView* creatureView, bool noRefresh) override {
      if (delegate)
        delegate->updateView(creatureView, noRefresh);
    }

    virtual void drawLevelMap(const CreatureView* view) override {
//      if (delegate)
//        delegate->drawLevelMap(view);
    }

    virtual void resetCenter() override {
      if (delegate)
        delegate->resetCenter();
    }

  private:
    InputArchive& input;
    View* delegate;
};

#endif
