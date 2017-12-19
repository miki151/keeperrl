#pragma once

#include "view.h"
#include "clock.h"
#include "player_role.h"

class DummyView : public View {
  public:
  DummyView(Clock* c) : clock(c) {}
  Clock* clock;
  virtual ~DummyView() {}
  virtual void initialize() {}
  virtual void reset() {}
  virtual void displaySplash(const ProgressMeter*, const string&, SplashType, function<void()> = nullptr) {}
  virtual void clearSplash() {}
  virtual void close() {}
  virtual void refreshView() {}
  virtual double getGameSpeed() { return 20; }
  virtual void updateView(CreatureView*, bool noRefresh) {}
  virtual void drawLevelMap(const CreatureView*) {}
  virtual void setScrollPos(Vec2) {}
  virtual void resetCenter() {}
  virtual UserInput getAction() { return UserInputId::IDLE; }
  virtual bool travelInterrupt() { return false; }
  virtual optional<int> chooseFromList(const string&, const vector<ListElem>&, int = 0,
      MenuType = MenuType::NORMAL, ScrollPosition* = nullptr, optional<UserInputId> = none) {
    return none;
  }
  virtual PlayerRoleChoice getPlayerRoleChoice(optional<PlayerRoleChoice> initial) {
    return PlayerRole::KEEPER;
  }
  virtual optional<Vec2> chooseDirection(Vec2 playerPos, const string& message) {
    return none;
  }
  virtual bool yesOrNoPrompt(const string& message, bool defaultNo = false) {
    return false;
  }
  virtual void presentText(const string&, const string&) {}
  virtual void presentList(const string&, const vector<ListElem>&, bool = false,
      MenuType = MenuType::NORMAL, optional<UserInputId> = none) {}
  virtual optional<int> getNumber(const string& title, int min, int max, int increments = 1) {
    return none;
  }
  virtual optional<string> getText(const string& title, const string& value, int maxLength,
      const string& hint = "") {
    return none;
  }
  virtual optional<UniqueEntity<Item>::Id> chooseTradeItem(const string& title, pair<ViewId, int> budget,
      const vector<ItemInfo>&, ScrollPosition* scrollPos) {
    return none;
  }
  virtual optional<int> choosePillageItem(const string& title, const vector<ItemInfo>&, ScrollPosition* scrollPos) {
    return none;
  }
  virtual optional<int> chooseItem(const vector<ItemInfo>& items, ScrollPosition* scrollpos) {
    return none;
  }
  virtual void presentHighscores(const vector<HighscoreList>&) {}
  virtual CampaignAction prepareCampaign(CampaignOptions, Options*, CampaignMenuState&) {
    return CampaignActionId::CANCEL;
  }
  virtual optional<UniqueEntity<Creature>::Id> chooseCreature(const string&, const vector<CreatureInfo>&,
      const string& cancelText) {
    return none;
  }
  virtual bool creatureInfo(const string& title, bool prompt, const vector<CreatureInfo>&) {
    return false;
  }
  virtual optional<Vec2> chooseSite(const string& message, const Campaign&, optional<Vec2> current = none) {
    return none;
  }
  virtual optional<int> chooseAtMouse(const vector<string>& elems) {
    return none;
  }
  virtual void presentWorldmap(const Campaign&) {}
  virtual void animateObject(Vec2 begin, Vec2 end, ViewId object) {}
  virtual void animation(Vec2 pos, AnimationId) {}
  virtual milliseconds getTimeMilli() { return clock->getMillis();}
  virtual milliseconds getTimeMilliAbsolute() { return clock->getRealMillis();}
  virtual void stopClock() {}
  virtual void continueClock() {}
  virtual bool isClockStopped() { return false; }
  virtual void addSound(const Sound&) {}
  virtual void logMessage(const string&) {}
};
