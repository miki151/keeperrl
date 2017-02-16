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
#include "debug.h"
#include "user_input.h"
#include "player_role_choice.h"

class CreatureView;
class Level;
class Jukebox;
class ProgressMeter;
class PlayerInfo;
struct ItemInfo;
struct CreatureInfo;
class Sound;
class Campaign;
class Options;
class RetiredGames;
class ScrollPosition;

enum class SplashType { BIG, AUTOSAVING, SMALL };

class ListElem {
  public:
  enum ElemMod {
    NORMAL,
    TEXT,
    TITLE,
    INACTIVE,
  };

  ListElem(const char*, ElemMod mod = NORMAL,
      optional<UserInputId> triggerAction = none);
  ListElem(const string& text = "", ElemMod mod = NORMAL,
      optional<UserInputId> triggerAction = none);
  ListElem(const string& text, const string& secColumn, ElemMod mod = NORMAL);

  ListElem& setTip(const string&);
  ListElem& setMessagePriority(MessagePriority);
  optional<MessagePriority> getMessagePriority() const;

  const string& getText() const;
  const string& getSecondColumn() const;
  const string& getTip() const;
  ElemMod getMod() const;
  optional<UserInputId> getAction() const;
  void setMod(ElemMod);

  static vector<ListElem> convert(const vector<string>&);

  private:
  string text;
  string secondColumn;
  string tooltip;
  ElemMod mod;
  optional<UserInputId> action;
  optional<MessagePriority> messagePriority;
};

enum class MenuType {
  NORMAL,
  MAIN,
  MAIN_NO_TILES,
  GAME_CHOICE,
  YES_NO
};

struct HighscoreList {
  string name;
  struct Elem {
    string text;
    string score;
    bool highlight;
  };
  vector<Elem> scores;
};

enum class CampaignActionId {
  CANCEL,
  CHOOSE_SITE,
  REROLL_MAP,
  UPDATE_MAP,
  CONFIRM,
  UPDATE_OPTION,
  CHANGE_TYPE,
};

class CampaignAction : public EnumVariant<CampaignActionId, TYPES(Vec2, OptionId, CampaignType),
  ASSIGN(CampaignType, CampaignActionId::CHANGE_TYPE),
  ASSIGN(Vec2, CampaignActionId::CHOOSE_SITE),
  ASSIGN(OptionId, CampaignActionId::UPDATE_OPTION)> {
    using EnumVariant::EnumVariant;
};

class GameExitException {};

class View {
  public:
  View();
  virtual ~View();

  /** Does all the library specific init.*/
  virtual void initialize() = 0;

  /** Resets the view before a new game.*/
  virtual void reset() = 0;

  /** Displays a splash screen in an active loop until \paramname{ready} is set to true in another thread.*/
  virtual void displaySplash(const ProgressMeter*, const string& text, SplashType type,
      function<void()> cancelFun = nullptr) = 0;

  virtual void clearSplash() = 0;

  /** Shutdown routine.*/
  virtual void close() = 0;

  /** Reads the game state from \paramname{creatureView} and refreshes the display.*/
  virtual void refreshView() = 0;

  /** Returns real-time game mode speed measured in turns per millisecond. **/
  virtual double getGameSpeed() = 0;

  /** Reads the game state from \paramname{creatureView}. If \paramname{noRefresh} is set,
      won't trigger screen to refresh.*/
  virtual void updateView(CreatureView*, bool noRefresh) = 0;

  /** Draw a blocking view of the whole level.*/
  virtual void drawLevelMap(const CreatureView*) = 0;

  virtual void setScrollPos(Vec2) = 0;

  /** Scrolls back to the center of the view on next refresh.*/
  virtual void resetCenter() = 0;

  /** Reads input in a non-blocking manner.*/
  virtual UserInput getAction() = 0;

  /** Returns whether a travel interrupt key is pressed at a given moment.*/
  virtual bool travelInterrupt() = 0;

  /** Draws a window with some options for the player to choose. \paramname{index} indicates the highlighted item. 
      Returns none if the player cancelled the choice.*/
  virtual optional<int> chooseFromList(const string& title, const vector<ListElem>& options, int index = 0,
      MenuType = MenuType::NORMAL, ScrollPosition* scrollPos = nullptr, optional<UserInputId> exitAction = none) = 0;

  virtual PlayerRoleChoice getPlayerRoleChoice(optional<PlayerRoleChoice> initial) = 0;

  /** Lets the player choose a direction from the main 8. Returns none if the player cancelled the choice.*/
  virtual optional<Vec2> chooseDirection(const string& message) = 0;

  /** Asks the player a yer-or-no question.*/
  virtual bool yesOrNoPrompt(const string& message, bool defaultNo = false) = 0;

  /** Draws a window with some text. The text is formatted to fit the window.*/
  virtual void presentText(const string& title, const string& text) = 0;

  /** Draws a window with a list of items.*/
  virtual void presentList(const string& title, const vector<ListElem>& options, bool scrollDown = false,
      MenuType = MenuType::NORMAL, optional<UserInputId> exitAction = none) = 0;

  /** Lets the player choose a number. Returns none if the player cancelled the choice.*/
  virtual optional<int> getNumber(const string& title, int min, int max, int increments = 1) = 0;

  /** Lets the player input a string. Returns none if the player cancelled the choice.*/
  virtual optional<string> getText(const string& title, const string& value, int maxLength,
      const string& hint = "") = 0;

  virtual optional<UniqueEntity<Item>::Id> chooseTradeItem(const string& title, pair<ViewId, int> budget,
      const vector<ItemInfo>&, ScrollPosition* scrollPos) = 0;

  virtual optional<int> choosePillageItem(const string& title, const vector<ItemInfo>&, ScrollPosition* scrollPos) = 0;

  virtual optional<int> chooseItem(const vector<ItemInfo>& items, ScrollPosition* scrollpos) = 0;

  virtual void presentHighscores(const vector<HighscoreList>&) = 0;

  struct CampaignMenuState {
    bool helpText;
    bool settings;
  };
  struct CampaignOptions {
    const Campaign& campaign;
    optional<RetiredGames&> retired;
    const Creature* player;
    vector<OptionId> primaryOptions;
    vector<OptionId> secondaryOptions;
    optional<string> mapTitle;
    string introText;
    vector<CampaignType> availableTypes;
    enum WarningType { NO_RETIRE, OTHER_MODES };
    optional<WarningType> warning;
  };

  virtual CampaignAction prepareCampaign(CampaignOptions, Options*, CampaignMenuState&) = 0;

  virtual optional<UniqueEntity<Creature>::Id> chooseTeamLeader(const string& title, const vector<CreatureInfo>&,
      const string& cancelText) = 0;

  virtual bool creaturePrompt(const string& title, const vector<CreatureInfo>&) = 0;

  virtual optional<Vec2> chooseSite(const string& message, const Campaign&, optional<Vec2> current = none) = 0;

  virtual void presentWorldmap(const Campaign&) = 0;

  /** Draws an animation of an object between two locations on a map.*/
  virtual void animateObject(vector<Vec2> trajectory, ViewObject object) = 0;

  /** Draws an special animation on the map.*/
  virtual void animation(Vec2 pos, AnimationId) = 0;

  /** Returns the current real time in milliseconds. The clock is stopped on blocking keyboard input,
      so it can be used to sync game time in real-time mode.*/
  virtual milliseconds getTimeMilli() = 0;

  /** Returns the absolute time, doesn't take pausing into account.*/
  virtual milliseconds getTimeMilliAbsolute() = 0;

  /** Stops the real time clock.*/
  virtual void stopClock() = 0;

  /** Continues the real time clock after it had been stopped.*/
  virtual void continueClock() = 0;

  /** Returns whether the real time clock is currently stopped.*/
  virtual bool isClockStopped() = 0;

  virtual void addSound(const Sound&) = 0;

  virtual void logMessage(const string&) = 0;
};

enum class AnimationId {
  EXPLOSION,
};

