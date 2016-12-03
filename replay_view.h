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

#pragma once

#include "util.h"
#include "logging_view.h"

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

    virtual milliseconds getTimeMilli() override {
      return milliseconds{readValue<int>(LoggingToken::GET_TIME)};
    }

    virtual milliseconds getTimeMilliAbsolute() override {
      return milliseconds{readValue<int>(LoggingToken::GET_TIME_ABSOLUTE)};
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

    virtual optional<int> chooseFromList(const string& title, const vector<ListElem>& options, int index,
        MenuType, double* scrollPos, optional<UserInputId> a) override {
      return readValue<optional<int>>(LoggingToken::CHOOSE_FROM_LIST);
    }

    virtual optional<GameTypeChoice> chooseGameType() override {
      return readValue<GameTypeChoice>(LoggingToken::CHOOSE_GAME_TYPE);
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

    virtual optional<int> chooseItem(const vector<ItemInfo>& items, double* scrollpos) override {
      return readValue<optional<int>>(LoggingToken::CHOOSE_ITEM);
    }

    virtual optional<UniqueEntity<Creature>::Id> chooseRecruit(const string& title, const string& warning,
        pair<ViewId, int> budget, const vector<CreatureInfo>& c, double* scrollPos) override {
      return readValue<optional<UniqueEntity<Creature>::Id>>(LoggingToken::CHOOSE_RECRUIT);
    }

    virtual optional<UniqueEntity<Item>::Id> chooseTradeItem(const string& title, pair<ViewId, int> budget,
        const vector<ItemInfo>& c, double* scrollPos) override {
      return readValue<optional<UniqueEntity<Item>::Id>>(LoggingToken::CHOOSE_TRADE_ITEM);
    }

    virtual bool travelInterrupt() override {
      return readValue<bool>(LoggingToken::TRAVEL_INTERRUPT);
    }

    virtual optional<string> getText(const string& title, const string& value, int maxLength,
        const string& hint = "") override {
      return readValue<optional<string>>(LoggingToken::GET_TEXT);
    }

    virtual void presentHighscores(const vector<HighscoreList>&) override {
    }

    virtual void initialize() override {
      if (delegate)
        delegate->initialize();
    }

    virtual void reset() override {
      if (delegate)
        delegate->reset();
    }

    virtual void addSound(const Sound& s) override {
      if (delegate)
        delegate->addSound(s);
    }

    virtual optional<Vec2> chooseSite(const string& message, const Campaign& c, optional<Vec2> cur) override {
      return delegate->chooseSite(message, c, cur);
    }

    /*virtual CampaignAction prepareCampaign(const Campaign& c, Options* options, RetiredGames& retired) override {
      return delegate->prepareCampaign(c, options, retired);
    }*/

    virtual optional<UniqueEntity<Creature>::Id> chooseTeamLeader(const string& title, const vector<CreatureInfo>& c,
        const string& cancelText) override {
      return delegate->chooseTeamLeader(title, c, cancelText);
    }

/*    virtual void displaySplash(const ProgressMeter& m, SplashType type, function<void()> cancelFun) override {
 //     if (delegate)
 //       delegate->displaySplash(m, type, cancelFun);
    }*/

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

    virtual void updateView(CreatureView* creatureView, bool noRefresh) override {
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

    virtual void setScrollPos(Vec2 v) override {
      if (delegate)
        delegate->setScrollPos(v);
    }

  private:
    InputArchive& input;
    View* delegate;
};
