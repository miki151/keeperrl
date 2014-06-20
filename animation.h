#ifndef _ANIMATION_H
#define _ANIMATION_H

#include "util.h"
#include "renderer.h"
#include "view_object.h"

class Animation {
  public:
  void render(Renderer&, Rectangle bounds, Vec2 origin, double time);
  bool isDone(double time) const;
  void setBegin(double time);
  static PAnimation thrownObject(Vec2 direction, ViewObject, bool useSprite);
  static PAnimation fromId(AnimationId);

  protected:
  Animation(double duration);
  virtual void renderSpec(Renderer&, Rectangle bounds, Vec2 origin, double state) = 0;

  private:
  double begin = -1;
  double duration;
};

#endif
