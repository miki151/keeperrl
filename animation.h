#pragma once

#include "util.h"

class ViewObject;
class Renderer;

class Animation {
  public:
  void render(Renderer&, Rectangle bounds, Vec2 origin, Vec2 squareSize, milliseconds time);
  bool isDone(milliseconds time) const;
  void setBegin(milliseconds time);
  static PAnimation thrownObject(Vec2 direction, ViewId, bool useSprite);
  static PAnimation fromId(AnimationId);
  static PAnimation perticleEffect(int id, milliseconds duration, unsigned int particleNum, Vec2 origin);
  virtual ~Animation() {}

  protected:
  Animation(milliseconds duration);
  virtual void renderSpec(Renderer&, Rectangle bounds, Vec2 origin, Vec2 squareSize, double state) = 0;

  private:
  optional<milliseconds> begin;
  milliseconds duration;
};

