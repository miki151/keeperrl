#pragma once

#include "creature_view.h"
#include "event_listener.h"

class View;

class Spectator : public OwnedObject<Spectator>, public CreatureView, public EventListener<Spectator> {
  public:
  Spectator(WLevel, View*);
  void onEvent(const GameEvent&);
  virtual const MapMemory& getMemory() const override;
  virtual void getViewIndex(Vec2 pos, ViewIndex&) const override;
  virtual void refreshGameInfo(GameInfo&) const override;
  virtual Vec2 getScrollCoord() const override;
  virtual Level* getCreatureViewLevel() const override;
  virtual double getAnimationTime() const override;
  virtual vector<Vec2> getVisibleEnemies() const override;
  virtual CenterType getCenterType() const override;
  virtual const vector<Vec2>& getUnknownLocations(WConstLevel) const override;

  private:
  WLevel level = nullptr;
  View* view = nullptr;
};

