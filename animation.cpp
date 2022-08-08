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

void Animation::render(Renderer& r, Rectangle bounds, Vec2 origin, Vec2 squareSize, milliseconds time, Color color) {
  CHECK(begin);
  CHECK(time - *begin <= duration) << time << " " << *begin << " " << duration;
  renderSpec(r, bounds, origin, squareSize, (double)(time - *begin).count() / duration.count(), color);
}

class ThrownObject : public Animation {
  public:
  ThrownObject(Vec2 dir, ViewId obj, bool sprite)
    : Animation(milliseconds{dir.length8()}), direction(dir), viewObject(obj), useSprite(sprite) {}

  virtual void renderSpec(Renderer& renderer, Rectangle bounds, Vec2 origin, Vec2 squareSize, double state,
      Color color) override {
    int x = origin.x + state * direction.x - squareSize.x / 2;
    int y = origin.y + state * direction.y - squareSize.y / 2;
    renderer.drawViewObject(Vec2(x, y), viewObject, useSprite, squareSize, color,
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

  virtual void renderSpec(Renderer& renderer, Rectangle bounds, Vec2 origin, Vec2 squareSize, double state,
      Color color) override {
    renderer.drawAnimation(id, origin, state, squareSize, orientation, color);
  }

  private:
  AnimationId id;
  Dir orientation;
};
 
PAnimation Animation::fromId(AnimationId id, Dir orientation) {
  return make_unique<SpriteAnim>(id, getDuration(id), orientation);
}
