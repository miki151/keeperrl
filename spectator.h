#pragma once

#include "creature_view.h"

class Spectator : public CreatureView {
  public:
  Spectator(WLevel);
  virtual const MapMemory& getMemory() const override;
  virtual void getViewIndex(Vec2 pos, ViewIndex&) const override;
  virtual void refreshGameInfo(GameInfo&) const override;
  virtual Vec2 getPosition() const override;
  virtual WLevel getLevel() const override;
  virtual double getAnimationTime() const override;
  virtual vector<Vec2> getVisibleEnemies() const override;
  virtual CenterType getCenterType() const override;
  virtual vector<Vec2> getUnknownLocations(WConstLevel) const override;

  private:
  WLevel level;
};

