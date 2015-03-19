#ifndef _SPECTATOR_H
#define _SPECTATOR_H

#include "creature_view.h"

class Spectator : public CreatureView {
  public:
  Spectator(const Level*);
  virtual const MapMemory& getMemory() const override;
  virtual void getViewIndex(Vec2 pos, ViewIndex&) const override;
  virtual void refreshGameInfo(GameInfo&) const override;
  virtual optional<Vec2> getPosition(bool force) const override;
  virtual optional<MovementInfo> getMovementInfo() const override;
  virtual const Level* getLevel() const override;
  virtual double getTime() const override;
  virtual vector<Vec2> getVisibleEnemies() const override;

  private:
  const Level* level;
};

#endif
