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

#ifndef _VIEW_H
#define _VIEW_H

#include "util.h"
#include "debug.h"
#include "view_object.h"
#include "user_input.h"
#include "game_info.h"

class CreatureView;
class Level;
class Jukebox;
class ProgressMeter;

class View {
  public:
  View();
  virtual ~View();

  /** Does all the library specific init.*/
  virtual void initialize() = 0;

  /** Resets the view before a new game.*/
  virtual void reset() = 0;

  enum SplashType { CREATING, LOADING, SAVING, UPLOADING, DOWNLOADING };

  /** Displays a splash screen in an active loop until \paramname{ready} is set to true in another thread.*/
  virtual void displaySplash(const ProgressMeter&, SplashType type, function<void()> cancelFun = nullptr) = 0;

  virtual void clearSplash() = 0;

  /** Shutdown routine.*/
  virtual void close() = 0;

  /** Reads the game state from \paramname{creatureView} and refreshes the display.*/
  virtual void refreshView() = 0;

  /** Returns real-time game mode speed measured in turns per millisecond. **/
  virtual double getGameSpeed() = 0;

  /** Reads the game state from \paramname{creatureView}. If \paramname{noRefresh} is set,
      won't trigger screen to refresh.*/
  virtual void updateView(const CreatureView* creatureView, bool noRefresh) = 0;

  /** Draw a blocking view of the whole level.*/
  virtual void drawLevelMap(const CreatureView*) = 0;

  /** Scrolls back to the center of the view on next refresh.*/
  virtual void resetCenter() = 0;

  /** Reads input in a non-blocking manner.*/
  virtual UserInput getAction() = 0;

  /** Returns whether a travel interrupt key is pressed at a given moment.*/
  virtual bool travelInterrupt() = 0;

  enum ElemMod {
    NORMAL,
    TEXT,
    TITLE,
    INACTIVE,
  };

  class ListElem {
    public:
    ListElem(const char*, ElemMod mod = NORMAL,
        optional<UserInputId> triggerAction = none);
    ListElem(const string& text = "", ElemMod mod = NORMAL,
        optional<UserInputId> triggerAction = none);
    ListElem(const string& text, const string& secColumn, ElemMod mod = NORMAL);

    ListElem& setTip(const string&);

    const string& getText() const;
    const string& getSecondColumn() const;
    const string& getTip() const;
    ElemMod getMod() const;
    optional<UserInputId> getAction() const;
    void setMod(ElemMod);

    private:
    string text;
    string secondColumn;
    string tooltip;
    ElemMod mod;
    optional<UserInputId> action;
  };

  static vector<ListElem> getListElem(const vector<string>&);

  enum MenuType {
    NORMAL_MENU,
    MAIN_MENU,
    MAIN_MENU_NO_TILES,
    MINION_MENU,
    GAME_CHOICE_MENU,
  };

  /** Draws a window with some options for the player to choose. \paramname{index} indicates the highlighted item. 
      Returns none if the player cancelled the choice.*/
  virtual optional<int> chooseFromList(const string& title, const vector<ListElem>& options, int index = 0,
      MenuType = NORMAL_MENU, double* scrollPos = nullptr, optional<UserInputId> exitAction = none) = 0;

  enum GameTypeChoice {
    KEEPER_CHOICE,
    ADVENTURER_CHOICE,
    LOAD_CHOICE,
    BACK_CHOICE,
  };

  virtual GameTypeChoice chooseGameType() = 0;

  /** Let's the player choose a direction from the main 8. Returns none if the player cancelled the choice.*/
  virtual optional<Vec2> chooseDirection(const string& message) = 0;

  /** Asks the player a yer-or-no question.*/
  virtual bool yesOrNoPrompt(const string& message) = 0;

  /** Draws a window with some text. The text is formatted to fit the window.*/
  virtual void presentText(const string& title, const string& text) = 0;

  /** Draws a window with a list of items.*/
  virtual void presentList(const string& title, const vector<ListElem>& options, bool scrollDown = false,
      MenuType = NORMAL_MENU, optional<UserInputId> exitAction = none) = 0;

  /** Let's the player choose a number. Returns none if the player cancelled the choice.*/
  virtual optional<int> getNumber(const string& title, int min, int max, int increments = 1) = 0;

  /** Let's the player input a string. Returns none if the player cancelled the choice.*/
  virtual optional<string> getText(const string& title, const string& value, int maxLength,
      const string& hint = "") = 0;

  /** Draws an animation of an object between two locations on a map.*/
  virtual void animateObject(vector<Vec2> trajectory, ViewObject object) = 0;

  /** Draws an special animation on the map.*/
  virtual void animation(Vec2 pos, AnimationId) = 0;

  /** Returns the current real time in milliseconds. The clock is stopped on blocking keyboard input,
      so it can be used to sync game time in real-time mode.*/
  virtual int getTimeMilli() = 0;

  /** Returns the absolute time, doesn't take pausing into account.*/
  virtual int getTimeMilliAbsolute() = 0;

  /** Stops the real time clock.*/
  virtual void stopClock() = 0;

  /** Continues the real time clock after it had been stopped.*/

  virtual void continueClock() = 0;

  /** Returns whether the real time clock is currently stopped.*/
  virtual bool isClockStopped() = 0;
};

enum class AnimationId {
  EXPLOSION,
};

#endif
