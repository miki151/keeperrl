#pragma once

#include "util.h"

class ViewObject;
class Renderer;
class Color;

class Animation {
  public:
  void render(Renderer&, Rectangle bounds, Vec2 origin, Vec2 squareSize, milliseconds time, Color);
  bool isDone(milliseconds time) const;
  void setBegin(milliseconds time);
  static PAnimation thrownObject(Vec2 direction, ViewId, bool useSprite);
  static PAnimation fromId(AnimationId, Dir orientation);
  virtual ~Animation() {}

  protected:
  Animation(milliseconds duration);
  virtual void renderSpec(Renderer&, Rectangle bounds, Vec2 origin, Vec2 squareSize, double state, Color) = 0;

  private:
  optional<milliseconds> begin;
  milliseconds duration;
};

