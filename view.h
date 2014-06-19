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

class CreatureView;
class Level;
class Jukebox;

class View {
  public:
  View();
  virtual ~View();

  /** Does all the library specific init.*/
  virtual void initialize() = 0;

  /** Resets the view before a new game.*/
  virtual void reset() = 0;

  enum SplashType { CREATING, LOADING, SAVING };

  /** Displays a splash screen in an active loop until \paramname{ready} is set to true in another thread.*/
  virtual void displaySplash(SplashType type, bool& ready) = 0;

  /** Shutdown routine.*/
  virtual void close() = 0;

  /** Reads the game state from \paramname{creatureView} and refreshes the display.*/
  virtual void refreshView() = 0;

  /** Reads the game state from \paramname{creatureView} and only updates internal data, doesn't refresh.*/
  virtual void updateView(const CreatureView* creatureView) = 0;

  /** Draw a blocking view of the whole level.*/
  virtual void drawLevelMap(const CreatureView*) = 0;

  /** Scrolls back to the center of the view on next refresh.*/
  virtual void resetCenter() = 0;

  //@{
  /** Adds a new message to the box. The message is remembered between refreshes,
      and is discarded when appropriate.*/
  virtual void addMessage(const string& message) = 0;
  virtual void addImportantMessage(const string& message) = 0;
  //@}

  /** Clears the message box.*/
  virtual void clearMessages() = 0;

  /** Current messages are shown as old.*/
  virtual void retireMessages() = 0;

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
        Optional<UserInput::Type> triggerAction = Nothing());
    ListElem(const string& text = "", ElemMod mod = NORMAL,
        Optional<UserInput::Type> triggerAction = Nothing());

    const string& getText() const;
    ElemMod getMod() const;
    Optional<UserInput::Type> getAction() const;

    private:
    string text;
    ElemMod mod;
    Optional<UserInput::Type> action;
  };

  static vector<ListElem> getListElem(const vector<string>&);

  enum MenuType {
    NORMAL_MENU,
    MAIN_MENU,
    MINION_MENU,
  };

  /** Draws a window with some options for the player to choose. \paramname{index} indicates the highlighted item. 
      Returns Nothing() if the player cancelled the choice.*/
  virtual Optional<int> chooseFromList(const string& title, const vector<ListElem>& options, int index = 0,
      MenuType = NORMAL_MENU, double* scrollPos = nullptr, Optional<UserInput::Type> exitAction = Nothing()) = 0;

  /** Let's the player choose a direction from the main 8. Returns Nothing() if the player cancelled the choice.*/
  virtual Optional<Vec2> chooseDirection(const string& message) = 0;

  /** Asks the player a yer-or-no question.*/
  virtual bool yesOrNoPrompt(const string& message) = 0;

  /** Draws a window with some text. The text is formatted to fit the window.*/
  virtual void presentText(const string& title, const string& text) = 0;

  /** Draws a window with a list of items.*/
  virtual void presentList(const string& title, const vector<ListElem>& options, bool scrollDown = false,
      Optional<UserInput::Type> exitAction = Nothing()) = 0;

  /** Let's the player choose a number. Returns Nothing() if the player cancelled the choice.*/
  virtual Optional<int> getNumber(const string& title, int min, int max, int increments = 1) = 0;

  /** Draws an animation of an object between two locations on a map.*/
  virtual void animateObject(vector<Vec2> trajectory, ViewObject object) = 0;

  /** Draws an special animation on the map.*/
  virtual void animation(Vec2 pos, AnimationId) = 0;

  /** Returns the current real time in milliseconds. The clock is stopped on blocking keyboard input,
      so it can be used to sync game time in real-time mode.*/
  virtual int getTimeMilli() = 0;

  /** Stops the real time clock.*/
  virtual void stopClock() = 0;

  /** Sets the real time clock.*/
  virtual void setTimeMilli(int) = 0;

  /** Continues the real time clock after it had been stopped.*/

  virtual void continueClock() = 0;

  /** Returns whether the real time clock is currently stopped.*/
  virtual bool isClockStopped() = 0;

  void setJukebox(Jukebox*);
  Jukebox* getJukebox();

  /** Returns a default View that additionally logs all player actions into a file.*/
  static View* createLoggingView(ofstream& of);

  /** Returns a default View that reads all player actions from a file instead of the keyboard.*/
  static View* createReplayView(ifstream& ifs);

  /** Represents all the game information displayed around the map window.*/
  class GameInfo {
    public:
    enum class InfoType { PLAYER, BAND} infoType;

    struct CreatureInfo {
      CreatureInfo(const Creature* c);
      ViewObject viewObject;
      UniqueId uniqueId;
      string name;
      string speciesName;
      int expLevel;
    };

    class BandInfo {
      public:
      string warning;
      struct Button {
        ViewObject object;
        string name;
        Optional<pair<ViewObject, int>> cost;
        string count;
        string inactiveReason;
        string help;
        char hotkey;

        struct Option {
          Optional<ViewObject> object;
          string text;
        };
        vector<Option> options;
        string optionsTitle;
      };
      vector<Button> buildings;
      vector<Button> workshop;
      vector<Button> minionButtons;
      vector<Button> libraryButtons;
      string monsterHeader;
      vector<CreatureInfo> creatures;
      vector<CreatureInfo> enemies;
      map<UniqueId, string> tasks;
      struct Resource {
        ViewObject viewObject;
        int count;
        string name;
      };
      vector<Resource> numResource;
      double time;
      bool gatheringTeam = false;
      vector<UniqueId> team;

      struct TechButton {
        ViewId viewId;
        string name;
        char hotkey;
      };
      vector<TechButton> techButtons;
    } bandInfo;

    class PlayerInfo {
      public:
      int speed;
      int speedBonus;
      int defense;
      int defBonus;
      int attack;
      int toHit;
      int toHitBonus;
      int attBonus;
      int strength;
      int strBonus;
      int dexterity;
      int dexBonus;
      bool possessed;
      bool spellcaster;
      double time;
      int numGold;
      string playerName;
      vector<string> adjectives;
      string title;
      string levelName;
      string weaponName;
      double elfStanding;
      double dwarfStanding;
      double goblinStanding;
      struct Effect {
        string name;
        bool bad;
      };
      vector<Effect> effects;
    } playerInfo;

    class VillageInfo {
      public:
      struct Village {
        string name;
        string tribeName;
        string state;
      };
      vector<Village> villages;
    } villageInfo;
    
    class SunlightInfo {
      public:
      string description;
      int timeRemaining;
    } sunlightInfo;

  } gameInfo;

  private:
  Jukebox* jukebox = nullptr;
};

#endif
