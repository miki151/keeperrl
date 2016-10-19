#pragma once

#include "util.h"
#include "renderer.h"

class ViewObject;

class Animation {
  public:
  void render(Renderer&, Rectangle bounds, Vec2 origin, milliseconds time);
  bool isDone(milliseconds time) const;
  void setBegin(milliseconds time);
  static PAnimation thrownObject(Vec2 direction, ViewObject, bool useSprite, Vec2 squareSize);
  static PAnimation fromId(AnimationId);

  protected:
  Animation(milliseconds duration);
  virtual void renderSpec(Renderer&, Rectangle bounds, Vec2 origin, double state) = 0;

  private:
  optional<milliseconds> begin;
  milliseconds duration;
};

