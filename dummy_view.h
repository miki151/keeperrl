#pragma once

#include "view.h"
#include "clock.h"
#include "user_input.h"

class DummyView : public View {
  public:
  DummyView(Clock* c) : clock(c) {}
  Clock* clock;
  virtual ~DummyView() override {}
  virtual void initialize(unique_ptr<fx::FXRenderer>, unique_ptr<FXViewManager>) override {}
  virtual void reset() override {}
  virtual void displaySplash(const ProgressMeter*, const TString&, function<void()> = nullptr) override {}
  virtual void clearSplash() override {}
  virtual void playVideo(const string& path) override {}
  virtual void close() override {}
  virtual void refreshView() override {}
  virtual double getGameSpeed() override { return 20; }
  virtual void updateView(CreatureView*, bool noRefresh) override {}
  virtual void setScrollPos(Position) override {}
  virtual void resetCenter() override {}
  virtual UserInput getAction() override { return UserInputId::IDLE; }
  virtual bool travelInterrupt() override { return false; }
  virtual optional<Vec2> chooseDirection(Vec2 playerPos, const TString& message) override {
    return none;
  }
  virtual TargetResult chooseTarget(Vec2 playerPos, TargetType, Table<PassableInfo> passable,
      const TString& message, optional<Keybinding> cycleKey) override {
    return none;
  }
  virtual optional<int> getNumber(const TString& title, Range range, int initial) override {
    return none;
  }
  virtual optional<string> getText(const TString& title, const string& value, int maxLength) override {
    return none;
  }
  virtual void scriptedUI(ScriptedUIId, const ScriptedUIData&, ScriptedUIState&) override {}
  virtual CampaignAction prepareCampaign(CampaignOptions, CampaignMenuState&) override {
    return CampaignActionId::CANCEL;
  }
  virtual optional<UniqueEntity<Creature>::Id> chooseCreature(const string&, const vector<PlayerInfo>&,
      const string& cancelText) override {
    return none;
  }
  virtual bool creatureInfo(const string& title, bool prompt, const vector<PlayerInfo>&) override {
    return false;
  }
  virtual optional<Vec2> chooseSite(const string& message, const Campaign&, Vec2 current) override {
    return none;
  }
  virtual optional<int> chooseAtMouse(const vector<TString>& elems) override {
    return none;
  }
  virtual void presentWorldmap(const Campaign&, Vec2 current) override {}
  virtual void animateObject(Vec2 begin, Vec2 end, optional<ViewId>, optional<FXInfo>) override {}
  virtual void animation(Vec2 pos, AnimationId, Dir) override {}
  virtual void animation(const FXSpawnInfo&) override{}
  virtual milliseconds getTimeMilli() override { return clock->getMillis();}
  virtual milliseconds getTimeMilliAbsolute() override { return clock->getRealMillis();}
  virtual void stopClock() override {}
  virtual void continueClock() override {}
  virtual bool isClockStopped() override { return false; }
  virtual void addSound(const Sound&) override {}
  virtual void logMessage(const string&) override {}
  virtual void setBugReportSaveCallback(BugReportSaveCallback) override {}
  virtual void dungeonScreenshot(Vec2 size) override {
  }
  virtual bool zoomUIAvailable() const override {
    return false;
  }
  virtual string translate(const TString&) const override {
    return "";
  }
};
