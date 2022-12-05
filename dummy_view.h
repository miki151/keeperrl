#pragma once

#include "view.h"
#include "clock.h"
#include "avatar_menu_option.h"
#include "user_input.h"

class DummyView : public View {
  public:
  DummyView(Clock* c) : clock(c) {}
  Clock* clock;
  virtual ~DummyView() override {}
  virtual void initialize(unique_ptr<fx::FXRenderer>, unique_ptr<FXViewManager>) override {}
  virtual void reset() override {}
  virtual void displaySplash(const ProgressMeter*, const string&, function<void()> = nullptr) override {}
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
  virtual optional<Vec2> chooseDirection(Vec2 playerPos, const string& message) override {
    return none;
  }
  virtual TargetResult chooseTarget(Vec2 playerPos, TargetType, Table<PassableInfo> passable,
      const string& message, optional<Keybinding> cycleKey) override {
    return none;
  }
  virtual optional<int> getNumber(const string& title, Range range, int initial) override {
    return none;
  }
  virtual optional<string> getText(const string& title, const string& value, int maxLength,
      const string& hint = "") override {
    return none;
  }
  virtual void scriptedUI(ScriptedUIId, const ScriptedUIData&, ScriptedUIState&) override {}
  virtual CampaignAction prepareCampaign(CampaignOptions, CampaignMenuState&) override {
    return CampaignActionId::CANCEL;
  }
  virtual vector<int> prepareWarlordGame(RetiredGames&, const vector<PlayerInfo>&, int maxTeam, int maxDungeons) override {
    return vector<int>();
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
  virtual optional<int> chooseAtMouse(const vector<string>& elems) override {
    return none;
  }
  virtual void presentWorldmap(const Campaign&) override {}
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
  virtual variant<AvatarChoice, AvatarMenuOption> chooseAvatar(const vector<AvatarData>&) override {
    return AvatarMenuOption::GO_BACK;
  }
  virtual void dungeonScreenshot(Vec2 size) override {
  }
  virtual bool zoomUIAvailable() const override {
    return false;
  }
};
