#include "stdafx.h"

#include "animation.h"
#include "animation_id.h"
#include "renderer.h"
#include "view_object.h"

Animation::Animation(milliseconds d) : duration(d) {}

bool Animation::isDone(milliseconds time) const {
  return begin && time > *begin + duration;
}

void Animation::setBegin(milliseconds time) {
  CHECK(!begin);
  begin = time;
}

void Animation::render(Renderer& r, Rectangle bounds, Vec2 origin, Vec2 squareSize, milliseconds time) {
  CHECK(begin);
  CHECK(time - *begin <= duration) << time << " " << *begin << " " << duration;
  renderSpec(r, bounds, origin, squareSize, (double)(time - *begin).count() / duration.count());
}

class ThrownObject : public Animation {
  public:
  ThrownObject(Vec2 dir, ViewId obj, bool sprite)
    : Animation(milliseconds{dir.length8()}), direction(dir), viewObject(obj), useSprite(sprite) {}

  virtual void renderSpec(Renderer& renderer, Rectangle bounds, Vec2 origin, Vec2 squareSize, double state) {
    int x = origin.x + state * direction.x;
    int y = origin.y + state * direction.y;
    renderer.drawViewObject(Vec2(x, y), viewObject, useSprite, squareSize, Color::WHITE,
        Renderer::SpriteOrientation(direction, false));
  }

  private:
  Vec2 direction;
  ViewId viewObject;
  bool useSprite;
};

PAnimation Animation::thrownObject(Vec2 direction, ViewId obj, bool useSprite) {
  return PAnimation(new ThrownObject(direction, obj, useSprite));
}

class SpriteAnim : public Animation {
  public:

  SpriteAnim(AnimationId id, milliseconds duration, Dir o) : Animation(duration), id(id), orientation(o) {}

  virtual void renderSpec(Renderer& renderer, Rectangle bounds, Vec2 origin, Vec2 squareSize, double state) override {
    renderer.drawAnimation(id, origin, state, squareSize, orientation);
  }

  private:
  AnimationId id;
  Dir orientation;
};
 
PAnimation Animation::fromId(AnimationId id, Dir orientation) {
  return unique<SpriteAnim>(id, getDuration(id), orientation);
}
